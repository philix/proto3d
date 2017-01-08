/* vim: set shiftwidth=4 softtabstop=4 tabstop=4: */
#include <dlfcn.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Needed for _NSGetProgname
#include <crt_externs.h>
#include <mach/mach_time.h>

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

// Includes for the monitor API
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVDisplayLink.h>
#include <ApplicationServices/ApplicationServices.h>

#include "gui_common.h"

static GlobalGui *_global_gui_hack;

// Internal NSGL context API {{{
static bool init_nsgl(GlobalGui *gui, char **error)
{
    gui->nsgl.framework =
        CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));

    if (gui->nsgl.framework == NULL) {
        *error = "NSGL: Failed to locate OpenGL framework";
        return false;
    }
    return true;
}

// Create the NSGL context.
//
// OS X only supports forward-compatible contexts for
// OpenGL 3.2 and above (4.1).
static bool create_nsgl_context(GWindow *window,
                                const GContextConfig *ctx_config,
                                const GFramebufferConfig *fb_config,
                                char **error)
{
    NSOpenGLPixelFormatAttribute attrs[32];  // Arbitrary array size here
    unsigned int attr_count = 0;

#define ADD_ATTR(x)      { attrs[attr_count++] = (x); }
#define ADD_ATTR2(x, y)  { ADD_ATTR(x); ADD_ATTR(y); }

    ADD_ATTR(NSOpenGLPFAAccelerated);
    ADD_ATTR(NSOpenGLPFAClosestPolicy);

    switch (ctx_config->major) {
        case 4:
            ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core);
            break;
        case 3:
            ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core);
            break;
        case 2:
        case 1:
            // const int accum_bits = fb_config->accum_red_bits +
            //                        fb_config->accum_green_bits +
            //                        fb_config->accum_blue_bits +
            //                        fb_config->accum_alpha_bits;
            // ADD_ATTR2(NSOpenGLPFAAccumSize, accum_bits);
            // ADD_ATTR2(NSOpenGLPFAAuxBuffers, fb_config->aux_buffers);
            break;
        default:
            assert(false && "Invalid requested OpenGL major version");
            break;
    }

    int color_bits = fb_config->red_bits +
                     fb_config->green_bits +
                     fb_config->blue_bits;
    // OS X needs non-zero color size, so set reasonable values
    if (color_bits == 0) {
        color_bits = 24;
    } else if (color_bits < 15) {
        color_bits = 15;
    }
    ADD_ATTR2(NSOpenGLPFAColorSize, color_bits);

    ADD_ATTR2(NSOpenGLPFAAlphaSize, fb_config->alpha_bits);
    ADD_ATTR2(NSOpenGLPFADepthSize, fb_config->depth_bits);
    ADD_ATTR2(NSOpenGLPFAStencilSize, fb_config->stencil_bits);
    if (fb_config->double_buffer) {
        ADD_ATTR(NSOpenGLPFADoubleBuffer);
    }

    if (fb_config->stereo) {
        ADD_ATTR(NSOpenGLPFAStereo);
    }

    if (fb_config->samples >= 0) {
        if (fb_config->samples == 0) {
            ADD_ATTR2(NSOpenGLPFASampleBuffers, 0);
        } else {
            ADD_ATTR2(NSOpenGLPFASampleBuffers, 1);
            ADD_ATTR2(NSOpenGLPFASamples, fb_config->samples);
        }
    }

    ADD_ATTR(0);
    assert(attr_count <= 32);

#undef ADD_ATTR
#undef ADD_ATTR2

    struct context_nsgl_s *nsgl = &window->context.nsgl;

    nsgl->pixel_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

    if (nsgl->pixel_format == nil) {
        *error = "NSGL: Failed to find a suitable pixel format";
        return false;
    }

    nsgl->object =
        [[NSOpenGLContext alloc] initWithFormat:window->context.nsgl.pixel_format
                                   shareContext:NULL];

    if (nsgl->object == nil) {
        *error = "NSGL: Failed to create OpenGL context";
        return false;
    }

    [nsgl->object setView:window->ns.view];
    return true;
}

// Destroy the OpenGL context
static void destroy_nsgl_context(GWindow *window)
{
    [window->context.nsgl.pixel_format release];
    window->context.nsgl.pixel_format = nil;

    [window->context.nsgl.object release];
    window->context.nsgl.object = nil;
}
// }}}

// OpenGL context public platform API {{{

void gui_gl_make_context_current(GWindow *window)
{
    if (window) {
        [window->context.nsgl.object makeCurrentContext];
    } else {
        [NSOpenGLContext clearCurrentContext];
    }

    // TODO: evaluate the need for a global context (TLS madness)
    // gui_gl_set_current_context(window);
}

void gl_swap_buffers(GWindow *window)
{
    // ARP appears to be unnecessary, but this is future-proof
    [window->context.nsgl.object flushBuffer];
}

// interval: 1 => V-Sync enabled
//           0 => V-Sync disabled
void gl_swap_interval(GWindow *window, int interval)
{
    GLint sync = interval;
    [window->context.nsgl.object setValues:&sync
                              forParameter:NSOpenGLCPSwapInterval];
}

GL_Proc gl_get_proc_address(GlobalGui *gui, const char *procname)
{
    CFStringRef symbolName = CFStringCreateWithCString(kCFAllocatorDefault,
                                                       procname,
                                                       kCFStringEncodingASCII);

    GL_Proc symbol = (GL_Proc) CFBundleGetFunctionPointerForName(
        gui->nsgl.framework,
        symbolName);

    CFRelease(symbolName);

    return symbol;
}

// }}}

// Internal monitor API {{{

// Get the name of the specified display
static char *get_display_name(CGDirectDisplayID displayID, char **error)
{
    // NOTE: This uses a deprecated function because Apple has
    //       (as of January 2015) not provided any alternative
    CFDictionaryRef info = IODisplayCreateInfoDictionary(
        CGDisplayIOServicePort(displayID),
        kIODisplayOnlyPreferredName);
    CFDictionaryRef names = CFDictionaryGetValue(info, CFSTR(kDisplayProductName));

    CFStringRef value;
    if (!names
        || !CFDictionaryGetValueIfPresent(names, CFSTR("en_US"), (const void**) &value)) {

        // This may happen if a desktop Mac is running headless
        *error = "Cocoa: Failed to retrieve display name.";
        CFRelease(info);
        return strdup("Unknown");
    }

    CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(value),
                                                     kCFStringEncodingUTF8);

    char *name = calloc(size + 1, sizeof(char));
    CFStringGetCString(value, name, size, kCFStringEncodingUTF8);

    CFRelease(info);
    return name;
}

// Check whether the display mode should be included in enumeration
static bool mode_is_good(CGDisplayModeRef mode)
{
    uint32_t flags = CGDisplayModeGetIOFlags(mode);
    if (!(flags & kDisplayModeValidFlag) || !(flags & kDisplayModeSafeFlag)) {
      return false;
    }

    if (flags & kDisplayModeInterlacedFlag) {
      return false;
    }

    if (flags & kDisplayModeStretchedFlag) {
      return false;
    }

    CFStringRef format = CGDisplayModeCopyPixelEncoding(mode);
    if (CFStringCompare(format, CFSTR(IO16BitDirectPixels), 0) &&
        CFStringCompare(format, CFSTR(IO32BitDirectPixels), 0)) {
        CFRelease(format);
        return false;
    }

    CFRelease(format);
    return true;
}

// Convert Core Graphics display mode to our video mode
static GVideoMode video_mode_from_CGDisplayMode(CGDisplayModeRef mode,
                                                CVDisplayLinkRef link)
{
    GVideoMode result;
    result.width = (int) CGDisplayModeGetWidth(mode);
    result.height = (int) CGDisplayModeGetHeight(mode);
    result.refresh_rate = (int) CGDisplayModeGetRefreshRate(mode);

    if (result.refresh_rate == 0) {
        const CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(link);
        if (!(time.flags & kCVTimeIsIndefinite)) {
            result.refresh_rate = (int) (time.timeScale / (double) time.timeValue);
        }
    }

    CFStringRef format = CGDisplayModeCopyPixelEncoding(mode);

    if (CFStringCompare(format, CFSTR(IO16BitDirectPixels), 0) == 0) {
        result.red_bits = 5;
        result.green_bits = 5;
        result.blue_bits = 5;
    } else {
        result.red_bits = 8;
        result.green_bits = 8;
        result.blue_bits = 8;
    }

    CFRelease(format);
    return result;
}

