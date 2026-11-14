enable_testing()

find_package(GTest "1.11.0" REQUIRED COMPONENTS GTest PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)

function(add_gtest test_name)
    add_executable(${test_name} ${ARGN} main.cpp)
    target_link_libraries(${test_name} PRIVATE tyr::core TBB::tbb GTest::gtest)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES FAIL_REGULAR_EXPRESSION "does NOT link in any test case;Running 0 tests")
endfunction()

# Compile related repository families together while keeping one source per public family.

add_gtest(
    formalism_core_types
    "formalism/row.cpp"
    "formalism/function.cpp"
    "formalism/object.cpp"
    "formalism/parameter.cpp"
    "formalism/predicate.cpp"
    "formalism/term.cpp"
    "formalism/variable.cpp"
)

add_gtest(
    formalism_datalog_types
    "formalism/datalog/atom.cpp"
    "formalism/datalog/binary_operator.cpp"
    "formalism/datalog/conjunctive_condition.cpp"
    "formalism/datalog/conjunctive_effect.cpp"
    "formalism/datalog/function_term.cpp"
    "formalism/datalog/function_term_value.cpp"
    "formalism/datalog/literal.cpp"
    "formalism/datalog/numeric_effect.cpp"
    "formalism/datalog/program.cpp"
    "formalism/datalog/rule.cpp"
    "formalism/datalog/multi_operator.cpp"
    "formalism/datalog/unary_operator.cpp"
)

add_gtest(
    formalism_planning_types
    "formalism/planning/action.cpp"
    "formalism/planning/atom.cpp"
    "formalism/planning/axiom.cpp"
    "formalism/planning/binary_operator.cpp"
    "formalism/planning/conditional_effect.cpp"
    "formalism/planning/conjunctive_condition.cpp"
    "formalism/planning/conjunctive_effect.cpp"
    "formalism/planning/domain.cpp"
    "formalism/planning/fdr_fact.cpp"
    "formalism/planning/fdr_variable.cpp"
    "formalism/planning/function_term.cpp"
    "formalism/planning/function_term_value.cpp"
    "formalism/planning/literal.cpp"
    "formalism/planning/numeric_effect.cpp"
    "formalism/planning/metric.cpp"
    "formalism/planning/multi_operator.cpp"
    "formalism/planning/task.cpp"
    "formalism/planning/unary_operator.cpp"
)

add_gtest(
    planning_match_tree_types
    "planning/ground/match_tree/nodes/atom.cpp"
    "planning/ground/match_tree/nodes/constraint.cpp"
    "planning/ground/match_tree/nodes/generator.cpp"
    "planning/ground/match_tree/nodes/negative_fact.cpp"
    "planning/ground/match_tree/nodes/variable.cpp"
)

# Type tests of one directory include the same headers and check their types against them: batched, the headers and
# the types are compiled once (brokkr's template rules, rule 11). Each file has its own namespace, tyr::tests::<path>.
brokkr_unity_directories(formalism_core_types ENABLED ${TYR_UNITY_BUILD} DIRECTORIES formalism)
brokkr_unity_directories(formalism_datalog_types ENABLED ${TYR_UNITY_BUILD} DIRECTORIES formalism/datalog)
brokkr_unity_directories(formalism_planning_types ENABLED ${TYR_UNITY_BUILD} DIRECTORIES formalism/planning)
brokkr_unity_directories(planning_match_tree_types ENABLED ${TYR_UNITY_BUILD} DIRECTORIES planning/ground/match_tree/nodes)

add_gtest(
    planning_state_types
    "planning/ground/state.cpp"
    "planning/lifted/state.cpp"
    "planning/state.cpp"
)

