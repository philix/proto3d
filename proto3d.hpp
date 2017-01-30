// Copyright 2014-2016 Felipe Oliveira Carvalho
// vim: fdm=marker
#ifndef PROTO3D_H_
#define PROTO3D_H_

#include <cassert>
#include <memory>
#include <cstdarg>
#ifdef PROTO3D_USE_EXCEPTIONS
# include <string>
#endif
#ifdef PROTO3D_USE_STB
# include <cstdio>
#endif

#ifdef PROTO3D_USE_GLM
/* # include "glm/glm.hpp" */
/* # include "glm/gtc/type_ptr.hpp" */
#endif

#ifdef PROTO3D_USE_STB
# include "stb_image.h"
stbi_uc* stbi_load(const char*, int*, int*, int*, int);
#endif

// proto3d general compiler support macros {{{
// A macro to disallow the copy constructor and operator= functions
#define PROTO3D_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  TypeName& operator=(const TypeName&) = delete

// g++ and clang++ expose their version as follows:
//
// g++ 4.8.3:
//   __GNUC__          = 4
//   __GNUC_MINOR__    = 8
//   __GNUC_PATCHLEVEL = 3
//
// clang++ 5.1 (claims compat with g++ 4.2.1):
//   __GNUC__          = 4
//   __GNUC_MINOR__    = 2
//   __GNUC_PATCHLEVEL = 1
//   __clang__         = 1
//   __clang_major__   = 5
//   __clang_minor__   = 1
//
// To view the default defines of these compilers, you can perform:
//
// $ g++ -E -dM - </dev/null
// $ echo | clang++ -dM -E -

#ifdef __GNUC__
  // Defines for all GNU-likes here, for now that's g++, clang++ and intel.

  // Place these after the argument list of the function declaration
  // (not definition), like so:
  //
  //   void myfunc(void) FATTR_ALWAYS_INLINE;
  #define FATTR_MALLOC __attribute__((malloc))
  #define FATTR_ALLOC_ALIGN(x) __attribute__((alloc_align(x)))
  #define FATTR_PURE __attribute__ ((pure))
  #define FATTR_CONST __attribute__((const))
  #define FATTR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
  #define FATTR_ALWAYS_INLINE __attribute__((always_inline))
  #define FATTR_UNUSED __attribute__((unused))
  #define FATTR_NONNULL_ALL __attribute__((nonnull))
  #define FATTR_NONNULL_ARG(...) __attribute__((nonnull(__VA_ARGS__)))

  #ifdef __clang__
    // clang only
  #elif defined(__INTEL_COMPILER)
    // intel only
  #else
    // gcc only
    #define GCC_VERSION \
          (__GNUC__ * 10000 + \
            __GNUC_MINOR__ * 100 + \
            __GNUC_PATCHLEVEL__)
    #define FATTR_ALLOC_SIZE(x) __attribute__((alloc_size(x)))
    #define FATTR_ALLOC_SIZE_PROD(x, y) __attribute__((alloc_size(x, y)))
    #if GCC_VERSION >= 40900
      #define FATTR_NONNULL_RET __attribute__((returns_nonnull))
    #endif
  #endif
#endif

// define function attributes that haven't been defined for this specific
// compiler.

#ifndef FATTR_MALLOC
  #define FATTR_MALLOC
#endif

#ifndef FATTR_ALLOC_SIZE
  #define FATTR_ALLOC_SIZE(x)
#endif

#ifndef FATTR_ALLOC_SIZE_PROD
  #define FATTR_ALLOC_SIZE_PROD(x, y)
#endif

#ifndef FATTR_ALLOC_ALIGN
  #define FATTR_ALLOC_ALIGN(x)
#endif

#ifndef FATTR_PURE
  #define FATTR_PURE
#endif

#ifndef FATTR_CONST
  #define FATTR_CONST
#endif

#ifndef FATTR_WARN_UNUSED_RESULT
  #define FATTR_WARN_UNUSED_RESULT
#endif

#ifndef FATTR_ALWAYS_INLINE
  #define FATTR_ALWAYS_INLINE
#endif

#ifndef FATTR_UNUSED
  #define FATTR_UNUSED
#endif

#ifndef FATTR_NONNULL_ALL
  #define FATTR_NONNULL_ALL
#endif

#ifndef FATTR_NONNULL_ARG
  #define FATTR_NONNULL_ARG(...)
#endif

#ifndef FATTR_NONNULL_RET
  #define FATTR_NONNULL_RET
#endif
// }}}

// Macro for logging that takes printf-style arguments.
// This version requires <cstdio>, but it can be defined differently before
// including proto3d headers.
#ifndef PROTO3D_TRACE
# include <cstdio>  // NOLINT
# define PROTO3D_TRACE(...) fprintf(stderr, __VA_ARGS__); fflush(stderr)
#endif

#ifndef PROTO3D_CHECK_GL_ERROR
# ifdef NDEBUG
#  define PROTO3D_CHECK_GL_ERROR(gl_proc_name_str)
# else
namespace proto3d { namespace gl { const char *LastErrorString(); } };
#  define PROTO3D_CHECK_GL_ERROR(gl_proc_name_str) \
  { \
    const char *_last_error_str = proto3d::gl::LastErrorString(); \
    if (_last_error_str != nullptr) { \
      PROTO3D_TRACE( \
          "OpenGL error:%s:%d: %s is set after %s.\n", \
          __FILE__, __LINE__, _last_error_str, (gl_proc_name_str)); \
    } \
  }
# endif
#endif

#ifdef PROTO3D_USE_GL_VERSION_4_3
extern "C" {
  void proto3d_DebugMessageCallbackImpl(
      GLenum source,
      GLenum type,
      GLuint id,
      GLenum severity,
      GLsizei length,
      const GLchar *message,
      void *user_param);
}
#endif

namespace proto3d {

#ifdef PROTO3D_USE_STB
namespace stb {
// STB Image {{{
class Image {
 private:
  unsigned char *stb_buffer;

 public:
  int width;
  int height;
  int pixel_format;

  Image(unsigned char *buf, int _width, int _height, int comp)
      : stb_buffer(buf),
        width(_width),
        height(_height),
        pixel_format(comp) {}

  ~Image() {
    stbi_image_free(stb_buffer);
  }

  unsigned char *raw() {
    return stb_buffer;
  }

  GLenum GLPixelFormat() {
    switch (pixel_format) {
      case STBI_grey:
        /* return GL_LUMINANCE; */
      case STBI_grey_alpha:
        /* return GL_LUMINANCE_ALPHA; */
      case STBI_rgb:
        return GL_RGB;
      case STBI_rgb_alpha:
        return GL_RGBA;
      default:
        assert(false && "Invalid stb::Image pixel format");
        return GL_ZERO;
    }
  }

