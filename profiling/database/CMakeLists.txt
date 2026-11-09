# The database engine's benchmarks (datalog/lifted/database).
find_package(Threads REQUIRED)

foreach(benchmark incremental distance query)
    add_executable(database_${benchmark}_benchmark "${benchmark}.cpp")
    target_link_libraries(database_${benchmark}_benchmark PRIVATE tyr::core benchmark::benchmark_main Threads::Threads)
endforeach()

# A plain executable, optimized even in Debug builds so its timings mean something.
add_executable(must_pass_sokoban "must_pass_sokoban.cpp")
target_link_libraries(must_pass_sokoban PRIVATE tyr::core)
target_compile_options(must_pass_sokoban PRIVATE -O2 -DNDEBUG)
