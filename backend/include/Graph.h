#pragma once
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Edge {
    string destination;
    double distanceKm{};
    string distanceSource;
};

struct Road {
    string from;
    string to;
    double distanceKm{};
    string distanceSource;
    bool twoWay{true};
};

struct PathResult {
    bool reachable{false};
    double distanceKm{};
    int estimatedTravelMinutes{};
    vector<string> path;
};

class Graph {
public:
    static constexpr double AVERAGE_SPEED_KMH = 30.0;

    void addLocation(const string& name);
    void addRoad(const string& from, const string& to, double distanceKm,
                 const string& distanceSource, bool twoWay = true);
    PathResult shortestPath(const string& from, const string& to) const;
    vector<string> locations() const;
    vector<Road> roads() const;
    bool hasLocation(const string& name) const;

private:
    unordered_map<string, vector<Edge>> adjacency;
    vector<string> locationOrder;
    vector<Road> roadList;
};
