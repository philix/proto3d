#!/usr/bin/env python

# This file is part of proto3d, hosted at https://github.com/philix/proto3d and
# it's based on gl3w's gl3w_gen.py (hosted at https://github.com/skaslev/gl3w).

# Allow Python 2.6+ to use the print() function
from __future__ import print_function

import re
import os
import ssl
import sys

# Try to import Python 3 library urllib.request
# and if it fails, fall back to Python 2 urllib2
try:
    import urllib.request as urllib2
except ImportError:
    import urllib2

INCLUDE_HELP = br'''// This file was GENERATED with proto3d_glcorearb_gen.py,
// part of proto3d (hosted at https://github.com/philix/proto3d).
//
// vim: fdm=marker
//
// You MUST
//
//     #define PROTO3D_GLCOREARB_IMPLEMENTATION
//
// in EXACTLY _one_ C or C++ file that includes this header,
// BEFORE the include, like this:
//
//     #define PROTO3D_GLCOREARB_IMPLEMENTATION
//     #include "proto3d_glcorearb.h"
//
// All other files should just #include "proto3d_glcorearb.h"
// without the #define.
//
//
// proto3d_glcorearb.h is for use with OpenGL core profile
// implementations.
//
// It includes only APIs in the latest OpenGL core profile
// implementation together with APIs in newer ARB extensions
// which can be supported by the core profile. It does not, and
// never will include functionality removed from the core
// profile, such as fixed-function vertex and fragment
// processing.
//
// DO NOT #include both proto3d_glcorearb.h and either of
// <GL/gl.h> or <GL/glext.h> in the same source file.
'''

GLCOREARB_URL = 'https://www.opengl.org/registry/api/GL/glcorearb.h'
dest_path = 'proto3d_glcorearb.h'
if len(sys.argv) > 1:
    dest_path = sys.argv[1]

# Download glcorearb.h
print('Downloading ' + GLCOREARB_URL + '...')
context = ssl._create_unverified_context()
web = urllib2.urlopen(GLCOREARB_URL, context=context)

# Parse function names from glcorearb.h
print('Parsing glcorearb.h...')
glcorearb_header_lines = []
procs = []
line_pattern = re.compile(r'GLAPI (.*?)APIENTRY\s+(\w+)\s*\((.*)\)')
last_id_pattern = re.compile(r'.*?(\w+)(\[\d*\])?$')
for line in web:
    glcorearb_header_lines.append(line)
    m = line_pattern.match(line)
    if m:
        ret_type = m.group(1)
        name = m.group(2)
        params_str = m.group(3)
        params = map(lambda s: s.strip(), params_str.split(','))
        param_names = filter(
            lambda s: s != 'void',
            map(lambda s: last_id_pattern.match(s).group(1), params))
        procs.append({
          'name': m.group(2),
          'params_str': params_str,
          'param_names': param_names,
          'ret_type': ret_type,
          'type': 'PFN' + name.upper() + 'PROC'
        })

# Generate proto3d_glcorearb.h
print('Generating ' + dest_path + '...')
f = open(dest_path, 'wb')
f.write(INCLUDE_HELP)
f.write(br'''#ifndef PROTO3D_GLCOREARB_H_
#define PROTO3D_GLCOREARB_H_

// In Debug mode we need the prototypes since we generate a debug
// implementation of all gl* functions.
#ifndef NDEBUG
# define GL_GLEXT_PROTOTYPES
#else
// API aliases {{{
''')
for proc in procs:
    f.write('# define {0[name]} _{0[name]}\n'.format(proc).encode('utf-8'))
