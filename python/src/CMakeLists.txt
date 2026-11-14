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
    pytyr/database/distance.cpp
    pytyr/database/evaluation.cpp
    pytyr/database/module.cpp
    pytyr/database/optimization.cpp
    pytyr/database/query/antijoin.cpp
    pytyr/database/query/difference.cpp
    pytyr/database/query/distance.cpp
    pytyr/database/query/empty.cpp
    pytyr/database/query/generic_join.cpp
    pytyr/database/query/input.cpp
    pytyr/database/query/join.cpp
    pytyr/database/query/project.cpp
    pytyr/database/query/query.cpp
    pytyr/database/query/rename.cpp
    pytyr/database/query/select_equal.cpp
    pytyr/database/query/select_value.cpp
    pytyr/database/query/semijoin.cpp
    pytyr/database/query/union.cpp
    ${PYTYR_FORMALISM_SOURCES}
    pytyr/datalog/module.cpp
    pytyr/datalog/ground/module.cpp
    pytyr/datalog/lifted/module.cpp
    pytyr/planning/module.cpp
    pytyr/planning/ground/module.cpp
    pytyr/planning/lifted/module.cpp
    pytyr/planning/ground/astar_eager/module.cpp
    pytyr/planning/ground/brfs/module.cpp
    pytyr/planning/ground/gbfs_lazy/module.cpp
    pytyr/planning/ground/iw/module.cpp
    pytyr/planning/ground/siw/module.cpp
    pytyr/planning/lifted/astar_eager/module.cpp
    pytyr/planning/lifted/brfs/module.cpp
    pytyr/planning/lifted/gbfs_lazy/module.cpp
    pytyr/planning/lifted/iw/module.cpp
    pytyr/planning/lifted/siw/module.cpp
    BACKEND_MODULE nanobind_backend
)

target_link_libraries(_pytyr PRIVATE tyr::core TBB::tbb)
target_include_directories(_pytyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The bindings of one directory bind types of one family and share most of their instantiations: batched, they compile
# them once (brokkr's template rules, rule 11). formalism and formalism/planning exceed the heaviest translation unit
# merged whole, so they are batched by at most 11 sources; database repeats file-local names.
brokkr_unity_directories(_pytyr ENABLED ${TYR_UNITY_BUILD} DIRECTORIES pytyr/database/query pytyr/formalism/datalog)
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
        pytyr/database/__init__.py
        pytyr/datalog/__init__.py
        pytyr/datalog/ground/__init__.py
        pytyr/datalog/lifted/__init__.py
        pytyr/planning/__init__.py
        pytyr/planning/ground/__init__.py
        pytyr/planning/ground/astar_eager/__init__.py
        pytyr/planning/ground/brfs/__init__.py
        pytyr/planning/ground/gbfs_lazy/__init__.py
        pytyr/planning/ground/iw/__init__.py
        pytyr/planning/ground/siw/__init__.py
        pytyr/planning/lifted/__init__.py
        pytyr/planning/lifted/astar_eager/__init__.py
        pytyr/planning/lifted/brfs/__init__.py
        pytyr/planning/lifted/gbfs_lazy/__init__.py
        pytyr/planning/lifted/iw/__init__.py
        pytyr/planning/lifted/siw/__init__.py
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
