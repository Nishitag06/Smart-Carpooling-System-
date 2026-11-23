#pragma once
#include <string>
#include <vector>

using namespace std;

struct Driver {
    int id{};
    string name;
    string origin;
    string destination;
    int departureMinutes{};
    int availableSeats{};
    int maxWaitMinutes{};
    int maxDetourMinutes{};
};

struct Passenger {
    int id{};
    string name;
    string pickup;
    string dropoff;
    int preferredPickupMinutes{};
    int earlyToleranceMinutes{};
    int lateToleranceMinutes{};
    int seatsNeeded{1};
};

struct Match {
    int driverId{};
    int passengerId{};
    vector<string> route;
    double baseRouteDistanceKm{};
    double sharedRouteDistanceKm{};
    int baseRouteMinutes{};
    int sharedRouteMinutes{};
    int detourMinutes{};
    int pickupArrivalMinutes{};
    int passengerWaitMinutes{};
    int passengerLatestPickupMinutes{};
};

struct Booking {
    int id{};
    int driverId{};
    int passengerId{};
    int seatsBooked{};
    bool active{true};
    Match match;
};
