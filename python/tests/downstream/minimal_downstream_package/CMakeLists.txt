cmake_minimum_required(VERSION 3.21)
project(downstream_tyr_user LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(nanobind CONFIG REQUIRED)
find_package(tyr CONFIG REQUIRED COMPONENTS core)

nanobind_add_module(
    _downstream_tyr
    src/downstream_tyr_user/_downstream_tyr.cpp
    BACKEND_MODULE nanobind_backend
)
target_link_libraries(_downstream_tyr PRIVATE tyr::core)

if(DOWNSTREAM_RUNTIME_LIBRARY_DIRS)
    set_target_properties(
        _downstream_tyr
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/downstream_tyr_user"
            BUILD_RPATH "${DOWNSTREAM_RUNTIME_LIBRARY_DIRS}"
            INSTALL_RPATH "${DOWNSTREAM_RUNTIME_LIBRARY_DIRS}"
    )
else()
    set_target_properties(
        _downstream_tyr
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/downstream_tyr_user"
    )
endif()

install(TARGETS _downstream_tyr LIBRARY DESTINATION downstream_tyr_user)
install(FILES src/downstream_tyr_user/__init__.py DESTINATION downstream_tyr_user)
