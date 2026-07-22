add_executable(astar_eager "astar_eager.cpp")
target_link_libraries(astar_eager tyr::core)

add_executable(gbfs_lazy "gbfs_lazy.cpp")
target_link_libraries(gbfs_lazy tyr::core)
