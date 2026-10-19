#include "planning_fixtures.hpp"

#include <gtest/gtest.h>

namespace tyr::tests
{

TEST(TyrSerialization, GroundPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<GroundTag>(); }

TEST(TyrSerialization, LiftedPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<LiftedTag>(); }

}  // namespace tyr::tests
