#include "catch2_common.h"

#include <numeric>
#include <tango/server/dserversignal.h>

SCENARIO("Synchronised queue delivers all values from one thread to another")
{
    GIVEN("a synchronised queue")
    {
        constexpr std::size_t num_values = 10000;
        auto queue = Tango::SynchronisedQueue<int>();
        WHEN("we enqeuue " << num_values << " running values from one thread")
        {
            auto enqueing_thread = std::thread{[&]()
                                               {
                                                   for(std::size_t i = 0; i != num_values; i++)
                                                   {
                                                       queue.put(i);
                                                   }
                                               }};

            AND_WHEN("another thread pops them from the same queue")
            {
                std::vector<int> consumed_values;
                auto consumer_thread = std::thread{[&]()
                                                   {
                                                       for(std::size_t i = 0; i != num_values; i++)
                                                       {
                                                           consumed_values.push_back(queue.get());
                                                       }
                                                   }};

                AND_WHEN("the enqueing thread finishes")
                {
                    REQUIRE_NOTHROW(enqueing_thread.join());
                    THEN("all values are eventually correctly consumed, in order")
                    {
                        REQUIRE_NOTHROW(consumer_thread.join());
                        REQUIRE(consumed_values.size() == num_values);
                        REQUIRE(consumed_values[0] == 0);
                        std::vector<int> diffs(consumed_values.size(), 0);
                        std::adjacent_difference(consumed_values.begin(), consumed_values.end(), diffs.begin());
                        // first element in diff is the value in consumed_values.begin()
                        REQUIRE(std::all_of(diffs.begin() + 1, diffs.end(), [](int diff) { return diff == 1; }));
                    }
                }
            }
        }
    }
}
