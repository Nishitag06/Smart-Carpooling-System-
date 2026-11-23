#include "CarpoolService.h"
#include "RoadNetworkData.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

using namespace std;

CarpoolService::CarpoolService() : matching(graph) {
    loadRoadNetwork();
    loadSampleUsers();
}

void CarpoolService::loadRoadNetwork() {
    const vector<string> places = {
        "JIIT Sector 128, Noida",
        "Botanical Garden Metro Station, Noida",
        "Noida Sector 61, Noida",
        "Noida Sector 59, Noida",
        "Noida Sector 62, Noida",
        "Noida Electronic City Metro Station, Noida",
        "JIIT Sector 62, Noida",
        "Indirapuram Habitat Centre, Ghaziabad"
    };
    for (const auto& place : places) graph.addLocation(place);

    for (const RoutedRoadDistance& route : ROAD_DISTANCE_DATA) {
        const string routeLabel = route.alternativeIndex == 0
            ? "primary route"
            : "alternative route " + to_string(route.alternativeIndex);
        graph.addRoad(route.from, route.to, route.distanceKm,
                      string(ROAD_DISTANCE_SOURCE) + "; " + routeLabel, false);
    }
}

void CarpoolService::loadSampleUsers() {
    driverList.push_back({nextDriverId++, "Nishita", "JIIT Sector 62, Noida",
                          "JIIT Sector 128, Noida",
                          8 * 60, 2, 10, 25});
    driverList.push_back({nextDriverId++, "Meera", "JIIT Sector 62, Noida",
                          "Indirapuram Habitat Centre, Ghaziabad",
                          8 * 60 + 15, 3, 8, 30});

    passengerList.push_back({nextPassengerId++, "Navya", "Noida Sector 59, Noida",
                             "JIIT Sector 128, Noida", 8 * 60 + 20, 10, 15, 1});
    passengerList.push_back({nextPassengerId++, "Kabir", "Noida Sector 59, Noida",
                             "Noida Electronic City Metro Station, Noida",
                             8 * 60 + 35, 5, 20, 1});
}

const vector<Driver>& CarpoolService::drivers() const { return driverList; }
const vector<Passenger>& CarpoolService::passengers() const { return passengerList; }
const vector<Booking>& CarpoolService::bookings() const { return bookingList; }
vector<string> CarpoolService::locations() const { return graph.locations(); }
vector<Road> CarpoolService::roads() const { return graph.roads(); }
bool CarpoolService::isLocation(const string& name) const { return graph.hasLocation(name); }

Driver* CarpoolService::getDriver(int id) {
    for (auto& item : driverList) if (item.id == id) return &item;
    return nullptr;
}
Passenger* CarpoolService::getPassenger(int id) {
    for (auto& item : passengerList) if (item.id == id) return &item;
    return nullptr;
}

int CarpoolService::addDriver(const Driver& input) {
    Driver d = input;
    d.id = nextDriverId++;
    driverList.push_back(d);
    return d.id;
}
int CarpoolService::addPassenger(const Passenger& input) {
    Passenger p = input;
    p.id = nextPassengerId++;
    passengerList.push_back(p);
    return p.id;
}

vector<Match> CarpoolService::findDriversForPassenger(int passengerId) const {
    vector<Match> result;
    const Passenger* passenger = nullptr;
    for (const auto& p : passengerList) if (p.id == passengerId) passenger = &p;
    if (!passenger) return result;

    for (const auto& driver : driverList) {
        int alreadyBooked = 0;
        for (const auto& b : bookingList)
            if (b.active && b.driverId == driver.id) alreadyBooked += b.seatsBooked;
        Driver available = driver;
        available.availableSeats -= alreadyBooked;
        auto match = matching.evaluate(available, *passenger);
        if (match) result.push_back(*match);
    }
    sort(result.begin(), result.end(), [](const Match& a, const Match& b) {
        if (a.detourMinutes != b.detourMinutes) return a.detourMinutes < b.detourMinutes;
        return a.passengerWaitMinutes < b.passengerWaitMinutes;
    });
    return result;
}

vector<Match> CarpoolService::findPassengersForDriver(int driverId) const {
    vector<Match> result;
    const Driver* driver = nullptr;
    for (const auto& d : driverList) if (d.id == driverId) driver = &d;
    if (!driver) return result;

    int alreadyBooked = 0;
    for (const auto& b : bookingList)
        if (b.active && b.driverId == driverId) alreadyBooked += b.seatsBooked;
    Driver available = *driver;
    available.availableSeats -= alreadyBooked;

    for (const auto& passenger : passengerList) {
        bool alreadyInThisCar = false;
        for (const auto& b : bookingList)
            if (b.active && b.driverId == driverId && b.passengerId == passenger.id)
                alreadyInThisCar = true;
        if (alreadyInThisCar) continue;
        auto match = matching.evaluate(available, passenger);
        if (match) result.push_back(*match);
    }
    sort(result.begin(), result.end(), [](const Match& a, const Match& b) {
        if (a.detourMinutes != b.detourMinutes) return a.detourMinutes < b.detourMinutes;
        return a.passengerWaitMinutes < b.passengerWaitMinutes;
    });
    return result;
}

bool CarpoolService::createBooking(int driverId, int passengerId, int& bookingId,
                                   string& error) {
    Driver* driver = getDriver(driverId);
    Passenger* passenger = getPassenger(passengerId);
    if (!driver || !passenger) { error = "Driver or passenger not found."; return false; }

    for (const auto& b : bookingList) {
        if (b.active && b.driverId == driverId && b.passengerId == passengerId) {
            error = "This passenger already has an active booking with this driver.";
            return false;
        }
    }

    auto options = findDriversForPassenger(passengerId);
    auto it = find_if(options.begin(), options.end(),
                           [driverId](const Match& m) { return m.driverId == driverId; });
    if (it == options.end()) {
        error = "This driver and passenger are no longer compatible.";
        return false;
    }

    bookingId = nextBookingId++;
    bookingList.push_back({bookingId, driverId, passengerId, passenger->seatsNeeded, true, *it});
    return true;
}

bool CarpoolService::cancelBooking(int bookingId, string& error) {
    for (auto& b : bookingList) {
        if (b.id == bookingId && b.active) {
            b.active = false;
            return true;
        }
    }
    error = "Active booking not found.";
    return false;
}

int parseTime(const string& value) {
    if (value.size() != 5 || value[2] != ':') throw invalid_argument("Time must be HH:MM.");
    int hour = stoi(value.substr(0, 2));
    int minute = stoi(value.substr(3, 2));
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
        throw invalid_argument("Time must be between 00:00 and 23:59.");
    return hour * 60 + minute;
}

string formatTime(int minutes) {
    minutes %= 1440;
    if (minutes < 0) minutes += 1440;
    int hour = minutes / 60, minute = minutes % 60;
    string h = hour < 10 ? "0" : "";
    string m = minute < 10 ? "0" : "";
    return h + to_string(hour) + ":" + m + to_string(minute);
}
