# The database engine's benchmarks (datalog/lifted/database).
find_package(Threads REQUIRED)

foreach(benchmark incremental query)
    add_executable(database_${benchmark}_benchmark "${benchmark}.cpp")
    target_link_libraries(database_${benchmark}_benchmark PRIVATE tyr::core benchmark::benchmark_main Threads::Threads)
endforeach()

