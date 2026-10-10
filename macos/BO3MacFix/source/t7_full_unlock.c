// Mac v98 equivalents of BOIII/Ezz loot.cpp unlock queries.
#include "t7_runtime.h"
#include "dobby.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <mach-o/dyld.h>
#include <limits.h>
#include <errno.h>
static atomic_bool full_active;
static bool (*orig_locked)(int,int,int);
static bool (*orig_purchase)(int,int,int);
static bool (*orig_attachment)(int,int,int,int);
static bool (*orig_attachment_slot)(int,int,int,int);
static bool (*orig_option)(int,int,int,int);
static bool (*orig_challenge)(int,int,unsigned short,int);
static bool (*orig_character)(int,int,unsigned,int,int);
static int (*orig_classes)(int,int);
static int (*orig_loot)(int,int,int);
// Limit query overrides to MP/ZM, preserving campaign and other modes.
static bool applies(int mode) { return atomic_load(&full_active) && (mode == 0 || mode == 1); }
static bool item_locked(int m,int c,int i) { return applies(m) ? false : orig_locked(m,c,i); }
static bool purchase(int m,int c,int i) { return applies(m) ? true : orig_purchase(m,c,i); }
static bool attachment(int m,int c,int i,int a) { return applies(m) ? false : orig_attachment(m,c,i,a); }
static bool attachment_slot(int m,int c,int i,int a) { return applies(m) ? false : orig_attachment_slot(m,c,i,a); }
// Mac Lua IsItemOptionLocked tests all three modes before returning a lock.
// Override these cross-mode cosmetic queries too, as BOIII does.
static bool option(int m,int c,int i,int a) { return atomic_load(&full_active) && m >= 0 && m <= 2 ? false : orig_option(m,c,i,a); }
static bool challenge(int m,int c,unsigned short i,int a) { return applies(m) ? true : orig_challenge(m,c,i,a); }
static bool character(int m,int c,unsigned i,int t,int a) { return applies(m) ? false : orig_character(m,c,i,t,a); }
static int classes(int m,int c) { return applies(m) ? 10 : orig_classes(m,c); }
static int loot(int c,int m,int i) { return applies(m) ? (m == 0 ? 255 : 1) : orig_loot(c,m,i); }
static void (*add_text)(int,const char *);
static int (*session_mode)(void);
static void (*load_table)(const char *, const void **);
static const char *(*table_value)(const void *,int,int);
static bool progress_ready;
static bool attempted[2];
static int last_mode = -1;
static int last_wait_reason = -1;
static bool table_warning[2];
static bool (*ui_level_running)(void);
static time_t menu_since;
static const char *progress_commands(int mode) {
    if (mode == 1) return "statsetbyname rankxp %ld\nstatsetbyname plevel 11\nstatsetbyname hasprestiged 1\nstatsetbyname rank 54\nPrestigeStatsMaster 1\nstatsetbyname paragon_rank 941\nstatsetbyname paragon_rankxp %ld\nuploadStats 1\n";
    if (mode == 0) return "statsetbyname rankxp %ld\nstatsetbyname plevel 11\nstatsetbyname hasprestiged 1\nstatsetbyname rank 34\nPrestigeStatsMaster 0\nstatsetbyname paragon_rank 961\nstatsetbyname paragon_rankxp %ld\nuploadStats 0\n";
    return NULL;
}
static bool rank_table_xp(const char *name,int row,long increment,long *out) {
    const void *table = NULL;
    load_table(name,&table);
    if (!table || !t7_memory_readable(table,24)) return false;
    const char *value=table_value(table,row,7);
    if (!value || !t7_memory_readable(value,32) || strnlen(value,32)>=32) return false;
    char *end; errno=0;
    long xp=strtol(value,&end,10);
    if (errno || end==value || *end || xp<0 || xp>INT_MAX-increment) return false;
    *out=xp+increment;
    return true;
}
// Native v98 DDL paths and context: write actual weapon XP, never query spoofing.
static const void *(*weapon_definition)(int,int);
static void *(*stats_context)(int,const char *,const char *,int,int,int);
static int (*weapon_xp_state)(int,void *,int);
static int (*ddl_get_int)(const void *,const void *);
static bool (*ddl_set_int)(const void *,void *,int);
static bool permanent_ready, permanent_done[2];
static bool weapon_max_xp(const void *definition,int item,int *out) {
    if (!definition || !t7_memory_readable(definition,40)) return false;
    const unsigned char *rows; memcpy(&rows,(const char *)definition+32,sizeof(rows));
    int maximum=0; bool terminated=false;
    for (int row=0;rows && row<64;++row,rows+=12) {
        if (!t7_memory_readable(rows,12)) return false;
        if (rows[8]!=item) { terminated=true;break; }
        int xp;memcpy(&xp,rows,sizeof(xp));
        if (xp<0 || xp>10000000 || xp<maximum) return false;
        maximum=xp;
    }
    if (!terminated || maximum<=0) return false;
    *out=maximum;return true;
}
static void permanent_weapons(int mode) {
    if (!permanent_ready || permanent_done[mode]) return;
    char path[PATH_MAX];bo3_data_path(mode ? "weapon-xp-mp.beta47.done" : "weapon-xp-zm.beta47.done",path,sizeof(path));
    struct stat info;
    if (!stat(path,&info)) { permanent_done[mode]=true;return; }
    void *context=stats_context(0,"","",0,mode,0);
    if (!context) return;
    int changed=0,verified=0,failed=0;
    for (int item=0;item<256;++item) {
        int target;
        if (!weapon_max_xp(weapon_definition(mode,item),item,&target)) continue;
        // Exact native DDL state layout, initialized by LiveStats_MoveToWeaponXP.
        uint64_t state[4]={0};
        if (!weapon_xp_state(mode,state,item)) { ++failed;continue; }
        int previous=ddl_get_int(state,context);
        if (previous<0) { ++failed;continue; }
        if (previous<target && !ddl_set_int(state,context,target)) { ++failed;continue; }
        if (ddl_get_int(state,context)<target) { ++failed;continue; }
        ++verified;if (previous<target) ++changed;
    }
    permanent_done[mode]=true;
    if (changed) add_text(0,mode ? "uploadStats 1\n" : "uploadStats 0\n");
    if (verified && !failed) {
        FILE *file=fopen(path,"wx");
        if (file) { fprintf(file,"beta47: %d weapon XP entries verified in memory; verify save after restart without unlock hooks\n",verified);fflush(file);fsync(fileno(file));fclose(file); }
    }
    t7_log("[BO3MacFix] Permanent weapon XP %s: changed=%d verified=%d failed=%d; disk persistence requires restart verification.\n",mode ? "MP":"ZM",changed,verified,failed);
}
static int menu_wait_reason(int mode,bool community,int connection,bool ui_level) {
    bool disconnected_menu = connection >= 0 && connection < 8 && !ui_level;
    return mode != 0 && mode != 1 ? 1 : community ? 2 :
        !(ui_level || disconnected_menu) ? 3 : 0;
}
void t7_full_unlock_frame(void) {
    if (!progress_ready || !atomic_load(&full_active)) return;
    int mode = session_mode();
    // Mac Lua Engine.IsRunningUILevel calls 0xe71737 directly.
    // That predicate identifies the menu; it is NOT a local gameplay server.
    // Also accept an unloaded/disconnected menu. Community sessions remain excluded.
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    const char *map = (const char *)(0x100000000ULL + slide + 0x1599c298);
    const int32_t *state = (const int32_t *)(0x100000000ULL + slide + 0x115324a8);
    bool map_ok = t7_memory_readable(map,64);
    bool state_ok = t7_memory_readable(state,sizeof(*state));
    int connection = state_ok ? *state : -1;
    bool ui_level = ui_level_running();
    int reason = menu_wait_reason(mode,boiii_session_active(),connection,ui_level);
    if (reason != last_wait_reason) {
        last_wait_reason = reason;
        t7_log("[BO3MacFix] Progress wait: mode=%d map='%.*s' connection=%d ui_level=%d reason=%d (0=menu,1=mode,2=community,3=map).\n",
               mode, map_ok ? (int)strnlen(map,64) : 0, map_ok ? map : "", connection,ui_level,reason);
    }
    if (reason) { last_mode = -1; menu_since = 0; return; }
    if (last_mode != mode) { last_mode = mode; menu_since = time(NULL); return; }
    if (time(NULL) - menu_since < 10) return;
    permanent_weapons(mode);
    if (attempted[mode]) return;
    char path[PATH_MAX];
    bo3_data_path(mode == 1 ? "unlock-mp.queued" : "unlock-zm.queued",path,sizeof(path));
    struct stat info;
    if (!stat(path,&info)) { attempted[mode] = true; return; }
    long base_xp,paragon_xp;
    // BOIII rank UI: maximum base rank plus its final XP interval; level 997
    // uses the start of its paragon interval, with no level-1000 increment.
    if (!rank_table_xp(mode ? "gamedata/tables/mp/mp_ranktable.csv" : "gamedata/tables/zm/zm_ranktable.csv",
                       mode ? 56 : 35,mode ? 55600 : 54244,&base_xp) ||
        !rank_table_xp(mode ? "gamedata/tables/mp/mp_paragonranktable.csv" : "gamedata/tables/zm/zm_paragonranktable.csv",
                       mode ? 942 : 962,0,&paragon_xp)) {
        if (!table_warning[mode]) { table_warning[mode]=true; t7_log("[BO3MacFix] Progress %s: base/paragon XP unavailable or invalid; not applied.\n",mode ? "MP":"ZM"); }
        return;
    }
    char commands[1024];
    snprintf(commands,sizeof(commands),progress_commands(mode),base_xp,paragon_xp);
    // Persist intent BEFORE queuing, so restarts never repeat automatically.
    FILE *file = fopen(path,"wx");
    if (!file) { attempted[mode] = true; t7_log("[BO3MacFix] Progress unlock: cannot record request; skipped.\n"); return; }
    if (fprintf(file,"beta47: commands queued; verify in game and after restart\n") < 0 || fflush(file) || fsync(fileno(file))) {
        fclose(file); unlink(path); attempted[mode] = true; return;
    }
    if (fclose(file)) { unlink(path); attempted[mode] = true; return; }
    attempted[mode] = true;
    add_text(0,commands);
    t7_log("[BO3MacFix] Progress unlock %s: commands queued for master prestige / level 997; save NOT yet verified.\n", mode == 1 ? "MP" : "ZM");
}
void t7_install_full_unlock(void) {
    const char *enabled = getenv("BO3MACFIX_UNLOCK_ALL");
    if (!enabled || strcmp(enabled,"1") || !t7_known_mac_build()) return;
    struct Entry { const char *name; void *hook; void **orig; void *address; } entries[] = {
        {"BG_UnlockablesIsItemLocked",(void *)item_locked,(void **)&orig_locked,NULL},
        {"BG_UnlockablesIsItemPurchased",(void *)purchase,(void **)&orig_purchase,NULL},
        {"BG_UnlockablesIsItemAttachmentLocked",(void *)attachment,(void **)&orig_attachment,NULL},
        {"BG_UnlockablesIsAttachmentSlotLocked",(void *)attachment_slot,(void **)&orig_attachment_slot,NULL},
        {"BG_UnlockablesItemOptionLocked",(void *)option,(void **)&orig_option,NULL},
        {"BG_UnlockedGetChallengeUnlockedForIndex",(void *)challenge,(void **)&orig_challenge,NULL},
        {"BG_UnlockablesCharacterCustomizationItemLocked",(void *)character,(void **)&orig_character,NULL},
        {"BG_UnlockablesGetCustomClassCount",(void *)classes,(void **)&orig_classes,NULL},
        {"Loot_GetItemQuantity",(void *)loot,(void **)&orig_loot,NULL},
    };
    size_t n = sizeof(entries)/sizeof(entries[0]);
    for (size_t i=0;i<n;++i) {
        entries[i].address=t7_resolve_function(entries[i].name);
        if (!entries[i].address) { t7_log("[BO3MacFix] Full unlock: missing %s; disabled.\n",entries[i].name);return; }
    }
    for (size_t i=0;i<n;++i) {
        if (DobbyHook(entries[i].address,entries[i].hook,entries[i].orig)) {
            while (i) { --i; DobbyDestroy(entries[i].address); }
            t7_log("[BO3MacFix] Full unlock: hook installation failed; passthrough retained.\n");return;
        }
    }
    atomic_store(&full_active,true);
    t7_log("[BO3MacFix] Full unlock: 9 hooks active (MP/ZM attachments, options, challenges, purchases, outfits, classes, loot).\n");
    const char *progress=getenv("BO3MACFIX_UNLOCK_PROGRESS");
    if (!progress || strcmp(progress,"1")) return;
    const char *backup=getenv("BO3MACFIX_UNLOCK_BACKUP");
    char ready[PATH_MAX]; struct stat info;
    int length = backup ? snprintf(ready,sizeof(ready),"%s/backup.ready",backup) : -1;
    if (length < 0 || (size_t)length >= sizeof(ready) ||
        stat(ready,&info) || !S_ISREG(info.st_mode)) {
        t7_log("[BO3MacFix] Progress unlock: backup marker missing; disabled.\n");return;
    }
    add_text=t7_resolve_function("Cbuf_AddText");
    session_mode=t7_resolve_function("Com_SessionMode_GetMode");
    load_table=t7_resolve_function("StringTable_LoadAsset");
    table_value=t7_resolve_function("StringTable_GetColumnValueForRow");
    ui_level_running=t7_resolve_function("Com_IsRunningUILevel");
    progress_ready=ui_level_running && load_table && table_value && add_text && session_mode && t7_resolve_function("Cmd_StatSetByName") &&
        t7_resolve_function("Cmd_PrestigeStatsMaster") && t7_resolve_function("Cmd_UploadStats");
    const char *permanent=getenv("BO3MACFIX_PERMANENT_WEAPONS");
    if (permanent && !strcmp(permanent,"1")) {
        weapon_definition=t7_resolve_function("BG_UnlockablesGetItemDefinition");
        stats_context=t7_resolve_function("LiveStats_GetContext");
        weapon_xp_state=t7_resolve_function("LiveStats_MoveToWeaponXP");
        ddl_get_int=t7_resolve_function("DDL_GetInt");
        ddl_set_int=t7_resolve_function("DDL_SetInt");
        permanent_ready=progress_ready && weapon_definition && stats_context && weapon_xp_state && ddl_get_int && ddl_set_int;
        t7_log("[BO3MacFix] Permanent weapon XP: %s.\n",permanent_ready ? "armed" : "unavailable");
    }
    t7_log("[BO3MacFix] Progress unlock: %s (enter each mode menu for 10 seconds).\n",progress_ready ? "armed" : "missing commands; disabled");
}
