#include "Graph.h"
#include "CarpoolService.h"
#include "RoadNetworkData.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

int main() {
    Graph graph;
    graph.addLocation("A");
    graph.addLocation("D");
    graph.addRoad("A", "B", 8.0, "test fixture");
    graph.addRoad("A", "C", 2.0, "test fixture");
    graph.addRoad("C", "B", 2.0, "test fixture");
    graph.addRoad("B", "D", 1.0, "test fixture");
    graph.addRoad("C", "D", 7.0, "test fixture");

    const PathResult shortest = graph.shortestPath("A", "D");
    if (!shortest.reachable || std::abs(shortest.distanceKm - 5.0) >= 1e-9 ||
        shortest.estimatedTravelMinutes != 10 ||
        shortest.path != vector<string>{"A", "C", "B", "D"}) {
        std::cerr << "Dijkstra distance, estimate, or reconstructed path is incorrect.\n";
        return 1;
    }

    const PathResult sameLocation = graph.shortestPath("A", "A");
    if (!sameLocation.reachable || sameLocation.distanceKm != 0.0 ||
        sameLocation.estimatedTravelMinutes != 0) {
        std::cerr << "Same-location route is incorrect.\n";
        return 1;
    }

    const PathResult disconnected = graph.shortestPath("D", "missing");
    Graph oneWayGraph;
    oneWayGraph.addRoad("from", "to", 1.0, "test fixture", false);
    if (disconnected.reachable || oneWayGraph.shortestPath("to", "from").reachable ||
        graph.roads().size() != 5 ||
        graph.roads().front().distanceSource != "test fixture") {
        std::cerr << "Disconnected route or edge source handling is incorrect.\n";
        return 1;
    }

    const vector<string> expectedLocations = {
        "JIIT Sector 128, Noida",
        "Botanical Garden Metro Station, Noida",
        "Noida Sector 61, Noida",
        "Noida Sector 59, Noida",
        "Noida Sector 62, Noida",
        "Noida Electronic City Metro Station, Noida",
        "JIIT Sector 62, Noida",
        "Indirapuram Habitat Centre, Ghaziabad"
    };
    set<pair<string, string>> primaryConnections;
    for (const RoutedRoadDistance& route : ROAD_DISTANCE_DATA) {
        const string from = route.from;
        const string to = route.to;
        if (find(expectedLocations.begin(), expectedLocations.end(), from) == expectedLocations.end() ||
            find(expectedLocations.begin(), expectedLocations.end(), to) == expectedLocations.end() ||
            !std::isfinite(route.distanceKm) || route.distanceKm <= 0.0) {
            std::cerr << "Sourced route data contains an invalid location or distance.\n";
            return 1;
        }
        if (route.alternativeIndex == 0) primaryConnections.insert({from, to});
    }
    if (primaryConnections.size() != expectedLocations.size() * (expectedLocations.size() - 1) ||
        ROAD_DISTANCE_SOURCE == nullptr || ROAD_DISTANCE_SOURCE[0] == '\0') {
        std::cerr << "Sourced route data is missing primary routes or attribution.\n";
        return 1;
    }

    CarpoolService service;
    const auto& drivers = service.drivers();
    const auto& passengers = service.passengers();
    const auto driver = find_if(drivers.begin(), drivers.end(), [](const Driver& item) {
        return item.name == "Nishita";
    });
    const auto passenger = find_if(passengers.begin(), passengers.end(), [](const Passenger& item) {
        return item.name == "Navya";
    });
    if (driver == drivers.end() || passenger == passengers.end() ||
        drivers.size() != 2 || passengers.size() != 2 ||
        driver->origin != "JIIT Sector 62, Noida" ||
        driver->destination != "JIIT Sector 128, Noida" ||
        driver->departureMinutes != 8 * 60 ||
        driver->availableSeats != 2 ||
        driver->maxWaitMinutes != 10 ||
        driver->maxDetourMinutes != 25 ||
        passenger->pickup != "Noida Sector 59, Noida" ||
        passenger->dropoff != "JIIT Sector 128, Noida" ||
        passenger->preferredPickupMinutes != 8 * 60 + 20 ||
        passenger->earlyToleranceMinutes != 10 ||
        passenger->lateToleranceMinutes != 15 ||
        passenger->seatsNeeded != 1) {
        std::cerr << "Default driver or passenger data is incorrect.\n";
        return 1;
    }

    const vector<Match> matches = service.findDriversForPassenger(passenger->id);
    const auto compatible = find_if(matches.begin(), matches.end(), [driver](const Match& match) {
        return match.driverId == driver->id;
    });
    if (compatible == matches.end() || compatible->route.empty() ||
        compatible->originalRoute.empty() ||
        compatible->route.front() != driver->origin ||
        compatible->route.back() != driver->destination ||
        compatible->originalRoute.front() != driver->origin ||
        compatible->originalRoute.back() != driver->destination ||
        compatible->pickupRouteIndex < 0 ||
        compatible->pickupRouteIndex >= static_cast<int>(compatible->route.size()) ||
        compatible->passengerDestinationRouteIndex < compatible->pickupRouteIndex ||
        compatible->passengerDestinationRouteIndex >= static_cast<int>(compatible->route.size()) ||
        compatible->route[compatible->pickupRouteIndex] != passenger->pickup ||
        compatible->route[compatible->passengerDestinationRouteIndex] != passenger->dropoff ||
        find(compatible->route.begin(), compatible->route.end(), passenger->pickup) == compatible->route.end() ||
        find(compatible->route.begin(), compatible->route.end(), passenger->dropoff) == compatible->route.end() ||
        count(compatible->route.begin(), compatible->route.end(), driver->destination) != 1 ||
        adjacent_find(compatible->route.begin(), compatible->route.end()) != compatible->route.end()) {
        std::cerr << "Navya should match Nishita when the existing route constraints permit it.\n";
        return 1;
    }

    const vector<Road> serviceRoads = service.roads();
    for (size_t i = 1; i < compatible->route.size(); ++i) {
        const bool isRoadEdge = any_of(serviceRoads.begin(), serviceRoads.end(), [&](const Road& road) {
            return road.from == compatible->route[i - 1] && road.to == compatible->route[i];
        });
        if (!isRoadEdge) {
            std::cerr << "Shared route contains a step that is not a sourced road edge.\n";
            return 1;
        }
    }

    const int earliestPickup = passenger->preferredPickupMinutes - passenger->earlyToleranceMinutes;
    const int expectedDriverWait = max(0, earliestPickup - compatible->pickupArrivalMinutes);
    const int expectedPassengerWait = max(0,
        compatible->pickupArrivalMinutes - passenger->preferredPickupMinutes);
    if (compatible->passengerEarliestPickupMinutes != earliestPickup ||
        compatible->passengerLatestPickupMinutes !=
            passenger->preferredPickupMinutes + passenger->lateToleranceMinutes ||
        compatible->pickupArrivalWithinWindow !=
            (compatible->pickupArrivalMinutes >= earliestPickup &&
             compatible->pickupArrivalMinutes <= compatible->passengerLatestPickupMinutes) ||
        compatible->driverWaitMinutes != expectedDriverWait ||
        compatible->passengerWaitMinutes != expectedDriverWait ||
        compatible->passengerWaitByArrivalMinutes != expectedPassengerWait ||
        std::abs(compatible->detourDistanceKm -
            max(0.0, compatible->sharedRouteDistanceKm - compatible->baseRouteDistanceKm)) >= 1e-9) {
        std::cerr << "Match time-window, wait, or detour estimates are inconsistent.\n";
        return 1;
    }
    return 0;
}
