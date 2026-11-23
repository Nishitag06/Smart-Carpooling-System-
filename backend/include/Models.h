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
    vector<string> originalRoute;
    int pickupRouteIndex{-1};
    int passengerDestinationRouteIndex{-1};
    double baseRouteDistanceKm{};
    double sharedRouteDistanceKm{};
    double detourDistanceKm{};
    int baseRouteMinutes{};
    int sharedRouteMinutes{};
    int detourMinutes{};
    int pickupArrivalMinutes{};
    int passengerWaitMinutes{};
    int driverWaitMinutes{};
    int passengerWaitByArrivalMinutes{};
    int passengerEarliestPickupMinutes{};
    int passengerLatestPickupMinutes{};
    bool pickupArrivalWithinWindow{};
};

struct Booking {
    int id{};
    int driverId{};
    int passengerId{};
    int seatsBooked{};
    bool active{true};
    Match match;
};