  /// Call `stbi_load` to load an image from a file and wraps the buffer in a
  /// proto3d::stb::Image class.
  ///
  /// POSSIBLE_CRASH: if sbti_load fails opening the file or llocating memory.
  /// Check if the return value is nullptr.
  ///
  /// @param req_comp STBI_rgb, STBI_rgb_alpha, ...
  /// @return nullptr in case of failure
  static std::unique_ptr<Image> CreateFromFile(char const *filename,
                                               int req_comp = STBI_rgb) {
    int width, height, comp;
    unsigned char *stb_buffer = stbi_load(filename,
                                          &width,
                                          &height,
                                          &comp,
                                          req_comp);
    if (stb_buffer == nullptr) {
      return nullptr;
    }
    return std::unique_ptr<Image>(new Image(stb_buffer, width, height, comp));
  }
};
// }}} END of STB Image
}  // namespace stb
#endif  // PROTO3D_USE_STB

namespace gl {

namespace detail {
// Uniform setters {{{
// UniformSetterFn covers glUniform{1|2|3|4}{f|i|ui}

template<typename T>
struct UniformSetterFn {
  void operator()(GLint location, T v0) {
    glUniform1f(location, v0);
    PROTO3D_CHECK_GL_ERROR("glUniform1f");
  }

  void operator()(GLint location, T v0, T v1) {
    glUniform2f(location, v0, v1);
    PROTO3D_CHECK_GL_ERROR("glUniform2f");
  }

  void operator()(GLint location, T v0, T v1, T v2) {
    glUniform3f(location, v0, v1, v2);
    PROTO3D_CHECK_GL_ERROR("glUniform3f");
  }

  void operator()(GLint location, T v0, T v1, T v2, T v3) {
    glUniform4f(location, v0, v1, v2, v3);
    PROTO3D_CHECK_GL_ERROR("glUniform4f");
  }
};

template<>
struct UniformSetterFn<GLint> {
  void operator()(GLint location, GLint v0) {
    glUniform1i(location, v0);
    PROTO3D_CHECK_GL_ERROR("glUniform1i");
  }

  void operator()(GLint location, GLint v0, GLint v1) {
    glUniform2i(location, v0, v1);
    PROTO3D_CHECK_GL_ERROR("glUniform2i");
  }

  void operator()(GLint location, GLint v0, GLint v1, GLint v2) {
    glUniform3i(location, v0, v1, v2);
    PROTO3D_CHECK_GL_ERROR("glUniform3i");
  }

  void operator()(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    glUniform4i(location, v0, v1, v2, v3);
    PROTO3D_CHECK_GL_ERROR("glUniform4i");
  }
};

template<>
struct UniformSetterFn<GLuint> {
  void operator()(GLint location, GLuint v0) {
    glUniform1ui(location, v0);
    PROTO3D_CHECK_GL_ERROR("glUniform1ui");
  }

  void operator()(GLint location, GLuint v0, GLuint v1) {
    glUniform2ui(location, v0, v1);
    PROTO3D_CHECK_GL_ERROR("glUniform2ui");
  }

  void operator()(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    glUniform3ui(location, v0, v1, v2);
    PROTO3D_CHECK_GL_ERROR("glUniform3ui");
  }

  void operator()(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    glUniform4ui(location, v0, v1, v2, v3);
    PROTO3D_CHECK_GL_ERROR("glUniform4ui");
  }
};

// Vec{1|2|3|4}UniformSetterFn covers glUniform{1|2|3|4}{f|i|ui}v

/// @param T the type of the vector component
template<typename T>
struct UniformVec1SetterFn;

// UniformVec1SetterFn {{{

template<>
struct UniformVec1SetterFn<GLfloat> {
  void operator()(GLint location, GLsizei count, const GLfloat* value_ptr) {
    glUniform1fv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform1fv");
  }
};

template<>
struct UniformVec1SetterFn<GLint> {
  void operator()(GLint location, GLsizei count, const GLint* value_ptr) {
    glUniform1iv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform1iv");
  }
};

template<>
struct UniformVec1SetterFn<GLuint> {
  void operator()(GLint location, GLsizei count, const GLuint* value_ptr) {
    glUniform1uiv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform1uiv");
  }
};

// }}}

/// @param T the type of the vector component
template<typename T>
struct UniformVec2SetterFn;

// UniformVec2SetterFn {{{

template<>
struct UniformVec2SetterFn<GLfloat> {
  void operator()(GLint location, GLsizei count, const GLfloat* value_ptr) {
    glUniform2fv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform2fv");
  }
};

template<>
struct UniformVec2SetterFn<GLint> {
  void operator()(GLint location, GLsizei count, const GLint* value_ptr) {
    glUniform2iv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform2iv");
  }
};

template<>
struct UniformVec2SetterFn<GLuint> {
  void operator()(GLint location, GLsizei count, const GLuint* value_ptr) {
    glUniform2uiv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform2uiv");
  }
};

// }}}

/// @param T the type of the vector component
template<typename T>
struct UniformVec3SetterFn;

// UniformVec3SetterFn {{{

template<>
struct UniformVec3SetterFn<GLfloat> {
  void operator()(GLint location, GLsizei count, const GLfloat* value_ptr) {
    glUniform3fv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform3fv");
  }
};

template<>
struct UniformVec3SetterFn<GLint> {
  void operator()(GLint location, GLsizei count, const GLint* value_ptr) {
    glUniform3iv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform3iv");
  }
};

template<>
struct UniformVec3SetterFn<GLuint> {
  void operator()(GLint location, GLsizei count, const GLuint* value_ptr) {
    glUniform3uiv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform3uiv");
  }
};

// }}}

/// @param T the type of the vector component
template<typename T>
struct UniformVec4SetterFn;

// UniformVec4SetterFn {{{

template<>
struct UniformVec4SetterFn<GLfloat> {
  void operator()(GLint location, GLsizei count, const GLfloat* value_ptr) {
    glUniform4fv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform4fv");
  }
};

template<>
struct UniformVec4SetterFn<GLint> {
  void operator()(GLint location, GLsizei count, const GLint* value_ptr) {
    glUniform4iv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform4iv");
  }
};

template<>
struct UniformVec4SetterFn<GLuint> {
  void operator()(GLint location, GLsizei count, const GLuint* value_ptr) {
    glUniform4uiv(location, count, value_ptr);
    PROTO3D_CHECK_GL_ERROR("glUniform4uiv");
  }
};

// }}}

// GLM versions {{{
#ifdef PROTO3D_USE_GLM

struct UniformGLMVecSetterFn {
  void operator()(GLint location, const glm::vec2& value) {
    glUniform2fv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2fv");
  }

  void operator()(GLint location, const glm::vec3& value) {
    glUniform3fv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform3fv");
  }