// Starts reservation for display fading
static CGDisplayFadeReservationToken begin_fade_reservation(void)
{
    CGDisplayFadeReservationToken token = kCGDisplayFadeReservationInvalidToken;

    if (CGAcquireDisplayFadeReservation(5, &token) == kCGErrorSuccess) {
        CGDisplayFade(token,
                      0.3,
                      kCGDisplayBlendNormal,
                      kCGDisplayBlendSolidColor,
                      0.0,
                      0.0,
                      0.0,
                      TRUE);
    }

    return token;
}

// Ends reservation for display fading
static void end_fade_reservation(CGDisplayFadeReservationToken token)
{
    if (token != kCGDisplayFadeReservationInvalidToken) {
        CGDisplayFade(
            token,
            0.5,
            kCGDisplayBlendSolidColor,
            kCGDisplayBlendNormal,
            0.0,
            0.0,
            0.0,
            FALSE);
        CGReleaseDisplayFadeReservation(token);
    }
}


// Change the current video mode
static bool gui_set_video_mode_ns(GMonitor *monitor,
                                  const GVideoMode *desired,
                                  char **error)
{
    const GVideoMode *best = gui_choose_video_mode(monitor, desired);

    GVideoMode current;
    gui_platform_get_video_mode(monitor, &current);
    if (gui_compare_video_modes(&current, best) == 0) {
        return true;
    }

    CVDisplayLinkRef link;
    CVDisplayLinkCreateWithCGDisplay(monitor->ns.display_id, &link);

    CFArrayRef modes = CGDisplayCopyAllDisplayModes(monitor->ns.display_id,
                                                    NULL);
    CFIndex count = CFArrayGetCount(modes);

    CGDisplayModeRef native = NULL;

    for (CFIndex i = 0;  i < count;  i++) {
        CGDisplayModeRef dm = (CGDisplayModeRef) CFArrayGetValueAtIndex(modes, i);
        if (!mode_is_good(dm)) {
            continue;
        }

        const GVideoMode mode = video_mode_from_CGDisplayMode(dm, link);
        if (gui_compare_video_modes(best, &mode) == 0) {
            native = dm;
            break;
        }
    }

    if (native) {
        if (monitor->ns.previous_mode == NULL) {
            monitor->ns.previous_mode =
                CGDisplayCopyDisplayMode(monitor->ns.display_id);
        }

        CGDisplayFadeReservationToken token = begin_fade_reservation();
        CGDisplaySetDisplayMode(monitor->ns.display_id, native, NULL);
        end_fade_reservation(token);
    }

    CFRelease(modes);
    CVDisplayLinkRelease(link);

    if (!native) {
        *error = "Cocoa: Monitor mode list changed";
        return false;
    }

    return true;
}

// Restore the previously saved (original) video mode
//
void gui_restore_video_mode_ns(GMonitor *monitor)
{
    if (monitor->ns.previous_mode) {
        CGDisplayFadeReservationToken token = begin_fade_reservation();
        CGDisplaySetDisplayMode(monitor->ns.display_id,
                                monitor->ns.previous_mode,
                                NULL);
        end_fade_reservation(token);

        CGDisplayModeRelease(monitor->ns.previous_mode);
        monitor->ns.previous_mode = NULL;
    }
}

// }}}

// Monitor public platform API {{{

// May raise a non-fatal error if the display name can't be determined.
GMonitor **gui_platform_get_monitors(size_t *count, char **error)
{
    *count = 0;

    uint32_t displayCount;
    CGGetOnlineDisplayList(0, NULL, &displayCount);
    CGDirectDisplayID *displays = calloc(displayCount,
                                         sizeof(CGDirectDisplayID));
    GMonitor **monitors = calloc(displayCount, sizeof(GMonitor*));

    CGGetOnlineDisplayList(displayCount, displays, &displayCount);

    uint32_t found = 0;
    for (uint32_t i = 0; i < displayCount; i++) {
        GMonitor *monitor;

        if (CGDisplayIsAsleep(displays[i])) {
            continue;
        }

        const CGSize size = CGDisplayScreenSize(displays[i]);
        char *name = get_display_name(displays[i], error);

        monitor = alloc_monitor(name, size.width, size.height);
        monitor->ns.display_id  = displays[i];
        monitor->ns.unit_number = CGDisplayUnitNumber(displays[i]);

        free(name);

        monitors[found++] = monitor;
    }

    free(displays);

    *count = found;
    return monitors;
}

bool gui_is_same_monitor(GMonitor *first, GMonitor *second)
{
    // HACK: Compare unit numbers instead of display IDs to work around display
    //       replacement on machines with automatic graphics switching
    return first->ns.unit_number == second->ns.unit_number;
}

void gui_platform_get_monitor_pos(GMonitor *monitor, int *x, int *y)
{
    const CGRect bounds = CGDisplayBounds(monitor->ns.display_id);

    if (x) {
        *x = (int) bounds.origin.x;
    }
    if (y) {
        *y = (int) bounds.origin.y;
    }
}

GVideoMode *gui_platform_get_video_modes(GMonitor *monitor, int *count)
{
    *count = 0;

    CVDisplayLinkRef link;
    CVDisplayLinkCreateWithCGDisplay(monitor->ns.display_id, &link);

    CFArrayRef modes = CGDisplayCopyAllDisplayModes(monitor->ns.display_id, NULL);
    CFIndex found = CFArrayGetCount(modes);
    GVideoMode *result = calloc(found, sizeof(GVideoMode));

    for (CFIndex i = 0; i < found; i++) {
        CGDisplayModeRef dm = (CGDisplayModeRef) CFArrayGetValueAtIndex(modes, i);
        if (!mode_is_good(dm)) {
            continue;
        }

        const GVideoMode mode = video_mode_from_CGDisplayMode(dm, link);

        for (CFIndex j = 0; j < *count; j++) {
            if (gui_compare_video_modes(result + j, &mode) == 0) {
                break;
            }
        }

        // Skip duplicate modes
        if (i < *count) {
            continue;
        }

        (*count)++;
        result[*count - 1] = mode;
    }

    CFRelease(modes);
    CVDisplayLinkRelease(link);
    return result;
}

void gui_platform_get_video_mode(GMonitor *monitor, GVideoMode *mode)
{
    CVDisplayLinkRef link;
    CVDisplayLinkCreateWithCGDisplay(monitor->ns.display_id, &link);

    CGDisplayModeRef displayMode;
    displayMode = CGDisplayCopyDisplayMode(monitor->ns.display_id);
    *mode = video_mode_from_CGDisplayMode(displayMode, link);
    CGDisplayModeRelease(displayMode);

    CVDisplayLinkRelease(link);
}

void gui_platform_get_gamma_ramp(GMonitor *monitor, GGammaRamp *ramp)
{
    uint32_t size = CGDisplayGammaTableCapacity(monitor->ns.display_id);
    CGGammaValue *values = calloc(size * 3, sizeof(CGGammaValue));

    CGGetDisplayTransferByTable(monitor->ns.display_id,
                                size,
                                values,
                                values + size,
                                values + size * 2,
                                &size);

    alloc_gamma_arrays(ramp, size);

    for (uint32_t i = 0; i < size; i++) {
        ramp->red[i]   = (unsigned short) (values[i] * 65535);
        ramp->green[i] = (unsigned short) (values[i + size] * 65535);
        ramp->blue[i]  = (unsigned short) (values[i + size * 2] * 65535);
    }

    free(values);
}

void gui_platform_set_gamma_ramp(GMonitor *monitor, const GGammaRamp *ramp)
{
    CGGammaValue *values = calloc(ramp->size * 3, sizeof(CGGammaValue));

    for (uint32_t i = 0; i < ramp->size; i++) {
        values[i]                  = ramp->red[i] / 65535.f;
        values[i + ramp->size]     = ramp->green[i] / 65535.f;
        values[i + ramp->size * 2] = ramp->blue[i] / 65535.f;
    }

    CGSetDisplayTransferByTable(monitor->ns.display_id,
                                ramp->size,
                                values,
                                values + ramp->size,
                                values + ramp->size * 2);

    free(values);
}

// }}}

// Internal window engine API {{{

// Key related functions {{{

static NSCursor *get_standard_cursor(GCursorShape shape)
{
    switch (shape) {
        case kArrowCursor:
            return [NSCursor arrowCursor];
        case kIBeamCursor:
            return [NSCursor IBeamCursor];
        case kCrosshairCursor:
            return [NSCursor crosshairCursor];
        case kHandCursor:
            return [NSCursor pointingHandCursor];
        case kHResizeCursor:
            return [NSCursor resizeLeftRightCursor];
        case kVResizeCursor:
            return [NSCursor resizeUpDownCursor];
    }

    return nil;
}

// Returns the style mask corresponding to the window settings
static NSUInteger get_style_mask(GWindow* window)
{
    NSUInteger styleMask = 0;

    if (window->monitor || !window->decorated) {
        styleMask |= NSBorderlessWindowMask;
    } else {
        styleMask |= NSTitledWindowMask | NSClosableWindowMask |
                     NSMiniaturizableWindowMask;

        if (window->resizable) {
            styleMask |= NSResizableWindowMask;
        }
    }

    return styleMask;
}

// Translates OS X key modifiers into cross-platform ones
static int translate_modifier_flags(NSUInteger flags)
{
    int mods = 0;

    if (flags & NSShiftKeyMask) {
        mods |= kModShift;
    }
    if (flags & NSControlKeyMask) {
        mods |= kModControl;
    }
    if (flags & NSAlternateKeyMask) {
        mods |= kModAlt;
    }
    if (flags & NSCommandKeyMask) {
        mods |= kModSuper;
    }

    return mods;
}

// Translates a OS X keycode to a GLFW keycode
static int translate_key(GlobalGui *gui, unsigned int key)
{
    if (key >= sizeof(gui->ns.public_keys) / sizeof(gui->ns.public_keys[0])) {
        return GUI_KEY_UNKNOWN;
    }

    return gui->ns.public_keys[key];
}

// Translate a GLFW keycode to a Cocoa modifier flag
static NSUInteger translateKeyToModifierFlag(int key)
{
    switch (key) {
        case GUI_KEY_LEFT_SHIFT:
        case GUI_KEY_RIGHT_SHIFT:
            return NSShiftKeyMask;
        case GUI_KEY_LEFT_CONTROL:
        case GUI_KEY_RIGHT_CONTROL:
            return NSControlKeyMask;
        case GUI_KEY_LEFT_ALT:
        case GUI_KEY_RIGHT_ALT:
            return NSAlternateKeyMask;
        case GUI_KEY_LEFT_SUPER:
        case GUI_KEY_RIGHT_SUPER:
            return NSCommandKeyMask;
    }

    return 0;
}

// }}}

// Cursor related API {{{

// Center the cursor in the view of the window
static void center_cursor(GWindow *window)
{
    // TODO: will work only when the monitor code lands
    int width, height;
    gui_platform_get_window_size(window, &width, &height);
    gui_platform_set_cursor_pos(window, width / 2.0, height / 2.0);
}

// Transforms the specified y-coordinate between the CG display and NS screen
// coordinate systems
static float transform_y(float y)
{
    // TODO: check if we can really use CGMainDisplayID() all the time
    return CGDisplayBounds(CGMainDisplayID()).size.height - y;
}

// }}}

// Defines a constant for empty ranges in NSTextInputClient
static const NSRange kEmptyRange = { NSNotFound, 0 };

// window->monitor functions {{{

// Make the specified window and its video mode active on its monitor
static bool acquire_monitor(GWindow *window, char **error)
{
    const bool status = gui_set_video_mode_ns(window->monitor,
                                              &window->video_mode,
                                              error);
    const CGRect bounds = CGDisplayBounds(window->monitor->ns.display_id);
    const float trans_y = transform_y(bounds.origin.y + bounds.size.height);
    const NSRect frame = NSMakeRect(bounds.origin.x,
                                    trans_y,
                                    bounds.size.width,
                                    bounds.size.height);

    [window->ns.object setFrame:frame display:YES];

    window->monitor->window = window;
    return status;
}

// Remove the window and restore the original video mode
static void release_monitor(GWindow *window)
{
    if (window->monitor->window != window) {
        return;
    }

    window->monitor->window = NULL;
    gui_restore_video_mode_ns(window->monitor);
}

// }}}

// Delegate for window related notifications {{{
@interface GWindowDelegate : NSObject
{
    GWindow *window;
}

- (id)initWithGWindow:(GWindow *)initWindow;

@end

@implementation GWindowDelegate

- (id)initWithGWindow:(GWindow *)initWindow
{
    self = [super init];
    if (self != nil) {
        window = initWindow;
    }

    return self;
}

// The user has attempted to close a window or the window has received a
// performClose: message.
- (BOOL)windowShouldClose:(id)sender
{
    gui_input_window_close_request(window);
    return NO;
}

- (void)windowDidResize:(NSNotification *)notification
{
    [window->context.nsgl.object update];

    if (window->gui->cursor_window == window &&
        window->cursor_mode == kCursorDisabled) {
        center_cursor(window);
    }

    const NSRect contentRect = [window->ns.view frame];
    const NSRect fbRect = [window->ns.view convertRectToBacking:contentRect];

    gui_input_framebuffer_size(window, fbRect.size.width, fbRect.size.height);
    gui_input_window_size(window, contentRect.size.width, contentRect.size.height);
}

- (void)windowDidMove:(NSNotification *)notification
{
    [window->context.nsgl.object update];

    if (window->gui->cursor_window == window &&
        window->cursor_mode == kCursorDisabled) {
        center_cursor(window);
    }

    int x, y;
    gui_get_window_pos(window, &x, &y);
    gui_input_window_pos(window, x, y);
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
    if (window->monitor) {
        release_monitor(window);
    }

    gui_input_window_iconify(window, true);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    char *error;
    if (window->monitor) {
        acquire_monitor(window, &error);
    }

    gui_input_window_iconify(window, false);
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    if (window->gui->cursor_window == window &&
        window->cursor_mode == kCursorDisabled) {
        center_cursor(window);
    }

    gui_input_window_focus(window, true);
    gui_platform_set_cursor_mode(window, window->cursor_mode);
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    if (window->monitor && window->auto_iconify) {
        gui_iconify_window(window);
    }

    gui_input_window_focus(window, false);
}

@end
// }}}

// Delegate for application related notifications {{{
@interface GApplicationDelegate : NSObject
@end

@implementation GApplicationDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    for (GWindow *window = _global_gui_hack->window_list_head;
         window != NULL;
         window = window->next) {

        gui_input_window_close_request(window);
    }

    return NSTerminateCancel;
}

- (void)applicationDidChangeScreenParameters:(NSNotification *) notification
{
    char *error;
    gui_input_monitor_change(_global_gui_hack, &error);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [NSApp stop:nil];

    gui_post_empty_event();
}

- (void)applicationDidHide:(NSNotification *)notification
{
    for (size_t i = 0; i < _global_gui_hack->monitor_count; i++) {
        gui_restore_video_mode_ns(_global_gui_hack->monitors[i]);
    }
}

@end
// }}}

// Content view class for the GLFW window {{{
@interface GContentView : NSView <NSTextInputClient>
{
    GWindow *window;
    NSTrackingArea *trackingArea;
    NSMutableAttributedString *markedText;
}

- (id)initWithGWindow:(GWindow *)initWindow;

@end

@implementation GContentView

+ (void)initialize
{
    if (self == [GContentView class]) {
        if (_global_gui_hack->ns.cursor == nil) {
            NSImage *data = [[NSImage alloc] initWithSize:NSMakeSize(16, 16)];
            _global_gui_hack->ns.cursor =
                [[NSCursor alloc] initWithImage:data hotSpot:NSZeroPoint];
            [data release];
        }
    }
}

- (id)initWithGWindow:(GWindow *)initWindow
{
    self = [super init];
    if (self != nil) {
        window = initWindow;
        trackingArea = nil;
        markedText = [[NSMutableAttributedString alloc] init];

        [self updateTrackingAreas];
        [self registerForDraggedTypes:[NSArray arrayWithObjects:
                                       NSFilenamesPboardType, nil]];
    }

    return self;
}

