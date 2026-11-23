#pragma once
#include "Graph.h"
#include "Models.h"
#include <optional>

using namespace std;

class MatchingEngine {
public:
    explicit MatchingEngine(const Graph& roadGraph);
    optional<Match> evaluate(const Driver& driver, const Passenger& passenger) const;

private:
    const Graph& graph;
};