f.write(br'''
// }}} End of API aliases
#endif

// Begin glcorearb.h (https://www.opengl.org/registry/api/GL/glcorearb.h) {{{
''')
f.write(''.join(glcorearb_header_lines))
f.write('// }}} End glcorearb.h')
f.write(br'''

#ifndef __gl_h_
# define __gl_h_
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Proto3dGlProc)(void);

// Proto3D OpenGL Loader API
void Proto3dOpenLibGl(void);
void Proto3dCloseLibGl(void);
void Proto3dGlLoadAllCoreProfileProcs(void);
int Proto3dOpenLibGlAndLoadCoreProfile(void);
int Proto3dGlLoadedVersion(GLint *major, GLint *minor);
Proto3dGlProc Proto3dGlGetProcAddress(const char *proc);
const char *Proto3dGlLastErrorString();

// OpenGL function pointer declarations {{{
''')
for proc in procs:
    f.write('extern {0[type]: <52} _{0[name]};\n'.format(proc).encode('utf-8'))
f.write(b'// }}} End OpenGL function pointer declarations\n')

f.write(br'''
#ifdef __cplusplus
};
#endif
''')

print('Generating loader API implementation...')
f.write(br'''
#if defined(PROTO3D_GLCOREARB_IMPLEMENTATION) && !defined(PROTO3D_GLCOREARB_IMPLEMENTATION_DONE)
# define PROTO2D_GLCOREARB_IMPLEMENTATION_DONE

// Macro for logging that takes printf-style arguments.
// This version requires <cstdio>, but it can be defined differently before
// including proto3d headers.
#ifndef PROTO3D_TRACE
#include <cstdio>  // NOLINT
#define PROTO3D_TRACE(...)      \
  fprintf(stderr, __VA_ARGS__); \
  fflush(stderr)
#endif

#ifndef PROTO3D_CHECK_GL_ERROR
# ifdef NDEBUG
#  define PROTO3D_CHECK_GL_ERROR(gl_proc_name_str)
# else
#  define PROTO3D_CHECK_GL_ERROR(gl_proc_name_str)                \
  {                                                               \
    const char *_last_error_str = Proto3dGlLastErrorString();     \
    if (_last_error_str != nullptr) {                             \
      PROTO3D_TRACE("OpenGL error:%s:%d: %s is set after %s.\n",  \
                    __FILE__,                                     \
                    __LINE__,                                     \
                    _last_error_str,                              \
                    (gl_proc_name_str));                          \
    }                                                             \
  }
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// OpenGL function pointers {{{
''')
for proc in procs:
    f.write('{0[type]: <52} _{0[name]};\n'.format(proc).encode('utf-8'))
f.write(b'// }}} End OpenGL function pointers\n\n')

f.write('#ifndef NDEBUG\n')
f.write('// Debug GL API implementations {{{\n')
for proc in procs:
    f.write('{0[ret_type]}{0[name]}({0[params_str]}) {{\n'.format(proc).encode('utf-8'))
    arguments = ', '.join(proc['param_names'])
    if proc['ret_type'] == 'void ':
        f.write('  _{0[name]}({1});\n'.format(proc, arguments).encode('utf-8'))
        if proc['name'] != 'glGetError':
            f.write('  PROTO3D_CHECK_GL_ERROR("{0[name]}");\n'.format(proc).encode('utf-8'))
    else:
        f.write('  {0[ret_type]}ret = _{0[name]}({1});\n'.format(proc, arguments).encode('utf-8'))
        if proc['name'] != 'glGetError':
            f.write('  PROTO3D_CHECK_GL_ERROR("{0[name]}");\n'.format(proc).encode('utf-8'))
        f.write('  return ret;\n')
    f.write('}\n')
f.write('// }}} End of Debug GL API implementations\n')
f.write('#endif  // !NDEBUG\n')

f.write(br'''
void Proto3dGlLoadAllCoreProfileProcs(void) {
// {{{
''')
for proc in procs:
    f.write('  _{0[name]} = ({0[type]})Proto3dGlGetProcAddress("{0[name]}");\n'.format(proc).encode('utf-8'))
