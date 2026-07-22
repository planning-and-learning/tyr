add_executable(successor_generator "successor_generator.cpp")
target_link_libraries(
    successor_generator
    PRIVATE
        tyr::core
        benchmark::benchmark
        Boost::json
)

add_executable(rpg_add "heuristics/rpg_add.cpp")
target_link_libraries(
    rpg_add
    PRIVATE
        tyr::core
        benchmark::benchmark
        Boost::json
)

add_executable(rpg_ff "heuristics/rpg_ff.cpp")
target_link_libraries(
    rpg_ff
    PRIVATE
        tyr::core
        benchmark::benchmark
        Boost::json
)

add_executable(lmcut "heuristics/lmcut.cpp")
target_link_libraries(
    lmcut
    PRIVATE
        tyr::core
        benchmark::benchmark
        Boost::json
)
