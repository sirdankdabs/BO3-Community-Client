#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../source/t7_unlock.c"
static unsigned resolved, installed, destroyed;
static int fail_hook = -1;
static bool known = true, missing;
static int native_quantity(int c, int i) { return c + i; }
static bool native_slots(int c) { return c == 2; }
static bool native_valid(int c) { return c == 3; }
static bool native_entitlement(int c, int i) { return c == i; }
void t7_log(const char *fmt, ...) { (void)fmt; }
bool t7_known_mac_build(void) { return known; }
void *t7_resolve_function(const char *name) {
    (void)name; ++resolved;
    return missing && resolved == 4 ? NULL : (void *)(uintptr_t)(0x1000 + resolved);
}
int DobbyHook(void *addr, void *hook, void **orig) {
    (void)addr; (void)hook;
    unsigned i = installed++;
    if ((int)i == fail_hook) return -1;
    void *functions[] = {(void *)native_quantity, (void *)native_slots,
        (void *)native_valid, (void *)native_entitlement};
    *orig = functions[i]; return 0;
}
int DobbyDestroy(void *addr) { (void)addr; ++destroyed; return 0; }
int main(int argc, char **argv) {
    assert(argc == 2);
    unsetenv("BO3MACFIX_SPOOF_UNLOCK");
    unsetenv("BO3MACFIX_SPOOF_GUM_COUNT");
    if (strcmp(argv[1], "disabled")) setenv("BO3MACFIX_SPOOF_UNLOCK", "1", 1);
    if (!strcmp(argv[1], "unknown")) known = false;
    if (!strcmp(argv[1], "missing")) missing = true;
    if (!strcmp(argv[1], "invalid")) setenv("BO3MACFIX_SPOOF_GUM_COUNT", "256", 1);
    if (!strcmp(argv[1], "garbage")) setenv("BO3MACFIX_SPOOF_GUM_COUNT", "42x", 1);
    if (!strcmp(argv[1], "custom")) setenv("BO3MACFIX_SPOOF_GUM_COUNT", "17", 1);
    if (!strcmp(argv[1], "rollback")) fail_hook = 3;
    t7_install_unlock();
    bool success = !strcmp(argv[1], "active") || !strcmp(argv[1], "custom");
    assert(atomic_load(&active) == success);
    if (success) {
        assert(resolved == 4 && installed == 4 && destroyed == 0);
        assert(quantity(0,1000000009) == 1);
        assert(quantity(0,1000000010) == (!strcmp(argv[1],"custom") ? 17 : 255));
        assert(quantity(0,1000000199) == gum_count);
        assert(quantity(0,1000000200) == 1);
        assert(slots(0) && valid(0));
        assert(!entitlement(0,29) && !entitlement(0,30) && !entitlement(0,34));
        assert(entitlement(0,21) && entitlement(0,22) && entitlement(0,23));
        atomic_store(&active,false);
        assert(quantity(2,7)==9 && slots(2) && !slots(0));
        assert(valid(3) && !valid(0));
        assert(entitlement(29,29) && !entitlement(1,29));
    } else if (!strcmp(argv[1],"rollback")) {
        assert(installed == 4 && destroyed == 3);
        assert(quantity(2,7)==9 && slots(2) && valid(3));
    } else if (!strcmp(argv[1],"missing")) assert(resolved == 4 && installed == 0);
    else assert(resolved == 0 && installed == 0);
    puts("unlock test: ok");
}
