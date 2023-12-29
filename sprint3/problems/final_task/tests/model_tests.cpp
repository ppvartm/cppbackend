#include "catch2/catch_test_macros.hpp"
#include "../src/model/model.h"





TEST_CASE("full dog's bag") {
	std::vector<model::LostObject> lost_objects = { {0, {2, 1}, 10}, {1, {3, 1}, 30}, {2, {2, 8}, 40},
		                                            {3, {2, 5}, 80}, {4, {2, 5}, 20}, {5, {2, 1}, 10} };
	model::Dog dog("Teddy");
	dog.SetBagCapacity(5);
	for (int i = 0; i < lost_objects.size(); ++i) //6 iteration
		dog.AddObjectInBag(i, lost_objects[i]);

	CHECK(dog.GetBag().size() == 5);

}