  void operator()(GLint location, const glm::vec4& value) {
    glUniform4fv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform4fv");
  }

  void operator()(GLint location, const glm::ivec2& value) {
    glUniform2iv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2iv");
  }

  void operator()(GLint location, const glm::ivec3& value) {
    glUniform2iv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2iv");
  }

  void operator()(GLint location, const glm::ivec4& value) {
    glUniform2iv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2iv");
  }

  void operator()(GLint location, const glm::uvec2& value) {
    glUniform2uiv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2uiv");
  }

  void operator()(GLint location, const glm::uvec3& value) {
    glUniform2uiv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2uiv");
  }

  void operator()(GLint location, const glm::uvec4& value) {
    glUniform2uiv(location, 1, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniform2uiv");
  }
};

struct UniformGLMMatSetterFn {
  void operator()(GLint location, GLboolean transpose, const glm::mat2& value) {
    glUniformMatrix2fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix2fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat3& value) {
    glUniformMatrix3fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix3fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat4& value) {
    glUniformMatrix4fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix4fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat2x3& value) {
    glUniformMatrix2x3fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix2x3fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat3x2& value) {
    glUniformMatrix3x2fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix3x2fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat2x4& value) {
    glUniformMatrix2x4fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix2x4fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat4x2& value) {
    glUniformMatrix4x2fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix4x2fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat3x4& value) {
    glUniformMatrix3x4fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix3x4fv");
  }

  void operator()(GLint location, GLboolean transpose, const glm::mat4x3& value) {
    glUniformMatrix4x3fv(location, 1, transpose, glm::value_ptr(value));
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix4x3fv");
  }
};

#endif
// }}}
// }}} END of Uniform setters

// Texture (detail) {{{

/// Sets a texture parameter of type T.
template<typename T>
struct TexParameterSetterFn {
  void operator()(GLenum target, GLenum pname, T param) {
    glTexParameteri(target, pname, param);
    PROTO3D_CHECK_GL_ERROR("glTexParameteri");
  }
};

template<>
struct TexParameterSetterFn<GLfloat> {
  void operator()(GLenum target, GLenum pname, GLfloat param) {
    glTexParameterf(target, pname, param);
    PROTO3D_CHECK_GL_ERROR("glTexParameterf");
  }
};

/// Sets a vector (e.g. border color) texture parameter of component type `T`.
/// `InternalFormatT` refers to the type OpenGL actually uses internally to store
/// the values. If you don't care about the internal format used it will most
/// likely be `GLfloat`. If you want integers to remain integers inside OpenGl
/// memory, use `GL[u]int` as `InternalFormatT` template parameter.
///
/// http://stackoverflow.com/questions/27787418/whats-the-use-case-of-gltexparameteriiv-and-gltexparameteriuiv/27788014
template<typename T, typename InternalFormatT = GLfloat>
struct VecTexParameterSetterFn;

template<>
struct VecTexParameterSetterFn<GLint> {
  void operator()(GLenum target, GLenum pname, const GLint* param) {
    glTexParameteriv(target, pname, param);
    PROTO3D_CHECK_GL_ERROR("glTexParameteriv");
  }
};

template<>
struct VecTexParameterSetterFn<GLfloat> {
  void operator()(GLenum target, GLenum pname, const GLfloat* param) {
    glTexParameterfv(target, pname, param);
    PROTO3D_CHECK_GL_ERROR("glTexParameterfv");
  }
};

template<>
struct VecTexParameterSetterFn<GLint, GLint> {
  void operator()(GLenum target, GLenum pname, const GLint* param) {
    glTexParameterIiv(target, pname, param);
    PROTO3D_CHECK_GL_ERROR("glTexParameterIiv");
  }
};

template<>
struct VecTexParameterSetterFn<GLuint, GLuint> {
  void operator()(GLenum target, GLenum pname, const GLuint* param) {
    glTexParameterIuiv(target, pname, param);
    PROTO3D_CHECK_GL_ERROR("glTexParameterIuiv");
  }
};
// }}} END of Texture (detail)
}  // namespace detail

// OpenGL debugging utilities {{{
#ifdef PROTO3D_USE_GL_VERSION_4_3
// OpenGL 4.3 only
void ReportAllErrorsViaCallback(const char *origin);
#endif

const char *LastErrorString();
const char *FramebufferStatusString();
void CheckLeaks();
// }}} END of OpenGL debugging utilities

// OpenGL Objects {{{

/// OpenGL Vertex Array Objects
class VAO {
 public:
  GLuint id;

  VAO(GLuint _id) : id(_id) {}  // NOLINT

  VAO() = default;

  void Create() {
    glGenVertexArrays(1, &id);
    PROTO3D_CHECK_GL_ERROR("glGenVertexArrays");
  }

  void Delete() {
    glDeleteVertexArrays(1, &id);
    PROTO3D_CHECK_GL_ERROR("glDeleteVertexArrays");
  }

  void Bind() const {
    glBindVertexArray(id);
    PROTO3D_CHECK_GL_ERROR("glBindVertexArray");
  }

  void Unbind() const {
    glBindVertexArray(0);
    PROTO3D_CHECK_GL_ERROR("glBindVertexArray");
  }
};

void Create(VAO *vao_arr, GLuint count);
void Delete(VAO *vao_arr, GLuint count);

#ifdef PROTO3D_IMPLEMENTATION
void Create(VAO *vao_arr, GLuint count) {
  glGenVertexArrays(count, reinterpret_cast<GLuint *>(vao_arr));
  PROTO3D_CHECK_GL_ERROR("glGenVertexArrays");
}

void Delete(VAO *vao_arr, GLuint count) {
  glDeleteVertexArrays(count, reinterpret_cast<GLuint *>(vao_arr));
  PROTO3D_CHECK_GL_ERROR("glDeleteVertexArrays");
}
#endif  // PROTO3D_IMPLEMENTATION

/// OpenGL Vertex Buffer Objects
class VBO {
 public:
  GLuint id;

  VBO(GLuint _id) : id(_id) {}  // NOLINT

  VBO() = default;

  void Create() {
    glGenBuffers(1, &id);
    PROTO3D_CHECK_GL_ERROR("glGenBuffers");
  }

  void Delete() {
    glDeleteBuffers(1, &id);
    PROTO3D_CHECK_GL_ERROR("glDeleteBuffers");
  }

  void Bind(GLenum target) const {
    glBindBuffer(target, id);
    PROTO3D_CHECK_GL_ERROR("glBindBuffer");
  }

  void Unbind(GLenum target) {
    glBindBuffer(target, 0);
    PROTO3D_CHECK_GL_ERROR("glBindBuffer");
  }

  void Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    PROTO3D_CHECK_GL_ERROR("glBindBuffer");
  }

