#include "planning_fixtures.hpp"

#include <gtest/gtest.h>

namespace tyr::tests
{

TEST(TyrSerialization, GroundPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<GroundTag>(); }

}  // namespace tyr::tests
