#include "Matching.h"
#include <algorithm>
#include <limits>
#include <utility>

using namespace std;

MatchingEngine::MatchingEngine(const Graph& roadGraph) : graph(roadGraph) {}

optional<Match> MatchingEngine::evaluate(const Driver& driver,
                                               const Passenger& passenger) const {
    if (driver.availableSeats < passenger.seatsNeeded) return nullopt;

    PathResult base = graph.shortestPath(driver.origin, driver.destination);
    if (!base.reachable) return nullopt;

    const int earliest = passenger.preferredPickupMinutes - passenger.earlyToleranceMinutes;
    const int latest = passenger.preferredPickupMinutes + passenger.lateToleranceMinutes;

    // Try every valid insertion position: pickup must come before drop-off.
    // This supports both a passenger joining the existing route and a driver
    // making a detour to collect/drop the passenger.
    int bestDetour = numeric_limits<int>::max();
    int bestWait = numeric_limits<int>::max();
    Match best;
    bool found = false;

    // Keep both passenger stops before the driver's final destination.
    for (size_t pickupIndex = 1; pickupIndex < base.path.size(); ++pickupIndex) {
        for (size_t dropIndex = pickupIndex + 1; dropIndex <= base.path.size(); ++dropIndex) {
            vector<string> stops = base.path;
            stops.insert(stops.begin() + static_cast<long>(pickupIndex), passenger.pickup);
            stops.insert(stops.begin() + static_cast<long>(dropIndex), passenger.dropoff);

            int total = 0;
            double totalDistanceKm = 0.0;
            int pickupArrival = 0;
            int pickupRouteIndex = -1;
            int passengerDestinationRouteIndex = -1;
            vector<string> sharedRoute;
            bool valid = true;

            for (size_t i = 1; i < stops.size(); ++i) {
                PathResult leg = graph.shortestPath(stops[i - 1], stops[i]);
                if (!leg.reachable) {
                    valid = false;
                    break;
                }
                total += leg.estimatedTravelMinutes;
                totalDistanceKm += leg.distanceKm;
                for (const string& location : leg.path) {
                    if (sharedRoute.empty() || sharedRoute.back() != location)
                        sharedRoute.push_back(location);
                }
                if (i == pickupIndex) {
                    pickupArrival = driver.departureMinutes + total;
                    pickupRouteIndex = static_cast<int>(sharedRoute.size()) - 1;
                }
                if (i == dropIndex)
                    passengerDestinationRouteIndex = static_cast<int>(sharedRoute.size()) - 1;
            }
            if (!valid) continue;

            int detour = total - base.estimatedTravelMinutes;
            if (detour < 0) detour = 0;
            if (detour > driver.maxDetourMinutes) continue;
            if (pickupArrival > latest) continue;

            int wait = max(0, earliest - pickupArrival);
            if (wait > driver.maxWaitMinutes) continue;

            if (!found || detour < bestDetour || (detour == bestDetour && wait < bestWait)) {
                found = true;
                bestDetour = detour;
                bestWait = wait;
                best.driverId = driver.id;
                best.passengerId = passenger.id;
                best.route = move(sharedRoute);
                best.originalRoute = base.path;
                best.pickupRouteIndex = pickupRouteIndex;
                best.passengerDestinationRouteIndex = passengerDestinationRouteIndex;
                best.baseRouteDistanceKm = base.distanceKm;
                best.sharedRouteDistanceKm = totalDistanceKm;
                best.detourDistanceKm = max(0.0, totalDistanceKm - base.distanceKm);
                best.baseRouteMinutes = base.estimatedTravelMinutes;
                best.sharedRouteMinutes = total;
                best.detourMinutes = detour;
                best.pickupArrivalMinutes = pickupArrival;
                best.passengerWaitMinutes = wait;
                best.driverWaitMinutes = wait;
                best.passengerWaitByArrivalMinutes =
                    max(0, pickupArrival - passenger.preferredPickupMinutes);
                best.passengerEarliestPickupMinutes = earliest;
                best.passengerLatestPickupMinutes = latest;
                best.pickupArrivalWithinWindow =
                    pickupArrival >= earliest && pickupArrival <= latest;
            }
        }
    }

    if (!found) return nullopt;
    return best;
}