- (void)dealloc
{
    [trackingArea release];
    [markedText release];
    [super dealloc];
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)canBecomeKeyView
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)cursorUpdate:(NSEvent *)event
{
    gui_platform_set_cursor_mode(window, window->cursor_mode);
}

- (void)mouseDown:(NSEvent *)event
{
    gui_input_mouse_click(window,
                          kMouseButtonLeft,
                          kPress,
                          translate_modifier_flags([event modifierFlags]));
}

- (void)mouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)mouseUp:(NSEvent *)event
{
    gui_input_mouse_click(window,
                          kMouseButtonLeft,
                          kRelease,
                          translate_modifier_flags([event modifierFlags]));
}

- (void)mouseMoved:(NSEvent *)event
{
    if (window->cursor_mode == kCursorDisabled) {
        gui_input_cursor_motion(window,
                                [event deltaX] - window->ns.warp_delta_x,
                                [event deltaY] - window->ns.warp_delta_y);
    } else {
        const NSRect contentRect = [window->ns.view frame];
        const NSPoint pos = [event locationInWindow];

        gui_input_cursor_motion(window, pos.x, contentRect.size.height - pos.y);
    }

    window->ns.warp_delta_x = 0;
    window->ns.warp_delta_y= 0;
}

- (void)rightMouseDown:(NSEvent *)event
{
    gui_input_mouse_click(window,
                          kMouseButtonRight,
                          kPress,
                          translate_modifier_flags([event modifierFlags]));
}

- (void)rightMouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
    gui_input_mouse_click(window,
                          kMouseButtonRight,
                          kRelease,
                          translate_modifier_flags([event modifierFlags]));
}

- (void)otherMouseDown:(NSEvent *)event
{
    gui_input_mouse_click(window,
                          (int) [event buttonNumber],
                          kPress,
                          translate_modifier_flags([event modifierFlags]));
}

- (void)otherMouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
    gui_input_mouse_click(window,
                          (int) [event buttonNumber],
                          kRelease,
                          translate_modifier_flags([event modifierFlags]));
}

- (void)mouseExited:(NSEvent *)event
{
    gui_input_cursor_enter(window, false);
}

- (void)mouseEntered:(NSEvent *)event
{
    gui_input_cursor_enter(window, true);
}

- (void)viewDidChangeBackingProperties
{
    const NSRect contentRect = [window->ns.view frame];
    const NSRect fbRect = [window->ns.view convertRectToBacking:contentRect];

    gui_input_framebuffer_size(window, fbRect.size.width, fbRect.size.height);
}

- (void)drawRect:(NSRect)rect
{
    gui_input_window_damage(window);
}

- (void)updateTrackingAreas
{
    if (trackingArea != nil) {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }

    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                          NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag |
                                          NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect |
                                          NSTrackingAssumeInside;

    trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                options:options
                                                  owner:self
                                               userInfo:nil];

    [self addTrackingArea:trackingArea];
    [super updateTrackingAreas];
}

- (void)keyDown:(NSEvent *)event
{
    const int key = translate_key(window->gui, [event keyCode]);
    const int mods = translate_modifier_flags([event modifierFlags]);

    gui_input_key(window, key, [event keyCode], kPress, mods);

    [self interpretKeyEvents:[NSArray arrayWithObject:event]];
}

- (void)flagsChanged:(NSEvent *)event
{
    int action;
    const unsigned int modifierFlags =
        [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;
    const int key = translate_key(window->gui, [event keyCode]);
    const int mods = translate_modifier_flags(modifierFlags);
    const NSUInteger keyFlag = translateKeyToModifierFlag(key);

    if (keyFlag & modifierFlags) {
        if (window->keys[key] == kPress) {
            action = kRelease;
        } else {
            action = kPress;
        }
    } else {
        action = kRelease;
    }

    gui_input_key(window, key, [event keyCode], action, mods);
}

- (void)keyUp:(NSEvent *)event
{
    const int key = translate_key(window->gui, [event keyCode]);
    const int mods = translate_modifier_flags([event modifierFlags]);
    gui_input_key(window, key, [event keyCode], kRelease, mods);
}

- (void)scrollWheel:(NSEvent *)event
{
    double deltaX = [event scrollingDeltaX];
    double deltaY = [event scrollingDeltaY];

    if ([event hasPreciseScrollingDeltas]) {
        deltaX *= 0.1;
        deltaY *= 0.1;
    }

    if (fabs(deltaX) > 0.0 || fabs(deltaY) > 0.0) {
        gui_input_scroll(window, deltaX, deltaY);
    }
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask])
        == NSDragOperationGeneric) {

        [self setNeedsDisplay:YES];
        return NSDragOperationGeneric;
    }

    return NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
    [self setNeedsDisplay:YES];
    return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pasteboard = [sender draggingPasteboard];
    NSArray *files = [pasteboard propertyListForType:NSFilenamesPboardType];

    const NSRect contentRect = [window->ns.view frame];
    gui_input_cursor_motion(window,
                            [sender draggingLocation].x,
                            contentRect.size.height - [sender draggingLocation].y);

    const int count = [files count];
    if (count) {
        NSEnumerator *e = [files objectEnumerator];
        char **paths = calloc(count, sizeof(char*));

        for (int i = 0; i < count; i++) {
            paths[i] = strdup([[e nextObject] UTF8String]);
        }

        gui_input_drop(window, count, (const char**) paths);

        for (int i = 0; i < count; i++) {
            free(paths[i]);
        }
        free(paths);
    }

    return YES;
}

- (void)concludeDragOperation:(id <NSDraggingInfo>)sender
{
    [self setNeedsDisplay:YES];
}

- (BOOL)hasMarkedText
{
    return [markedText length] > 0;
}

- (NSRange)markedRange
{
    if ([markedText length] > 0) {
        return NSMakeRange(0, [markedText length] - 1);
    } else {
        return kEmptyRange;
    }
}

