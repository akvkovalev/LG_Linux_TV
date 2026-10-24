#include <linux/module.h>
#include <linux/init.h>
extern int GDMA_init_module(void);
static int __init codex_gdma_trigger_init(void)
{
    pr_emerg("CODEX GDMA trigger: entering GDMA_init_module\n");
    return GDMA_init_module();
}
static void __exit codex_gdma_trigger_exit(void) { }
module_init(codex_gdma_trigger_init);
module_exit(codex_gdma_trigger_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Late trigger for built-in Realtek GDMA diagnostics");
