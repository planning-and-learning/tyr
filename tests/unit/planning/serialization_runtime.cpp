#include "serialization_fixtures.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/plans/plan_view.hpp"

#include <gtest/gtest.h>

namespace tyr::tests
{

TEST(TyrSerialization, GroundPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<GroundTag>(); }

TEST(TyrSerialization, LiftedPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<LiftedTag>(); }

}  // namespace tyr::tests
