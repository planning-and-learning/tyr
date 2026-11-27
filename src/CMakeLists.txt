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
    formalism/grounder.cpp
    formalism/repository.cpp
    formalism/repository_builder.cpp
    formalism/repository_symbols_0.cpp
    formalism/repository_symbols_1.cpp
    formalism/repository_symbols_2.cpp
    formalism/variable_dependency_graph.cpp
    formalism/datalog/formatter.cpp

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
    formalism/planning/loki_to_tyr.cpp
    formalism/planning/parser.cpp
    formalism/planning/planning_task.cpp

    datalog/formatter.cpp
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

    planning/formatter.cpp
    planning/states/storages/policies/temporal.cpp
    planning/states/storages/policies/numeric.cpp
    planning/states/storages/ground/state_storage.cpp

    planning/heuristics/heuristic.cpp
    planning/heuristics/goal_count.cpp
    planning/heuristics/lmcut.cpp
    planning/heuristics/rpg_add.cpp
    planning/heuristics/rpg_ff.cpp
    planning/heuristics/rpg_max.cpp
    planning/states/state.cpp
    planning/states/state_delta.cpp
    planning/states/state_repository.cpp

    planning/axioms/ground/axiom_evaluator.cpp
    planning/successors/ground/match_tree.cpp
    planning/successors/ground/successor_generator.cpp
    planning/axioms/lifted/axiom_evaluator.cpp
    planning/successors/lifted/effect_atom_index.cpp
    planning/successors/lifted/successor_generator.cpp
    planning/tasks/lifted/task_grounder.cpp

    planning/programs/lifted/action.cpp
    planning/programs/lifted/axiom.cpp
    planning/programs/common.cpp
    planning/programs/lifted/rpg.cpp
    planning/programs/lifted/ground.cpp
    planning/programs/ground/axiom.cpp
    planning/programs/ground/rpg.cpp

    planning/algorithms/astar_eager.cpp
    planning/algorithms/astar_eager/event_handler.cpp
    planning/algorithms/brfs.cpp
    planning/algorithms/brfs/event_handler.cpp
    planning/algorithms/gbfs_lazy.cpp
    planning/algorithms/gbfs_lazy/event_handler.cpp
    planning/algorithms/iw.cpp
    planning/algorithms/utils.cpp

    planning/algorithms/state_routing/dist_hash.cpp

    planning/successors/applicability.cpp
    planning/successors/applicability_lifted.cpp
    planning/successors/action_executor.cpp
    planning/tasks/task.cpp
    planning/successors/successors.cpp
    planning/tasks/task_utils.cpp
)
add_library(tyr::core ALIAS core)

# The sources of each directory listed share most of their instantiations: batched, they compile them once (brokkr's
# template rules, rule 11), each batch below the heaviest translation unit of the build. formalism/planning exceeds it
# merged whole and is batched by three; planning/lifted exceeds it in any batch.
brokkr_unity_directories(
    core
    ENABLED ${TYR_UNITY_BUILD}
    DIRECTORIES
        algorithms/kckp
        analysis
        datalog
        datalog/ground/workspaces
        datalog/lifted
        datalog/lifted/kckp
        datalog/lifted/kckp/workspaces
        datalog/lifted/workspaces
        datalog/policies
        formalism
        formalism/datalog
        formalism/planning/invariants
        planning
        planning/algorithms
        planning/axioms/ground
        planning/heuristics
        planning/programs/ground
        planning/programs/lifted
        planning/states
        planning/states/storages/ground
        planning/states/storages/policies/tree_bitset
        planning/successors
        planning/successors/ground
        planning/tasks
)
brokkr_unity_directories(core ENABLED ${TYR_UNITY_BUILD} BATCH_SIZE 3 DIRECTORIES formalism/planning)
# The repository's members, instantiated in their own source: the heaviest of the library's, alone.
set_source_files_properties(
    formalism/repository.cpp
    formalism/repository_builder.cpp
    formalism/repository_symbols_0.cpp
    formalism/repository_symbols_1.cpp
    formalism/repository_symbols_2.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)
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

# The pools of the state storage policy the build chooses, and only those: a state's storage keys are that policy's
# handles (urd::key_value_of in tyr/planning/states/storages/state_storage.hpp).
if(TYR_STATE_STORAGE_POLICY STREQUAL "TreeBitset")
    target_sources(core PRIVATE planning/states/storages/policies/tree_bitset/atom.cpp planning/states/storages/policies/tree_bitset/fact.cpp)
    set(TYR_STATE_STORAGE_POLICY_HEADER "tyr/planning/states/storages/policies/tree_bitset.hpp")
else()
    target_sources(core PRIVATE planning/states/storages/policies/ground/vectors.cpp planning/states/storages/policies/lifted/vectors.cpp)
    set(TYR_STATE_STORAGE_POLICY_HEADER "tyr/planning/states/storages/policies/vectors.hpp")
endif()
target_compile_definitions(core PUBLIC "TYR_STATE_STORAGE_POLICY_HEADER=\"${TYR_STATE_STORAGE_POLICY_HEADER}\"")
target_compile_definitions(core PUBLIC TYR_STATE_STORAGE_POLICY=${TYR_STATE_STORAGE_POLICY})

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
