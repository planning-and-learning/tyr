cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/read_project_version.cmake")
tyr_read_project_version(TYR_PROJECT_VERSION "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml")

project(tyr VERSION "${TYR_PROJECT_VERSION}")


##############################################################
# Language setup
##############################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


##############################################################
# Options
##############################################################



add_compile_definitions(YGG_ENABLE_FMT_FORMATTERS=1)

option(TYR_USE_LLD "Use LLVM lld linker when available" ON)

message("Build shared libraries enabled.")

option(TYR_ENABLE_LTO "Enable link-time optimization for optimized builds." ON)

option(TYR_HEADER_INSTANTIATION "Enable stronger inlining at higher compile time costs." OFF)
if(TYR_HEADER_INSTANTIATION)
    add_compile_definitions(TYR_HEADER_INSTANTIATION)
endif()

set(TYR_STATE_STORAGE_POLICY "Tree" CACHE STRING "Policy used for storing states.")

if("${TYR_STATE_STORAGE_POLICY}" STREQUAL "Tree")
    add_compile_definitions(TYR_STATE_STORAGE_TREE)
elseif("${TYR_STATE_STORAGE_POLICY}" STREQUAL "Hashset")
    add_compile_definitions(TYR_STATE_STORAGE_HASHSET)
else()
    message(FATAL_ERROR "TYR_STATE_STORAGE_POLICY must be Tree or Hashset")
endif()


##############################################################
# Build Targets
##############################################################

option(TYR_BUILD_PYTYR "Build pytyr Python bindings." OFF)
option(TYR_BUILD_TESTS "Build Tyr tests." OFF)
option(TYR_BUILD_EXECUTABLES "Build Tyr executables." OFF)
option(TYR_BUILD_PROFILING "Build Tyr profiling targets." OFF)


##############################################################
# Common Settings
##############################################################

# make cache variables for install destinations
include(GNUInstallDirs)

if(MSVC)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1 /EHsc /bigobj /MP")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " /IGNORE:4006,4044,4075")
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    endif()
    # TODO: Add -Wextra and fix all warnings

    message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    add_compile_options(-Wall)

    # add_compile_options(-ftime-report)

    if(TYR_USE_LLD AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        find_program(LLD_LINKER NAMES ld.lld)
        if(LLD_LINKER)
            message(STATUS "Using lld: ${LLD_LINKER}")
            add_link_options(-fuse-ld=lld)
        else()
            message(STATUS "lld not found, falling back to default linker")
        endif()
    elseif(TYR_USE_LLD AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Skipping lld with GNU because GCC LTO requires the GCC linker plugin")
    endif()

    if(TYR_ENABLE_LTO AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

        # Release: enable LTO but cap parallelism to reduce RAM spikes
        set(CMAKE_CXX_FLAGS_RELEASE
            "${CMAKE_CXX_FLAGS_RELEASE} -flto=2")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE
            "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=2")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
            "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto=2")

        # RelWithDebInfo: make debug info cheap (big compile-time/RAM win)
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
            "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g2 -fno-omit-frame-pointer -fno-lto")

        # Debug: keep your choices (CMake will supply -O0 or -Og depending on toolchain,
        # but if you want -Og specifically, keep it here; otherwise omit -Og)
        set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} -Og -g2 -ggdb")

    elseif(TYR_ENABLE_LTO AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))

        # Release: prefer ThinLTO for much lower compile/link cost than full LTO
        set(CMAKE_CXX_FLAGS_RELEASE
            "${CMAKE_CXX_FLAGS_RELEASE} -flto=thin")
        if(APPLE)
            set(CMAKE_EXE_LINKER_FLAGS_RELEASE
                "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,-mllvm,-threads=2")
            set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
                "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-mllvm,-threads=2")
        else()
            set(CMAKE_EXE_LINKER_FLAGS_RELEASE
                "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--thinlto-jobs=2")
            set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
                "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--thinlto-jobs=2")
        endif()

        # RelWithDebInfo: cheap debug info
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
            "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g2 -fno-omit-frame-pointer -fno-lto")

        # Debug (same note as above)
        set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} -Og -g2 -ggdb")

    endif()
endif()

