#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void)
{
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    int fd = open("/dev/fb0", O_RDWR);

    if (fd < 0) {
        perror("open /dev/fb0");
        return 1;
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
        perror("FBIOGET_VSCREENINFO");
        return 1;
    }
    var.xres = var.xres_virtual = 1920;
    var.yres = var.yres_virtual = 1080;
    var.xoffset = var.yoffset = 0;
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_NOW;
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &var) < 0) {
        perror("FBIOPUT_VSCREENINFO");
        return 1;
    }
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("FBIOGET_FSCREENINFO");
        return 1;
    }
    printf("LG framebuffer: %ux%ux%u, pitch=%u\n",
           var.xres, var.yres, var.bits_per_pixel, fix.line_length);
    close(fd);
    return fix.line_length == 3840 ? 0 : 1;
}