  void Unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    PROTO3D_CHECK_GL_ERROR("glBindBuffer");
  }
};

void Create(VBO *vbo_arr, GLuint count);
void Delete(VBO *vbo_arr, GLuint count);

#ifdef PROTO3D_IMPLEMENTATION
void Create(VBO *vbo_arr, GLuint count) {
  glGenBuffers(count, reinterpret_cast<GLuint *>(vbo_arr));
  PROTO3D_CHECK_GL_ERROR("glGenBuffers");
}

void Delete(VBO *vbo_arr, GLuint count) {
  glDeleteBuffers(count, reinterpret_cast<GLuint *>(vbo_arr));
  PROTO3D_CHECK_GL_ERROR("glDeleteBuffers");
}
#endif  // PROTO3D_IMPLEMENTATION
// }}} END of OpenGL objects

// OpenGL Shaders {{{

/// OpenGL Shader class
///
/// Usage example:
///
///     Shader shader;
///     shader.Create(GL_VERTEX_SHADER);
///     // You can get the type
///     assert(shader.GetType() == GL_VERTEX_SHADER);
///     // set the source from a C string or a proto3d::sdl::SourceFile
///     shader.SetSource(sdl::SourceFile("shader.vert");
///     // and compile the shader.
///     std::unique_ptr<char> compilation_error = shader.Compile();
///     if (compilation_error == nullptr) {
///       bool compiled = shader.IsCompiled(); // true
///       ...
///     }
///     // Shaders have to be deleted manually
///     shader.Delete();
class Shader {
 public:
  /// The shader name.
  GLuint id;

  Shader() = default;

  Shader(GLuint _id) : id(_id) {}  // NOLINT

  /// Create the Shader in OpenGL memory.
  ///
  /// @param shader_type Specifies the type of shader to be created.
  ///                    Must be one of GL_VERTEX_SHADER,
  ///                    GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER.
  void Create(GLenum shader_type) {
    id = glCreateShader(shader_type);
    PROTO3D_CHECK_GL_ERROR("glCreateShader");
  }

  void Delete() {
    // It's OK to delete a shader even if it's linked to one or many programs.
    // The shader will simply be flagged for deletion and once the programs are
    // deleted, the shader is finally deleted.
    //
    // Program::Link(shaders...) attaches, links, and then detaches the shaders.
    // It's recommended that you Delete(shaders...) after linking.
    glDeleteShader(id);
    PROTO3D_CHECK_GL_ERROR("glDeleteShader");
  }

  void SetSource(const char *source) {
    glShaderSource(id, 1, &source, nullptr);
    PROTO3D_CHECK_GL_ERROR("glShaderSource");
  }

  void SetSources(GLsizei count, const char **sources) {
    glShaderSource(id, count, sources, nullptr);
    PROTO3D_CHECK_GL_ERROR("glShaderSource");
  }

  /// @return nullptr if success or the compilation error message in case of
  /// failure
  std::unique_ptr<char> Compile() {
    glCompileShader(id);
    PROTO3D_CHECK_GL_ERROR("glCompileShader");
    if (IsCompiled()) {
      return nullptr;
    }
    return GetInfoLog();
  }

  std::unique_ptr<char> GetInfoLog(GLsizei *length_ptr = nullptr) {
    char *info_log;

    // The size of the info log. size is better name than length because it
    // includes the 0 terminating byte. size may be 0 though, in case it has no
    // information log.
    GLint size;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &size);
    PROTO3D_CHECK_GL_ERROR("glGetShaderiv");

    if (size == 0) {
      return nullptr;
    }
    info_log = new char[size + 1];
    glGetShaderInfoLog(id, size, length_ptr, info_log);
    PROTO3D_CHECK_GL_ERROR("glGetShaderInfoLog");

    return std::unique_ptr<char>(info_log);
  }

  /// Returns the concatenation of the source strings that make up the shader
  /// source for the shader, including the null termination character.
  std::unique_ptr<char> GetSource(GLsizei *length_ptr = nullptr) {
    char *source;

    // The size includes the 0 terminating character.
    GLint size;
    glGetShaderiv(id, GL_SHADER_SOURCE_LENGTH, &size);
    PROTO3D_CHECK_GL_ERROR("glGetShaderiv");

    if (size == 0) {
      return nullptr;
    }
    source = new char[size + 1];
    glGetShaderSource(id, size, length_ptr, source);
    PROTO3D_CHECK_GL_ERROR("glGetShaderSource");

    return std::unique_ptr<char>(source);
  }

  GLenum GetType() {
    GLint type;
    glGetShaderiv(id, GL_SHADER_TYPE, &type);
    PROTO3D_CHECK_GL_ERROR("glGetShaderiv");
    return type;
  }

  /// Returns whether the shader is flagged for deletion.
  bool IsDeleted() {
    GLint deleted;
    glGetShaderiv(id, GL_DELETE_STATUS, &deleted);
    PROTO3D_CHECK_GL_ERROR("glGetShaderiv");
    return deleted == GL_TRUE;
  }

  bool IsCompiled() {
    GLint compiled;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
    PROTO3D_CHECK_GL_ERROR("glGetShaderiv");
    return compiled == GL_TRUE;
  }
};

class Program {
 public:
  /// The program name.
  GLuint id;

  Program() = default;

  Program(GLuint _id) : id(_id) {}  // NOLINT

  void Create() {
    id = glCreateProgram();
    PROTO3D_CHECK_GL_ERROR("glCreateProgram");
  }

  void Delete() {
    glDeleteProgram(id);
    PROTO3D_CHECK_GL_ERROR("glDeleteProgram");
  }

  void AttachShaders(Shader shader) {
    glAttachShader(id, shader.id);
    PROTO3D_CHECK_GL_ERROR("glAttachShader");
  }

  /// Attach shaders to this program.
  template<class Shader, class... Shaders>
  void AttachShaders(Shader shader, Shaders... shaders) {
    glAttachShader(id, shader.id);
    PROTO3D_CHECK_GL_ERROR("glAttachShader");
    AttachShaders(shaders...);
  }

  void AttachShaders(Shader *shaders, int size) {
    for (int i = 0; i < size; i++) {
      AttachShaders(shaders[i]);
    }
  }

  void DetachShaders(Shader shader) {
    glDetachShader(id, shader.id);
    PROTO3D_CHECK_GL_ERROR("glDetachShader");
  }

  /// Detach shaders from this program.
  template<class... Shaders>
  void DetachShaders(Shader shader, Shaders... shaders) {
    glDetachShader(id, shader.id);
    PROTO3D_CHECK_GL_ERROR("glDetachShader");
    DetachShaders(shaders...);
  }

