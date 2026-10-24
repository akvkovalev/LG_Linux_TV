#include <linux/init.h>
#include <linux/module.h>

extern int GDMA_ConfigOSDEnable(bool flag);
extern int GDMA_ConfigOSDxEnable(int plane, bool flag);
extern int GDMA_OSD1OnTop(bool on_top);

#define GDMA_PLANE_OSD1 1

static int __init codex_gdma_show_init(void)
{
    int mixer;
    int plane;
    int top;

    mixer = GDMA_ConfigOSDEnable(true);
    plane = GDMA_ConfigOSDxEnable(GDMA_PLANE_OSD1, true);
    top = GDMA_OSD1OnTop(true);
    pr_emerg("CODEX GDMA show: mixer=%d plane=%d top=%d\n",
             mixer, plane, top);
    return mixer || plane || top ? -EIO : 0;
}

static void __exit codex_gdma_show_exit(void)
{
}

module_init(codex_gdma_show_init);
module_exit(codex_gdma_show_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Enable the Realtek OSD1 mixer plane after late GDMA init");