- (NSRange)selectedRange
{
    return kEmptyRange;
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange
{
    if ([string isKindOfClass:[NSAttributedString class]]) {
        [markedText initWithAttributedString:string];
    } else {
        [markedText initWithString:string];
    }
}

- (void)unmarkText
{
    [[markedText mutableString] setString:@""];
}

- (NSArray*)validAttributesForMarkedText
{
    return [NSArray array];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange
{
    return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actualRange
{
    int xpos, ypos;
    gui_get_window_pos(window, &xpos, &ypos);
    const NSRect contentRect = [window->ns.view frame];
    return NSMakeRect(xpos, transform_y(ypos + contentRect.size.height), 0.0, 0.0);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    NSString *characters;
    NSEvent *event = [NSApp currentEvent];
    const int mods = translate_modifier_flags([event modifierFlags]);
    const int plain = !(mods & kModSuper);

    if ([string isKindOfClass:[NSAttributedString class]]) {
        characters = [string string];
    } else {
        characters = (NSString*) string;
    }

    NSUInteger length = [characters length];

    for (NSUInteger i = 0; i < length; i++) {
        const unichar codepoint = [characters characterAtIndex:i];
        if ((codepoint & 0xff00) == 0xf700) {
            continue;
        }

        gui_input_char(window, codepoint, mods, plain);
    }
}

- (void)doCommandBySelector:(SEL)selector
{
}

@end
// }}}

// Cocoa specific Window class {{{
@interface GNSWindow : NSWindow {}
@end

@implementation GNSWindow

- (BOOL)canBecomeKeyWindow
{
    // Required for NSBorderlessWindowMask windows
    return YES;
}

@end
// }}}

// GApplication (NSApplication subclass) {{{
@interface GApplication : NSApplication
@end

@implementation GApplication

// From http://cocoadev.com/index.pl?GameKeyboardHandlingAlmost
// This works around an AppKit bug, where key up events while holding
// down the command key don't get sent to the key window.
- (void)sendEvent:(NSEvent *)event
{
    if ([event type] == NSKeyUp && ([event modifierFlags] & NSCommandKeyMask)) {
        [[self keyWindow] sendEvent:event];
    } else {
        [super sendEvent:event];
    }
}

@end
// }}}

// AppKit {{{

// Menu Bar {{{

// Try to figure out what the calling application is called
static NSString *find_app_name(void)
{
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];

    // Keys to search for as potential application names
    NSString *GLFWNameKeys[] =
    {
        @"CFBundleDisplayName",
        @"CFBundleName",
        @"CFBundleExecutable",
    };

    for (size_t i = 0; i < sizeof(GLFWNameKeys) / sizeof(GLFWNameKeys[0]); i++) {
        id name = [infoDictionary objectForKey:GLFWNameKeys[i]];
        if (name &&
            [name isKindOfClass:[NSString class]] &&
            ![name isEqualToString:@""]) {

            return name;
        }
    }

    char **progname = _NSGetProgname();
    if (progname && *progname) {
        return [NSString stringWithUTF8String:*progname];
    }

    // Really shouldn't get here
    return @"Application";
}

// Set up the menu bar (manually)
// This is nasty, nasty stuff -- calls to undocumented semi-private APIs that
// could go away at any moment, lots of stuff that really should be
// localize(d|able), etc.  Loading a nib would save us this horror...
//
static void create_menu_bar(void)
{
    NSString *appName = find_app_name();

    NSMenu *bar = [[NSMenu alloc] init];
    [NSApp setMainMenu:bar];

    NSMenuItem *appMenuItem =
        [bar addItemWithTitle:@"" action:NULL keyEquivalent:@""];
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenuItem setSubmenu:appMenu];

    [appMenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName]
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    NSMenu *servicesMenu = [[NSMenu alloc] init];
    [NSApp setServicesMenu:servicesMenu];
    [[appMenu addItemWithTitle:@"Services"
                       action:NULL
                keyEquivalent:@""] setSubmenu:servicesMenu];
    [servicesMenu release];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName]
                       action:@selector(hide:)
                keyEquivalent:@"h"];
    [[appMenu addItemWithTitle:@"Hide Others"
                       action:@selector(hideOtherApplications:)
                keyEquivalent:@"h"]
        setKeyEquivalentModifierMask:NSAlternateKeyMask | NSCommandKeyMask];
    [appMenu addItemWithTitle:@"Show All"
                       action:@selector(unhideAllApplications:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
                       action:@selector(terminate:)
                keyEquivalent:@"q"];

    NSMenuItem *windowMenuItem =
        [bar addItemWithTitle:@"" action:NULL keyEquivalent:@""];
    [bar release];
    NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    [NSApp setWindowsMenu:windowMenu];
    [windowMenuItem setSubmenu:windowMenu];

    [windowMenu addItemWithTitle:@"Minimize"
                          action:@selector(performMiniaturize:)
                   keyEquivalent:@"m"];
    [windowMenu addItemWithTitle:@"Zoom"
                          action:@selector(performZoom:)
                   keyEquivalent:@""];
    [windowMenu addItem:[NSMenuItem separatorItem]];
    [windowMenu addItemWithTitle:@"Bring All to Front"
                          action:@selector(arrangeInFront:)
                   keyEquivalent:@""];

    // TODO: Make this appear at the bottom of the menu (for consistency)
    [windowMenu addItem:[NSMenuItem separatorItem]];
    [[windowMenu addItemWithTitle:@"Enter Full Screen"
                           action:@selector(toggleFullScreen:)
                    keyEquivalent:@"f"]
     setKeyEquivalentModifierMask:NSControlKeyMask | NSCommandKeyMask];

    // Prior to Snow Leopard, we need to use this oddly-named semi-private API
    // to get the application menu working properly.
    SEL setAppleMenuSelector = NSSelectorFromString(@"setAppleMenu:");
    [NSApp performSelector:setAppleMenuSelector withObject:appMenu];
}

// }}}

// Initialize the Cocoa Application Kit
static bool initialize_app_kit(GlobalGui *gui, char **error)
{
    // Implicitly create shared NSApplication instance
    [GApplication sharedApplication];

    // In case we are unbundled, make us a proper UI application
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Menu bar setup must go between sharedApplication above and
    // finishLaunching below, in order to properly emulate the behavior
    // of NSApplicationMain
    // TODO: create_menu_bar()
    create_menu_bar();

    // There can only be one application delegate, but we allocate it the
    // first time a window is created to keep all window code in this file
    gui->ns.delegate = [[GApplicationDelegate alloc] init];
    if (gui->ns.delegate == nil) {
        fprintf(stderr, "Cocoa: Failed to create application delegate");
        return false;
    }

    [NSApp setDelegate:gui->ns.delegate];
    [NSApp run];

    return true;
}

// Create the Cocoa window
//
static bool create_window(GWindow *window,
                          const GWindowConfig *window_config,
                          char **error)
{
    window->ns.delegate = [[GWindowDelegate alloc] initWithGWindow:window];
    if (window->ns.delegate == nil) {
        *error = "Cocoa: Failed to create window delegate";
        return false;
    }

    NSRect contentRect;

    if (window->monitor) {
        GVideoMode mode;
        int xpos, ypos;

        gui_platform_get_video_mode(window->monitor, &mode);
        gui_platform_get_monitor_pos(window->monitor, &xpos, &ypos);

        contentRect = NSMakeRect(xpos, ypos, mode.width, mode.height);
    } else {
        contentRect = NSMakeRect(0,
                                 0,
                                 window_config->width,
                                 window_config->height);
    }

    window->ns.object = [[GNSWindow alloc]
        initWithContentRect:contentRect
                  styleMask:get_style_mask(window)
                    backing:NSBackingStoreBuffered
                      defer:NO];

    if (window->ns.object == nil) {
        *error = "Cocoa: Failed to create window";
        return false;
    }

    if (window->monitor) {
        [window->ns.object setLevel:NSMainMenuWindowLevel + 1];
    } else {
        [window->ns.object center];

        if (window_config->resizable) {
            [window->ns.object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
        }

        if (window_config->floating) {
            [window->ns.object setLevel:NSFloatingWindowLevel];
        }

        if (window_config->maximized) {
            [window->ns.object zoom:nil];
        }
    }

    window->ns.view = [[GContentView alloc] initWithGWindow:window];

    [window->ns.view setWantsBestResolutionOpenGLSurface:YES];

    [window->ns.object makeFirstResponder:window->ns.view];
    [window->ns.object setTitle:[NSString stringWithUTF8String:window_config->title]];
    [window->ns.object setDelegate:window->ns.delegate];
    [window->ns.object setAcceptsMouseMovedEvents:YES];
    [window->ns.object setContentView:window->ns.view];
    [window->ns.object setRestorable:NO];

    return true;
}

// }}}

// }}}

// Window engine public platform API {{{

// Create key code translation tables
static void create_key_tables(GlobalGui *gui)
{
    memset(gui->ns.public_keys, -1, sizeof(gui->ns.public_keys));
    memset(gui->ns.native_keys, -1, sizeof(gui->ns.native_keys));

    gui->ns.public_keys[0x1D] = GUI_KEY_0;
    gui->ns.public_keys[0x12] = GUI_KEY_1;
    gui->ns.public_keys[0x13] = GUI_KEY_2;
    gui->ns.public_keys[0x14] = GUI_KEY_3;
    gui->ns.public_keys[0x15] = GUI_KEY_4;
    gui->ns.public_keys[0x17] = GUI_KEY_5;
    gui->ns.public_keys[0x16] = GUI_KEY_6;
    gui->ns.public_keys[0x1A] = GUI_KEY_7;
    gui->ns.public_keys[0x1C] = GUI_KEY_8;
    gui->ns.public_keys[0x19] = GUI_KEY_9;
    gui->ns.public_keys[0x00] = GUI_KEY_A;
    gui->ns.public_keys[0x0B] = GUI_KEY_B;
    gui->ns.public_keys[0x08] = GUI_KEY_C;
    gui->ns.public_keys[0x02] = GUI_KEY_D;
    gui->ns.public_keys[0x0E] = GUI_KEY_E;
    gui->ns.public_keys[0x03] = GUI_KEY_F;
    gui->ns.public_keys[0x05] = GUI_KEY_G;
    gui->ns.public_keys[0x04] = GUI_KEY_H;
    gui->ns.public_keys[0x22] = GUI_KEY_I;
    gui->ns.public_keys[0x26] = GUI_KEY_J;
    gui->ns.public_keys[0x28] = GUI_KEY_K;
    gui->ns.public_keys[0x25] = GUI_KEY_L;
    gui->ns.public_keys[0x2E] = GUI_KEY_M;
    gui->ns.public_keys[0x2D] = GUI_KEY_N;
    gui->ns.public_keys[0x1F] = GUI_KEY_O;
    gui->ns.public_keys[0x23] = GUI_KEY_P;
    gui->ns.public_keys[0x0C] = GUI_KEY_Q;
    gui->ns.public_keys[0x0F] = GUI_KEY_R;
    gui->ns.public_keys[0x01] = GUI_KEY_S;
    gui->ns.public_keys[0x11] = GUI_KEY_T;
    gui->ns.public_keys[0x20] = GUI_KEY_U;
    gui->ns.public_keys[0x09] = GUI_KEY_V;
    gui->ns.public_keys[0x0D] = GUI_KEY_W;
    gui->ns.public_keys[0x07] = GUI_KEY_X;
    gui->ns.public_keys[0x10] = GUI_KEY_Y;
    gui->ns.public_keys[0x06] = GUI_KEY_Z;

    gui->ns.public_keys[0x27] = GUI_KEY_APOSTROPHE;
    gui->ns.public_keys[0x2A] = GUI_KEY_BACKSLASH;
    gui->ns.public_keys[0x2B] = GUI_KEY_COMMA;
    gui->ns.public_keys[0x18] = GUI_KEY_EQUAL;
    gui->ns.public_keys[0x32] = GUI_KEY_GRAVE_ACCENT;
    gui->ns.public_keys[0x21] = GUI_KEY_LEFT_BRACKET;
    gui->ns.public_keys[0x1B] = GUI_KEY_MINUS;
    gui->ns.public_keys[0x2F] = GUI_KEY_PERIOD;
    gui->ns.public_keys[0x1E] = GUI_KEY_RIGHT_BRACKET;
    gui->ns.public_keys[0x29] = GUI_KEY_SEMICOLON;
    gui->ns.public_keys[0x2C] = GUI_KEY_SLASH;
    gui->ns.public_keys[0x0A] = GUI_KEY_WORLD_1;

    gui->ns.public_keys[0x33] = GUI_KEY_BACKSPACE;
    gui->ns.public_keys[0x39] = GUI_KEY_CAPS_LOCK;
    gui->ns.public_keys[0x75] = GUI_KEY_DELETE;
    gui->ns.public_keys[0x7D] = GUI_KEY_DOWN;
    gui->ns.public_keys[0x77] = GUI_KEY_END;
    gui->ns.public_keys[0x24] = GUI_KEY_ENTER;
    gui->ns.public_keys[0x35] = GUI_KEY_ESCAPE;
    gui->ns.public_keys[0x7A] = GUI_KEY_F1;
    gui->ns.public_keys[0x78] = GUI_KEY_F2;
    gui->ns.public_keys[0x63] = GUI_KEY_F3;
    gui->ns.public_keys[0x76] = GUI_KEY_F4;
    gui->ns.public_keys[0x60] = GUI_KEY_F5;
    gui->ns.public_keys[0x61] = GUI_KEY_F6;
    gui->ns.public_keys[0x62] = GUI_KEY_F7;
    gui->ns.public_keys[0x64] = GUI_KEY_F8;
    gui->ns.public_keys[0x65] = GUI_KEY_F9;
    gui->ns.public_keys[0x6D] = GUI_KEY_F10;
    gui->ns.public_keys[0x67] = GUI_KEY_F11;
    gui->ns.public_keys[0x6F] = GUI_KEY_F12;
    gui->ns.public_keys[0x69] = GUI_KEY_F13;
    gui->ns.public_keys[0x6B] = GUI_KEY_F14;
    gui->ns.public_keys[0x71] = GUI_KEY_F15;
    gui->ns.public_keys[0x6A] = GUI_KEY_F16;
    gui->ns.public_keys[0x40] = GUI_KEY_F17;
    gui->ns.public_keys[0x4F] = GUI_KEY_F18;
    gui->ns.public_keys[0x50] = GUI_KEY_F19;
    gui->ns.public_keys[0x5A] = GUI_KEY_F20;
    gui->ns.public_keys[0x73] = GUI_KEY_HOME;
    gui->ns.public_keys[0x72] = GUI_KEY_INSERT;
    gui->ns.public_keys[0x7B] = GUI_KEY_LEFT;
    gui->ns.public_keys[0x3A] = GUI_KEY_LEFT_ALT;
    gui->ns.public_keys[0x3B] = GUI_KEY_LEFT_CONTROL;
    gui->ns.public_keys[0x38] = GUI_KEY_LEFT_SHIFT;
    gui->ns.public_keys[0x37] = GUI_KEY_LEFT_SUPER;
    gui->ns.public_keys[0x6E] = GUI_KEY_MENU;
    gui->ns.public_keys[0x47] = GUI_KEY_NUM_LOCK;
    gui->ns.public_keys[0x79] = GUI_KEY_PAGE_DOWN;
    gui->ns.public_keys[0x74] = GUI_KEY_PAGE_UP;
    gui->ns.public_keys[0x7C] = GUI_KEY_RIGHT;
    gui->ns.public_keys[0x3D] = GUI_KEY_RIGHT_ALT;
    gui->ns.public_keys[0x3E] = GUI_KEY_RIGHT_CONTROL;
    gui->ns.public_keys[0x3C] = GUI_KEY_RIGHT_SHIFT;
    gui->ns.public_keys[0x36] = GUI_KEY_RIGHT_SUPER;
    gui->ns.public_keys[0x31] = GUI_KEY_SPACE;
    gui->ns.public_keys[0x30] = GUI_KEY_TAB;
    gui->ns.public_keys[0x7E] = GUI_KEY_UP;

    gui->ns.public_keys[0x52] = GUI_KEY_KP_0;
    gui->ns.public_keys[0x53] = GUI_KEY_KP_1;
    gui->ns.public_keys[0x54] = GUI_KEY_KP_2;
    gui->ns.public_keys[0x55] = GUI_KEY_KP_3;
    gui->ns.public_keys[0x56] = GUI_KEY_KP_4;
    gui->ns.public_keys[0x57] = GUI_KEY_KP_5;
    gui->ns.public_keys[0x58] = GUI_KEY_KP_6;
    gui->ns.public_keys[0x59] = GUI_KEY_KP_7;
    gui->ns.public_keys[0x5B] = GUI_KEY_KP_8;
    gui->ns.public_keys[0x5C] = GUI_KEY_KP_9;
    gui->ns.public_keys[0x45] = GUI_KEY_KP_ADD;
    gui->ns.public_keys[0x41] = GUI_KEY_KP_DECIMAL;
    gui->ns.public_keys[0x4B] = GUI_KEY_KP_DIVIDE;
    gui->ns.public_keys[0x4C] = GUI_KEY_KP_ENTER;
    gui->ns.public_keys[0x51] = GUI_KEY_KP_EQUAL;
    gui->ns.public_keys[0x43] = GUI_KEY_KP_MULTIPLY;
    gui->ns.public_keys[0x4E] = GUI_KEY_KP_SUBTRACT;

    for (int scancode = 0; scancode < 256; scancode++) {
        // Store the reverse translation for faster key name lookup
        if (gui->ns.public_keys[scancode] >= 0) {
            gui->ns.native_keys[gui->ns.public_keys[scancode]] = scancode;
        }
    }
}

bool gui_platform_init(GlobalGui *gui, char **error)
{
    _global_gui_hack = gui;
    gui->ns.auto_release_pool = [[NSAutoreleasePool alloc] init];

    create_key_tables(gui);

    gui->ns.event_source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!gui->ns.event_source) {
        return false;
    }

    CGEventSourceSetLocalEventsSuppressionInterval(gui->ns.event_source, 0.0);

    // TODO: Catch kTISNotifySelectedKeyboardInputSourceChanged and update
    gui->ns.input_source = TISCopyCurrentKeyboardLayoutInputSource();
    if (!gui->ns.input_source) {
        return false;
    }

    gui->ns.unicode_data = TISGetInputSourceProperty(
            gui->ns.input_source,
            kTISPropertyUnicodeKeyLayoutData);
    if (!gui->ns.unicode_data) {
        return false;
    }

    /*
    if (!_glfwInitThreadLocalStoragePOSIX()) {
        return false;
    }
    */

    if (!init_nsgl(gui, error)) {
        return false;
    }

    gui_init_timer_ns(gui);

    //return GUI_TRUE;

    // --

    gui->monitors = gui_platform_get_monitors(&gui->monitor_count, error);
    gui->timer_offset = gui_get_timer_value(gui);

    if (!initialize_app_kit(gui, error)) {
        return false;
    }

    // TODO:
    return true;
}

bool gui_platform_cleanup(GlobalGui *gui, char **error) {
  // TODO:
  return true;
}

// AppKit should be initialized before calling this
int gui_platform_create_window(GWindow *window,
                               const GWindowConfig *win_config,
                               const GContextConfig *ctx_config,
                               const GFramebufferConfig *fb_config,
                               char **error)
{
    if (!create_window(window, win_config, error)) {
        return false;
    }

    if (!create_nsgl_context(window, ctx_config, fb_config, error)) {
      return false;
    }

    if (window->monitor) {
        gui_platform_show_window(window);
        gui_platform_focus_window(window);
        if (!acquire_monitor(window, error)) {
            return false;
        }
    }

    return true;
}

void gui_platform_destroy_window(GWindow *window)
{
    [window->ns.object orderOut:nil];

    if (window->monitor) {
        release_monitor(window);
    }

    destroy_nsgl_context(window);

    [window->ns.object setDelegate:nil];
    [window->ns.delegate release];
    window->ns.delegate = nil;

    [window->ns.view release];
    window->ns.view = nil;

    [window->ns.object close];
    window->ns.object = nil;

    [window->gui->ns.auto_release_pool drain];
    window->gui->ns.auto_release_pool = [[NSAutoreleasePool alloc] init];
}

void gui_set_window_title(GWindow *window, const char *title)
{
    [window->ns.object setTitle:[NSString stringWithUTF8String:title]];
}

void gui_set_window_icon(GWindow *window,
                         int count,
                         const GImage *images)
{
    // Regular windows do not have icons on Mac OS X
}

void gui_get_window_pos(GWindow *window, int *x, int *y)
{
    const NSRect contentRect =
        [window->ns.object contentRectForFrameRect:[window->ns.object frame]];

    if (x) {
        *x = contentRect.origin.x;
    }
    if (y) {
        *y = transform_y(contentRect.origin.y + contentRect.size.height);
    }
}

void gui_set_window_pos(GWindow *window, int x, int y)
{
    const NSRect contentRect = [window->ns.view frame];
    const NSRect dummyRect =
        NSMakeRect(x, transform_y(y + contentRect.size.height), 0, 0);

    const NSRect frameRect =
        [window->ns.object frameRectForContentRect:dummyRect];

    [window->ns.object setFrameOrigin:frameRect.origin];
}

void gui_platform_get_window_size(GWindow *window, int *width, int *height)
{
    const NSRect contentRect = [window->ns.view frame];

    if (width) {
        *width = contentRect.size.width;
    }
    if (height) {
        *height = contentRect.size.height;
    }
}

void gui_set_window_size(GWindow *window, int width, int height)
{
    if (window->monitor) {
        if (window->monitor->window == window) {
            char *error;
            acquire_monitor(window, &error);
        }
    } else {
        [window->ns.object setContentSize:NSMakeSize(width, height)];
    }
}

/*
void gui_set_window_size_limits(GWindow *window,
                                int minwidth,
                                int minheight,
                                int maxwidth,
                                int maxheight)
{
    if (minwidth == -1 || minheight == -1)
        [window->ns.object setContentMinSize:NSMakeSize(0, 0)];
    else
        [window->ns.object setContentMinSize:NSMakeSize(minwidth, minheight)];

    if (maxwidth == -1 || maxheight == -1)
        [window->ns.object setContentMaxSize:NSMakeSize(DBL_MAX, DBL_MAX)];
    else
        [window->ns.object setContentMaxSize:NSMakeSize(maxwidth, maxheight)];
}

void gui_set_window_aspect_ration(GWindow *window, int numer, int denom)
{
    if (numer == -1 || denom == -1) {
        [window->ns.object setContentAspectRatio:NSMakeSize(0, 0)];
    } else {
        [window->ns.object setContentAspectRatio:NSMakeSize(numer, denom)];
    }
}
*/

void gui_get_framebuffer_size(GWindow *window, int *width, int *height)
{
    const NSRect contentRect = [window->ns.view frame];
    const NSRect fbRect = [window->ns.view convertRectToBacking:contentRect];

    if (width) {
        *width = (int) fbRect.size.width;
    }
    if (height) {
        *height = (int) fbRect.size.height;
    }
}

void gui_get_window_framesize(GWindow *window,
                              int *left,
                              int *top,
                              int *right,
                              int *bottom)
{
    const NSRect contentRect = [window->ns.view frame];
    const NSRect frameRect = [window->ns.object frameRectForContentRect:contentRect];

    if (left) {
        *left = contentRect.origin.x - frameRect.origin.x;
    }
    if (top) {
        *top = frameRect.origin.y + frameRect.size.height -
               contentRect.origin.y - contentRect.size.height;
    }
    if (right) {
        *right = frameRect.origin.x + frameRect.size.width -
                 contentRect.origin.x - contentRect.size.width;
    }
    if (bottom) {
        *bottom = contentRect.origin.y - frameRect.origin.y;
    }
}

void gui_iconify_window(GWindow *window)
{
    [window->ns.object miniaturize:nil];
}

void gui_restore_window(GWindow *window)
{
    if ([window->ns.object isMiniaturized]) {
        [window->ns.object deminiaturize:nil];
    } else if ([window->ns.object isZoomed]) {
        [window->ns.object zoom:nil];
    }
}

void gui_maximize_window(GWindow *window)
{
    if (![window->ns.object isZoomed]) {
        [window->ns.object zoom:nil];
    }
}

void gui_platform_show_window(GWindow *window)
{
    [window->ns.object orderFront:nil];
}

void gui_hide_window(GWindow *window)
{
    [window->ns.object orderOut:nil];
}

void gui_platform_focus_window(GWindow *window)
{
    // Make us the active application
    // HACK: This has been moved here from initializeAppKit to prevent
    //       applications using only hidden windows from being activated, but
    //       should probably not be done every time any window is shown
    [NSApp activateIgnoringOtherApps:YES];

    [window->ns.object makeKeyAndOrderFront:nil];
}

void gui_platform_set_window_monitor(GWindow *window,
                                     GMonitor *monitor,
                                     int xpos,
                                     int ypos,
                                     int width,
                                     int height,
                                     int refresh_rate)
{
    if (window->monitor == monitor) {
        if (monitor) {
            if (monitor->window == window) {
                char *error;
                acquire_monitor(window, &error);
            }
        } else {
            const NSRect contentRect =
                NSMakeRect(xpos, transform_y(ypos + height), width, height);
            const NSRect frameRect =
                [window->ns.object frameRectForContentRect:contentRect
                                                 styleMask:get_style_mask(window)];

            [window->ns.object setFrame:frameRect display:YES];
        }

        return;
    }

    if (window->monitor) {
        release_monitor(window);
    }

    gui_input_window_monitor_change(window, monitor);

    const NSUInteger styleMask = get_style_mask(window);
    [window->ns.object setStyleMask:styleMask];
    [window->ns.object makeFirstResponder:window->ns.view];

    NSRect contentRect;

    if (monitor) {
        GVideoMode mode;

        gui_platform_get_video_mode(window->monitor, &mode);
        gui_platform_get_monitor_pos(window->monitor, &xpos, &ypos);

        contentRect = NSMakeRect(xpos,
                                 transform_y(ypos + mode.height),
                                 mode.width,
                                 mode.height);
    } else {
        contentRect = NSMakeRect(xpos,
                                 transform_y(ypos + height),
                                 width,
                                 height);
    }

    NSRect frameRect = [window->ns.object frameRectForContentRect:contentRect
                                                        styleMask:styleMask];
    [window->ns.object setFrame:frameRect display:YES];

    if (monitor) {
        [window->ns.object setLevel:NSMainMenuWindowLevel + 1];
        [window->ns.object setHasShadow:NO];

        char *error;
        acquire_monitor(window, &error);
    } else {
        if (window->numer != -1 &&
            window->denom != -1) {
            [window->ns.object setContentAspectRatio:NSMakeSize(window->numer,
                                                                window->denom)];
        }

        if (window->minwidth != -1 &&
            window->minheight != -1) {
            [window->ns.object setContentMinSize:NSMakeSize(window->minwidth,
                                                            window->minheight)];
        }

        if (window->maxwidth != -1 &&
            window->maxheight != -1) {
            [window->ns.object setContentMaxSize:NSMakeSize(window->maxwidth,
                                                            window->maxheight)];
        }

        if (window->floating) {
            [window->ns.object setLevel:NSFloatingWindowLevel];
        } else {
            [window->ns.object setLevel:NSNormalWindowLevel];
        }

        [window->ns.object setHasShadow:YES];
    }
}

