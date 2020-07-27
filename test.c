#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include "constants.h"

void draw(cairo_t *cr, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
{
    cairo_set_source_rgba(cr, ((double)r) / 255, ((double)g) / 255, ((double)b) / 255, 1.0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
}

int main()
{
    Display *d = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(d);

    XRRScreenResources *xrrsr = XRRGetScreenResources(d, root);
    RROutput primary = XRRGetOutputPrimary(d, root);
    XRROutputInfo *xrro = XRRGetOutputInfo(d, xrrsr, primary);
    if (xrro->connection != RR_Connected)
    {
        XRRFreeOutputInfo(xrro);
        XCloseDisplay(d);
        return EXIT_FAILURE;
    }

    XRRCrtcInfo *xrrci = XRRGetCrtcInfo(d, xrrsr, xrro->crtc);
    int screen_width = xrrci->width, screen_height = xrrci->height, x = xrrci->x, y = xrrci->y;
    XRRFreeCrtcInfo(xrrci);
    XRRFreeOutputInfo(xrro);
    XRRFreeScreenResources(xrrsr);

    int window_width = screen_width;
    int section_count = H_LEDS;
    int section_width = window_width / section_count;
    int window_height = section_width;

    // these two lines are really all you need
    XSetWindowAttributes attrs;
    attrs.override_redirect = true;

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(d, DefaultScreen(d), 32, TrueColor, &vinfo))
    {
        fprintf(stderr, "No visual found supporting 32 bit color, terminating\n");
        return EXIT_FAILURE;
    }
    // these next three lines add 32 bit depth, remove if you dont need and change the flags below
    attrs.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    Window overlay = XCreateWindow(
        d, root,
        x, y + screen_height - window_height, window_width, window_height, 0,
        vinfo.depth, InputOutput,
        vinfo.visual,
        CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel, &attrs);

    XMapWindow(d, overlay);

    cairo_surface_t *surf = cairo_xlib_surface_create(d, overlay, vinfo.visual, window_width, window_height);
    cairo_t *cr = cairo_create(surf);

    int section_idx = 0;
    int buf[3];
    int bidx = 0;
    while ((buf[bidx] = getchar()) != EOF)
    {
        // Jump to start line resets
        if (buf[bidx] == '\n')
        {
            section_idx = bidx = 0;
            continue;
        }

        if (bidx++ >= 2)
        {
            draw(cr, section_idx * section_width, 0, section_width, window_height, buf[0], buf[1], buf[2]);
            XFlush(d);

            bidx = 0;
            section_idx = (section_idx + 1) % section_count;
        }
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surf);

    XUnmapWindow(d, overlay);

    XCloseDisplay(d);
    return EXIT_SUCCESS;
}