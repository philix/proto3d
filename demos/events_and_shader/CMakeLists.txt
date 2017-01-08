add_executable(events_and_shader events_and_shader.cpp ../../gui_cocoa.m)
target_link_libraries(events_and_shader ${OPENGL_LIBRARIES}
                                        ${APPLE_LIBRARIES})

file(COPY hazard.png fragment1.frag vertex1.vert DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
