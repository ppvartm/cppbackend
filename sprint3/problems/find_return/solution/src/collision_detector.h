#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace collision_detector {

    struct Point {
         double x;
         double y;
    };

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }
    double sq_distance;
    double proj_ratio;
};

CollectionResult TryCollectPoint(Point a, Point b, Point c);

struct Item {
    Point position;
    double width;
};

struct Gatherer {
    Point start_pos;
    Point end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};


std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector