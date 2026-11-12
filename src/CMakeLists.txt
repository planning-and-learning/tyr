set(SUBDIRECTORIES "algorithms" "analysis" "buffer" "common" "formalism" "datalog" "planning" "serialization")

set(TYR_SRC_FILES "")
set(TYR_PRIVATE_HEADER_FILES "")
set(TYR_PUBLIC_HEADER_FILES "")

foreach(subdir ${SUBDIRECTORIES})
    file(GLOB_RECURSE TMP_PRIVATE_HEADER_FILES "${subdir}/*.hpp" "${subdir}/**/*.hpp")
    list(APPEND TYR_PRIVATE_HEADER_FILES ${TMP_PRIVATE_HEADER_FILES})

    file(
        GLOB_RECURSE TMP_PUBLIC_HEADER_FILES
        "../include/tyr/${subdir}/*.hpp"
        "../include/tyr/${subdir}/**/*.hpp"
    )
    list(APPEND TYR_PUBLIC_HEADER_FILES ${TMP_PUBLIC_HEADER_FILES})
endforeach()


add_library(
    core SHARED ${TYR_PRIVATE_HEADER_FILES} ${TYR_PUBLIC_HEADER_FILES}
    algorithms/kckp/graphs.cpp
    algorithms/kckp/kckp.cpp
    algorithms/kckp/delta_kckp.cpp

    analysis/listeners.cpp
    analysis/program_domains.cpp
    analysis/stratification.cpp
    analysis/task_domains.cpp
    analysis/domains_translation.cpp

    formalism/formatter.cpp
    formalism/repository.cpp
    formalism/datalog/formatter.cpp
    formalism/datalog/grounder.cpp
    formalism/datalog/variable_dependency_graph.cpp

    formalism/planning/invariants/constraints.cpp
    formalism/planning/invariants/matching.cpp
    formalism/planning/invariants/matcher.cpp
    formalism/planning/invariants/mutexes.cpp
    formalism/planning/invariants/normalization.cpp
    formalism/planning/invariants/ordering.cpp
    formalism/planning/invariants/proof.cpp
    formalism/planning/invariants/refinement.cpp
    formalism/planning/invariants/synthesis.cpp
    formalism/planning/formatter.cpp
    formalism/planning/fdr_context.cpp
    formalism/planning/grounder.cpp
    formalism/planning/merge_datalog.cpp
    formalism/planning/merge_planning.cpp
    formalism/planning/loki_to_tyr.cpp
    formalism/planning/parser.cpp
    formalism/planning/planning_task.cpp

    datalog/rule_splitting.cpp
    datalog/static_literal_decomposition.cpp
    datalog/static_rule_filter.cpp
    datalog/policies/annotation.cpp
    datalog/policies/cost.cpp
    datalog/policies/termination.cpp
    datalog/lifted/programs/program.cpp
    datalog/lifted/workspaces/facts.cpp
    datalog/lifted/workspaces/program.cpp
    datalog/lifted/kckp/workspaces/rule.cpp
    datalog/lifted/kckp/workspaces/program.cpp
    datalog/lifted/applicability.cpp
    datalog/lifted/applicability_lifted.cpp
    datalog/lifted/kckp/assignment_sets.cpp
    datalog/lifted/solver.cpp
    datalog/lifted/kckp/consistency_graph.cpp
    datalog/ground/programs/program.cpp
    datalog/ground/workspaces/facts.cpp
    datalog/ground/workspaces/program.cpp
    datalog/ground/solver.cpp
    datalog/lifted/scheduler.cpp
    datalog/lifted/database/optimization/details/cost.cpp
    datalog/lifted/database/optimization/details/estimation.cpp
    datalog/lifted/database/optimization/details/join_block.cpp
    datalog/lifted/database/optimization/details/normalization.cpp
    datalog/lifted/database/optimization/details/optimization.cpp
    datalog/lifted/database/optimization/details/structural.cpp
    datalog/lifted/database/semantics/bound/query_evaluation.cpp
    datalog/lifted/database/semantics/bound/union_normal_form.cpp
    datalog/lifted/database/semantics/details/distance.cpp
    datalog/lifted/database/semantics/details/min_distance.cpp
    datalog/lifted/database/semantics/details/plans.cpp
    datalog/lifted/database/semantics/generic_join.cpp
    datalog/lifted/database/semantics/incremental/delta.cpp
    datalog/lifted/database/semantics/incremental/details/distance_repair.cpp
    datalog/lifted/database/semantics/incremental/distance.cpp
    datalog/lifted/database/semantics/incremental/join.cpp
    datalog/lifted/database/semantics/incremental/projection.cpp
    datalog/lifted/database/semantics/incremental/semijoin.cpp
    datalog/lifted/database/semantics/query_evaluation.cpp
    datalog/lifted/database/semantics/search/backward_frontier.cpp
    datalog/lifted/database/syntax/copy.cpp
    datalog/lifted/database/syntax/formatter.cpp
    datalog/lifted/database/syntax/operator_builder.cpp
    datalog/lifted/database/syntax/query.cpp
    datalog/lifted/database/syntax/query_construction.cpp
    datalog/lifted/database/syntax/query_repository.cpp

    planning/state_storage/numeric.cpp
    planning/state_storage/tree_bitset/atom.cpp
    planning/state_storage/tree_bitset/fact.cpp
    planning/ground/state_storage.cpp
    planning/ground/state_storage/vectors.cpp
    planning/lifted/state_storage/vectors.cpp

    planning/heuristic.cpp
    planning/heuristics/goal_count.cpp
    planning/heuristics/lmcut.cpp
    planning/heuristics/rpg_add.cpp
    planning/heuristics/rpg_ff.cpp
    planning/heuristics/rpg_max.cpp
    planning/state.cpp
    planning/state_delta.cpp
    planning/state_repository.cpp

    planning/ground/axiom_evaluator.cpp
    planning/ground/axiom_stratification.cpp
    planning/ground/match_tree.cpp
    planning/ground/successor_generator.cpp
    planning/lifted/axiom_evaluator.cpp
    planning/lifted/effect_atom_index.cpp
    planning/lifted/successor_generator.cpp
    planning/lifted/task_grounder.cpp

    planning/lifted/programs/action.cpp
    planning/lifted/programs/axiom.cpp
    planning/programs/common.cpp
    planning/lifted/programs/rpg.cpp
    planning/lifted/programs/ground.cpp
    planning/ground/programs/rpg.cpp

    planning/algorithms/astar_eager.cpp
    planning/algorithms/astar_eager/event_handler.cpp
    planning/algorithms/brfs.cpp
    planning/algorithms/brfs/event_handler.cpp
    planning/algorithms/gbfs_lazy.cpp
    planning/algorithms/gbfs_lazy/event_handler.cpp
    planning/algorithms/iw.cpp
    planning/algorithms/utils.cpp

    planning/state_routing/dist_hash.cpp

    planning/applicability.cpp
    planning/applicability_lifted.cpp
    planning/action_executor.cpp
    planning/task.cpp
    planning/plan.cpp
    planning/task_utils.cpp
)
add_library(tyr::core ALIAS core)
set_target_properties(core PROPERTIES OUTPUT_NAME tyr_core)

# An executable shares the state of inline functions (a function's static, as yggdrasil's next_layer) with this
# library. Under link-time optimization Apple's linker gives the executable its own copy unless it keeps its
# global symbols.
target_link_options(core INTERFACE "$<$<AND:$<PLATFORM_ID:Darwin>,$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>>:LINKER:-export_dynamic>")

target_compile_features(core PUBLIC cxx_std_20)

target_compile_definitions(
    core
    PUBLIC
        # Boost.MPL's configuration comes from loki::parsers, so every translation unit agrees on it.
        FMT_HEADER_ONLY
)

if(TYR_ENABLE_INNER_PARALLELISM)
    target_compile_definitions(core PUBLIC TYR_ENABLE_INNER_PARALLELISM)
endif()

if(TYR_ENABLE_SEMI_NAIVE)
    target_compile_definitions(core PUBLIC TYR_ENABLE_SEMI_NAIVE)
endif()

target_compile_definitions(core PUBLIC TYR_STATE_STORAGE_POLICY=${TYR_STATE_STORAGE_POLICY})

if("${TYR_RELATION_STORAGE_POLICY}" STREQUAL "Word")
    target_compile_definitions(core PUBLIC TYR_RELATION_STORAGE_WORD)
elseif("${TYR_RELATION_STORAGE_POLICY}" STREQUAL "Bit")
    target_compile_definitions(core PUBLIC TYR_RELATION_STORAGE_BIT)
endif()

target_include_directories(
    core
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${BROKKR_NATIVE_DEPENDENCY_INCLUDE_DIRECTORIES}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${prefix}/include")
        target_include_directories(core PUBLIC "$<BUILD_INTERFACE:${prefix}/include>")
    endif()
endforeach()

target_link_libraries(
    core
    PUBLIC
        yggdrasil::yggdrasil
        TBB::tbb
        fmt::fmt-header-only
        gtl::gtl
        loki::parsers
        Boost::json
    PRIVATE
        Threads::Threads
)

if(APPLE)
    set(TYR_LIBRARY_ORIGIN "@loader_path")
elseif(UNIX)
    set(TYR_LIBRARY_ORIGIN "$ORIGIN")
endif()

foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${prefix}/${CMAKE_INSTALL_LIBDIR}")
        list(APPEND TYR_CORE_BUILD_RPATHS "${prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif()
endforeach()

set_target_properties(
    core
    PROPERTIES
        BUILD_RPATH "${TYR_CORE_BUILD_RPATHS}"
        INSTALL_RPATH "${TYR_LIBRARY_ORIGIN}"
)

install(
    TARGETS core
    EXPORT tyrcoreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tyr
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tyr
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT tyr
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT tyrcoreTargets
    NAMESPACE tyr::
    COMPONENT tyr
    FILE "tyrcoreTargets.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tyr"
)

export(
    EXPORT tyrcoreTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/tyrcoreTargets.cmake"
    NAMESPACE tyr::
)
