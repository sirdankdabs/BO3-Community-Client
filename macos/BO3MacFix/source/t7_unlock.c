// Inventory subset of Scroptss/T7Patch-src SPOOF_UNLOCK.
// Mac v98 uses a 32-bit item quantity. No profile/stat writes or license hook.
#include "t7_runtime.h"
#include "dobby.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>

static atomic_bool active;
static int gum_count = 255;
static int (*original_quantity)(int, int);
static bool (*original_slots)(int);
static bool (*original_valid)(int);
static bool (*original_entitlement)(int, int);

static int quantity(int controller, int item) {
    if (!atomic_load(&active)) return original_quantity(controller, item);
    return item >= 1000000010 && item < 1000000200 ? gum_count : 1;
}
static bool slots(int controller) {
    return atomic_load(&active) ? true : original_slots(controller);
}
static bool valid(int controller) {
    return atomic_load(&active) ? true : original_valid(controller);
}
static bool entitlement(int controller, int incentive) {
    if (!atomic_load(&active)) return original_entitlement(controller, incentive);
    // Invalid/duplicate incentives, matching upstream (SPOOF_SKIP_CWL=false).
    return incentive != 29 && incentive != 30 && incentive != 34;
}

void t7_install_unlock(void) {
    const char *enabled = getenv("BO3MACFIX_SPOOF_UNLOCK");
    if (!enabled || strcmp(enabled, "1")) {
        t7_log("[BO3MacFix] Inventory spoof: disabled.\n");
        return;
    }
    if (!t7_known_mac_build()) {
        t7_log("[BO3MacFix] Inventory spoof: unsupported executable; disabled.\n");
        return;
    }
    const char *count = getenv("BO3MACFIX_SPOOF_GUM_COUNT");
    if (count) {
        char *end; errno = 0;
        long parsed = strtol(count, &end, 10);
        if (errno || end == count || *end || parsed < 1 || parsed > 255) {
            t7_log("[BO3MacFix] Inventory spoof: invalid gum count (use 1..255); disabled.\n");
            return;
        }
        gum_count = (int)parsed;
    }
    struct Entry { const char *name; void *hook; void **original; void *address; } entries[] = {
        {"LiveInventory_GetItemQuantity", (void *)quantity, (void **)&original_quantity, NULL},
        {"LiveInventory_AreExtraSlotsPurchased", (void *)slots, (void **)&original_slots, NULL},
        {"LiveInventory_IsValid", (void *)valid, (void **)&original_valid, NULL},
        {"LiveEntitlements_IsEntitlementActiveForController", (void *)entitlement, (void **)&original_entitlement, NULL},
    };
    const size_t n = sizeof(entries) / sizeof(entries[0]);
    // Resolve/verify every address before patching anything.
    for (size_t i = 0; i < n; ++i) {
        entries[i].address = t7_resolve_function(entries[i].name);
        if (!entries[i].address) {
            t7_log("[BO3MacFix] Inventory spoof: missing %s; disabled.\n", entries[i].name);
            return;
        }
    }
    for (size_t i = 0; i < n; ++i) {
        if (DobbyHook(entries[i].address, entries[i].hook, entries[i].original)) {
            t7_log("[BO3MacFix] Inventory spoof: failed %s; disabled.\n", entries[i].name);
            while (i) {
                --i;
                if (DobbyDestroy(entries[i].address))
                    t7_log("[BO3MacFix] Inventory spoof: rollback failed %s; passthrough retained.\n", entries[i].name);
            }
            return;
        }
    }
    atomic_store(&active, true);
    t7_log("[BO3MacFix] Inventory spoof: 4 hooks active, GobbleGums=%d (profile unchanged).\n", gum_count);
}
