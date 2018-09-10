#include "stdafx.h"
#include "adv_config.h"

namespace smp::config::advanced
{

advconfig_branch_factory branch_smp(
    "Spider Monkey Panel", g_guid_smp_adv_branch, advconfig_branch::guid_branch_display, 0
);
advconfig_branch_factory branch_gc(
    "GC: restart is required", g_guid_smp_adv_branch_gc, g_guid_smp_adv_branch, 0
);
#ifdef _DEBUG
advconfig_branch_factory branch_zeal(
    "Zeal", g_guid_smp_adv_branch_zeal, g_guid_smp_adv_branch_gc, 4
);
#endif

advconfig_integer_factory g_var_max_heap(
    "Maximum heap size (in bytes)", g_guid_smp_adv_var_max_heap, g_guid_smp_adv_branch_gc, 0,
    1000UL * 1000 * 1000, 128UL * 1000 * 1000, 4000UL * 1000 * 1000
);
advconfig_integer_factory g_var_max_heap_growth(
    "Allowed heap growth before GC trigger (in bytes)", g_guid_smp_adv_var_max_heap_growth, g_guid_smp_adv_branch_gc, 1,
    50UL * 1000 * 1000, 1UL * 1000 * 1000, 128UL * 1000 * 1000
);
advconfig_integer_factory g_var_gc_budget(
    "GC cycle time budget (in ms)", g_guid_smp_adv_var_gc_budget, g_guid_smp_adv_branch_gc, 2,
    30, 1, 100
);
advconfig_integer_factory g_var_gc_delay(
    "Delay before next GC trigger (in ms)", g_guid_smp_adv_var_gc_delay, g_guid_smp_adv_branch_gc, 3,
    50, 1, 100
);
#ifdef _DEBUG
advconfig_checkbox_factory g_var_gc_zeal(
    "Enable", g_guid_smp_adv_var_gc_zeal, g_guid_smp_adv_branch_zeal, 0,
    false
);
advconfig_integer_factory g_var_gc_zeal_level(
    "Level", g_guid_smp_adv_var_gc_zeal_level, g_guid_smp_adv_branch_zeal, 1,
    2, 0, 14
);
advconfig_integer_factory g_var_gc_zeal_freq(
    "Frequency (in ms)", g_guid_smp_adv_var_gc_zeal_freq, g_guid_smp_adv_branch_zeal, 2,
    200, 1, 5000
);
#endif

}