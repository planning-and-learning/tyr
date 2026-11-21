find_package(benchmark 1.8.3 REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)

enable_testing()

add_subdirectory(planning)
