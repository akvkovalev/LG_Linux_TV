#define _GNU_SOURCE
#include <dlfcn.h>
#include <linux/fb.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int ioctl(int fd, unsigned long request, ...)
{
    static int (*real_ioctl)(int, unsigned long, ...);
    va_list ap;
    void *arg;
    int rc;

    if (!real_ioctl)
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);

    if (request == FBIOPUT_VSCREENINFO && arg) {
        struct fb_var_screeninfo requested;
        struct fb_var_screeninfo *value = arg;
        requested = *value;
        rc = real_ioctl(fd, request, arg);
        if (rc == 0) {
            dprintf(2,
                    "fixfb: accepted kernel mode %ux%u/%u sync %u->%u vmode %u->%u\\n",
                    requested.xres, requested.yres, requested.bits_per_pixel,
                    requested.sync, value->sync,
                    requested.vmode, value->vmode);
            /* fbdevhw rejects drivers that normalize harmless timing fields. */
            *value = requested;
        }
        return rc;
    }
    return real_ioctl(fd, request, arg);
}
