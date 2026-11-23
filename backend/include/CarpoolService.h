#pragma once
#include "Graph.h"
#include "Matching.h"
#include "Models.h"
#include <string>
#include <vector>

using namespace std;

class CarpoolService {
public:
    CarpoolService();

    const vector<Driver>& drivers() const;
    const vector<Passenger>& passengers() const;
    const vector<Booking>& bookings() const;
    vector<string> locations() const;
    vector<Road> roads() const;
    bool isLocation(const string& name) const;

    int addDriver(const Driver& driver);
    int addPassenger(const Passenger& passenger);
    vector<Match> findDriversForPassenger(int passengerId) const;
    vector<Match> findPassengersForDriver(int driverId) const;
    bool createBooking(int driverId, int passengerId, int& bookingId, string& error);
    bool cancelBooking(int bookingId, string& error);

private:
    Graph graph;
    MatchingEngine matching;
    vector<Driver> driverList;
    vector<Passenger> passengerList;
    vector<Booking> bookingList;
    int nextDriverId{1};
    int nextPassengerId{1};
    int nextBookingId{1};

    void loadRoadNetwork();
    void loadSampleUsers();
    Driver* getDriver(int id);
    Passenger* getPassenger(int id);
};

int parseTime(const string& timeText);
string formatTime(int minutes);