int gui_window_focused(GWindow *window)
{
    return [window->ns.object isKeyWindow];
}

int gui_window_inconified(GWindow *window)
{
    return [window->ns.object isMiniaturized];
}

int gui_window_visible(GWindow *window)
{
    return [window->ns.object isVisible];
}

int gui_window_maximized(GWindow *window)
{
    return [window->ns.object isZoomed];
}

void gui_poll_events(GlobalGui *gui)
{
    for (;;) {
        NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event == nil) {
            break;
        }

        [NSApp sendEvent:event];
    }

    [gui->ns.auto_release_pool drain];
    gui->ns.auto_release_pool = [[NSAutoreleasePool alloc] init];
}

void gui_wait_events(GlobalGui *gui)
{
    // I wanted to pass NO to dequeue:, and rely on PollEvents to
    // dequeue and send.  For reasons not at all clear to me, passing
    // NO to dequeue: causes this method never to return.
    NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    [NSApp sendEvent:event];

    gui_poll_events(gui);
}

void gui_wait_events_timeout(GlobalGui *gui, double timeout)
{
    NSDate *date = [NSDate dateWithTimeIntervalSinceNow:timeout];
    NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                        untilDate:date
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if (event) {
        [NSApp sendEvent:event];
    }

    gui_poll_events(gui);
}

void gui_post_empty_event()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSEvent *event = [NSEvent otherEventWithType:NSApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0];
    [NSApp postEvent:event atStart:YES];
    [pool drain];
}

const char *gui_get_key_name(GlobalGui *gui, int key, int scancode)
{
    if (key != GUI_KEY_UNKNOWN) {
        scancode = gui->ns.native_keys[key];
    }

    // TODO: _glfwIsPrintable
    /* if (!_glfwIsPrintable(gui->ns.public_keys[scancode])) { */
    /*     return NULL; */
    /* } */

    UInt32 dead_key_state = 0;
    UniChar characters[8];
    UniCharCount character_count = 0;

    if (UCKeyTranslate([(NSData*) gui->ns.unicode_data bytes],
                       scancode,
                       kUCKeyActionDisplay,
                       0,
                       LMGetKbdType(),
                       kUCKeyTranslateNoDeadKeysBit,
                       &dead_key_state,
                       sizeof(characters) / sizeof(characters[0]),
                       &character_count,
                       characters) != noErr) {
        return NULL;
    }

    if (!character_count) {
        return NULL;
    }

    CFStringRef string = CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                                            characters,
                                                            character_count,
                                                            kCFAllocatorNull);
    CFStringGetCString(string,
                       gui->ns.key_name,
                       sizeof(gui->ns.key_name),
                       kCFStringEncodingUTF8);
    CFRelease(string);

    return gui->ns.key_name;
}

void gui_platform_get_cursor_pos(GWindow *window, double *xpos, double *ypos)
{
    const NSRect contentRect = [window->ns.view frame];
    const NSPoint pos = [window->ns.object mouseLocationOutsideOfEventStream];

    if (xpos) {
        *xpos = pos.x;
    }
    if (ypos) {
        *ypos = contentRect.size.height - pos.y - 1;
    }
}

void gui_platform_set_cursor_pos(GWindow *window, double x, double y)
{
/*
    TODO: will work when the monitor code lands
    gui_platform_set_cursor_mode(window, window->cursor_mode);

    const NSRect contentRect = [window->ns.view frame];
    const NSPoint pos = [window->ns.object mouseLocationOutsideOfEventStream];

    window->ns.warp_delta_x += x - pos.x;
    window->ns.warp_delta_y += y - contentRect.size.height + pos.y;

    if (window->monitor) {
        CGDisplayMoveCursorToPoint(window->monitor->ns.display_id,
                                   CGPointMake(x, y));
    } else {
        const NSRect localRect = NSMakeRect(x, contentRect.size.height - y - 1, 0, 0);
        const NSRect globalRect = [window->ns.object convertRectToScreen:localRect];
        const NSPoint globalPoint = globalRect.origin;

        CGWarpMouseCursorPosition(CGPointMake(globalPoint.x,
                                              transform_y(globalPoint.y)));
    }
*/
}

void gui_platform_set_cursor_mode(GWindow *window, GCursorMode mode)
{
    if (mode == kCursorNormal) {
        if (window->cursor) {
            [(NSCursor*) window->cursor->ns.object set];
        } else {
            [[NSCursor arrowCursor] set];
        }
    } else {
        [(NSCursor*) window->gui->ns.cursor set];
    }

    CGAssociateMouseAndMouseCursorPosition(mode != kCursorDisabled);
}

// AppKit should be initialized before calling this
bool gui_platform_create_cursor(GCursor *cursor,
                                const GImage *image,
                                int xhot,
                                int yhot)
{
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL
                      pixelsWide:image->width
                      pixelsHigh:image->height
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSCalibratedRGBColorSpace
                    bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                     bytesPerRow:image->width  *4
                    bitsPerPixel:32];

    if (rep == nil) {
        return false;
    }

    memcpy([rep bitmapData], image->pixels, image->width * image->height * 4);

    NSImage *native =
      [[NSImage alloc] initWithSize:NSMakeSize(image->width, image->height)];
    [native addRepresentation: rep];

    cursor->ns.object = [[NSCursor alloc] initWithImage:native
                                                hotSpot:NSMakePoint(xhot, yhot)];

    [native release];
    [rep release];

    if (cursor->ns.object == nil) {
        return false;
    }

    return true;
}

// AppKit should be initialized before calling this
int gui_platform_create_standard_cursor(GCursor *cursor,
                                        GCursorShape shape,
                                        char **error)
{
    cursor->ns.object = get_standard_cursor(shape);
    if (!cursor->ns.object) {
        *error = "Cocoa: Failed to retrieve standard cursor";
        return false;
    }

    [cursor->ns.object retain];
    return true;
}

void gui_platform_destroy_cursor(GCursor *cursor)
{
    if (cursor->ns.object) {
        [(NSCursor*) cursor->ns.object release];
    }
}

void gui_platform_set_cursor(GWindow *window, GCursor *cursor)
{
  const NSPoint pos = [window->ns.object mouseLocationOutsideOfEventStream];

  if (window->cursor_mode == kCursorNormal &&
      [window->ns.view mouse:pos inRect:[window->ns.view frame]]) {
      if (cursor) {
          [(NSCursor*) cursor->ns.object set];
      } else {
          [[NSCursor arrowCursor] set];
      }
  }
}

void gui_set_clipboard_string(GWindow *window, const char *string)
{
    NSArray *types = [NSArray arrayWithObjects:NSStringPboardType, nil];

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard declareTypes:types owner:nil];
    [pasteboard setString:[NSString stringWithUTF8String:string]
                  forType:NSStringPboardType];
}

const char *gui_get_clipboard_string(GWindow *window, char **error)
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

    if (![[pasteboard types] containsObject:NSStringPboardType]) {
        *error = "Cocoa: Failed to retrieve string from pasteboard";
        return NULL;
    }

    NSString *object = [pasteboard stringForType:NSStringPboardType];
    if (!object) {
        *error = "Cocoa: Failed to retrieve object from pasteboard";
        return NULL;
    }

    free(window->gui->ns.clipboard_string);
    window->gui->ns.clipboard_string = strdup([object UTF8String]);

    return window->gui->ns.clipboard_string;
}

// }}}

// Timer public platform API {{{

void gui_init_timer_ns(GlobalGui *gui)
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    gui->ns_time.frequency = (info.denom * 1e9) / info.numer;
}

uint64_t gui_get_timer_value(GlobalGui *gui)
{
    return mach_absolute_time();
}

uint64_t gui_get_timer_frequency(GlobalGui *gui)
{
    return gui->ns_time.frequency;
}
// }}}
