#include "applicability_schema.hpp"

#include <gtest/gtest.h>

namespace tyr::tests
{

TEST(TyrPlanningApplicabilityTest, LiftedSchemaQueriesMatchFilteredSuccessors)
{
    expect_schema_queries_match_filtered_successors<LiftedTag>();
    expect_schema_queries_match_filtered_successors<LiftedTag, true>();
}

}  // namespace tyr::tests
