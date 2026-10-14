/* Quest progress lives in the save's story record (the port's story notebook): the quest "valoo" is
 * declared in manifest.json; its step and flags are reported with wwhd_quest_*, so progress follows the
 * save file, its copies, backups and save states. Progress kept by earlier versions in
 * dragon-quest-slotN.txt is moved into the record once, the first time that Quest Log is played. */
#include "quest.h"
#include "wwhd/functions.h"
#include "wwhd/data.h"

dragon_quest_state dragon_quest={.slot=-1};
static char filename[]="dragon-quest-slot0.txt";
static char progress_bytes[128];
static int migrated_slot=-1;
static const char quest_id[]="valoo";
#ifndef DRAGON_SAVE_U8
#define DRAGON_SAVE_U8(save,offset) (*(u8*)((save)+(offset)))
#endif

/* the Quest Log in play (dSv_info_c::mDataNum), -1 without a save */
static int current_slot(void) {
    u32 save=WWHD_GAME_DATA(WWHD_ADDR_dComIfG_save_info_pointer,u32);
    if (!save) return -1;
    unsigned slot=DRAGON_SAVE_U8(save,0x20+0x1290);
    return slot<3?(int)slot:-1;
}
/* report the progress to the record (steps only go forward there) */
void dragon_quest_persist(void) {
    dragon_progress p=dragon_progress_normalize(dragon_quest.progress);
    int step=dragon_record_step(p);
    if (step==DRAGON_STEP_DONE) { wwhd_quest_done(quest_id); return; }
    for (unsigned i=0;i<3;++i) if (p.mask&(1u<<i)) wwhd_quest_set_flag(quest_id,i+1,1);
    if (step>0) wwhd_quest_step(quest_id,(u32)step,0,0);
}
/* once per Quest Log and session: an old progress file moves into the record, then is marked moved */
static void migrate(int slot) {
    if (slot==migrated_slot) return;
    migrated_slot=slot;
    if (wwhd_quest_state(quest_id)!=0) return;
    filename[17]=(char)('0'+slot);
    s32 length=wwhd_file_read(filename,progress_bytes,sizeof progress_bytes);
    dragon_progress p={0,0};
    if (length<=0 || length>=(s32)sizeof progress_bytes || !dragon_progress_decode(progress_bytes,(unsigned)length,&p) || !p.phase) return;
    dragon_quest.progress=p;
    dragon_quest_persist();
    static const char moved[]="WWHD_DRAGON_QUEST moved to the save file\n";
    wwhd_file_write(filename,moved,sizeof moved-1);
    wwhd_log("[dragon] quest progress moved from its old file into the save");
}
/* every tick: the progress as the record has it */
void dragon_quest_sync(void) {
    int slot=current_slot();
    if (slot<0) { dragon_quest.slot=-1; return; }
    migrate(slot);
    s32 step=wwhd_quest_state(quest_id);
    if (step<0) return;
    unsigned flags=0;
    for (unsigned i=0;i<3;++i) if (wwhd_quest_flag(quest_id,i+1)==1) flags|=1u<<i;
    dragon_progress p=dragon_progress_from_record(step,flags);
    if (slot!=dragon_quest.slot) dragon_quest_load(&dragon_quest,slot,p);
    else dragon_quest.progress=p;
}
