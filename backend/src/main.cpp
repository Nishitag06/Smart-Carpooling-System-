#include "CarpoolService.h"
#include "crow.h"
#include <iostream>
#include <string>

using namespace std;
using crow::json::wvalue;

static wvalue driverJson(const Driver& d) {
    wvalue x;
    x["id"] = d.id; x["name"] = d.name; x["origin"] = d.origin;
    x["destination"] = d.destination; x["departureTime"] = formatTime(d.departureMinutes);
    x["availableSeats"] = d.availableSeats; x["maxWaitMinutes"] = d.maxWaitMinutes;
    x["maxDetourMinutes"] = d.maxDetourMinutes;
    return x;
}
static wvalue passengerJson(const Passenger& p) {
    wvalue x;
    x["id"] = p.id; x["name"] = p.name; x["pickup"] = p.pickup; x["dropoff"] = p.dropoff;
    x["preferredPickupTime"] = formatTime(p.preferredPickupMinutes);
    x["earlyToleranceMinutes"] = p.earlyToleranceMinutes;
    x["lateToleranceMinutes"] = p.lateToleranceMinutes; x["seatsNeeded"] = p.seatsNeeded;
    return x;
}
static wvalue matchJson(const Match& m) {
    wvalue x;
    x["driverId"] = m.driverId; x["passengerId"] = m.passengerId;
    x["baseRouteMinutes"] = m.baseRouteMinutes; x["sharedRouteMinutes"] = m.sharedRouteMinutes;
    x["baseRouteDistanceKm"] = m.baseRouteDistanceKm;
    x["sharedRouteDistanceKm"] = m.sharedRouteDistanceKm;
    x["detourDistanceKm"] = m.detourDistanceKm;
    x["detourMinutes"] = m.detourMinutes; x["pickupArrivalTime"] = formatTime(m.pickupArrivalMinutes);
    x["passengerWaitMinutes"] = m.passengerWaitMinutes;
    x["driverWaitMinutes"] = m.driverWaitMinutes;
    x["passengerWaitByArrivalMinutes"] = m.passengerWaitByArrivalMinutes;
    x["passengerEarliestPickupTime"] = formatTime(m.passengerEarliestPickupMinutes);
    x["passengerLatestPickupTime"] = formatTime(m.passengerLatestPickupMinutes);
    x["pickupArrivalWithinWindow"] = m.pickupArrivalWithinWindow;
    x["pickupRouteIndex"] = m.pickupRouteIndex;
    x["passengerDestinationRouteIndex"] = m.passengerDestinationRouteIndex;
    wvalue route, originalRoute;
    for (size_t i = 0; i < m.route.size(); ++i) route[i] = m.route[i];
    for (size_t i = 0; i < m.originalRoute.size(); ++i) originalRoute[i] = m.originalRoute[i];
    x["route"] = move(route);
    x["originalRoute"] = move(originalRoute);
    return x;
}
static wvalue bookingJson(const Booking& b, const CarpoolService& service) {
    wvalue x;
    x["id"] = b.id; x["driverId"] = b.driverId; x["passengerId"] = b.passengerId;
    x["seatsBooked"] = b.seatsBooked; x["active"] = b.active;
    x["status"] = b.active ? "Confirmed" : "Cancelled";
    for (const auto& driver : service.drivers()) if (driver.id == b.driverId) {
        x["driverName"] = driver.name;
        x["departureTime"] = formatTime(driver.departureMinutes);
        break;
    }
    for (const auto& passenger : service.passengers()) if (passenger.id == b.passengerId) {
        x["passengerName"] = passenger.name; x["pickup"] = passenger.pickup;
        x["dropoff"] = passenger.dropoff;
        break;
    }
    wvalue route;
    for (size_t i = 0; i < b.match.route.size(); ++i) route[i] = b.match.route[i];
    x["route"] = move(route);
    return x;
}
static string errorJson(const string& message) {
    wvalue x; x["error"] = message;
    return x.dump();
}
static bool hasString(const crow::json::rvalue& body, const char* key) {
    return body.has(key) && body[key].t() == crow::json::type::String;
}
static bool hasNumber(const crow::json::rvalue& body, const char* key) {
    return body.has(key) && body[key].t() == crow::json::type::Number;
}