f.write('// }}}\n')
f.write(br'''}

#ifdef __cplusplus
};
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>

static HMODULE proto3d_libgl;

#ifdef __cplusplus
extern "C" {
#endif

void Proto3dOpenLibGl(void) {
  proto3d_libgl = LoadLibraryA("opengl32.dll");
}

void Proto3dCloseLibGl(void) {
  FreeLibrary(proto3d_libgl);
}

Proto3dGlProc Proto3dGlGetProcAddress(const char *proc) {
  Proto3dGlProc res;

  res = (Proto3dGlProc)wglGetProcAddress(proc);
  if (!res) {
    res = (Proto3dGlProc)GetProcAddress(proto3d_libgl, proc);
  }
  return res;
}

#ifdef __cplusplus
};
#endif
#elif defined(__APPLE__) || defined(__APPLE_CC__)
# include <Carbon/Carbon.h>

static CFBundleRef bundle;
static CFURLRef bundle_url;

#ifdef __cplusplus
extern "C" {
#endif

void Proto3dOpenLibGl(void) {
  bundle_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
    CFSTR("/System/Library/Frameworks/OpenGL.framework"),
    kCFURLPOSIXPathStyle, true);

  bundle = CFBundleCreate(kCFAllocatorDefault, bundle_url);
  assert(bundle != NULL && "CFBundleCreate failed");
}

void Proto3dCloseLibGl(void) {
  CFRelease(bundle);
  CFRelease(bundle_url);
}

Proto3dGlProc Proto3dGlGetProcAddress(const char *proc) {
  Proto3dGlProc res;

  CFStringRef procname = CFStringCreateWithCString(kCFAllocatorDefault, proc,
    kCFStringEncodingASCII);
  *(void **)(&res) = CFBundleGetFunctionPointerForName(bundle, procname);
  CFRelease(procname);
  return res;
}

#ifdef __cplusplus
};
#endif

#else  // Linux
# include <dlfcn.h>
# include <GL/glx.h>

static void *proto3d_libgl;
static PFNGLXGETPROCADDRESSPROC glx_get_proc_address;

#ifdef __cplusplus
extern "C" {
#endif

void Proto3dOpenLibGl(void) {
  proto3d_libgl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
  *(void **)(&glx_get_proc_address) = dlsym(proto3d_libgl, "glXGetProcAddressARB");
}

void Proto3dCloseLibGl(void) {
  dlclose(proto3d_libgl);
}

Proto3dGlProc Proto3dGlGetProcAddress(const char *proc) {
  Proto3dGlProc res;

  res = glx_get_proc_address((const GLubyte *)proc);
  if (!res) {
    *(void **)(&res) = dlsym(proto3d_libgl, proc);
  }
  return res;
}

#ifdef __cplusplus
};
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

int Proto3dGlLoadedVersion(GLint *major, GLint *minor) {
  if (!glGetIntegerv) {
    *major = 0;
    *minor = 0;
    return -1;
  }
  glGetIntegerv(GL_MAJOR_VERSION, major);
  glGetIntegerv(GL_MINOR_VERSION, minor);
  return 0;
}

int Proto3dOpenLibGlAndLoadCoreProfile(void)
{
  Proto3dOpenLibGl();
  Proto3dGlLoadAllCoreProfileProcs();
  Proto3dCloseLibGl();

  GLint major = 0;
  GLint minor = 0;
  if (Proto3dGlLoadedVersion(&major, &minor) < 0) {
    return -1;
  }
  return major < 3 ? -1 : 0;
}

const char *Proto3dGlLastErrorString() {
  GLenum error = glGetError();
  switch (error) {
    case GL_NO_ERROR:
      return nullptr;
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
    default:
      return "unknown error type";
  }
}


#ifdef __cplusplus
};
#endif
''')

f.write(br'''
#endif  // PROTO3D_GLCOREARB_IMPLEMENTATION
#endif  // PROTO3D_GLCOREARB_H_
''')