  void DetachShaders(Shader *shaders, int size) {
    for (int i = 0; i < size; i++) {
      DetachShaders(shaders[i]);
    }
  }

  /// Link all the attached shaders to finish building the program.
  std::unique_ptr<char> Link() {
    glLinkProgram(id);
    PROTO3D_CHECK_GL_ERROR("glLinkProgram");
    if (IsLinked()) {
      return nullptr;
    }
    return GetInfoLog();
  }

  template<class Shader, class... Shaders>
  std::unique_ptr<char> Link(Shader shader, Shaders... shaders) {
    AttachShaders(shader, shaders...);
    auto message = Link();
    DetachShaders(shader, shaders...);
    return message;
  }

  std::unique_ptr<char> Link(Shader *shaders, int size) {
    AttachShaders(shaders, size);
    auto message = Link();
    DetachShaders(shaders, size);
    return message;
  }

  std::unique_ptr<char> GetInfoLog(GLsizei *length_p = nullptr) {
    char *info_log;

    // The size of the info log. size is better name than length because it
    // includes the 0 terminating byte. size may be 0 though, in case it has no
    // information log.
    GLint size;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &size);
    PROTO3D_CHECK_GL_ERROR("glGetProgramiv");

    if (size == 0) {
      return nullptr;
    }
    info_log = new char[size + 1];
    glGetProgramInfoLog(id, size, length_p, info_log);
    PROTO3D_CHECK_GL_ERROR("glGetProgramInfoLog");
    return std::unique_ptr<char>(info_log);
  }

  std::unique_ptr<char> ValidationLog(bool *is_valid, GLsizei *length_ptr = nullptr) {
    // Validate the program
    glValidateProgram(id);
    PROTO3D_CHECK_GL_ERROR("glValidateProgram");

    // Query the validation status
    GLint is_valid_i;
    glGetProgramiv(id, GL_VALIDATE_STATUS, &is_valid_i);
    PROTO3D_CHECK_GL_ERROR("glGetProgramiv");
    *is_valid = is_valid_i;

    // Return the info log that contains validation information
    return GetInfoLog(length_ptr);
  }

  std::unique_ptr<Shader> GetAttachedShaders(GLint *count_ptr) {
    glGetProgramiv(id, GL_ATTACHED_SHADERS, count_ptr);
    PROTO3D_CHECK_GL_ERROR("glGetProgramiv");
    if (*count_ptr == 0) {
      return nullptr;
    }

    auto shaders = new GLuint[*count_ptr];
    glGetAttachedShaders(id, *count_ptr, count_ptr, shaders);
    PROTO3D_CHECK_GL_ERROR("glGetAttachedShaders");

    return std::unique_ptr<Shader>(reinterpret_cast<Shader *>(shaders));
  }

  bool IsLinked() {
    GLint linked;
    glGetProgramiv(id, GL_LINK_STATUS, &linked);
    PROTO3D_CHECK_GL_ERROR("glGetProgramiv");
    return linked == GL_TRUE;
  }

  bool IsDeleted() {
    GLint deleted;
    glGetProgramiv(id, GL_DELETE_STATUS, &deleted);
    PROTO3D_CHECK_GL_ERROR("glGetProgramiv");
    return deleted == GL_TRUE;
  }

  void Use() const {
    glUseProgram(id);
    PROTO3D_CHECK_GL_ERROR("glUseProgram");
  }

  void Bind() const {
    glUseProgram(id);
    PROTO3D_CHECK_GL_ERROR("glUseProgram");
  }

  void Unbind() const {
    glUseProgram(0);
    PROTO3D_CHECK_GL_ERROR("glUseProgram");
  }

  /// Get the location of an attribute variable.
  ///
  /// Attribute variables can have a different value for each vertex (e.g. the
  /// color of a corner of a triangle).
  GLint AttribLocation(const GLchar* attrib_name) const {
    GLint location = glGetAttribLocation(id, attrib_name);
    PROTO3D_CHECK_GL_ERROR("glGetAttribLocation");
    return location;
  }

  /// Get the location of an uniform variable.
  ///
  /// Uniform variables keep the same value for multiple vertices (e.g. the
  /// color of the whole triangle).
  GLint UniformLocation(const GLchar* uniform_name) const {
    GLint location = glGetUniformLocation(id, uniform_name);
    PROTO3D_CHECK_GL_ERROR("glGetUniformLocation");
    return location;
  }

  // Uniform setters using locations {{{

  /// Sets a shader uniform with up to 4 parameters for the vector components.
  template<typename T, typename... U>
  void SetUniform(GLint location, T v0, U... values) {
    detail::UniformSetterFn<T> setter;
    setter(location, v0, values...);
  }

  // Versions for pointer parameters

  template<typename T>
  void SetUniformVec1(GLint location, GLsizei count, const T* value_ptr) {
    detail::UniformVec1SetterFn<T> setter;
    setter(location, count, value_ptr);
  }

  template<typename T>
  void SetUniformVec2(GLint location, GLsizei count, const T* value_ptr) {
    detail::UniformVec2SetterFn<T> setter;
    setter(location, count, value_ptr);
  }

  template<typename T>
  void SetUniformVec3(GLint location, GLsizei count, const T* value_ptr) {
    detail::UniformVec3SetterFn<T> setter;
    setter(location, count, value_ptr);
  }

  template<typename T>
  void SetUniformVec4(GLint location, GLsizei count, const T* value_ptr) {
    detail::UniformVec4SetterFn<T> setter;
    setter(location, count, value_ptr);
  }

#define UNIFORM_MATRIX_SETTER(DIMENSIONS) \
  void SetUniformMat ## DIMENSIONS(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value_ptr) { \
    glUniformMatrix ## DIMENSIONS ## fv(location, count, transpose, value_ptr); \
    PROTO3D_CHECK_GL_ERROR("glUniformMatrix"); \
  }

  UNIFORM_MATRIX_SETTER(2)
  UNIFORM_MATRIX_SETTER(3)
  UNIFORM_MATRIX_SETTER(4)
  UNIFORM_MATRIX_SETTER(2x3)
  UNIFORM_MATRIX_SETTER(3x2)
  UNIFORM_MATRIX_SETTER(2x4)
  UNIFORM_MATRIX_SETTER(4x2)
  UNIFORM_MATRIX_SETTER(3x4)
  UNIFORM_MATRIX_SETTER(4x3)

#undef UNIFORM_MATRIX_SETTER

#ifdef PROTO3D_USE_GLM
  // Versions for GLM types

  /// Sets a GLM vector as a shader uniform variable.
  template<class T>
  void SetVecUniform(GLint location, const T& value) {
    detail::UniformGLMVecSetterFn setter;
    setter(location, value);
  }

  /// Sets a GLM matrix as a shader uniform variable.
  template<class T>
  void SetUniformMat(GLint location, const T& value) {
    detail::UniformGLMMatSetterFn setter;
    setter(location, value, GL_FALSE);
  }

  /// Sets a GLM matrix as a shader uniform variable.
  template<class T>
  void SetMatUniform(GLint location, GLboolean transpose, const T& value) {
    detail::UniformGLMMatSetterFn setter;
    setter(location, value, transpose);
  }
