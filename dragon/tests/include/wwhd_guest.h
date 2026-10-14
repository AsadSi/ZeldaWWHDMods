/* Host test double for the services used by persistence.c. */
#pragma once
typedef unsigned int u32;
typedef int s32;
void wwhd_log(const char*);
s32 wwhd_file_read(const char*,void*,u32);
s32 wwhd_file_write(const char*,const void*,u32);
typedef unsigned char u8;
/* story record services (the port's notebook) */
s32 wwhd_quest_state(const char*);
s32 wwhd_quest_step(const char*,u32,const char*,const char*);
s32 wwhd_quest_done(const char*);
s32 wwhd_quest_set_flag(const char*,u32,s32);
s32 wwhd_quest_flag(const char*,u32);
/* game data: the save pointer of the test */
extern u32 test_save_pointer;
#define WWHD_ADDR_dComIfG_save_info_pointer 0
#define WWHD_GAME_DATA(addr,type) (*(type*)&test_save_pointer)
#define WWHD_GAME_ORIGINAL(addr,ret,name,params) ret name params
#define WWHD_REPLACE(addr,ret,name,params) static ret name params

/* HUD service test double; geometry is validated from actual emitted commands. */
enum { WWHD_HUD_TV=0, WWHD_HUD_RECT=0, WWHD_HUD_TEXT=1, WWHD_HUD_TOP_LEFT=1 };
typedef struct {
    u32 kind,anchor,blend;
    float x,y,w,h,size,thickness,rotation,u0,v0,u1,v1;
    u32 rgba,image;
    const char* text;
    u32 text_bytes;
} wwhd_hud_element;
u32 wwhd_hud_register(void (*callback)(u32),u32 screen);
u32 wwhd_hud_emit(u32,const wwhd_hud_element*);