add_gtest(planning_applicability "planning/applicability.cpp" "planning/applicability_schema.cpp")
add_gtest(planning_plan_reconstruction "planning/plan_reconstruction.cpp")
add_gtest(planning_search_engine "planning/search_engine.cpp")
target_include_directories(planning_search_engine PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_gtest(planning_workers "planning/workers.cpp")
add_gtest(planning_state_routing "planning/state_routing.cpp")
target_include_directories(planning_state_routing PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_gtest(formalism_planning_invariants_synthesis "formalism/planning/invariants/synthesis.cpp")
target_link_libraries(formalism_planning_invariants_synthesis PRIVATE Boost::json)
add_gtest(planning_invariant_order "planning/lifted/invariant_order.cpp")
add_gtest(formalism_unification "formalism/unification.cpp")
add_gtest(formalism_numeric_expression_utils "formalism/numeric_expression_utils.cpp")
add_gtest(algorithms_kckp "algorithms/kckp/kckp.cpp")
target_include_directories(algorithms_kckp PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_gtest(datalog_cost_policy "datalog/cost_policy.cpp")
add_gtest(datalog_ground_solver "datalog/ground/solver.cpp")
add_gtest(datalog_lifted_assignment_sets "datalog/assignment_sets.cpp")
add_gtest(datalog_lifted_solver "datalog/lifted/solver.cpp")
target_link_libraries(datalog_lifted_solver PRIVATE Boost::iostreams Boost::json)
add_gtest(algorithms_delta_kckp "algorithms/kckp/delta_kckp.cpp")

# The database engine (datalog/lifted/database).
add_gtest(datalog_database_allocations "datalog/lifted/database/allocations.cpp")
add_gtest(datalog_database_distance "datalog/lifted/database/distance.cpp")
add_gtest(datalog_database_dpccp "datalog/lifted/database/dpccp.cpp")
add_gtest(datalog_database_formatter "datalog/lifted/database/formatter.cpp")
add_gtest(datalog_database_generic_join "datalog/lifted/database/generic_join.cpp")
add_gtest(datalog_database_incremental "datalog/lifted/database/incremental.cpp")
add_gtest(datalog_database_incremental_min_distance "datalog/lifted/database/incremental_min_distance.cpp")
add_gtest(datalog_database_min_distance "datalog/lifted/database/min_distance.cpp")
add_gtest(datalog_database_must_pass "datalog/lifted/database/must_pass.cpp")
add_gtest(datalog_database_operations "datalog/lifted/database/operations.cpp")
add_gtest(datalog_database_optimization "datalog/lifted/database/optimization.cpp")
add_gtest(datalog_database_optimization_estimates "datalog/lifted/database/optimization_estimates.cpp")
add_gtest(datalog_database_query "datalog/lifted/database/query.cpp")
add_gtest(datalog_database_relation_repository "datalog/lifted/database/relation_repository.cpp")
add_gtest(datalog_database_relaxation "datalog/lifted/database/relaxation.cpp")
add_gtest(datalog_database_typed_operations "datalog/lifted/database/typed_operations.cpp")
target_link_libraries(datalog_database_allocations PRIVATE urd::count_allocations)

foreach(heuristic rpg_max rpg_add rpg_ff lmcut)
    add_gtest(planning_heuristics_${heuristic} "planning/heuristics/${heuristic}.cpp")
    target_link_libraries(planning_heuristics_${heuristic} PRIVATE Boost::json)
endforeach()

foreach(algorithm brfs astar gbfs iw siw)
    add_gtest(planning_algorithms_statistics_${algorithm} "planning/algorithms/statistics/${algorithm}.cpp")
    target_link_libraries(planning_algorithms_statistics_${algorithm} PRIVATE Boost::json)
endforeach()
add_gtest(planning_algorithms_utils "planning/algorithms/utils.cpp")
add_gtest(planning_algorithms_iw_novelty_table "planning/algorithms/iw/novelty_table.cpp")
add_gtest(planning_algorithms_serialized "planning/algorithms/serialized.cpp")
target_link_libraries(planning_algorithms_serialized PRIVATE Boost::json)

add_gtest(serialization_planning "serialization/planning.cpp" "serialization/planning_runtime.cpp")
target_link_libraries(serialization_planning PRIVATE Boost::json)
