#include <fcntl.h>
#include <pthread.h>
#include <cstdio>
#include <string>

#define GUI_COMMON_IMPLEMENTATION
#include "gui_common.h"
#define PROTO3D_GLCOREARB_IMPLEMENTATION
#include "proto3d_glcorearb.h"
#define PROTO3D_IMPLEMENTATION
#include "proto3d.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
using namespace proto3d;
using namespace proto3d::gl;

GlobalGui gui;
GWindow *main_window;

Shader shaders[2];
Program program;
VBO vbo;
VAO vao;
Texture2D texture;

void FlushFrame();

void DumpEventString(GWindowEvent *event) {
  GWindowEventData *e = &event->e;

  switch (event->type) {
    case kWindowKey:
      printf("kWindowKey scancode=%d action=%d mods=%d\n",
             e->key.scancode,
             e->key.action,
             e->key.mods);
      break;
    case kWindowChar:
      printf("kWindowChar codepoint=%u mods=%d plain=%d\n",
             e->char_.codepoint,
             e->char_.mods,
             e->char_.plain);
      break;
    case kWindowScroll:
      printf("kWindowScroll xoffset=%lf yoffset=%lf\n", e->scroll.xoffset, e->scroll.yoffset);
      break;
    case kWindowMouse:
      printf("kWindowMouse\n");
      break;
    case kWindowCursorMotion:
      printf("kWindowCursorMotion x=%lf y=%lf\n", e->cursor.x, e->cursor.y);
      break;
    case kWindowCursorEnterChange:
      printf("kWindowCursorEnterChange entered=%d\n", e->cursor.entered);
      break;
    case kWindowDrop:
      printf("kWindowDrop\n");
      break;
    case kWindowFocusChange:
      printf("kWindowFocusChange focused=%d\n", e->focused);
      break;
    case kWindowMove:
      printf("kWindowMove x=%d y=%d\n", e->pos.x, e->pos.y);
      break;
    case kWindowResize:
      printf("kWindowResize width=%d height=%d\n", e->size.width, e->size.height);
      break;
    case kWindowFramebufferResize:
      printf("kWindowFramebufferResize\n");
      break;
    case kWindowIconifyChange:
      printf("kWindowIconifyChange\n");
      break;
    case kWindowDamage:
      printf("kWindowDamage\n");
      break;
    case kWindowClose:
      printf("kWindowClose\n");
      break;
    case kWindowNullEvent:
      printf("kWindowNullEvent\n");
      break;
    default:
      assert(false && "Unknown event type\n");
      break;
  }
}

void HandleEvent(GWindowEvent event) {
  static GWindowEventType last_event_type = kWindowNullEvent;
  static char pending_newline             = 0;
  // Open new window at this position
  static int pos_x = 0, pos_y = 0;

  if (event.type == last_event_type) {
    putchar('.');
    pending_newline = '\n';
    fflush(stdout);
  } else {
    if (pending_newline) {
      putchar(pending_newline);
    }
    DumpEventString(&event);
    pending_newline = 0;
    last_event_type = event.type;
  }

  GWindow *window;
  switch (event.type) {
    case kWindowKey:
      switch (event.e.key.scancode) {
        case 31:  // o
          window = gui_create_window(&gui, 800, 600, "proto3d", NULL, NULL);
          gui_set_window_pos(window, pos_x += 10, pos_y += 10);
          break;
        case 12:  // q
          if (event.window == main_window) {
            event.window->closed = true;
          } else {
            // TODO: close
          }
          break;
      }
      break;
    case kWindowDamage:
      FlushFrame();
      break;
    case kWindowClose:
      event.window->closed = true;
      break;
    default:
      break;
  }
}

int ReadShaderSource(const char *path, char *buffer, size_t size) {
  puts(path);
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return fd;
  }

  ssize_t count_read;
  while ((count_read = read(fd, buffer, size)) > 0) {
    buffer[count_read] = 0;
  }
  return count_read;
}

void FlushFrame() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  gl_swap_buffers(main_window);
}

std::string GetBaseRelativePath(const char *argv_0) {
  std::string binary_relative_path = argv_0;

  int sep_pos;
#if defined(_WIN32) || defined(WIN32)
  sep_pos = binary_relative_path.rfind('\\');
#else
  sep_pos = binary_relative_path.rfind('/');
#endif
  return binary_relative_path.substr(0, sep_pos + 0);
}

