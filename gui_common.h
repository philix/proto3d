#ifndef GUI_COMMON_H
#define GUI_COMMON_H
/// @file This header is included by all platform specific GUI headers.
/// And contains the headers and implementation of common gui functions that
/// work in any platform. COMMON_GUI_IMPLEMENTATION should be defined before
/// including to enable the implementation code.
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// Platform-specific headers {{{
#ifdef __APPLE__
# if defined(__OBJC__)
#  import <Carbon/Carbon.h>
#  import <Cocoa/Cocoa.h>
# else
#  include <Carbon/Carbon.h>
#  include <ApplicationServices/ApplicationServices.h>
typedef void *id;
# endif
#endif
// }}}

// Cursor enums {{{
typedef enum {
  kArrowCursor,      // Regular arrow cursor
  kIBeamCursor,      // Text input I-beam cursor shape
  kCrosshairCursor,  // Crosshair shape
  kHandCursor,       // Hand shape
  kHResizeCursor,    // Horizontal resize arrow shape
  kVResizeCursor     // Vertical resize arrow shape
} GCursorShape;

typedef enum {
  kCursorMode,
  kStickyKeysMode,
  kStickyMouseButtonsMode,
} GWindowInputMode;

typedef enum {
  kCursorNormal,
  kCursorHidden,
  kCursorDisabled
} GCursorMode;
// }}}

// Keyboard enums {{{

// Keyboard keys
//
// These key codes are inspired by the _USB HID Usage Tables v1.12_ (p. 53-60),
// but re-arranged to map to 7-bit ASCII for printable keys (function keys are
// put in the 256+ range).
//
// The naming of the key codes follow these rules:
//  - The US keyboard layout is used
//  - Names of printable alpha-numeric characters are used (e.g. "A", "R",
//    "3", etc.)
//  - For non-alphanumeric characters, Unicode:ish names are used (e.g.
//    "COMMA", "LEFT_SQUARE_BRACKET", etc.). Note that some names do not
//    correspond to the Unicode standard (usually for brevity)
//  - Keys that lack a clear US mapping are named "WORLD_x"
//  - For non-printable keys, custom names are used (e.g. "F4",
//    "BACKSPACE", etc.)

typedef enum {
  GUI_KEY_STICK              = -2,
  // The unknown key
  GUI_KEY_UNKNOWN            = -1,

  // Printable keys
  GUI_KEY_SPACE              = 32,
  GUI_KEY_APOSTROPHE         = 39,   // '
  GUI_KEY_COMMA              = 44,   // ,
  GUI_KEY_MINUS              = 45,   // -
  GUI_KEY_PERIOD             = 46,   // .
  GUI_KEY_SLASH              = 47,   // /
  GUI_KEY_0                  = 48,
  GUI_KEY_1                  = 49,
  GUI_KEY_2                  = 50,
  GUI_KEY_3                  = 51,
  GUI_KEY_4                  = 52,
  GUI_KEY_5                  = 53,
  GUI_KEY_6                  = 54,
  GUI_KEY_7                  = 55,
  GUI_KEY_8                  = 56,
  GUI_KEY_9                  = 57,
  GUI_KEY_SEMICOLON          = 59,   // ;
  GUI_KEY_EQUAL              = 61,   // =
  GUI_KEY_A                  = 65,
  GUI_KEY_B                  = 66,
  GUI_KEY_C                  = 67,
  GUI_KEY_D                  = 68,
  GUI_KEY_E                  = 69,
  GUI_KEY_F                  = 70,
  GUI_KEY_G                  = 71,
  GUI_KEY_H                  = 72,
  GUI_KEY_I                  = 73,
  GUI_KEY_J                  = 74,
  GUI_KEY_K                  = 75,
  GUI_KEY_L                  = 76,
  GUI_KEY_M                  = 77,
  GUI_KEY_N                  = 78,
  GUI_KEY_O                  = 79,
  GUI_KEY_P                  = 80,
  GUI_KEY_Q                  = 81,
  GUI_KEY_R                  = 82,
  GUI_KEY_S                  = 83,
  GUI_KEY_T                  = 84,
  GUI_KEY_U                  = 85,
  GUI_KEY_V                  = 86,
  GUI_KEY_W                  = 87,
  GUI_KEY_X                  = 88,
  GUI_KEY_Y                  = 89,
  GUI_KEY_Z                  = 90,
  GUI_KEY_LEFT_BRACKET       = 91,   // [
  GUI_KEY_BACKSLASH          = 92,   // \ //
  GUI_KEY_RIGHT_BRACKET      = 93,   // ]
  GUI_KEY_GRAVE_ACCENT       = 96,   // `
  GUI_KEY_WORLD_1            = 161,  // non-US #1
  GUI_KEY_WORLD_2            = 162,  // non-US #2

  // Function keys
  GUI_KEY_ESCAPE             = 256,
  GUI_KEY_ENTER              = 257,
  GUI_KEY_TAB                = 258,
  GUI_KEY_BACKSPACE          = 259,
  GUI_KEY_INSERT             = 260,
  GUI_KEY_DELETE             = 261,
  GUI_KEY_RIGHT              = 262,
  GUI_KEY_LEFT               = 263,
  GUI_KEY_DOWN               = 264,
  GUI_KEY_UP                 = 265,
  GUI_KEY_PAGE_UP            = 266,
  GUI_KEY_PAGE_DOWN          = 267,
  GUI_KEY_HOME               = 268,
  GUI_KEY_END                = 269,
  GUI_KEY_CAPS_LOCK          = 280,
  GUI_KEY_SCROLL_LOCK        = 281,
  GUI_KEY_NUM_LOCK           = 282,
  GUI_KEY_PRINT_SCREEN       = 283,
  GUI_KEY_PAUSE              = 284,
  GUI_KEY_F1                 = 290,
  GUI_KEY_F2                 = 291,
  GUI_KEY_F3                 = 292,
  GUI_KEY_F4                 = 293,
  GUI_KEY_F5                 = 294,
  GUI_KEY_F6                 = 295,
  GUI_KEY_F7                 = 296,
  GUI_KEY_F8                 = 297,
  GUI_KEY_F9                 = 298,
  GUI_KEY_F10                = 299,
  GUI_KEY_F11                = 300,
  GUI_KEY_F12                = 301,
  GUI_KEY_F13                = 302,
  GUI_KEY_F14                = 303,
  GUI_KEY_F15                = 304,
  GUI_KEY_F16                = 305,
  GUI_KEY_F17                = 306,
  GUI_KEY_F18                = 307,
  GUI_KEY_F19                = 308,
  GUI_KEY_F20                = 309,
  GUI_KEY_F21                = 310,
  GUI_KEY_F22                = 311,
  GUI_KEY_F23                = 312,
  GUI_KEY_F24                = 313,
  GUI_KEY_F25                = 314,
  GUI_KEY_KP_0               = 320,
  GUI_KEY_KP_1               = 321,
  GUI_KEY_KP_2               = 322,
  GUI_KEY_KP_3               = 323,
  GUI_KEY_KP_4               = 324,
  GUI_KEY_KP_5               = 325,
  GUI_KEY_KP_6               = 326,
  GUI_KEY_KP_7               = 327,
  GUI_KEY_KP_8               = 328,
  GUI_KEY_KP_9               = 329,
  GUI_KEY_KP_DECIMAL         = 330,
  GUI_KEY_KP_DIVIDE          = 331,
  GUI_KEY_KP_MULTIPLY        = 332,
  GUI_KEY_KP_SUBTRACT        = 333,
  GUI_KEY_KP_ADD             = 334,
  GUI_KEY_KP_ENTER           = 335,
  GUI_KEY_KP_EQUAL           = 336,
  GUI_KEY_LEFT_SHIFT         = 340,
  GUI_KEY_LEFT_CONTROL       = 341,
  GUI_KEY_LEFT_ALT           = 342,
  GUI_KEY_LEFT_SUPER         = 343,
  GUI_KEY_RIGHT_SHIFT        = 344,
  GUI_KEY_RIGHT_CONTROL      = 345,
  GUI_KEY_RIGHT_ALT          = 346,
  GUI_KEY_RIGHT_SUPER        = 347,
  GUI_KEY_MENU               = 348,
  GUI_KEY_LAST               = GUI_KEY_MENU,
} GKey;

