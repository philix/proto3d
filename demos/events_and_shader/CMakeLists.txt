add_executable(events_and_shader events_and_shader.cpp ../../cocoa_gui.m)
target_link_libraries(events_and_shader ${OPENGL_LIBRARIES}
                                        ${APPLE_LIBRARIES})