int main(int argc, char *argv[]) {
  char *error = NULL;
  gui_init(&gui, &error);

  // Create window and OpenGL context
  main_window = gui_create_window(&gui, 800, 600, "proto3d", NULL, &error);
  if (error) {
    puts(error);
    return 1;
  }

  // Load OpenGL core profile
  Proto3dOpenLibGlAndLoadCoreProfile();
  int major, minor;
  if (Proto3dGlLoadedVersion(&major, &minor) < 0) {
    fprintf(stderr, "proto3d: failed to load OpenGL library");
    return 2;
  }
#ifndef NDEBUG
  fprintf(stderr, "proto3d: Loaded OpenGL %d.%d Core Profile\n", major, minor);
#endif

  // Clear the window
  glClearColor(1.0, 0.5, 0.0, 1.0);  // white
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl_swap_buffers(main_window);

  // Poll events to let the OS paint the window ASAP, but we're still ignoring
  // all events.
  gui_poll_events(&gui);

  string base_relative_path = GetBaseRelativePath(argv[0]);

  // Compile shaders
  char shader_source[4096];
  string shader_path = base_relative_path + "/shaders.glsl";
  if (ReadShaderSource(shader_path.c_str(), shader_source, 4096) < 0) {
    perror("ReadShaderSource");
    return 3;
  }
  const char *sources_vert[] = {"#version 330\n#define VERTEX_SHADER\n", shader_source};
  shaders[0]                 = shader::Compile(GL_VERTEX_SHADER, 2, (const char **)sources_vert);
  const char *sources_frag[] = {"#version 330\n#define FRAGMENT_SHADER\n", shader_source};
  shaders[1]                 = shader::Compile(GL_FRAGMENT_SHADER, 2, (const char **)sources_frag);

  // Link into a Program
  //
  // Can be done in a single line: program = shader::Link(shaders, 2);
  program.Create();
  program.AttachShaders(shaders[0], shaders[1]);
  {
    auto log = program.Link();
    assert(log == nullptr);
    GLint count;
    auto attached_shaders = program.GetAttachedShaders(&count);
    printf("Count attached shaders: %d\n", count);
    if (attached_shaders != nullptr) {
      puts("Source of first shader:");
      puts(attached_shaders.get()[0].GetSource().get());
    }
  }
  program.DetachShaders(shaders[0], shaders[1]);
  shaders[0].Delete();
  shaders[1].Delete();

  // Load the triangle
  vao.Create();
  vbo.Create();
  vao.Bind();
  vbo.Bind();

  // clang-format off
  // put the three triangle verticies into the VBO
  GLfloat vertex_data[] = {
     // x    y     z      u    v
     0.0f,  0.8f, 0.0f,  0.5f, 1.0f,
    -0.8f, -0.8f, 0.0f,  0.0f, 0.0f,
     0.8f, -0.8f, 0.0f,  1.0f, 0.0f
  };
  // clang-format on
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

  // connect the xyz to the "vert" attribute of the vertex shader
  glEnableVertexAttribArray(program.AttribLocation("vert"));
  glVertexAttribPointer(
      program.AttribLocation("vert"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);

  // Load the texture into the triangle
  auto image = stb::Image::CreateFromFile((base_relative_path + "/hazard.png").c_str());

  glActiveTexture(GL_TEXTURE0);
  program.SetUniform("tex", 0);

  bool valid;
  auto validation_log = program.ValidationLog(&valid).get();
  if (!valid) {
    puts("Shader program is invalid.");
    puts(validation_log);
  }

  texture.Gen();
  texture.Bind();
  texture.SetFilterAndWrap(GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE);
  texture.LoadImage(image.get());
  texture.GenerateMipmaps();

  // connect the uv coords to the "vertTexCoord" attribute of the vertex
  // shader
  glEnableVertexAttribArray(program.AttribLocation("vertTexCoord"));
  glVertexAttribPointer(program.AttribLocation("vertTexCoord"),
                        2,
                        GL_FLOAT,
                        GL_TRUE,
                        5 * sizeof(GLfloat),
                        (const GLvoid *)(3 * sizeof(GLfloat)));

  program.Bind();
  vao.Bind();
  texture.Bind();

  // Attach our even handler
  gui.handle_event = HandleEvent;

  // Render the first frame
  FlushFrame();

  do {
    gui_poll_events(&gui);
    gui_wait_events(&gui);
  } while (!main_window->closed);

  program.Unbind();

#ifndef NDEBUG
  vao.Delete();
  vbo.Delete();
  texture.Delete();
  program.Delete();

  gl::CheckLeaks();
#endif

  return 0;
}