typedef enum {
  kModShift =   0x1,
  kModControl = 0x2,
  kModAlt =     0x4,
  kModSuper =   0x8
} GModifierKey;
// }}}

#ifdef __cplusplus
extern "C" {
#endif

// Platform-specific structs {{{
#ifdef __APPLE__
// NSGL-specific per-context data
struct context_nsgl_s {
  id pixel_format;
  id object;
};

// NSGL-specific global data
struct global_nsgl_s {
  // dlopen (gl_get_proc_address()) handle for OpenGL.framework
  CFBundleRef framework;
};

// Cocoa-specific per-window data
struct window_ns_s {
  id object;
  id delegate;
  id view;

  // The total sum of the distances the cursor has been warped since the last
  // cursor motion event was processed.
  // This is kept to counteract Cocoa doing the same internally.
  double warp_delta_x;
  double warp_delta_y;
};

// Cocoa-specific global data
struct global_ns_s {
    CGEventSourceRef    event_source;
    id                  delegate;
    id                  auto_release_pool;
    id                  cursor;
    TISInputSourceRef   input_source;
    id                  unicode_data;

    char                key_name[64];
    short int           public_keys[256];
    short int           native_keys[GUI_KEY_LAST + 1];

    char               *clipboard_string;
};

// Cocoa-specific per-monitor data
struct monitor_ns_s {
  CGDirectDisplayID display_id;
  CGDisplayModeRef  previous_mode;
  uint32_t          unit_number;
};

// Cocoa-specific per-cursor data
struct cursor_ns_s {
  id object;
};

// Cocoa-specific global timer data
struct global_ns_time_s {
  uint64_t frequency;
};
#endif  // __APPLE__
// }}}

// Configuration structs {{{
// OpenGL context configuration.
//
// Parameters relating to the creation of the context but not directly
// related to the framebuffer.  This is used to pass context creation
// parameters from shared code to the platform API.
//
// Mac OS X(NSGL):
//
//  Forward-compatible Core Profiles for OpenGL 3.2 and 4.1
typedef struct GContextConfig {
  //               setting        support
  // int           api;        // !nsgl
  int              major;      // nsgl
  // int           minor;      // !nsgl
  bool             forward;    // !nsgl
  // bool          debug;
  // bool          noerror;
  // int           profile;
  // GL_KHR_robustness extensions
  // int           robustness; // !nsgl
  // GL_KHR_context_flush_control
  // int           release;    // !nsgl

  // struct GWindow*  share;
} GContextConfig;

// Framebuffer configuration.
//
// This describes buffers and their sizes.  It also contains
// a platform-specific ID used to map back to the backend API object.
//
// It is used to pass framebuffer parameters from shared code to the platform
// API and also to enumerate and select available framebuffer configs.
typedef struct GFramebufferConfig {
  // These are used as hints from GlobalGui::hints.framebuffer
  int         red_bits;
  int         green_bits;
  int         blue_bits;
  int         alpha_bits;
  int         depth_bits;
  int         stencil_bits;
  bool        double_buffer;
  
  // For OpenGL 1.x and 2.x
  //         accum_red_bits;
  //         accum_green_bits;
  //         accum_blue_bits;
  //         accum_alpha_bits;
  //         aux_buffers;

  bool        stereo;
  int         samples; // TODO: check this
  // GLFWbool    s_rgb; // default on Mac OS X
 
  // Platform-specific config
  // ...
} GFramebufferConfig;
// }}}

// Miscellaneous structs (GL context, GVideoMode...) {{{
// GUI OpenGL context
typedef struct {
#ifdef __APPLE__
  struct context_nsgl_s nsgl;  // NSGL
#endif
} GContext;

typedef struct {
    int width;         // The width, in screen coordinates
    int height;        // The height, in screen coordinates
    int red_bits;      // The bit depth of the red channel
    int green_bits;    // The bit depth of the green channel
    int blue_bits;     // The bit depth of the blue channel
    int refresh_rate;  // The refresh rate, in Hz
} GVideoMode;

// Gamma ramp.
typedef struct GGammaRamp {
  // An array of value describing the response of the red channel.
  uint16_t *red;
  // An array of value describing the response of the green channel.
  uint16_t *green;
  // An array of value describing the response of the blue channel.
  uint16_t *blue;
  // The number of elements in each array.
  uint32_t size;
} GGammaRamp;

// Image data
typedef struct GImage {
  // Size in pixels
  int width;
  int height;
  // Image raw data
  uint8_t *pixels;
} GImage;
// }}}

// Mouse struct and enums {{{
struct GCursor;
typedef struct GCursor {
  struct GCursor     *next;
#ifdef __APPLE__
  struct cursor_ns_s ns;
#endif
} GCursor;

// Key and button actions
typedef enum {
  kRelease,  // The key or mouse button was released
  kPress,    // The key or mouse button was pressed
  kRepeat    // The key was held down until it repeated
} GInputAction;

// Mouse buttons
typedef enum {
  kMouseButton1      = 0,
  kMouseButton2      = 1,
  kMouseButton3      = 2,
  kMouseButton4      = 3,
  kMouseButton5      = 4,
  kMouseButton6      = 5,
  kMouseButton7      = 6,
  kMouseButton8      = 7,
  kMouseButtonLast   = kMouseButton8,
  kMouseButtonLeft   = kMouseButton1,
  kMouseButtonRight  = kMouseButton2,
  kMouseButtonMiddle = kMouseButton3,

  kMouseButtonStick = 9,
} GMouseButton;

// Mouse actions
typedef enum {
  kMouseActionClick,
  // TODO: more mouse actions
} GMouseAction;
// }}} 

struct GlobalGui;

// Window and Monitor structs {{{
typedef enum {
  kWindowNullEvent = 0,
  kWindowKey,
  kWindowChar,
  kWindowScroll,
  kWindowMouse,
  kWindowCursorMotion,
  kWindowCursorEnterChange,
  kWindowDrop,
  kWindowFocusChange,
  kWindowMove,
  kWindowResize,
  kWindowFramebufferResize,
  kWindowIconifyChange,
  kWindowDamage,
  kWindowClose,
} GWindowEventType;

struct GWindow;

// GWindowEventData
typedef union {
  // kWindowKey
  struct {
    GKey key;
    int scancode;
    int action;
    int mods;
  } key;
  // kWindowChar
  struct {
    unsigned int codepoint;
    int mods;
    bool plain;
  } char_;
  // kWindowScroll
  struct {
    double xoffset;
    double yoffset;
  } scroll;
  // kWindowMouse
  struct {
    GMouseButton button;
    GMouseAction action;
    int            mods;
  } mouse;
  struct {
    // kWindowCursorMotion
    double x;
    double y;

    // kWindowCursorEnterChange
    bool entered;
  } cursor;
  // kWindowDrop
  struct {
    int count;
    const char **paths;
  } drop;
  // kWindowFocusChange
  bool focused;
  // kWindowMove
  // Position of the upper-left corner of the client area of the window.
  struct {
    int x;
    int y;
  } pos;
  // kWindowResize
  // kWindowFramebufferResize
  // New size, in screen coordinates
  struct {
    int width;
    int height;
  } size;
  // kWindowIconifyChange
  bool iconified;
} GWindowEventData;

typedef struct {
  GWindowEventType  type;
  struct GWindow   *window;  // The window that was moved, resized...
  GWindowEventData  e;

  /*
    GLFWwindowrefreshfun    refresh;
    GLFWwindowiconifyfun    iconify;
    GLFWframebuffersizefun  fbsize;
    GLFWmousebuttonfun      mouseButton;
    GLFWcursorposfun        cursorPos;
    GLFWcursorenterfun      cursorEnter;
    GLFWscrollfun           scroll;
    GLFWkeyfun              key;
    GLFWcharfun             character;
    GLFWcharmodsfun         charmods;
    GLFWdropfun             drop;
    */
} GWindowEvent;

typedef enum {
  // TODO: monitor events
  k_a_monitor_event,
} GMonitorEventType;

typedef struct GMonitorEvent {
  GMonitorEventType type;
} GMonitorEvent;

// Parameters relating to the creation of the window but not directly related
// to the framebuffer. This is used to pass window creation parameters from
// shared code to the platform API.
typedef struct GWindowConfig {
  int           width;
  int           height;
  bool          floating;
  bool          maximized;
  const char*   title;

  // These are used as hints from GlobalGui::hints.window
  bool          resizable;
  bool          visible;
  bool          decorated;
  bool          focused;
  bool          auto_iconify;
} GWindowConfig;

struct GMonitor;

// GUI Window
struct GWindow {
  struct GlobalGui *gui;  // Pointer to the parent GlobalGui

  struct GWindow *next;

  // Window settings and state
  bool                resizable;
  bool                decorated;
  bool                auto_iconify;
  bool                floating;
  bool                closed;
  //void*               userPointer;
  GVideoMode          video_mode;
  struct GMonitor    *monitor;
  GCursor            *cursor;

  int                 minwidth, minheight;
  int                 maxwidth, maxheight;
  int                 numer, denom;

  // Window input state
  bool                sticky_keys;
  bool                sticky_mouse_buttons;
  double              cursor_pos_x;
  double              cursor_pos_y;
  int                 cursor_mode;

  char                mouse_buttons[kMouseButtonLast + 1];
  char                keys[GUI_KEY_LAST + 1];

  GContext context;         // OpenGL context

  /*
  struct {
    windowposfun        pos;
    windowsizefun       size;
    windowclosefun      close;
    windowrefreshfun    refresh;
    windowfocusfun      focus;
    windowiconifyfun    iconify;
    framebuffersizefun  fbsize;
    mousebuttonfun      mouseButton;
    cursorposfun        cursorPos;
    cursorenterfun      cursorEnter;
    scrollfun           scroll;
    keyfun              key;
    charfun             character;
    charmodsfun         charmods;
    dropfun             drop;
  } callbacks;
  */

#ifdef __APPLE__
  struct window_ns_s ns;      // Cocoa
#endif
};
typedef struct GWindow GWindow;

typedef struct GMonitor {
    char                *name;

    // Physical dimensions in millimeters
    int                  width_mm;
    int                  height_mm;

    // The window whose video mode is current on this monitor
    struct GWindow *window;

    // Video modes and current mode
    GVideoMode        *modes;
    int                  mode_count;
    GVideoMode         current_mode;

    GGammaRamp         original_ramp;
    GGammaRamp         current_ramp;

#ifdef __APPLE__
    struct monitor_ns_s ns;
#endif
} GMonitor;
// }}} 

// Global GUI state {{{
struct GlobalGui {
  double         cursor_pos_x;
  double         cursor_pos_y;

  GCursor       *cursor_list_head;

  GWindow       *window_list_head;
  GWindow       *cursor_window;

  GMonitor     **monitors;
  size_t         monitor_count;

  uint64_t       timer_offset;

  void (*handle_event)(GWindowEvent event);
  // TODO: unify GMonitorEvent and GWindowEvent
  void (*handle_monitor_event)(GMonitorEvent);

  struct {
    GFramebufferConfig framebuffer;
    GWindowConfig window;
    GContextConfig context;
    // int refresh_rate;
  } hints;

#ifdef __APPLE__
  struct global_ns_time_s ns_time;
  struct global_ns_s      ns;      // Cocoa
  struct global_nsgl_s    nsgl;    // NSGL
#endif
};
typedef struct GlobalGui GlobalGui;
// }}} 

typedef void (*GL_Proc)(void);

// Platform API {{{
// OpenGL context public platform API {{{
void gui_gl_make_context_current(GWindow *window);
void gl_swap_buffers(GWindow *window);
void gl_swap_interval(GWindow *window, int interval);
GL_Proc gl_get_proc_address(GlobalGui *gui, const char *procname);
// }}}

// Window engine platform API {{{
// -> monitor
GMonitor **gui_platform_get_monitors(size_t *count, char **error);
bool gui_is_same_monitor(GMonitor *first, GMonitor *second);
void gui_platform_get_monitor_pos(GMonitor *monitor, int *x, int *y);
GVideoMode *gui_platform_get_video_modes(GMonitor *monitor, int *count);
void gui_platform_get_video_mode(GMonitor *monitor, GVideoMode *mode);
void gui_platform_get_gamma_ramp(GMonitor *monitor, GGammaRamp *ramp);
void gui_platform_set_gamma_ramp(GMonitor *monitor, const GGammaRamp *ramp);

// -> window
bool gui_platform_init(GlobalGui *gui, char **error);
bool gui_platform_cleanup(GlobalGui *gui, char **error);
int gui_platform_create_window(GWindow *window,
                               const GWindowConfig *win_config,
                               const GContextConfig *ctx_config,
                               const GFramebufferConfig *fb_config,
                               char **error);
void gui_platform_destroy_window(GWindow *window);
void gui_set_window_title(GWindow *window, const char *title);
void gui_set_window_icon(GWindow *window,
                         int count,
                         const GImage *images);
void gui_get_window_pos(GWindow *window, int *x, int *y);
void gui_set_window_pos(GWindow *window, int x, int y);
void gui_platform_get_window_size(GWindow *window, int *width, int *height);
void gui_set_window_size(GWindow *window, int width, int height);
#if 0
void gui_set_window_size_limits(GWindow *window,
                                int minwidth,
                                int minheight,
                                int maxwidth,
                                int maxheight);
void gui_set_window_aspect_ratio(GWindow *window, int numer, int denom);
#endif
void gui_get_framebuffer_size(GWindow *window, int *width, int *height);
void gui_get_window_framesize(GWindow *window,
                              int *left,
                              int *top,
                              int *right,
                              int *bottom);
void gui_iconify_window(GWindow *window);
void gui_restore_window(GWindow *window);
void gui_maximize_window(GWindow *window);
void gui_platform_show_window(GWindow *window);
void gui_hide_window(GWindow *window);
void gui_platform_focus_window(GWindow *window);
void gui_platform_set_window_monitor(GWindow *window,
                                     GMonitor *monitor,
                                     int xpos,
                                     int ypos,
                                     int width,
                                     int height,
                                     int refresh_rate);
int gui_window_focused(GWindow *window);
int gui_window_inconified(GWindow *window);
int gui_window_visible(GWindow *window);
int gui_window_maximized(GWindow *window);
void gui_poll_events(GlobalGui *gui);
void gui_wait_events(GlobalGui *gui);
void gui_wait_events_timeout(GlobalGui *gui, double timeout);
void gui_post_empty_event();
const char *gui_get_key_name(GlobalGui *gui, int key, int scancode);

// -> cursor
void gui_platform_get_cursor_pos(GWindow *window, double *xpos, double *ypos);
void gui_platform_set_cursor_pos(GWindow *window, double x, double y);
void gui_platform_set_cursor_mode(GWindow *window, GCursorMode mode);
bool gui_platform_create_cursor(GCursor *cursor,
                                const GImage *image,
                                int xhot,
                                int yhot);
int gui_platform_create_standard_cursor(GCursor *cursor,
                                        GCursorShape shape,
                                        char **error);
void gui_platform_destroy_cursor(GCursor *cursor);
void gui_platform_set_cursor(GWindow *window, GCursor *cursor);
// -> clipboard
void gui_set_clipboard_string(GWindow *window, const char *string);
const char *gui_get_clipboard_string(GWindow *window, char **error);
// }}}

// Timer public platform API {{{
void gui_init_timer_ns(GlobalGui *gui);
uint64_t gui_get_timer_value(GlobalGui *gui);
uint64_t gui_get_timer_frequency(GlobalGui *gui);
// }}} 
// }}} 

// Gui abstract API {{{
bool gui_init(GlobalGui *gui, char **error);
void gui_terminate(GlobalGui *gui);
// Internal monitor abstract API {{{
void gui_input_monitor_change(GlobalGui *gui, char **error);

GMonitor *alloc_monitor(const char *name, int width_mm, int height_mm);
void free_monitor(GMonitor *monitor);
void alloc_gamma_arrays(GGammaRamp *ramp, uint32_t size);
void free_gamma_arrays(GGammaRamp *ramp);
void free_monitors(GMonitor **monitors, int count);
const GVideoMode *gui_choose_video_mode(GMonitor *monitor,
                                        const GVideoMode *desired);
int gui_compare_video_modes(const GVideoMode *fm, const GVideoMode *sm);
void gui_split_bpp(int bpp, int *red, int *green, int *blue);
// }}}

// Public monitor abstract API {{{
GMonitor **gui_get_monitors(GlobalGui *gui, size_t *count);
GMonitor *gui_get_primary_monitor(GlobalGui *gui);
void gui_get_monitor_pos(GMonitor *monitor, int *xpos, int *ypos);
void gui_get_monitor_physical_size(GMonitor *monitor, int *width_mm, int *height_mm);
// gui_get_monitor_name(monitor) -> monitor->name
// gui_set_monitor_callback(cb) -> gui->handle_monitor_event = cb;
const GVideoMode *gui_get_video_modes(GMonitor *monitor, int *count);
const GVideoMode *gui_get_video_mode(GMonitor *monitor);
void gui_set_gamma(GMonitor *monitor, float gamma, char **error);
const GGammaRamp *gui_get_gamma_ramp(GMonitor *monitor);
void gui_set_gamma_ramp(GMonitor *monitor, const GGammaRamp *ramp);
// }}}

// Internal input abstract API {{{
bool gui_is_printable(GKey key);
// }}} 

// Public event abstract API {{{
void gui_input_key(GWindow *window, GKey key, int scancode, int action, int mods);
void gui_input_char(GWindow *window, unsigned int codepoint, int mods, bool plain);
void gui_input_scroll(GWindow *window, double xoffset, double yoffset);
void gui_input_mouse_click(GWindow *window, GMouseButton button, int action, int mods);
void gui_input_cursor_motion(GWindow *window, double x, double y);
void gui_input_cursor_enter(GWindow *window, bool entered);
void gui_input_drop(GWindow *window, int count, const char **paths);

void gui_input_window_focus(GWindow *window, bool focused);
void gui_input_window_pos(GWindow *window, int x, int y);
void gui_input_window_size(GWindow *window, int width, int height);
void gui_input_window_iconify(GWindow *window, bool iconified);
void gui_input_framebuffer_size(GWindow *window, int width, int height);
void gui_input_window_damage(GWindow *window);
void gui_input_window_close_request(GWindow *window);
void gui_input_window_monitor_change(GWindow *window, GMonitor *monitor);
// }}} 

// Public window manipulation abstract API {{{
GWindow *gui_create_window(GlobalGui *gui,
                           int width,
                           int height,
                           const char *title,
                           GMonitor *monitor,
                           char **error);
void gui_destroy_window(GWindow *window);
/*
void gui_set_window_size_limits(GWindow *window,
                                int minwidth, int minheight,
                                int maxwidth, int maxheight);
void gui_set_window_aspect_ratio(GWindow *window, int numer, int denom);
*/
void gui_show_window(GWindow *window);
void gui_set_window_monitor(GWindow *window,
                            GMonitor *monitor,
                            int xpos, int ypos,
                            int width, int height,
                            int refresh_rate);
// }}} 

// Public input abstract API {{{
#if 0
int gui_get_input_mode(GWindow *window, GWindowInputMode mode);
void gui_set_input_mode(GWindow *window, GWindowInputMode mode, int value);
#endif
// gui_get_key_name(int key, int scancode) defined in platform code
int gui_get_key(GWindow *window, GKey key, char **error);
int gui_get_mouse_button(GWindow *window, GMouseButton button);
void gui_get_cursor_pos(GWindow *window, double *xpos, double *ypos);
void gui_set_cursor_pos(GWindow *window, double xpos, double ypos);
GCursor *gui_create_cursor(GlobalGui *gui, const GImage *image, int xhot, int yhot);
GCursor *gui_create_standard_cursor(GlobalGui *gui, GCursorShape shape, char **error);
void gui_destroy_cursor(GlobalGui *gui, GCursor *cursor);
void gui_set_cursor(GWindow *window, GCursor *cursor);
// gui_set_clipboard_string(window, string) is implemented in platform code
// gui_get_clipboard_string(window) is implemented in platform code
// }}} 

// Abtract Time API {{{
double gui_get_time(GlobalGui *gui);
void gui_set_time(GlobalGui *gui, double time);
// gui_get_timer_value() is implemented in platform code
// gui_get_timer_frequency() is implemented in platform code
// }}} 
// }}} 

#ifdef __cplusplus
};
#endif