#endif  // PROTO3D_USE_GLM

  // }}}

  // Uniform setters using names {{{

  /// Sets a shader uniform variable.
  ///
  /// @return the location of the uniform variable of -1 in case of error
  template<typename... T>
  GLint SetUniform(const GLchar* uniform_name, T... values) {
    GLint location = UniformLocation(uniform_name);
    if (location >= 0) {
      SetUniform(location, values...);
    }
    return location;
  }

#define UNIFORM_VECTOR_SETTER_BY_NAME(DIMENSION) \
  template<typename T> \
  GLint SetVec ## DIMENSION ## Uniform(const GLchar *uniform_name, GLsizei count, const T *value_ptr) { \
    GLint location = UniformLocation(uniform_name); \
    if (location >= 0) { \
      SetVec ## DIMENSION ## Uniform(location, count, value_ptr); \
    } \
    return location; \
  }

  UNIFORM_VECTOR_SETTER_BY_NAME(1)
  UNIFORM_VECTOR_SETTER_BY_NAME(2)
  UNIFORM_VECTOR_SETTER_BY_NAME(3)
  UNIFORM_VECTOR_SETTER_BY_NAME(4)

#undef UNIFORM_VECTOR_SETTER_BY_NAME

#define UNIFORM_MATRIX_SETTER_BY_NAME(DIMENSIONS) \
  GLint Set ## DIMENSIONS ## UniformMat(const GLchar* uniform_name, GLsizei count, GLboolean transpose, const GLfloat* value_ptr) { \
    GLint location = UniformLocation(uniform_name); \
    if (location >= 0) { \
      glUniformMatrix ## DIMENSIONS ## fv(location, count, transpose, value_ptr); \
    } \
    return location; \
  }

  UNIFORM_MATRIX_SETTER_BY_NAME(2)
  UNIFORM_MATRIX_SETTER_BY_NAME(3)
  UNIFORM_MATRIX_SETTER_BY_NAME(4)
  UNIFORM_MATRIX_SETTER_BY_NAME(2x3)
  UNIFORM_MATRIX_SETTER_BY_NAME(3x2)
  UNIFORM_MATRIX_SETTER_BY_NAME(2x4)
  UNIFORM_MATRIX_SETTER_BY_NAME(4x2)
  UNIFORM_MATRIX_SETTER_BY_NAME(3x4)
  UNIFORM_MATRIX_SETTER_BY_NAME(4x3)

#undef UNIFORM_MATRIX_SETTER_BY_NAME

  /// Sets a shader uniform variable.
  ///
  /// @return the location of the uniform variable of -1 in case of error
  template<class T>
  GLint SetVecUniform(const GLchar *uniform_name, const T& value) {
    GLint location = UniformLocation(uniform_name);
    if (location >= 0) {
      SetVecUniform(location, value);
    }
    return location;
  }

  /// Sets a shader uniform variable.
  ///
  /// @return the location of the uniform variable of -1 in case of error
  template<class T>
  GLint SetUniformMat(const GLchar* uniform_name, GLboolean transpose, const T& value) {
    GLint location = UniformLocation(uniform_name);
    if (location >= 0) {
      SetUniformMat(location, transpose, value);
    }
    return location;
  }

  /// Sets a shader uniform variable.
  ///
  /// @return the location of the uniform variable of -1 in case of error
  template<class T>
  GLint SetUniformMat(const GLchar* uniform_name, const T& value) {
    GLint location = UniformLocation(uniform_name);
    if (location >= 0) {
      SetUniformMat(location, value);
    }
    return location;
  }

  // }}}
};
// }}} END of OpenGL Shaders

// OpenGL Textures {{{

class Texture {
 public:
  GLuint id;

  Texture() = default;

  Texture(GLuint _id) : id(_id) {}  // NOLINT

  void Gen() {
    glGenTextures(1, &id);
    PROTO3D_CHECK_GL_ERROR("glGenTextures");
  }

  void Delete() {
    glDeleteTextures(1, &id);
    PROTO3D_CHECK_GL_ERROR("glDeleteTextures");
  }
};

namespace detail {

template<GLenum kTarget, GLenum kBinding>
class TextureCommonTemplate : public Texture {
 public:
  TextureCommonTemplate() = default;
  TextureCommonTemplate(GLuint id) : Texture(id) {}  // NOLINT

  void Bind() const {
    glBindTexture(kTarget, id);
    PROTO3D_CHECK_GL_ERROR("glBindTexture");
  }

  void Unbind() const {
    glBindTexture(kTarget, 0);
    PROTO3D_CHECK_GL_ERROR("glBindTexture");
  }

  bool Bound() {
    GLint current_texture;
    glGetIntegerv(kBinding, &current_texture);
    PROTO3D_CHECK_GL_ERROR("glGetIntegerv");
    return this->id && this->id == (GLuint)current_texture;
  }

  template<typename T>
  void SetParameter(GLenum pname, T param) {
    assert(Bound());
    detail::TexParameterSetterFn<T> setter;
    setter(kTarget, pname, param);
  }

  template<typename T, typename InternalFormatT = GLfloat>
  void SetVecParameter(GLenum pname, const T *params) {
    assert(Bound());
    detail::VecTexParameterSetterFn<T, InternalFormatT> setter;
    setter(kTarget, pname, params);
  }
};

};  // namespace detail

// Since OpenGL 1.1
class Texture2D : public detail::TextureCommonTemplate<GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D> {
 public:
  Texture2D() = default;
  Texture2D(GLuint id)  // NOLINT
    : detail::TextureCommonTemplate<GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D>(id) {}

  /// Sets GL_TEXTURE_WRAP_(S|T) and GL_TEXTURE_(MIN|MAG)_FILTER
  ///
  /// @param wrap
  ///   GL_CLAMP_TO_EDGE
  ///   GL_CLAMP_TO_BORDER
  ///   GL_REPEAT
  ///   GL_MIRRORED_REPEAT.
  ///
  /// @param filter
  ///  - `GL_NEAREST`: Returns the pixel that is closest to the coordinates.
  ///  - `GL_LINEAR`:  Returns the weighted average of the 4 pixels surrounding the
  ///    given coordinates.
  ///  - `GL_NEAREST_MIPMAP_NEAREST`, `GL_LINEAR_MIPMAP_NEAREST`,
  ///  `GL_NEAREST_MIPMAP_LINEAR`, `GL_LINEAR_MIPMAP_LINEAR`: Sample from mipmaps
  ///  instead.
  ///
  void SetFilterAndWrap(GLint filter = GL_LINEAR, GLint wrap = GL_CLAMP_TO_EDGE) {
    assert(Bound());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    PROTO3D_CHECK_GL_ERROR("glTexParameteri");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    PROTO3D_CHECK_GL_ERROR("glTexParameteri");
  }

