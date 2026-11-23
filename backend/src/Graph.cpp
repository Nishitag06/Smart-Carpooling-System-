#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <functional>

using namespace std;

void Graph::addLocation(const string& name) {
    if (name.empty()) throw invalid_argument("Location name cannot be empty.");
    if (adjacency.try_emplace(name).second) locationOrder.push_back(name);
}

void Graph::addRoad(const string& from, const string& to, double distanceKm,
                    const string& distanceSource, bool twoWay) {
    if (from.empty() || to.empty() || from == to)
        throw invalid_argument("Road endpoints must be distinct, named locations.");
    if (!isfinite(distanceKm) || distanceKm <= 0.0)
        throw invalid_argument("Road distance must be a positive finite number of kilometres.");
    if (distanceSource.empty())
        throw invalid_argument("Road distance source is required.");

    addLocation(from);
    addLocation(to);
    roadList.push_back({from, to, distanceKm, distanceSource, twoWay});
    adjacency[from].push_back({to, distanceKm, distanceSource});
    if (twoWay) adjacency[to].push_back({from, distanceKm, distanceSource});
}

bool Graph::hasLocation(const string& name) const {
    return adjacency.find(name) != adjacency.end();
}

vector<string> Graph::locations() const {
    return locationOrder;
}

vector<Road> Graph::roads() const {
    return roadList;
}

PathResult Graph::shortestPath(const string& from, const string& to) const {
    PathResult result;
    if (!hasLocation(from) || !hasLocation(to)) return result;
    if (from == to) return {true, 0.0, 0, {from}};

    const double INF = numeric_limits<double>::infinity();
    unordered_map<string, double> distance;
    unordered_map<string, string> parent;

    for (const auto& item : adjacency) distance[item.first] = INF;
    distance[from] = 0.0;

    using QueueItem = pair<double, string>;
    priority_queue<QueueItem, vector<QueueItem>, greater<QueueItem>> pq;
    pq.push({0.0, from});

    while (!pq.empty()) {
        const QueueItem currentItem = pq.top();
        pq.pop();
        const double currentDistance = currentItem.first;
        const string& current = currentItem.second;
        if (currentDistance > distance[current]) continue;
        if (current == to) break;

        for (const Edge& edge : adjacency.at(current)) {
            double candidate = currentDistance + edge.distanceKm;
            if (candidate < distance[edge.destination]) {
                distance[edge.destination] = candidate;
                parent[edge.destination] = current;
                pq.push({candidate, edge.destination});
            }
        }
    }

    if (!isfinite(distance[to])) return result;

    result.reachable = true;
    result.distanceKm = distance[to];
    result.estimatedTravelMinutes = static_cast<int>(
        ceil(result.distanceKm * 60.0 / AVERAGE_SPEED_KMH));
    string current = to;
    while (current != from) {
        result.path.push_back(current);
        current = parent.at(current);
    }
    result.path.push_back(from);
    reverse(result.path.begin(), result.path.end());
    return result;
}
