#include <assert.h>
#include <string.h>
#include "wwhd_guest.h"
static unsigned char save_bytes[0x1400];
u32 test_save_pointer;
#define DRAGON_SAVE_U8(save,offset) (save_bytes[offset])
#include "../src/persistence_impl.h"
/* the record of the test: step and flags of "valoo" */
static s32 record_step;
static unsigned record_flags, file_reads, file_writes;
static char file_bytes[3][128];
static unsigned file_size[3];
void wwhd_log(const char* message) { (void)message; }
s32 wwhd_quest_state(const char* id) { assert(!strcmp(id, "valoo")); return record_step; }
s32 wwhd_quest_step(const char* id, u32 step, const char* a, const char* b) {
    (void)a; (void)b; assert(!strcmp(id, "valoo"));
    if ((s32)step > record_step && record_step != 255) record_step = (s32)step;  /* steps only go forward */
    return record_step;
}
s32 wwhd_quest_done(const char* id) { assert(!strcmp(id, "valoo")); record_step = 255; return 255; }
s32 wwhd_quest_set_flag(const char* id, u32 n, s32 on) { assert(!strcmp(id, "valoo") && n >= 1 && n <= 8); if (on) record_flags |= 1u << (n - 1); return 1; }
s32 wwhd_quest_flag(const char* id, u32 n) { assert(!strcmp(id, "valoo")); return (s32)((record_flags >> (n - 1)) & 1); }
s32 wwhd_file_read(const char* name, void* output, u32 size) {
    unsigned slot = (unsigned)(name[17] - '0'); assert(slot < 3); ++file_reads;
    unsigned count = file_size[slot] < size ? file_size[slot] : size;
    memcpy(output, file_bytes[slot], count); return (s32)count;
}
s32 wwhd_file_write(const char* name, const void* input, u32 size) {
    unsigned slot = (unsigned)(name[17] - '0'); assert(slot < 3 && size < 128); ++file_writes;
    memcpy(file_bytes[slot], input, size); file_size[slot] = size; return (s32)size;
}
static void play(unsigned slot) { test_save_pointer = 1; save_bytes[0x20 + 0x1290] = (unsigned char)slot; }
int main(void) {
    /* no save loaded: nothing */
    dragon_quest_sync();
    assert(dragon_quest.slot == -1 && file_reads == 0);
    /* Quest Log 2 (slot 1) of an older version: its progress file moves into the record once */
    strcpy(file_bytes[1], "WWHD_DRAGON_QUEST 1\n2 5\n"); file_size[1] = 24;
    play(1);
    dragon_quest_sync();
    assert(record_step == 2 && record_flags == 5 && dragon_quest.slot == 1 && dragon_quest.progress.phase == 2 && dragon_quest.progress.mask == 5);
    assert(file_writes == 1 && strstr(file_bytes[1], "moved"));
    dragon_quest_sync();  /* once per session and Quest Log */
    assert(file_reads == 1);
    /* the last chime: the record has all three, the quest's step 3 */
    assert(dragon_quest_chime(&dragon_quest, 1));
    dragon_quest_persist();
    assert(record_step == 3 && record_flags == 7);
    dragon_quest_sync();
    assert(dragon_quest.progress.phase == 3);
    /* learned: done */
    assert(dragon_quest_learn(&dragon_quest));
    dragon_quest_persist();
    dragon_quest_sync();
    assert(record_step == 255 && dragon_quest.progress.phase == 4);
    /* a save state or a reload going back: the record decides */
    record_step = 1, record_flags = 0;
    dragon_quest_sync();
    assert(dragon_quest.progress.phase == 1 && dragon_quest.progress.mask == 0);
    /* a moved file is not read as progress again in a later session */
    record_step = 0;
    migrated_slot = -1;
    dragon_quest_sync();
    assert(record_step == 0 && dragon_quest.progress.phase == 0);
    /* another Quest Log without a file: new */
    play(2);
    dragon_quest_sync();
    assert(dragon_quest.slot == 2 && dragon_quest.progress.phase == 0);
    return 0;
}
