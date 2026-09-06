find_package(
    Python 3.11 REQUIRED
    COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule
)

find_package(nanobind CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
message(STATUS "Found nanobind: ${nanobind_DIR} (found version ${nanobind_VERSION})")

yggdrasil_python_package_init(PYTYR pytyr)

file(
    GLOB_RECURSE PYTYR_FORMALISM_SOURCES
    CONFIGURE_DEPENDS
    "pytyr/formalism/*.cpp"
)

nanobind_add_module(
    _pytyr
    module.cpp
    pytyr/module.cpp
    pytyr/serialization/module.cpp
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

yggdrasil_python_module_rpaths(PYTYR _pytyr)

install(TARGETS _pytyr DESTINATION "pytyr" COMPONENT pytyr)

set_property(
    TARGET core PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
)
yggdrasil_install_native_export(
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
    CMAKE_SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake"
)

yggdrasil_install_runtime_path_fixup(PYTYR pytyr)

set(PYTYR_GENERATED_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/pytyr/_version.py")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pytyr")
file(WRITE "${PYTYR_GENERATED_VERSION_FILE}" "__version__ = \"${PROJECT_VERSION}\"\n")
install(
    FILES "${PYTYR_GENERATED_VERSION_FILE}"
    DESTINATION "pytyr"
    COMPONENT pytyr
)

yggdrasil_install_python_files(
    pytyr "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        pytyr/py.typed
        pytyr/__init__.py
        pytyr/__main__.py
        pytyr/__init__.pyi
        pytyr/formalism/__init__.py
        pytyr/formalism/datalog/__init__.py
        pytyr/formalism/planning/__init__.py
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

file(
    GLOB_RECURSE PYTYR_SERIALIZATION_PYTHON_FILES
    CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "pytyr/serialization/*.py"
    "pytyr/serialization/*.pyi"
)
yggdrasil_install_python_files(
    pytyr "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${PYTYR_SERIALIZATION_PYTHON_FILES}
)

yggdrasil_install_provider_env(
    PYTYR pytyr
    PREFIXES "${LOKI_NATIVE_PREFIX}" "${YGGDRASIL_NATIVE_PREFIX}"
)

yggdrasil_install_python_stubs(
    PYTYR pytyr
    MODULE pytyr._pytyr
    PRIVATE_MODULE _pytyr
    RENAME_PACKAGES pytyr pypddl pyyggdrasil
)

add_custom_target(pytyr_wheel DEPENDS _pytyr)
