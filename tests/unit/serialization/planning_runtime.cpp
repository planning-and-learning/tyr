#include "planning_fixtures.hpp"

#include <gtest/gtest.h>
#include <tyr/serialization/planning/lifted/task.hpp>

namespace tyr::tests
{

TEST(TyrSerialization, GroundPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<GroundTag>(); }

TEST(TyrSerialization, LiftedPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<LiftedTag>(); }

}  // namespace tyr::tests
