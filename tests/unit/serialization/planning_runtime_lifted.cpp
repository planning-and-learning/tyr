#include "planning_fixtures.hpp"

#include <gtest/gtest.h>

namespace tyr::tests
{

TEST(TyrSerialization, LiftedPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<LiftedTag>(); }

}  // namespace tyr::tests
