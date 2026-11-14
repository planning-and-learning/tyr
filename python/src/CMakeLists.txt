find_package(
    Python 3.11 REQUIRED
    COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule
)

find_package(nanobind CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
message(STATUS "Found nanobind: ${nanobind_DIR} (found version ${nanobind_VERSION})")

brokkr_python_package_init(PYTYR pytyr)

file(
    GLOB_RECURSE PYTYR_FORMALISM_SOURCES
    CONFIGURE_DEPENDS
    "pytyr/formalism/*.cpp"
)

nanobind_add_module(
    _pytyr
    NB_SUPPRESS_WARNINGS
    module.cpp
    pytyr/module.cpp
    pytyr/serialization/module.cpp
    pytyr/datalog/lifted/database/distance.cpp
    pytyr/datalog/lifted/database/evaluation.cpp
    pytyr/datalog/lifted/database/module.cpp
    pytyr/datalog/lifted/database/optimization.cpp
    pytyr/datalog/lifted/database/syntax/antijoin.cpp
    pytyr/datalog/lifted/database/syntax/difference.cpp
    pytyr/datalog/lifted/database/syntax/distance.cpp
    pytyr/datalog/lifted/database/syntax/empty.cpp
    pytyr/datalog/lifted/database/syntax/generic_join.cpp
    pytyr/datalog/lifted/database/syntax/input.cpp
    pytyr/datalog/lifted/database/syntax/join.cpp
    pytyr/datalog/lifted/database/syntax/project.cpp
    pytyr/datalog/lifted/database/syntax/query.cpp
    pytyr/datalog/lifted/database/syntax/rename.cpp
    pytyr/datalog/lifted/database/syntax/select_equal.cpp
    pytyr/datalog/lifted/database/syntax/select_value.cpp
    pytyr/datalog/lifted/database/syntax/semijoin.cpp
    pytyr/datalog/lifted/database/syntax/union.cpp
    ${PYTYR_FORMALISM_SOURCES}
    pytyr/datalog/module.cpp
    pytyr/datalog/ground/module.cpp
    pytyr/datalog/lifted/module.cpp
    pytyr/planning/module.cpp
    pytyr/planning/ground/module.cpp
    pytyr/planning/lifted/module.cpp
    pytyr/planning/algorithms/astar_eager/ground.cpp
    pytyr/planning/algorithms/brfs/ground.cpp
    pytyr/planning/algorithms/gbfs_lazy/ground.cpp
    pytyr/planning/algorithms/iw/ground.cpp
    pytyr/planning/algorithms/siw/ground.cpp
    pytyr/planning/algorithms/astar_eager/lifted.cpp
    pytyr/planning/algorithms/brfs/lifted.cpp
    pytyr/planning/algorithms/gbfs_lazy/lifted.cpp
    pytyr/planning/algorithms/iw/lifted.cpp
    pytyr/planning/algorithms/siw/lifted.cpp
    BACKEND_MODULE nanobind_backend
)

target_link_libraries(_pytyr PRIVATE tyr::core TBB::tbb)
target_include_directories(_pytyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The bindings of one directory bind types of one family and share most of their instantiations: batched, they compile
# them once (brokkr's template rules, rule 11). formalism and formalism/planning exceed the heaviest translation unit
# merged whole, so they are batched by at most 11 sources; database repeats file-local names.
brokkr_unity_directories(_pytyr ENABLED ${TYR_UNITY_BUILD} DIRECTORIES pytyr/datalog/lifted/database/syntax pytyr/formalism/datalog)
brokkr_unity_directories(_pytyr ENABLED ${TYR_UNITY_BUILD} BATCH_SIZE 11 DIRECTORIES pytyr/formalism pytyr/formalism/planning)
# The bindings are the build's heaviest translation units: where memory is tight, cap how many compile at once
# (-DTYR_PYTHON_COMPILE_JOBS=6 at -j12 lowers the peak memory by about 3 GB for 10 s more), so that they do not take
# every parallel slot at the same time.
set(TYR_PYTHON_COMPILE_JOBS "" CACHE STRING "Most pytyr translation units compiled at once (Ninja); empty: no limit.")
brokkr_limit_compile_jobs(_pytyr JOBS "${TYR_PYTHON_COMPILE_JOBS}")

brokkr_python_module_rpaths(PYTYR _pytyr)

install(TARGETS _pytyr DESTINATION "pytyr" COMPONENT pytyr)

set_property(
    TARGET core PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
)
brokkr_install_native_export(
    PKG_UPPER PYTYR
    PACKAGE pytyr
    CONFIG_NAME tyr
    NAMESPACE tyr::
    EXPORT_NAME pytyrTyrcoreTargets
    EXPORT_FILE tyrcoreTargets.cmake
    TARGETS core
    HEADER_DIRS "${CMAKE_SOURCE_DIR}/include/tyr"
    CONFIG_TEMPLATE "${CMAKE_SOURCE_DIR}/Config.cmake.in"
    VERSION_FILE "${CMAKE_BINARY_DIR}/tyrConfigVersion.cmake"
)

brokkr_install_runtime_path_fixup(PYTYR pytyr)

set(PYTYR_GENERATED_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/pytyr/_version.py")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pytyr")
file(WRITE "${PYTYR_GENERATED_VERSION_FILE}" "__version__ = \"${PROJECT_VERSION}\"\n")
install(
    FILES "${PYTYR_GENERATED_VERSION_FILE}"
    DESTINATION "pytyr"
    COMPONENT pytyr
)

brokkr_install_python_files(
    pytyr "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        pytyr/py.typed
        pytyr/__init__.py
        pytyr/__main__.py
        pytyr/__init__.pyi
        pytyr/serialization/__init__.py
        pytyr/formalism/__init__.py
        pytyr/formalism/datalog/__init__.py
        pytyr/formalism/planning/__init__.py
        pytyr/datalog/lifted/database/__init__.py
        pytyr/datalog/__init__.py
        pytyr/datalog/ground/__init__.py
        pytyr/datalog/lifted/__init__.py
        pytyr/planning/__init__.py
        pytyr/planning/ground/__init__.py
        pytyr/planning/lifted/__init__.py
        pytyr/planning/algorithms/__init__.py
        pytyr/planning/algorithms/astar_eager/__init__.py
        pytyr/planning/algorithms/astar_eager/ground/__init__.py
        pytyr/planning/algorithms/astar_eager/lifted/__init__.py
        pytyr/planning/algorithms/brfs/__init__.py
        pytyr/planning/algorithms/brfs/ground/__init__.py
        pytyr/planning/algorithms/brfs/lifted/__init__.py
        pytyr/planning/algorithms/gbfs_lazy/__init__.py
        pytyr/planning/algorithms/gbfs_lazy/ground/__init__.py
        pytyr/planning/algorithms/gbfs_lazy/lifted/__init__.py
        pytyr/planning/algorithms/iw/__init__.py
        pytyr/planning/algorithms/iw/ground/__init__.py
        pytyr/planning/algorithms/iw/lifted/__init__.py
        pytyr/planning/algorithms/siw/__init__.py
        pytyr/planning/algorithms/siw/ground/__init__.py
        pytyr/planning/algorithms/siw/lifted/__init__.py
)

brokkr_install_provider_env(
    PYTYR pytyr
    PREFIXES "${LOKI_NATIVE_PREFIX}" "${YGGDRASIL_NATIVE_PREFIX}" "${URD_NATIVE_PREFIX}"
)

brokkr_install_python_stubs(
    PYTYR pytyr
    MODULE pytyr._pytyr
    PRIVATE_MODULE _pytyr
    RENAME_PACKAGES pytyr pypddl pyyggdrasil
)

add_custom_target(pytyr_wheel DEPENDS _pytyr)
