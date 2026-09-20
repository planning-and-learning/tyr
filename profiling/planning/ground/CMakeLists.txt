add_executable(ground_successor_generator "successor_generator.cpp")
target_link_libraries(ground_successor_generator PRIVATE tyr::core benchmark::benchmark Boost::json)
