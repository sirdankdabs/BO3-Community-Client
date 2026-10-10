#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <mach-o/dyld.h>
#include <unistd.h>
static char fixture_map[32]="core_frontend";
static time_t ticks=100;
static bool local_fixture=true;
static intptr_t test_slide(uint32_t i) { (void)i; return (intptr_t)fixture_map-(intptr_t)(0x100000000ULL+0x1599c298); }
static time_t test_time(time_t *out) { if(out)*out=ticks;return ticks; }
#define _dyld_get_image_vmaddr_slide test_slide
#define time test_time
#include "../source/t7_full_unlock.c"
#undef time
#undef _dyld_get_image_vmaddr_slide
static char directory[PATH_MAX];
static int current_mode, queued;
static bool connected;
static char last_commands[1024];
void t7_log(const char *fmt,...) { (void)fmt; }
bool t7_known_mac_build(void) { return true; }
bool t7_memory_readable(const void *p,size_t n) {
    (void)n;
    uintptr_t state = (uintptr_t)fixture_map + 0x115324a8 - 0x1599c298;
    if ((uintptr_t)p == state) return false;
    return p != NULL;
}
bool boiii_session_active(void) { return connected; }
void bo3_data_path(const char *name,char *out,size_t n) { snprintf(out,n,"%s/%s",directory,name); }
void *t7_resolve_function(const char *name) { (void)name;return NULL; }
int DobbyHook(void *a,void *b,void **c) { (void)a;(void)b;(void)c;return -1; }
int DobbyDestroy(void *a) { (void)a;return 0; }
static int table_fixture[6];
static void table_load(const char *name,const void **out) { assert(strstr(name,"ranktable.csv"));*out=table_fixture; }
static const char *table_read(const void *table,int row,int column) { assert(table==table_fixture && column==7 && (row==942 || row==962 || row==56 || row==35));return "1000000"; }
static bool local_running(void) { return local_fixture; }
static int mode(void) { return current_mode; }
static void queue(int c,const char *text) { assert(c==0);++queued;snprintf(last_commands,sizeof(last_commands),"%s",text); }
static bool native_purchase(int a,int b,int c) { return a+b+c==6; }
static unsigned char weapon_fixture[40], rank_rows[36];
static int weapon_saved_xp, upload_calls;
static const void *fixture_weapon(int m,int i) { (void)m;return i==7 ? weapon_fixture : NULL; }
static void *fixture_context(int c,const char *a,const char *b,int x,int m,int y) { (void)c;(void)a;(void)b;(void)x;(void)m;(void)y;return &weapon_saved_xp; }
static int fixture_state(int m,void *s,int i) { (void)m;(void)s;return i==7; }
static int fixture_get(const void *s,const void *c) { (void)s;return *(const int *)c; }
static bool fixture_set(const void *s,void *c,int xp) { (void)s;*(int *)c=xp;return true; }
static void fixture_upload(int c,const char *text) { assert(c==0 && strstr(text,"uploadStats"));++upload_calls; }
static void test_permanent(void) {
    const void *rows=rank_rows;memcpy(weapon_fixture+32,&rows,sizeof(rows));
    int xp=100;memcpy(rank_rows,&xp,4);rank_rows[8]=7;
    xp=200;memcpy(rank_rows+12,&xp,4);rank_rows[20]=7;rank_rows[32]=8;
    int result=0;assert(weapon_max_xp(weapon_fixture,7,&result) && result==200);
    xp=-1;memcpy(rank_rows+12,&xp,4);assert(!weapon_max_xp(weapon_fixture,7,&result));
    xp=200;memcpy(rank_rows+12,&xp,4);
    weapon_definition=fixture_weapon;stats_context=fixture_context;weapon_xp_state=fixture_state;
    ddl_get_int=fixture_get;ddl_set_int=fixture_set;add_text=fixture_upload;permanent_ready=true;
    permanent_weapons(1);assert(weapon_saved_xp==200 && upload_calls==1);
    permanent_weapons(1);assert(upload_calls==1);
    permanent_done[1]=false;permanent_weapons(1);assert(upload_calls==1); // durable completion
    char path[PATH_MAX];bo3_data_path("weapon-xp-mp.beta47.done",path,sizeof(path));unlink(path);
    weapon_saved_xp=500;permanent_weapons(0);assert(weapon_saved_xp==500 && upload_calls==1);
    bo3_data_path("weapon-xp-zm.beta47.done",path,sizeof(path));unlink(path);
    permanent_ready=false;
}
int main(void) {
    strcpy(directory,"/tmp/bo3-full-unlock-test.XXXXXX");assert(mkdtemp(directory));
    assert(menu_wait_reason(0,false,0,false)==0); // no frontend server yet
    assert(menu_wait_reason(1,false,7,false)==0);
    assert(menu_wait_reason(0,false,8,false)==3); // network connected
    assert(menu_wait_reason(0,false,11,true)==0); // actual Mac menu: empty map, active UI
    assert(menu_wait_reason(0,false,11,false)==3); // active gameplay, no UI level
    assert(menu_wait_reason(0,true,0,false)==2); // community session
    assert(menu_wait_reason(2,false,0,false)==1); // campaign
    assert(menu_wait_reason(0,false,-1,false)==3); // unreadable state
    assert(progress_commands(2)==NULL);
    assert(strstr(progress_commands(1),"paragon_rank 941\n"));
    assert(strstr(progress_commands(0),"paragon_rank 961\n"));
    assert(strstr(progress_commands(1),"uploadStats 1\n"));
    assert(strstr(progress_commands(0),"uploadStats 0\n"));
    orig_purchase=native_purchase;orig_locked=native_purchase;
    assert(item_locked(1,2,3));
    assert(purchase(1,2,3));assert(!purchase(1,0,0));
    atomic_store(&full_active,true);
    assert(purchase(1,0,0));assert(purchase(0,0,0));
    assert(!item_locked(1,2,3));assert(item_locked(2,1,3));
    assert(!purchase(2,0,0)); // campaign uses original
    assert(!attachment(1,0,0,0));assert(!option(0,0,0,0));
    assert(!option(2,0,0,0)); // Lua evaluates cosmetic locks across all three modes
    assert(challenge(1,0,0,0));assert(classes(0,0)==10);
    assert(loot(0,0,1)==255 && loot(0,1,1)==1);
    progress_ready=true;session_mode=mode;add_text=queue;ui_level_running=local_running;load_table=table_load;table_value=table_read;
    current_mode=2;t7_full_unlock_frame();assert(queued==0);
    current_mode=1;connected=true;t7_full_unlock_frame();assert(queued==0);
    connected=false;strcpy(fixture_map,"mp_hunted");t7_full_unlock_frame();assert(queued==0);
    strcpy(fixture_map,"core_frontend");t7_full_unlock_frame();
    ticks+=9;t7_full_unlock_frame();assert(queued==0);
    ticks++;t7_full_unlock_frame();assert(queued==1);
    assert(strstr(last_commands,"PrestigeStatsMaster 1"));
    assert(strstr(last_commands,"rankxp 1055600"));
    assert(strstr(last_commands,"paragon_rankxp 1000000\n"));
    assert(strstr(last_commands,"rankxp 1055600") < strstr(last_commands,"PrestigeStatsMaster 1"));
    t7_full_unlock_frame();assert(queued==1);
    attempted[1]=false;t7_full_unlock_frame();assert(queued==1); // durable queued marker
    current_mode=0;t7_full_unlock_frame();ticks+=10;t7_full_unlock_frame();assert(queued==2);
    assert(strstr(last_commands,"PrestigeStatsMaster 0"));
    assert(strstr(last_commands,"rankxp 1054244"));
    char path[PATH_MAX];bo3_data_path("unlock-mp.queued",path,sizeof(path));unlink(path);
    bo3_data_path("unlock-zm.queued",path,sizeof(path));unlink(path);
    test_permanent();rmdir(directory);
    puts("Full unlock guards, mode separation, commands and duplicate prevention passed.");
}
