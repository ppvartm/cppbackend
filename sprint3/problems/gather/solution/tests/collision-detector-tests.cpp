#define _USE_MATH_DEFINES
#include "catch2/catch_test_macros.hpp"
#include "../src/collision_detector.h"
#include "catch2/matchers/catch_matchers_templated.hpp"
#include <sstream>
// Напишите здесь тесты для функции collision_detector::FindGatherEvents

class ItemGatherer : public collision_detector::ItemGathererProvider {
public:
    size_t ItemsCount() const {
        return items_.size();
    }
    size_t GatherersCount() const {
        return gatherers_.size();
    }
    collision_detector::Item GetItem(size_t idx) const {
        return items_[idx];
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const {
        return gatherers_[idx];
    }

    void AddItem(collision_detector::Item item) {
        items_.push_back(item);
    }

    void AddGatherer(collision_detector::Gatherer gatherer) {
        gatherers_.push_back(gatherer);
    }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};
namespace Catch {
    template<>
    struct StringMaker<collision_detector::GatheringEvent> {
        static std::string convert(collision_detector::GatheringEvent const& value) {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << value.item_id << value.sq_distance << value.time << ")";

            return tmp.str();
        }
    };
}  // namespace Catch
namespace {

    template <typename Range, typename Predicate>
    struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase {
        EqualsRangeMatcher(Range const& range, Predicate predicate)
            : range_{ range }
            , predicate_{ predicate } {
        }

        template <typename OtherRange>
        bool match(const OtherRange& other) const {
            using std::begin;
            using std::end;

            return std::equal(begin(range_), end(range_), begin(other), end(other), predicate_);
        }

        std::string describe() const override {
            return "Equals: " + Catch::rangeToString(range_);
        }

    private:
        const Range& range_;
        Predicate predicate_;
    };

    template <typename Range, typename Predicate>
    auto EqualsRange(const Range& range, Predicate prediate) {
        return EqualsRangeMatcher<Range, Predicate>{range, prediate};
    }

class CompareEvents {
public:
    bool operator()(const collision_detector::GatheringEvent& l,
        const collision_detector::GatheringEvent& r) {
        if (l.gatherer_id != r.gatherer_id || l.item_id != r.item_id)
            return false;

        static const double eps = 1e-10;

        if (std::abs(l.sq_distance - r.sq_distance) > eps) {
            return false;
        }

        if (std::abs(l.time - r.time) > eps) {
            return false;
        }
        return true;
    }
};
}

TEST_CASE("test collision detector/ count of item") {
	collision_detector::Item item0({ 5.001, 1.99 }, 0.5);
	collision_detector::Gatherer gatherer0({0, 0}, {5, 0}, 1.5);
    collision_detector::Item item1({ 4.99, 1.99 }, 0.5);
	ItemGatherer item_gatherer;
	item_gatherer.AddItem(item0);
    item_gatherer.AddItem(item1);
	item_gatherer.AddGatherer(gatherer0);

	CHECK(collision_detector::FindGatherEvents(item_gatherer).size() == 1);
}

TEST_CASE("test collision detector/ order of event") {
    collision_detector::Item item0({ 4.001, 1.99 }, 0.5);
    collision_detector::Gatherer gatherer0({ 0, 0 }, { 5, 0 }, 1.5);
    collision_detector::Item item1({ 2.99, 1.99 }, 0.5);
    ItemGatherer item_gatherer;
    item_gatherer.AddItem(item0);
    item_gatherer.AddItem(item1);
    item_gatherer.AddGatherer(gatherer0);

    auto events = collision_detector::FindGatherEvents(item_gatherer);

    CHECK(events[0].time < events[1].time);
}

TEST_CASE("test collision detector/ correct data") {
    collision_detector::Item item0({ 2, 2 }, 0.5);
    collision_detector::Gatherer gatherer0({ 0, 0 }, { 5, 0 }, 2);
    ItemGatherer item_gatherer;
    item_gatherer.AddItem(item0);
    item_gatherer.AddGatherer(gatherer0);
    auto res = collision_detector::FindGatherEvents(item_gatherer);
    collision_detector::GatheringEvent event(0, 0, 4, 0.4);
    
    CHECK_THAT(collision_detector::FindGatherEvents(item_gatherer), EqualsRange(std::vector{ event }, CompareEvents()));
}