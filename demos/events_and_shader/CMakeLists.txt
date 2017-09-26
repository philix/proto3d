add_executable(events_and_shader events_and_shader.cpp ../../gui_cocoa.m)
set_property(TARGET events_and_shader PROPERTY CXX_STANDARD 11)
TargetLinkProto3D(events_and_shader)

file(COPY hazard.png shaders.glsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
