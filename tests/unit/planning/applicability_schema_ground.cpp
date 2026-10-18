#include "applicability_schema.hpp"

#include <gtest/gtest.h>

namespace tyr::tests
{

TEST(TyrPlanningApplicabilityTest, GroundSchemaQueriesMatchFilteredSuccessors)
{
    expect_schema_queries_match_filtered_successors<GroundTag>();
    expect_schema_queries_match_filtered_successors<GroundTag, true>();
}

}  // namespace tyr::tests