int main() {
    CarpoolService service;
    crow::SimpleApp app;

    CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::GET)([] {
        wvalue x; x["status"] = "ok"; x["message"] = "Smart Carpool API is running"; return x;
    });

    CROW_ROUTE(app, "/api/locations").methods(crow::HTTPMethod::GET)([&service] {
        wvalue result;
        auto places = service.locations();
        for (size_t i = 0; i < places.size(); ++i) result[i] = places[i];
        return result;
    });

    CROW_ROUTE(app, "/api/roads").methods(crow::HTTPMethod::GET)([&service] {
        wvalue result;
        const auto places = service.locations();
        const auto roads = service.roads();
        wvalue locationList;
        for (size_t i = 0; i < places.size(); ++i) locationList[i] = places[i];
        wvalue edgeList = wvalue::list{};
        for (size_t i = 0; i < roads.size(); ++i) {
            wvalue edge;
            edge["from"] = roads[i].from;
            edge["to"] = roads[i].to;
            edge["distanceKm"] = roads[i].distanceKm;
            edge["distanceSource"] = roads[i].distanceSource;
            edge["twoWay"] = roads[i].twoWay;
            edgeList[i] = move(edge);
        }
        result["locations"] = move(locationList);
        result["edges"] = move(edgeList);
        result["averageSpeedKmh"] = Graph::AVERAGE_SPEED_KMH;
        result["distanceDataStatus"] = roads.empty() ? "unavailable" : "sourced";
        result["unavailableReason"] = roads.empty()
            ? "OpenStreetMap geocoding failed TLS certificate validation, so no road distances are verified."
            : "";
        return result;
    });

    CROW_ROUTE(app, "/api/drivers").methods(crow::HTTPMethod::GET)([&service] {
        wvalue result;
        for (size_t i = 0; i < service.drivers().size(); ++i)
            result[i] = driverJson(service.drivers()[i]);
        return result;
    });
    CROW_ROUTE(app, "/api/passengers").methods(crow::HTTPMethod::GET)([&service] {
        wvalue result;
        for (size_t i = 0; i < service.passengers().size(); ++i)
            result[i] = passengerJson(service.passengers()[i]);
        return result;
    });

    CROW_ROUTE(app, "/api/drivers").methods(crow::HTTPMethod::POST)
    ([&service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasString(body, "name") || !hasString(body, "origin") ||
            !hasString(body, "destination") || !hasString(body, "departureTime") ||
            !hasNumber(body, "availableSeats") || !hasNumber(body, "maxWaitMinutes") ||
            !hasNumber(body, "maxDetourMinutes"))
            return crow::response(400, errorJson("Missing or invalid driver fields."));
        try {
            Driver d;
            d.name = body["name"].s(); d.origin = body["origin"].s();
            d.destination = body["destination"].s();
            if (!service.isLocation(d.origin) || !service.isLocation(d.destination))
                return crow::response(400, errorJson("Origin and destination must be selected from the listed carpool locations."));
            d.departureMinutes = parseTime(body["departureTime"].s());
            d.availableSeats = body["availableSeats"].i();
            d.maxWaitMinutes = body["maxWaitMinutes"].i();
            d.maxDetourMinutes = body["maxDetourMinutes"].i();
            if (d.availableSeats < 1 || d.maxWaitMinutes < 0 || d.maxDetourMinutes < 0)
                return crow::response(400, errorJson("Seats must be positive; wait and detour cannot be negative."));
            int id = service.addDriver(d);
            wvalue result; result["id"] = id; result["message"] = "Driver registered.";
            return crow::response(201, result);
        } catch (const exception& e) { return crow::response(400, errorJson(e.what())); }
    });

    CROW_ROUTE(app, "/api/passengers").methods(crow::HTTPMethod::POST)
    ([&service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasString(body, "name") || !hasString(body, "pickup") ||
            !hasString(body, "dropoff") || !hasString(body, "preferredPickupTime") ||
            !hasNumber(body, "earlyToleranceMinutes") || !hasNumber(body, "lateToleranceMinutes") ||
            !hasNumber(body, "seatsNeeded"))
            return crow::response(400, errorJson("Missing or invalid passenger fields."));
        try {
            Passenger p;
            p.name = body["name"].s(); p.pickup = body["pickup"].s();
            p.dropoff = body["dropoff"].s();
            if (!service.isLocation(p.pickup) || !service.isLocation(p.dropoff))
                return crow::response(400, errorJson("Pickup and drop-off must be selected from the listed carpool locations."));
            p.preferredPickupMinutes = parseTime(body["preferredPickupTime"].s());
            p.earlyToleranceMinutes = body["earlyToleranceMinutes"].i();
            p.lateToleranceMinutes = body["lateToleranceMinutes"].i();
            p.seatsNeeded = body["seatsNeeded"].i();
            if (p.earlyToleranceMinutes < 0 || p.lateToleranceMinutes < 0 || p.seatsNeeded < 1)
                return crow::response(400, errorJson("Tolerances cannot be negative and seats must be positive."));
            int id = service.addPassenger(p);
            wvalue result; result["id"] = id; result["message"] = "Passenger registered.";
            return crow::response(201, result);
        } catch (const exception& e) { return crow::response(400, errorJson(e.what())); }
    });

    CROW_ROUTE(app, "/api/matches/passenger/<int>").methods(crow::HTTPMethod::GET)
    ([&service](int id) {
        auto matches = service.findDriversForPassenger(id);
        wvalue result;
        for (size_t i = 0; i < matches.size(); ++i) result[i] = matchJson(matches[i]);
        return result;
    });
    CROW_ROUTE(app, "/api/matches/driver/<int>").methods(crow::HTTPMethod::GET)
    ([&service](int id) {
        auto matches = service.findPassengersForDriver(id);
        wvalue result;
        for (size_t i = 0; i < matches.size(); ++i) result[i] = matchJson(matches[i]);
        return result;
    });

    CROW_ROUTE(app, "/api/bookings").methods(crow::HTTPMethod::GET)([&service] {
        wvalue result;
        for (size_t i = 0; i < service.bookings().size(); ++i)
            result[i] = bookingJson(service.bookings()[i], service);
        return result;
    });
    CROW_ROUTE(app, "/api/bookings").methods(crow::HTTPMethod::POST)
    ([&service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !hasNumber(body, "driverId") || !hasNumber(body, "passengerId"))
            return crow::response(400, errorJson("driverId and passengerId are required."));
        int bookingId = 0; string error;
        if (!service.createBooking(body["driverId"].i(), body["passengerId"].i(), bookingId, error))
            return crow::response(409, errorJson(error));
        wvalue result; result["id"] = bookingId;
        result["message"] = "Ride booked successfully!";
        for (const auto& booking : service.bookings()) if (booking.id == bookingId) {
            result["booking"] = bookingJson(booking, service);
            break;
        }
        return crow::response(201, result);
    });
    CROW_ROUTE(app, "/api/bookings/<int>").methods(crow::HTTPMethod::Delete)
    ([&service](int id) {
        string error;
        if (!service.cancelBooking(id, error)) return crow::response(404, errorJson(error));
        wvalue result; result["message"] = "Booking cancelled and seats released.";
        return crow::response(result);
    });

    app.port(18080).multithreaded().run();
}
