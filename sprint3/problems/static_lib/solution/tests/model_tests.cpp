#include <catch2/catch_test_macros.hpp>
#include "../src/model.h"


TEST_CASE("test model::Dog") {
	model::Dog dog1("bob");
	CHECK(dog1.GetName() == "bob");
}