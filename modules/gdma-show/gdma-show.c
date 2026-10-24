#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>

extern int GDMA_ConfigOSDEnable(bool flag);
extern int GDMA_ConfigOSDxEnable(int plane, bool flag);
extern int GDMA_OSD1OnTop(bool on_top);

#define GDMA_PLANE_OSD1 1

static struct delayed_work keepalive_work;

static int codex_gdma_show_apply(void)
{
    int mixer;
    int plane;
    int top;

    mixer = GDMA_ConfigOSDEnable(true);
    plane = GDMA_ConfigOSDxEnable(GDMA_PLANE_OSD1, true);
    top = GDMA_OSD1OnTop(true);
    if (mixer || plane || top)
        pr_err("CODEX GDMA keepalive failed: mixer=%d plane=%d top=%d\n",
               mixer, plane, top);
    return mixer || plane || top ? -EIO : 0;
}

static void codex_gdma_show_keepalive(struct work_struct *work)
{
    codex_gdma_show_apply();
    schedule_delayed_work(&keepalive_work, 2 * HZ);
}

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
    if (mixer || plane || top)
        return -EIO;

    INIT_DELAYED_WORK(&keepalive_work, codex_gdma_show_keepalive);
    schedule_delayed_work(&keepalive_work, 2 * HZ);
    return 0;
}

static void __exit codex_gdma_show_exit(void)
{
    cancel_delayed_work_sync(&keepalive_work);
}

module_init(codex_gdma_show_init);
module_exit(codex_gdma_show_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Enable and maintain the Realtek OSD1 mixer plane");