  void GenerateMipmaps() {
    assert(Bound());
    glGenerateMipmap(GL_TEXTURE_2D);
    PROTO3D_CHECK_GL_ERROR("glGenerateMipmap");
  }

  /// @param pixels For format=GL_RGBA it's a GLubyte[width][height][4] matrix
  void LoadImage(GLsizei width, GLsizei height, GLubyte *pixels, GLenum format = GL_RGBA) {
    assert(Bound());
    glTexImage2D(
        GL_TEXTURE_2D,  // target
        0,  // level (here the maximum level of detail)
        format,  // internalFormat (here the same as the image format)
        width,
        height,
        0,  // border (spec says it should always be 0)
        format,  // format
        GL_UNSIGNED_BYTE,  // color component datatype
        pixels);
    PROTO3D_CHECK_GL_ERROR("glTexImage2D");
  }

#ifdef PROTO3D_USE_STB
  /// Load an stb::Image into the texture
  ///
  /// @param level Specifies the level-of-detail number.  Level 0 is the base
  /// image level.  Level n is the nth mipmap reduction image.
  /// @param internal_format The texture internal format. It's the same as the
  /// image format by default.
  void LoadImage(proto3d::stb::Image *img, GLint level = 0, GLint internal_format = GL_INVALID_VALUE) {
    GLsizei width = img->width;
    GLsizei height = img->height;
    GLubyte *pixels = img->raw();
    GLenum format = img->GLPixelFormat();

    assert(Bound());
    glTexImage2D(
        GL_TEXTURE_2D,  // target
        level,
        (internal_format == GL_INVALID_VALUE) ? format : internal_format,  // internalFormat
        width,
        height,
        0,  // border (spec says it should always be 0)
        format,  // format
        GL_UNSIGNED_BYTE,  // color component datatype
        pixels);
    PROTO3D_CHECK_GL_ERROR("glTexImage2D");
  }
#endif  // PROTO3D_USE_STB
};

class Textures {
 public:
  GLuint *ids;
  GLsizei size;

  Textures() : ids(nullptr), size(0) {}

  Textures(Texture *textures, GLsizei _size) : size(_size) {
    ids = reinterpret_cast<GLuint *>(textures);
  }

  void Gen() {
    glGenTextures(size, ids);
    PROTO3D_CHECK_GL_ERROR("glGenTextures");
  }

  void Delete() {
    glDeleteTextures(size, ids);
    PROTO3D_CHECK_GL_ERROR("glDeleteTextures");
  }
};

class Textures2D : public Textures {
 public:
  Textures2D() : Textures() {}

  Textures2D(Texture2D *textures, GLsizei _size) : Textures(textures, _size) {}

  void Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
    PROTO3D_CHECK_GL_ERROR("glBindTexture");
  }

  Texture2D operator[](GLsizei i) const {
    return this->ids[i];
  }

  Texture2D& operator[](GLsizei i) {
    return reinterpret_cast<Texture2D&>(this->ids[i]);
  }
};
// }}} END of OpenGL Textures

namespace shader {
// Shader Facade {{{
#ifdef PROTO3D_USE_EXCEPTIONS
Shader Compile(GLenum shader_type, const char *source);
template<class... Shaders> Program Link(Shaders... shaders);
Program Link(proto3d::gl::Shader *shaders, int size);
Program CompileAndLink(GLenum shader_type, const char *source);
#endif
// }}} END of Facade
}  // namespace shader

}  // namespace gl

}  // namespace proto3d

#ifdef PROTO3D_IMPLEMENTATION
// OpenGL debugging utilities implementation {{{
#ifdef PROTO3D_USE_GL_VERSION_4_3

void proto3d_DebugMessageCallbackImpl(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *user_param) {

  PROTO3D_TRACE("OpenGL debug message callback called (%s):\n", (char *)user_param);

  char *source_str;
  switch (source) {
#ifdef GL_DEBUG_SOURCE_API
    case GL_DEBUG_SOURCE_API:
      source_str = "GL_DEBUG_SOURCE_API";
      break;
#endif
#ifdef GL_DEBUG_SOURCE_WINDOW_SYSTEM
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      source_str = "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
      break;
#endif
#ifdef GL_DEBUG_SOURCE_SHADER_COMPILER
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      source_str = "GL_DEBUG_SOURCE_SHADER_COMPILER";
      break;
#endif
#ifdef GL_DEBUG_SOURCE_THIRD_PARTY
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      source_str = "GL_DEBUG_SOURCE_THIRD_PARTY";
      break;
#endif
#ifdef GL_DEBUG_SOURCE_APPLICATION
    case GL_DEBUG_SOURCE_APPLICATION:
      source_str = "GL_DEBUG_SOURCE_APPLICATION";
      break;
#endif
#ifdef GL_DEBUG_SOURCE_OTHER
    case GL_DEBUG_SOURCE_OTHER:
      source_str = "GL_DEBUG_SOURCE_OTHER";
      break;
#endif
    default:
      source_str = "unknown GL_DEBUG_SOURCE";
      break;
  }
  PROTO3D_TRACE("source: %s\n", source_str);

  char *type_str;
  switch (type) {
#ifdef GL_DEBUG_TYPE_ERROR
    case GL_DEBUG_TYPE_ERROR:
      type_str = "GL_DEBUG_TYPE_ERROR";
      break;
#endif
#ifdef GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      type_str = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
      break;
#endif
#ifdef GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      type_str = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
      break;
#endif
#ifdef GL_DEBUG_TYPE_PORTABILITY
    case GL_DEBUG_TYPE_PORTABILITY:
      type_str = "GL_DEBUG_TYPE_PORTABILITY";
      break;
#endif
#ifdef GL_DEBUG_TYPE_PERFORMANCE
    case GL_DEBUG_TYPE_PERFORMANCE:
      type_str = "GL_DEBUG_TYPE_PERFORMANCE";
      break;
#endif
#ifdef GL_DEBUG_TYPE_MARKER
    case GL_DEBUG_TYPE_MARKER:
      type_str = "GL_DEBUG_TYPE_MARKER";
      break;
#endif
#ifdef GL_DEBUG_TYPE_PUSH_GROUP
    case GL_DEBUG_TYPE_PUSH_GROUP:
      type_str = "GL_DEBUG_TYPE_PUSH_GROUP";
      break;
#endif
#ifdef GL_DEBUG_TYPE_POP_GROUP
    case GL_DEBUG_TYPE_POP_GROUP:
      type_str = "GL_DEBUG_TYPE_POP_GROUP";
      break;
#endif
#ifdef GL_DEBUG_TYPE_OTHER
    case GL_DEBUG_TYPE_OTHER:
      type_str = "GL_DEBUG_TYPE_OTHER";
      break;
#endif
    default:
      type_str = "unknown GL_DEBUG_TYPE";
  }
  PROTO3D_TRACE("type: %s\n", type_str);

  // id
  PROTO3D_TRACE("id: %d\n", id);

  char *severity_str;
  switch (severity) {
#ifdef GL_DEBUG_SEVERITY_LOW
    case GL_DEBUG_SEVERITY_LOW:
      severity_str = "GL_DEBUG_SEVERITY_LOW";
      break;
#endif
#ifdef GL_DEBUG_SEVERITY_MEDIUM
    case GL_DEBUG_SEVERITY_MEDIUM:
      severity_str = "GL_DEBUG_SEVERITY_MEDIUM";
      break;
#endif
#ifdef GL_DEBUG_SEVERITY_HIGH
    case GL_DEBUG_SEVERITY_HIGH :
      severity_str = "GL_DEBUG_SEVERITY_HIGH ";
      break;
#endif
#ifdef GL_DEBUG_SEVERITY_NOTIFICATION
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      severity_str = "GL_DEBUG_SEVERITY_NOTIFICATION";
      break;
#endif
    default:
      severity_str = "unknown severity";
      break;
  }
  PROTO3D_TRACE("severity: %s\n", severity_str);

  PROTO3D_TRACE("message: %s\n", message);
}

