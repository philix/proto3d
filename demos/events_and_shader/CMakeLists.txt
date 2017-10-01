add_executable(events_and_shader events_and_shader.cpp)
set_property(TARGET events_and_shader PROPERTY CXX_STANDARD 11)

# proto3d
target_include_directories(events_and_shader PUBLIC ${PROTO3D_INCLUDE_DIRS})
target_link_libraries(events_and_shader PUBLIC ${PROTO3D_LIBRARIES})
target_compile_definitions(events_and_shader PUBLIC ${PROTO3D_DEFINITIONS})

# gui
target_include_directories(events_and_shader PUBLIC ${GUI_INCLUDE_DIRS})
target_link_libraries(events_and_shader PUBLIC ${GUI_LIBRARIES})

file(COPY hazard.png shaders.glsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