message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
add_definitions(-DROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
message("ROOT_DIR: ${ROOT_DIR}")


##############################################################
# CMake modules and macro files
##############################################################

list(APPEND CMAKE_MODULE_PATH
  "${PROJECT_SOURCE_DIR}/cmake"
)
include("configure_boost")
include("configure_ccache")

##############################################################
# CCache
##############################################################

# CCache
configure_ccache()


##############################################################
# Dependency Handling
##############################################################

# set(CMAKE_FIND_DEBUG_MODE TRUE)

include("${PROJECT_SOURCE_DIR}/cmake/bootstrap_pyyggdrasil.cmake")

# ---------
# yggdrasil
# ---------

find_package(yggdrasil 0.0.18 CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(yggdrasil_FOUND)
  message(STATUS "Found yggdrasil: ${yggdrasil_DIR} (found version ${yggdrasil_VERSION})")
endif()

yggdrasil_find_python_native_prefix(pypddl PYPDDL_NATIVE_PREFIX)
yggdrasil_register_native_dependency_prefix("${YGGDRASIL_NATIVE_PREFIX}")
yggdrasil_register_native_dependency_prefix("${PYPDDL_NATIVE_PREFIX}")
yggdrasil_register_python_native_runtime_prefix("pypddl/native" "${PYPDDL_NATIVE_PREFIX}")
yggdrasil_register_python_native_runtime_prefix("pyyggdrasil" "${YGGDRASIL_NATIVE_PREFIX}")

# -----
# argparse
# -----

find_package(argparse CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(argparse_FOUND)
  message(STATUS "Found argparse: ${argparse_DIR} (found version ${argparse_VERSION})")
endif()

# -----
# Boost
# -----

# Find Boost headers only according to https://cmake.org/cmake/help/latest/module/FindBoost.html
configure_boost()
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS iostreams json PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(Boost_FOUND)
  message(STATUS "Found Boost: ${Boost_DIR} (found version ${Boost_VERSION})")
endif()

# -----
# GTL
# -----

find_package(gtl CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(gtl_FOUND)
  message(STATUS "Found gtl: ${gtl_DIR}")
endif()

# -----
# Cista
# -----

find_package(cista CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(cista_FOUND)
  message(STATUS "Found cista: ${cista_DIR} (found version ${cista_VERSION})")
endif()

# -----
# Fmt
# -----

find_package(fmt CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(fmt_FOUND)
  message(STATUS "Found fmt: ${fmt_DIR} (found version ${fmt_VERSION})")
endif()

# -----
# Threads
# -----

# Prefer -pthread over -lpthread on platforms that support it
set(THREADS_PREFER_PTHREAD_FLAG ON)
# CMake's built-in module; no PATHS/NO_DEFAULT_PATH here
find_package(Threads REQUIRED)
if(Threads_FOUND)
  message(STATUS "Found Threads: ${CMAKE_THREAD_LIBS_INIT}")
endif()

# -----
# TBB
# -----

find_package(TBB CONFIG REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(TBB_FOUND)
  include_directories(${TBB_INCLUDE_DIRS})
  message(STATUS "Found TBB: ${TBB_DIR} (found version ${TBB_VERSION})")
endif()

# -----
# valla
# -----

find_package(valla CONFIG COMPONENTS core REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(valla_FOUND)
  message(STATUS "Found valla: ${valla_DIR} (found version ${valla_VERSION})")
endif()

# -----
# Loki
# -----

find_package(loki 1.0.17 COMPONENTS parsers REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)
if(loki_FOUND)
  message(STATUS "Found loki: ${loki_DIR} (found version ${loki_VERSION})")
endif()


##############################################################
# Add library and executable targets
##############################################################

# ------------
# Target Tyr
# ------------
if(TYR_BUILD_TESTS OR TYR_BUILD_PROFILING)
    # Benchmark PDDL comes from the pypddl-datasets cache, resolved (and
    # downloaded on demand) at configure time.
    if(NOT Python_EXECUTABLE)
        find_package(Python 3.8 REQUIRED COMPONENTS Interpreter)
    endif()
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import pypddl_datasets, sys; sys.stdout.write(str(pypddl_datasets.data_root()))"
        OUTPUT_VARIABLE TYR_BENCHMARKS_DIR
        RESULT_VARIABLE TYR_BENCHMARKS_RESULT)
    if(NOT TYR_BENCHMARKS_RESULT EQUAL 0)
        message(FATAL_ERROR "Could not resolve the benchmark data root; install 'pypddl-datasets>=0.0.7,<0.1' into the configuring Python (${Python_EXECUTABLE}).")
    endif()
    message(STATUS "Benchmark data: ${TYR_BENCHMARKS_DIR}")
    add_compile_definitions(BENCHMARKS_DIR="${TYR_BENCHMARKS_DIR}")
endif()

add_subdirectory(src)

# -------------------
# Target Python Tyr
# -------------------
if(TYR_BUILD_PYTYR)
    add_subdirectory(python/src)
endif()

# ----------
# Target Exe
# ----------
if(TYR_BUILD_EXECUTABLES)
    add_subdirectory(exe)
endif()

# ----------------
# Target Profiling
# ----------------
if(TYR_BUILD_PROFILING)
    add_subdirectory(profiling)
endif()

# -----------
# Target Test
# -----------
if(TYR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()



##############################################################
# Install Package Files
##############################################################

include(CMakePackageConfigHelpers)

set(TYR_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/tyr")
set(TYR_INSTALL_CMAKE_HELPERDIR "${TYR_INSTALL_CMAKEDIR}/cmake")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/tyr"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake/"
    DESTINATION "${TYR_INSTALL_CMAKE_HELPERDIR}")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/tyrConfigVersion.cmake"
    VERSION ${tyr_VERSION}
    COMPATIBILITY SameMinorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tyrConfig.cmake"
    INSTALL_DESTINATION "${TYR_INSTALL_CMAKEDIR}"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/tyrConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/tyrConfigVersion.cmake"
    DESTINATION "${TYR_INSTALL_CMAKEDIR}"
)