void proto3d::gl::ReportAllErrorsViaCallback(const char *origin) {
  // Set the callback
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  PROTO3D_CHECK_GL_ERROR("glEnable");
  glDebugMessageCallback(&proto3d_DebugMessageCallbackImpl, nullptr);

  // Enable all messages
  glDebugMessageControl(
      GL_DONT_CARE, // all sources
      GL_DONT_CARE, // all types
      GL_DONT_CARE, // all severities
      0,
      nullptr,
      GL_TRUE);
  PROTO3D_CHECK_GL_ERROR("glDebugMessageControl");
}

#endif  // PROTO3D_USE_GL_VERSION_4_3

namespace proto3d {

namespace gl {

const char *LastErrorString() {
  GLenum error = glGetError();
  switch (error) {
    case GL_NO_ERROR: return nullptr;
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
    default: return "unknown error type";
  }
}

const char *FramebufferStatusString() {
  switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
    case GL_FRAMEBUFFER_UNSUPPORTED:
      return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
      return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
    case GL_FRAMEBUFFER_UNDEFINED:
      return "GL_FRAMEBUFFER_UNDEFINED";
    default: return nullptr;
  }
}

/// Check if we're leaking OpenGL objects. Very inneficient, use for debug only.
///
/// Idea and implementation by @rygorous.
void CheckLeaks() {
  // Let's assume names > 10000 do not exist
  GLuint max_id = 10000;

  // If brute force doesn't work, you're not applying it hard enough
  for (GLuint id = 1; id <= max_id; ++id) {
#define CHECK_GL_LEAK(type) \
    if (glIs ##type(id)) { \
      PROTO3D_TRACE("OpenGL: leaked " #type " handle %u\n", (unsigned int) id); \
    }
    CHECK_GL_LEAK(Texture);
    CHECK_GL_LEAK(Buffer);
    CHECK_GL_LEAK(Framebuffer);
    CHECK_GL_LEAK(Renderbuffer);
    CHECK_GL_LEAK(VertexArray);
    CHECK_GL_LEAK(Shader);
    CHECK_GL_LEAK(Program);
    CHECK_GL_LEAK(ProgramPipeline);
    CHECK_GL_LEAK(Query);
#undef CHECK_GL_LEAK
  }
  PROTO3D_TRACE("OpenGL: leak check done.\n");
  glGetError();  // Reset GL error flag
}

}  // namespace gl

}  // namespace proto3d

// }}} END of OpenGL debugging utilities implementation

namespace proto3d {

namespace gl {

namespace shader {

#ifdef PROTO3D_USE_EXCEPTIONS
// Shader Facade Implementation {{{

/// Compile shader source code and return a Shader.
///
/// POSSIBLE_CRASH: std::runtime_error in case of compilation error
///
/// @return the compiled Shader
proto3d::gl::Shader Compile(GLenum shader_type, const char *source) {
  proto3d::gl::Shader shader;
  shader.Create(shader_type);
  shader.SetSource(source);
  auto message = shader.Compile();
  if (message != nullptr) {
    throw std::runtime_error(std::string(message.get()));
  }
  return shader;
}

proto3d::gl::Shader Compile(GLenum shader_type, GLsizei count, const char **sources) {
  proto3d::gl::Shader shader;
  shader.Create(shader_type);
  shader.SetSources(count, sources);
  auto message = shader.Compile();
  if (message != nullptr) {
    throw std::runtime_error(std::string(message.get()));
  }
  return shader;
}

/// Link compiled Shaders and return a Program.
///
/// POSSIBLE_CRASH: std::runtime_error in case of link error.
template<class... Shaders>
proto3d::gl::Program Link(Shaders... shaders) {
  proto3d::gl::Program program;
  program.Create();
  auto message = program.template Link<Shaders...>(shaders...);
  if (message != nullptr) {
    throw std::runtime_error(std::string(message.get()));
  }
  return program;
}

/// Link compiled Shaders and return a Program.
///
/// POSSIBLE_CRASH: std::runtime_error in case of Link error.
///
/// @return The linked Program
proto3d::gl::Program Link(proto3d::gl::Shader *shaders, int size) {
  proto3d::gl::Program program;
  program.Create();
  auto message = program.template Link(shaders, size);
  if (message != nullptr) {
    throw std::runtime_error(std::string(message.get()));
  }
  return program;
}

/// Compile the shader source code and link it.
///
/// POSSIBLE_CRASH: std::runtime_error in case of compilation or link error.
///
/// @return The linked Program
proto3d::gl::Program CompileAndLink(GLenum shader_type, const char *source) {
  proto3d::gl::Program program;
  program.Create();
  auto shader = Compile(shader_type, source);
  auto message = program.Link(shader);
  shader.Delete();
  if (message != nullptr) {
    throw std::runtime_error(std::string(message.get()));
  }
  return program;
}
// }}} END of Shader Facade Implementation
#endif  // PROTO3D_USE_EXCEPTIONS

}  // namespace shader

}  // namespace gl

}  // namespace proto3d
#endif  // PROTO3D_IMPLEMENTATION

#endif  // PROTO3D_H_