#ifdef GUI_COMMON_IMPLEMENTATION
// GUI Common Implementation {{{
#ifdef __cplusplus
extern "C" {
#endif
#include <math.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <assert.h>

// Initialization API {{{
static void fail_event_handler(GWindowEvent event) {
  assert(false &&
      "Events should not be triggered before fui GlobalGui initialization.");
}

static void null_event_handler(GWindowEvent event) {
  // do nothing
}

static void init_hints(GlobalGui *gui)
{
    memset(&gui->hints, 0, sizeof(gui->hints));

    // The default is OpenGL 3.x
    // gui->hints.context.api = (OpenGL | OpenES);
    gui->hints.context.major = 3;
    // gui->hints.context.minor = 0;

    // The default is a focused, visible, resizable window with decorations
    gui->hints.window.resizable    = true;
    gui->hints.window.visible      = true;
    gui->hints.window.decorated    = true;
    gui->hints.window.focused      = true;
    gui->hints.window.auto_iconify = true;

    // The default is
    // - 32 bits of color
    // - 24 bits of depth
    // - 8 bits of stencil
    // - double buffered
    gui->hints.framebuffer.red_bits      = 8;
    gui->hints.framebuffer.green_bits    = 8;
    gui->hints.framebuffer.blue_bits     = 8;
    gui->hints.framebuffer.alpha_bits    = 8;
    gui->hints.framebuffer.depth_bits    = 24;
    gui->hints.framebuffer.stencil_bits  = 8;
    gui->hints.framebuffer.double_buffer = true;

    gui->hints.framebuffer.stereo        = false;
    gui->hints.framebuffer.samples       = -1;

    // The default is to select the highest available refresh rate
    // gui->hints.refresh_rate = -1;
}

bool gui_init(GlobalGui *gui, char **error)
{
  memset(gui, 0, sizeof(GlobalGui));
  gui->handle_event = fail_event_handler;
  init_hints(gui);
  bool status = gui_platform_init(gui, error);
  gui->handle_event = null_event_handler;
  return status;
}

void gui_terminate(GlobalGui *gui)
{
    while (gui->window_list_head) {
      gui_destroy_window(gui->window_list_head);
    }

    while (gui->cursor_list_head) {
        gui_destroy_cursor(gui, gui->cursor_list_head);
    }

    for (size_t i = 0;  i < gui->monitor_count; i++) {
        GMonitor* monitor = gui->monitors[i];
        if (monitor->original_ramp.size) {
            gui_platform_set_gamma_ramp(monitor, &monitor->original_ramp);
        }
    }

    free_monitors(gui->monitors, gui->monitor_count);
    gui->monitors = NULL;
    gui->monitor_count = 0;

    // TODO
    /* gui_platform_terminate(); */

    memset(gui, 0, sizeof(*gui));
}
// }}} 

// Internal monitor abstract API {{{

// Lexically compare video modes (used by qsort)
static int compare_video_modes(const void *fp, const void *sp)
{
    const GVideoMode *fm = (GVideoMode *)fp;
    const GVideoMode *sm = (GVideoMode *)sp;
    const int fbpp = fm->red_bits + fm->green_bits + fm->blue_bits;
    const int sbpp = sm->red_bits + sm->green_bits + sm->blue_bits;
    const int farea = fm->width * fm->height;
    const int sarea = sm->width * sm->height;

    // First sort on color bits per pixel
    if (fbpp != sbpp) {
        return fbpp - sbpp;
    }

    // Then sort on screen area
    if (farea != sarea) {
        return farea - sarea;
    }

    // Lastly sort on refresh rate
    return fm->refresh_rate - sm->refresh_rate;
}

// Retrieves the available modes for the specified monitor
static bool refresh_video_modes(GMonitor *monitor)
{
    if (monitor->modes) {
        return true;
    }

    int mode_count;
    GVideoMode *modes = gui_platform_get_video_modes(monitor, &mode_count);
    if (!modes) {
        return false;
    }

    qsort(modes, mode_count, sizeof(GVideoMode), compare_video_modes);

    free(monitor->modes);
    monitor->modes = modes;
    monitor->mode_count = mode_count;

    return true;
}

// May raise a non-fatal error.
void gui_input_monitor_change(GlobalGui *gui, char **error)
{
    GMonitor **monitors = gui->monitors;
    size_t monitor_count = gui->monitor_count;

    gui->monitors = gui_platform_get_monitors(&gui->monitor_count, error);

    // Re-use still connected monitor objects

    for (size_t i = 0; i < gui->monitor_count; i++) {
        for (size_t j = 0; j < monitor_count; j++) {
            if (gui_is_same_monitor(gui->monitors[i], monitors[j])) {
                free_monitor(gui->monitors[i]);
                gui->monitors[i] = monitors[j];
                break;
            }
        }
    }

    // Find and report disconnected monitors (not in the new list)

    for (size_t i = 0; i < monitor_count; i++) {
        GWindow *window;

        size_t j;
        for (j = 0; j < gui->monitor_count; j++) {
            if (monitors[i] == gui->monitors[j]) {
                break;
            }
        }

        if (j < gui->monitor_count) {
            continue;
        }

        for (window = gui->window_list_head; window; window = window->next) {
            if (window->monitor == monitors[i]) {
                int width, height;
                gui_platform_get_window_size(window, &width, &height);
                gui_set_window_monitor(window, NULL, 0, 0, width, height, 0);
            }
        }

        // TODO: dipatch the monitor disconnected event
        /* if (gui->callbacks.monitor) */
        /*     gui->callbacks.monitor((GMonitor*) monitors[i], GLFW_DISCONNECTED); */
    }

    // Find and report newly connected monitors (not in the old list)
    // Re-used monitor objects are then removed from the old list to avoid
    // having them destroyed at the end of this function

    for (size_t i = 0; i < gui->monitor_count; i++) {
      size_t j;
        for (j = 0; j < monitor_count; j++) {
            if (gui->monitors[i] == monitors[j]) {
                monitors[j] = NULL;
                break;
            }
        }

        if (j < monitor_count) {
            continue;
        }

        // TODO: dispatch the monitor connected event
        /* if (gui->callbacks.monitor) */
        /*     gui->callbacks.monitor((GMonitor*) gui->monitors[i], GLFW_CONNECTED); */
    }

    free_monitors(monitors, monitor_count);
}

GMonitor *alloc_monitor(const char *name, int width_mm, int height_mm)
{
    GMonitor *monitor = (GMonitor *)calloc(1, sizeof(GMonitor));
    monitor->name = strdup(name);
    monitor->width_mm = width_mm;
    monitor->height_mm = height_mm;

    return monitor;
}

void free_monitor(GMonitor *monitor)
{
    if (monitor == NULL) {
        return;
    }

    free_gamma_arrays(&monitor->original_ramp);
    free_gamma_arrays(&monitor->current_ramp);

    free(monitor->modes);
    free(monitor->name);
    free(monitor);
}

void alloc_gamma_arrays(GGammaRamp *ramp, uint32_t size)
{
    ramp->red = (uint16_t *)calloc(size, sizeof(uint16_t));
    ramp->green = (uint16_t *)calloc(size, sizeof(uint16_t));
    ramp->blue = (uint16_t *)calloc(size, sizeof(uint16_t));
    ramp->size = size;
}

void free_gamma_arrays(GGammaRamp *ramp)
{
    free(ramp->red);
    free(ramp->green);
    free(ramp->blue);

    memset(ramp, 0, sizeof(GGammaRamp));
}

void free_monitors(GMonitor **monitors, int count)
{
    for (int i = 0; i < count; i++) {
        free_monitor(monitors[i]);
    }
    free(monitors);
}

const GVideoMode *gui_choose_video_mode(GMonitor *monitor,
                                        const GVideoMode *desired)
{
  if (!refresh_video_modes(monitor)) {
    return NULL;
  }

  const GVideoMode *closest = NULL;

  unsigned int least_size_diff = UINT_MAX;
  unsigned int least_rate_diff = UINT_MAX;
  unsigned int least_color_diff = UINT_MAX;
  for (int i = 0; i < monitor->mode_count; i++) {
    const GVideoMode *current = monitor->modes + i;

    unsigned int color_diff = 0;

    if (desired->red_bits != -1) {
      color_diff += abs(current->red_bits - desired->red_bits);
    }
    if (desired->green_bits != -1) {
      color_diff += abs(current->green_bits - desired->green_bits);
    }
    if (desired->blue_bits != -1) {
      color_diff += abs(current->blue_bits - desired->blue_bits);
    }

    unsigned int size_diff;
    size_diff = abs((current->width - desired->width) *
                    (current->width - desired->width) +
                    (current->height - desired->height) *
                    (current->height - desired->height));

    unsigned int rate_diff;
    if (desired->refresh_rate != -1) {
      rate_diff = abs(current->refresh_rate - desired->refresh_rate);
    } else {
      rate_diff = UINT_MAX - current->refresh_rate;
    }

    if ((color_diff < least_color_diff) ||
        (color_diff == least_color_diff && size_diff < least_size_diff) ||
        (color_diff == least_color_diff && size_diff == least_size_diff && rate_diff < least_rate_diff)) {

      closest = current;
      least_size_diff = size_diff;
      least_rate_diff = rate_diff;
      least_color_diff = color_diff;
    }
  }

  return closest;
}

int gui_compare_video_modes(const GVideoMode *fm, const GVideoMode *sm)
{
    return compare_video_modes(fm, sm);
}

void gui_split_bpp(int bpp, int *red, int *green, int *blue)
{
    // We assume that by 32 the user really meant 24
    if (bpp == 32) {
        bpp = 24;
    }

    // Convert "bits per pixel" to red, green & blue sizes

    *red = *green = *blue = bpp / 3;
    int delta = bpp - (*red * 3);
    if (delta >= 1) {
        *green = *green + 1;
    }
    if (delta == 2) {
        *red = *red + 1;
    }
}

// }}}

// Public monitor abstract API {{{
GMonitor **gui_get_monitors(GlobalGui *gui, size_t *count)
{
    assert(count != NULL);
    *count = 0;

    *count = gui->monitor_count;
    return (GMonitor **) gui->monitors;
}

GMonitor *gui_get_primary_monitor(GlobalGui *gui)
{
    if (!gui->monitor_count) {
        return NULL;
    }

    return (GMonitor *) gui->monitors[0];
}

void gui_get_monitor_pos(GMonitor *monitor, int *xpos, int *ypos)
{
    assert(monitor != NULL);

    if (xpos) {
        *xpos = 0;
    }
    if (ypos) {
        *ypos = 0;
    }

    gui_platform_get_monitor_pos(monitor, xpos, ypos);
}

void gui_get_monitor_physical_size(GMonitor *monitor, int *width_mm, int *height_mm)
{
    assert(monitor != NULL);

    if (width_mm) {
        *width_mm = 0;
    }
    if (height_mm) {
        *height_mm = 0;
    }

    if (width_mm) {
        *width_mm = monitor->width_mm;
    }
    if (height_mm) {
        *height_mm = monitor->height_mm;
    }
}

// const char *gui_get_monitor_name(GMonitor *monitor)
// monitor->name

const GVideoMode *gui_get_video_modes(GMonitor *monitor, int *count)
{
    assert(monitor != NULL);
    assert(count != NULL);
    *count = 0;

    if (!refresh_video_modes(monitor)) {
        return NULL;
    }

    *count = monitor->mode_count;
    return monitor->modes;
}

const GVideoMode *gui_get_video_mode(GMonitor *monitor)
{
    assert(monitor != NULL);
    gui_platform_get_video_mode(monitor, &monitor->current_mode);
    return &monitor->current_mode;
}

void gui_set_gamma(GMonitor *monitor, float gamma, char **error)
{
  uint16_t values[256];
  GGammaRamp ramp;

  if (gamma != gamma || gamma <= 0.f || gamma > FLT_MAX) {
    *error = "Invalid gamma value";
    return;
  }

  for (int i = 0;  i < 256;  i++) {
    // Calculate intensity
    double value = i / 255.0;
    // Apply gamma curve
    value = pow(value, 1.0 / gamma) * 65535.0 + 0.5;

    // Clamp to value range
    if (value > 65535.0) {
      value = 65535.0;
    }

    values[i] = (uint16_t) value;
  }

  ramp.red = values;
  ramp.green = values;
  ramp.blue = values;
  ramp.size = 256;

  gui_set_gamma_ramp(monitor, &ramp);
}

const GGammaRamp *gui_get_gamma_ramp(GMonitor *monitor)
{
    assert(monitor != NULL);

    free_gamma_arrays(&monitor->current_ramp);
    gui_platform_get_gamma_ramp(monitor, &monitor->current_ramp);

    return &monitor->current_ramp;
}

void gui_set_gamma_ramp(GMonitor *monitor, const GGammaRamp *ramp)
{
    assert(monitor != NULL);
    assert(ramp != NULL);

    if (!monitor->original_ramp.size) {
        gui_platform_get_gamma_ramp(monitor, &monitor->original_ramp);
    }

    gui_platform_set_gamma_ramp(monitor, ramp);
}
// }}}

// Internal input abstract API {{{

// Sets the cursor mode for the specified window
static void set_cursor_mode(GWindow *window, GCursorMode new_mode, char **error)
{
  const int old_mode = window->cursor_mode;

  if (old_mode == new_mode) {
    return;
  }

  GlobalGui *gui = window->gui;
  window->cursor_mode = new_mode;

  if (gui->cursor_window == window) {
    if (old_mode == kCursorDisabled) {
      gui_platform_set_cursor_pos(window, gui->cursor_pos_x, gui->cursor_pos_y);
    } else if (new_mode == kCursorDisabled) {
      gui_platform_set_cursor_pos(window, gui->cursor_pos_x, gui->cursor_pos_y);

      window->cursor_pos_x = gui->cursor_pos_x;
      window->cursor_pos_y = gui->cursor_pos_y;

      int width, height;
      gui_platform_get_window_size(window, &width, &height);
      gui_platform_set_cursor_pos(window, width / 2, height / 2);
    }

    gui_platform_set_cursor_mode(window, (GCursorMode)window->cursor_mode);
  }
}

// Set sticky keys mode for the specified window
static void set_sticky_keys(GWindow *window, int enabled)
{
  if (window->sticky_keys == enabled) {
    return;
  }

  if (!enabled) {
    // Release all sticky keys
    for (int i = 0;  i <= GUI_KEY_LAST;  i++) {
      if (window->keys[i] == GUI_KEY_STICK) {
        window->keys[i] = kRelease;
      }
    }
  }

  window->sticky_keys = enabled;
}

// Set sticky mouse buttons mode for the specified window
static void set_sticky_mouse_button(GWindow *window, int enabled)
{
  if (window->sticky_mouse_buttons == enabled) {
    return;
  }

  if (!enabled) {
    // Release all sticky mouse buttons
    for (int i = 0; i <= kMouseButtonLast; i++) {
      if (window->mouse_buttons[i] == kMouseButtonStick) {
        window->mouse_buttons[i] = kRelease;
      }
    }
  }

  window->sticky_mouse_buttons = enabled;
}

bool gui_is_printable(GKey key)
{
  return (key >= GUI_KEY_APOSTROPHE && key <= GUI_KEY_WORLD_2) ||
         (key >= GUI_KEY_KP_0 && key <= GUI_KEY_KP_ADD) ||
         key == GUI_KEY_KP_EQUAL;
}
// }}}

// Public event abstract API {{{
void gui_input_key(GWindow *window, GKey key, int scancode, int action, int mods)
{
  if (key >= 0 && key <= GUI_KEY_LAST) {
    bool repeated = false;

    if (action == kRelease && window->keys[key] == kRelease) {
      return;
    }

    repeated = (action == kPress && window->keys[key] == kPress);

    if (action == kRelease && window->sticky_keys) {
      window->keys[key] = GUI_KEY_STICK;
    } else {
      window->keys[key] = (char) action;
    }

    if (repeated) {
      action = kRepeat;
    }
  }

  GWindowEvent event;
  event.type = kWindowKey;
  event.window = window;
  event.e.key.key = key;
  event.e.key.scancode = scancode;
  event.e.key.action = action;
  event.e.key.mods = mods;

  window->gui->handle_event(event);
}

void gui_input_char(GWindow *window, unsigned int codepoint, int mods, bool plain)
{
  if (codepoint < 32 || (codepoint > 126 && codepoint < 160)) {
    return;
  }

  GWindowEvent event;
  event.type = kWindowChar;
  event.window = window;
  event.e.char_.codepoint = codepoint;
  event.e.char_.mods = mods;
  event.e.char_.plain = plain;

  window->gui->handle_event(event);
}

void gui_input_scroll(GWindow *window, double xoffset, double yoffset)
{
  GWindowEvent event;
  event.type = kWindowScroll;
  event.window = window;
  event.e.scroll.xoffset = xoffset;
  event.e.scroll.yoffset = yoffset;

  window->gui->handle_event(event);
}

void gui_input_mouse_click(GWindow *window, GMouseButton button, int action, int mods)
{
  if (button > kMouseButtonLast) {
    return;
  }

  // Register mouse button action
  if (action == kRelease && window->sticky_mouse_buttons) {
    window->mouse_buttons[button] = GUI_KEY_STICK;
  } else {
    window->mouse_buttons[button] = (char) action;
  }

  GWindowEvent event;
  event.type = kWindowMouse;
  event.window = window;
  event.e.mouse.button = (GMouseButton)button;
  event.e.mouse.action = (GMouseAction)action;
  event.e.mouse.mods = mods;

  window->gui->handle_event(event);
}

void gui_input_cursor_motion(GWindow *window, double x, double y)
{
  if (window->cursor_mode == kCursorDisabled) {
    if (x == 0.0 && y == 0.0) {
      return;
    }

    window->cursor_pos_x += x;
    window->cursor_pos_y += y;

    x = window->cursor_pos_x;
    y = window->cursor_pos_y;
  }

  GWindowEvent event;
  event.type = kWindowCursorMotion;
  event.window = window;
  event.e.cursor.x = x;
  event.e.cursor.y = y;

  window->gui->handle_event(event);
}

void gui_input_cursor_enter(GWindow *window, bool entered)
{
  GWindowEvent event;
  event.type = kWindowCursorEnterChange;
  event.window = window;
  event.e.cursor.entered = entered;

  window->gui->handle_event(event);
}

void gui_input_drop(GWindow *window, int count, const char **paths)
{
  GWindowEvent event;
  event.type = kWindowDrop;
  event.window = window;
  event.e.drop.count = count;
  event.e.drop.paths = paths;

  window->gui->handle_event(event);
}

void gui_input_window_focus(GWindow *window, bool focused)
{
  GWindowEvent event;
  event.type = kWindowFocusChange;
  event.window = window;
  event.e.focused = focused;

  if (focused) {
    window->gui->cursor_window = window;

    window->gui->handle_event(event);
  } else {
    window->gui->cursor_window = NULL;

    window->gui->handle_event(event);

    // Release all pressed keyboard keys
    for (int i = 0; i <= GUI_KEY_LAST; i++) {
      if (window->keys[i] == kPress) {
        gui_input_key(window, (GKey)i, 0, kRelease, 0);
      }
    }

    // Release all pressed mouse buttons
    for (int i = 0;  i <= kMouseButtonLast;  i++) {
      if (window->mouse_buttons[i] == kPress) {
        gui_input_mouse_click(window, (GMouseButton)i, kRelease, 0);
      }
    }
  }
}

void gui_input_window_pos(GWindow *window, int x, int y)
{
  GWindowEvent event;
  event.type = kWindowMove;
  event.window = window;
  event.e.pos.x = x;
  event.e.pos.y = y;

  window->gui->handle_event(event);
}

void gui_input_window_size(GWindow *window, int width, int height)
{
  GWindowEvent event;
  event.type = kWindowResize;
  event.window = window;
  event.e.size.width = width;
  event.e.size.height = height;

  window->gui->handle_event(event);
}

void gui_input_window_iconify(GWindow *window, bool iconified)
{
  GWindowEvent event;
  event.type = kWindowIconifyChange;
  event.window = window;
  event.e.iconified = iconified;

  window->gui->handle_event(event);
}

void gui_input_framebuffer_size(GWindow *window, int width, int height)
{
  GWindowEvent event;
  event.type = kWindowFramebufferResize;
  event.window = window;
  event.e.size.width = width;
  event.e.size.height = height;

  window->gui->handle_event(event);
}

void gui_input_window_damage(GWindow *window)
{
  GWindowEvent event;
  event.type = kWindowDamage;
  event.window = window;

  window->gui->handle_event(event);
}

void gui_input_window_close_request(GWindow *window)
{
    window->closed = true;

    GWindowEvent event;
    event.type = kWindowClose;
    event.window = window;

    window->gui->handle_event(event);
}

void gui_input_window_monitor_change(GWindow *window, GMonitor *monitor)
{
    window->monitor = monitor;
}

// }}}

// Public window manipulation abstract API {{{

// TODO: check how this is done in GLFW again
GWindow *gui_create_window(GlobalGui *gui,
                           int width,
                           int height,
                           const char *title,
                           GMonitor *monitor,
                           char **error)
{
  assert(title != NULL);

  GWindowConfig win_config = gui->hints.window;
  win_config.width   = width;
  win_config.height  = height;
  win_config.title   = title;

  GWindow *window = (GWindow *)calloc(1, sizeof(GWindow));
  window->gui = gui;
  window->next = gui->window_list_head;
  gui->window_list_head = window;

  window->video_mode.width        = width;
  window->video_mode.height       = height;
  window->video_mode.red_bits     = 8;
  window->video_mode.green_bits   = 8;
  window->video_mode.blue_bits    = 8;

  window->monitor       = (GMonitor*) monitor;
  window->resizable     = gui->hints.window.resizable;
  window->decorated     = gui->hints.window.decorated;
  window->auto_iconify  = gui->hints.window.auto_iconify;
  window->floating      = gui->hints.window.floating;
  window->cursor_mode   = kCursorNormal;

  window->minwidth     = -1;
  window->minheight    = -1;
  window->maxwidth     = -1;
  window->maxheight    = -1;
  window->numer        = -1;
  window->denom        = -1;

  // Save the currently current context so it can be restored later
  // GWindow *previous = _glfwPlatformGetCurrentContext();

  // Open the actual window and create its context
  if (!gui_platform_create_window(window,
                                  &win_config,
                                  &gui->hints.context,
                                  &gui->hints.framebuffer,
                                  error)) {
    gui_destroy_window(window);
    // gui_gl_make_context_current(previous);
    return NULL;
  }

  gui_gl_make_context_current(window);

  if (window->monitor) {
    int width, height;
    gui_platform_get_window_size(window, &width, &height);

    window->cursor_pos_x = width / 2;
    window->cursor_pos_y = height / 2;

    gui_platform_set_cursor_pos(window, window->cursor_pos_x, window->cursor_pos_y);
  } else {
    if (win_config.visible) {
      gui_show_window(window);
      if (win_config.focused) {
        gui_platform_focus_window(window);
      }
    }
  }

  return window;
}

void gui_destroy_window(GWindow *window)
{
    // Allow closing of NULL (to match the behavior of free)
    if (window == NULL) {
        return;
    }

    // The window's context must not be current on another thread when the
    // window is destroyed
    // if (window == _glfwPlatformGetCurrentContext())
    //   _glfwPlatformMakeContextCurrent(NULL);

    GlobalGui *gui = window->gui;

    // Clear the focused window pointer if this is the focused window
    if (gui->cursor_window == window) {
        gui->cursor_window = NULL;
    }

    gui_platform_destroy_window(window);

    // Unlink window from global linked list
    {
        GWindow **prev = &gui->window_list_head;

        while (*prev != window) {
          prev = &((*prev)->next);
        }

        *prev = window->next;

        if (*prev != NULL) {
          gui_platform_focus_window(*prev);
        }
    }

    free(window);
}

// Trivial:
// gui_set_window_title() is implemented in the platform code
// gui_set_window_icon() is implemented in the platform code
// gui_get_window_pos() is implemented in the platform code
// gui_set_window_pos() is implemented in the platform code
//  (may set whether the window is fullscreen before setting pos)
// gui_get_window_size() is implemented in the platform code
// gui_set_window_size() is implemented in the platform code
//  (update the video mode before setting the window size)
/*
void gui_set_window_size_limits(GWindow *window,
                                int minwidth, int minheight,
                                int maxwidth, int maxheight)
{
    assert(window != NULL);

    window->minwidth  = minwidth;
    window->minheight = minheight;
    window->maxwidth  = maxwidth;
    window->maxheight = maxheight;

    if (window->monitor || !window->resizable) {
        return;
    }

    gui_platform_set_window_size_limits(window,
                                        minwidth, minheight,
                                        maxwidth, maxheight);
}

void gui_set_window_aspect_ratio(GWindow *window, int numer, int denom)
{
    assert(window != NULL);
    assert(denom != 0);

    window->numer = numer;
    window->denom = denom;

    if (window->monitor || !window->resizable) {
        return;
    }

    gui_platform_set_window_aspect_ratio(window, numer, denom);
}
*/


// gui_get_framebuffer_size() is implemented in the platform code
// gui_set_window_frame_size() is implemented in the platform code
// gui_iconify_window() is implemented in the platform code
// gui_restore_window() is implemented in the platform code
// gui_maximize_window() is implemented in the platform code

void gui_show_window(GWindow *window)
{
    assert(window != NULL);

    if (window->monitor) {
        return;
    }

    gui_platform_show_window(window);
    gui_platform_focus_window(window);
}

void gui_set_window_monitor(GWindow *window,
                            GMonitor *monitor,
                            int xpos,
                            int ypos,
                            int width,
                            int height,
                            int refresh_rate)
{
    assert(window);

    window->video_mode.width = width;
    window->video_mode.height = height;
    window->video_mode.refresh_rate = refresh_rate;

    gui_platform_set_window_monitor(window, monitor,
                                    xpos, ypos, width, height,
                                    refresh_rate);
}

// void gui_set_window_user_pointer(GWindow *window, void *pointer)
// void *get_window_user_pointer(GWindow *window)

// }}}

// Public input abstract API {{{

#if 0
int gui_get_input_mode(GWindow *window, GWindowInputMode mode)
{
  assert(window != NULL);

  switch (mode) {
    case kCursorMode:
      return window->cursor_mode;
    case kStickyKeysMode:
      return window->sticky_keys;
    case kStickyMouseButtonsMode:
      return window->sticky_mouse_buttons;
  }
}

void gui_set_input_mode(GWindow *window, GWindowInputMode mode, int value)
{
  assert(window != NULL);

  switch (mode) {
    case kCursorMode:
      set_cursor_mode(window, value);
      break;
    case kStickyKeysMode:
      set_sticky_keys(window, (bool)value);
      break;
    case kStickyMouseButtonsMode:
      set_sticky_mouse_button(window, (bool)value);
      break;
  }
}
#endif

// gui_get_key_name(int key, int scancode) defined in platform code

int gui_get_key(GWindow *window, GKey key, char **error)
{
  assert(window != NULL);

  if (key < 0 || key > GUI_KEY_LAST) {
    *error = "Invalid key";
    return kRelease;
  }

  if (window->keys[key] == GUI_KEY_STICK) {
    // Sticky mode: release key now
    window->keys[key] = kRelease;
    return kPress;
  }

  return (int) window->keys[key];
}

int gui_get_mouse_button(GWindow *window, GMouseButton button)
{
  assert(window != NULL);

  if (window->mouse_buttons[button] == (char)kMouseButtonStick) {
    // Sticky mode: release mouse button now
    window->mouse_buttons[button] = kRelease;
    return kPress;
  }

  return (int) window->mouse_buttons[button];
}

void gui_get_cursor_pos(GWindow *window, double *xpos, double *ypos)
{
  assert(window != NULL);

  if (xpos) {
    *xpos = 0;
  }
  if (ypos) {
    *ypos = 0;
  }

  if (window->cursor_mode == kCursorDisabled) {
    if (xpos) {
      *xpos = window->cursor_pos_x;
    }
    if (ypos) {
      *ypos = window->cursor_pos_y;
    }
  } else {
    gui_platform_get_cursor_pos(window, xpos, ypos);
  }
}

void gui_set_cursor_pos(GWindow *window, double xpos, double ypos)
{
  assert(window != NULL);

  if (window->gui->cursor_window != window) {
    return;
  }

  if (window->cursor_mode == kCursorDisabled) {
    // Only update the accumulated position if the cursor is disabled
    window->cursor_pos_x = xpos;
    window->cursor_pos_y = ypos;
  } else {
    // Update system cursor position
    gui_platform_set_cursor_pos(window, xpos, ypos);
  }
}

GCursor *gui_create_cursor(GlobalGui *gui, const GImage *image, int xhot, int yhot) {
    assert(image != NULL);

    GCursor *cursor = (GCursor *)calloc(1, sizeof(GCursor));
    cursor->next = gui->cursor_list_head;
    gui->cursor_list_head = cursor;

    if (!gui_platform_create_cursor(cursor, image, xhot, yhot)) {
        gui_destroy_cursor(gui, (GCursor *) cursor);
        return NULL;
    }

    return (GCursor *) cursor;
}

GCursor *gui_create_standard_cursor(GlobalGui *gui, GCursorShape shape, char **error)
{
  if (shape != kArrowCursor &&
      shape != kIBeamCursor &&
      shape != kCrosshairCursor &&
      shape != kHandCursor &&
      shape != kHResizeCursor &&
      shape != kVResizeCursor) {
    *error = "Invalid standard cursor";
    return NULL;
  }

  GCursor *cursor = (GCursor *)calloc(1, sizeof(GCursor));
  cursor->next = gui->cursor_list_head;
  gui->cursor_list_head = cursor;

  if (!gui_platform_create_standard_cursor(cursor, shape, error)) {
    gui_destroy_cursor(gui, (GCursor*) cursor);
    return NULL;
  }

  return (GCursor *) cursor;
}

void gui_destroy_cursor(GlobalGui *gui, GCursor *cursor)
{
  if (cursor == NULL) {
    return;
  }

  // Make sure the cursor is not being used by any window
  for (GWindow *window = gui->window_list_head; window; window = window->next) {
    if (window->cursor == cursor) {
      gui_set_cursor((GWindow *) window, NULL);
    }
  }

  gui_platform_destroy_cursor(cursor);

  // Unlink cursor from global linked list
  {
    GCursor **prev = &gui->cursor_list_head;
    while (*prev != cursor) {
      prev = &((*prev)->next);
    }
    *prev = cursor->next;
  }

  free(cursor);
}

void gui_set_cursor(GWindow *window, GCursor *cursor)
{
    assert(window != NULL);
    gui_platform_set_cursor(window, cursor);
    window->cursor = cursor;
}

// }}}

// gui_set_clipboard_string(window, string) is implemented in platform code
// gui_get_clipboard_string(window) is implemented in platform code

// Abstract Time API {{{
double gui_get_time(GlobalGui *gui)
{
  return (double) (gui_get_timer_value(gui) - gui->timer_offset) /
    gui_get_timer_frequency(gui);
}

void gui_set_time(GlobalGui *gui, double time)
{
  gui->timer_offset = gui_get_timer_value(gui) -
    (uint64_t)(time * gui_get_timer_frequency(gui));
}

// gui_get_timer_value() is implemented in platform code
// gui_get_timer_frequency() is implemented in platform code
// }}}
// }}}
#ifdef __cplusplus
};
#endif
#endif  // GUI_COMMON_IMPLEMENTATION

#endif  // GUI_COMMON_H_
