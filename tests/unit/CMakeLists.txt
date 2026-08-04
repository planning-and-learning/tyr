enable_testing()

find_package(GTest "1.11.0" REQUIRED COMPONENTS GTest PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)

function(add_gtest test_name)
    add_executable(${test_name} ${ARGN} main.cpp)
    target_link_libraries(${test_name} PRIVATE tyr::core TBB::tbb GTest::gtest)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Compile related repository families together while keeping one source per public family.

add_gtest(
    formalism_core_types
    "formalism/binding.cpp"
    "formalism/function.cpp"
    "formalism/object.cpp"
    "formalism/parameter.cpp"
    "formalism/predicate.cpp"
    "formalism/term.cpp"
    "formalism/variable.cpp"
)

add_gtest(
    formalism_datalog_types
    "formalism/datalog/arithmetic_operator.cpp"
    "formalism/datalog/atom.cpp"
    "formalism/datalog/binary_operator.cpp"
    "formalism/datalog/boolean_operator.cpp"
    "formalism/datalog/conditional_effect.cpp"
    "formalism/datalog/conjunctive_condition.cpp"
    "formalism/datalog/conjunctive_effect.cpp"
    "formalism/datalog/function_expression.cpp"
    "formalism/datalog/function_term.cpp"
    "formalism/datalog/ground_atom.cpp"
    "formalism/datalog/ground_conditional_effect.cpp"
    "formalism/datalog/ground_conjunctive_condition.cpp"
    "formalism/datalog/ground_conjunctive_effect.cpp"
    "formalism/datalog/ground_function_expression.cpp"
    "formalism/datalog/ground_function_term.cpp"
    "formalism/datalog/ground_function_term_value.cpp"
    "formalism/datalog/ground_literal.cpp"
    "formalism/datalog/ground_numeric_effect.cpp"
    "formalism/datalog/ground_numeric_effect_operator.cpp"
    "formalism/datalog/ground_program.cpp"
    "formalism/datalog/ground_rule.cpp"
    "formalism/datalog/literal.cpp"
    "formalism/datalog/metric.cpp"
    "formalism/datalog/multi_operator.cpp"
    "formalism/datalog/numeric_effect.cpp"
    "formalism/datalog/numeric_effect_operator.cpp"
    "formalism/datalog/program.cpp"
    "formalism/datalog/rule.cpp"
    "formalism/datalog/unary_operator.cpp"
)

add_gtest(
    formalism_planning_types
    "formalism/planning/action.cpp"
    "formalism/planning/arithmetic_operator.cpp"
    "formalism/planning/atom.cpp"
    "formalism/planning/axiom.cpp"
    "formalism/planning/binary_operator.cpp"
    "formalism/planning/boolean_operator.cpp"
    "formalism/planning/conditional_effect.cpp"
    "formalism/planning/conjunctive_condition.cpp"
    "formalism/planning/conjunctive_effect.cpp"
    "formalism/planning/domain.cpp"
    "formalism/planning/fdr_fact.cpp"
    "formalism/planning/fdr_task.cpp"
    "formalism/planning/fdr_variable.cpp"
    "formalism/planning/function_expression.cpp"
    "formalism/planning/function_term.cpp"
    "formalism/planning/ground_action.cpp"
    "formalism/planning/ground_atom.cpp"
    "formalism/planning/ground_axiom.cpp"
    "formalism/planning/ground_conditional_effect.cpp"
    "formalism/planning/ground_conjunctive_condition.cpp"
    "formalism/planning/ground_conjunctive_effect.cpp"
    "formalism/planning/ground_function_expression.cpp"
    "formalism/planning/ground_function_term.cpp"
    "formalism/planning/ground_function_term_value.cpp"
    "formalism/planning/ground_literal.cpp"
    "formalism/planning/ground_numeric_effect.cpp"
    "formalism/planning/ground_numeric_effect_operator.cpp"
    "formalism/planning/literal.cpp"
    "formalism/planning/metric.cpp"
    "formalism/planning/multi_operator.cpp"
    "formalism/planning/numeric_effect.cpp"
    "formalism/planning/numeric_effect_operator.cpp"
    "formalism/planning/task.cpp"
    "formalism/planning/unary_operator.cpp"
)

add_gtest(
    planning_match_tree_types
    "planning/ground/match_tree/nodes/atom.cpp"
    "planning/ground/match_tree/nodes/constraint.cpp"
    "planning/ground/match_tree/nodes/generator.cpp"
    "planning/ground/match_tree/nodes/negative_fact.cpp"
    "planning/ground/match_tree/nodes/node.cpp"
    "planning/ground/match_tree/nodes/variable.cpp"
)

add_gtest(
    planning_state_types
    "planning/ground/state.cpp"
    "planning/lifted/state.cpp"
    "planning/state.cpp"
)

add_gtest(planning_applicability "planning/applicability.cpp")
add_gtest(planning_plan_reconstruction "planning/plan_reconstruction.cpp")
add_gtest(planning_workers "planning/workers.cpp")
add_gtest(planning_state_routing "planning/state_routing.cpp")
add_gtest(formalism_planning_invariants_synthesis "formalism/planning/invariants/synthesis.cpp")
target_link_libraries(formalism_planning_invariants_synthesis PRIVATE Boost::json)
add_gtest(formalism_unification "formalism/unification.cpp")

add_gtest(datalog_cost_policy "datalog/cost_policy.cpp")
add_gtest(datalog_ground_queue "datalog/ground/queue.cpp")
add_gtest(datalog_lifted_bottom_up "datalog/lifted/bottom_up.cpp")
target_link_libraries(datalog_lifted_bottom_up PRIVATE Boost::iostreams Boost::json)
add_gtest(datalog_lifted_delta_kpkc "datalog/lifted/delta_kpkc.cpp")

foreach(task_kind ground lifted)
    foreach(heuristic rpg_max rpg_add rpg_ff lmcut)
        add_gtest(planning_heuristics_${task_kind}_${heuristic} "planning/heuristics/${task_kind}/${heuristic}.cpp")
        target_link_libraries(planning_heuristics_${task_kind}_${heuristic} PRIVATE Boost::json)
    endforeach()
endforeach()

foreach(task_kind ground lifted)
    foreach(algorithm brfs astar gbfs iw siw)
        add_gtest(
            planning_algorithms_statistics_${task_kind}_${algorithm}
            "planning/algorithms/statistics/${task_kind}/${algorithm}.cpp"
        )
        target_link_libraries(planning_algorithms_statistics_${task_kind}_${algorithm} PRIVATE Boost::json)
    endforeach()
endforeach()
add_gtest(planning_algorithms_utils "planning/algorithms/utils.cpp")
add_gtest(planning_algorithms_iw_novelty_table "planning/algorithms/iw/novelty_table.cpp")
add_gtest(planning_algorithms_serialized "planning/algorithms/serialized.cpp")
target_link_libraries(planning_algorithms_serialized PRIVATE Boost::json)
