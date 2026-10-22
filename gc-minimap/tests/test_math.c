#include <assert.h>
#include <stddef.h>
#include "../src/minimap_math.h"
#include "../src/dungeon_math.h"
static int near(float a,float b) { return a-b<.00001f && b-a<.00001f; }
int main(void) {
    _Static_assert(sizeof(dungeon_actor)==32,"TRES entry size");
    _Static_assert(sizeof(dungeon_door)==36,"DOOR entry size");
    _Static_assert(offsetof(dungeon_actor,x)==12,"world position offset");
    _Static_assert(offsetof(dungeon_actor,rooms)==24,"chest/door room encoding offset");
    assert(dungeon_chest_number((31u<<7)|6u)==31);
    assert(dungeon_chest_on_room(4u<<7,7,7));
    assert(!dungeon_chest_on_room(4u<<7,7,8));
    assert(!dungeon_chest_on_room(15u<<7,7,7));
    assert(!dungeon_chest_on_room(16u<<7,7,7));
    assert(dungeon_door_on_room(3u|(42u<<6),3));
    assert(dungeon_door_on_room(3u|(42u<<6),42));
    assert(!dungeon_door_on_room(3u|(42u<<6),4));
    /* Every sector centre and both half-open outer edges. */
    for(int z=-3;z<=3;++z) for(int x=-3;x<=3;++x)
        assert(minimap_sea_room(x*100000.0f,z*100000.0f)==(z+3)*7+x+4);
    assert(minimap_sea_room(-350000,-350000)==1);
    assert(minimap_sea_room(349999,349999)==49);
    assert(minimap_sea_room(350000,0)==0);
    assert(minimap_sea_room(0,-350001)==0);
    assert(minimap_sea_room(-50001,0)==24);
    assert(minimap_sea_room(-50000,0)==25);
    assert(minimap_sea_room(49999,0)==25);
    assert(minimap_sea_room(50000,0)==26);
    union { unsigned int bits;float f; } invalid={0x7fc00000};
    assert(minimap_sea_room(invalid.f,0)==0);
    invalid.bits=0x7f800000;assert(minimap_sea_room(0,invalid.f)==0);
    float bounds[]={-100,-200,100,200};
    minimap_point p=minimap_project(0,0,bounds);
    assert(p.valid && !p.clipped && near(p.x,.5f) && near(p.z,.5f));
    p=minimap_project(-100,-200,bounds);
    assert(p.valid && p.clipped && near(p.x,.025f) && near(p.z,.025f));
    p=minimap_project(1000,1000,bounds);
    assert(p.valid && p.clipped && near(p.x,.975f) && near(p.z,.975f));
    bounds[2]=bounds[0];assert(!minimap_project(0,0,bounds).valid);
    bounds[2]=100;bounds[3]=invalid.f;assert(!minimap_project(0,0,bounds).valid);
    /* -100, -200, 100, 200 in network-order IEEE754. */
    const unsigned char bytes[]={0xc2,0xc8,0,0,0xc3,0x48,0,0,0x42,0xc8,0,0,0x43,0x48,0,0};
    assert(minimap_decode_bounds(bytes,16,bounds));
    assert(bounds[0]==-100 && bounds[1]==-200 && bounds[2]==100 && bounds[3]==200);
    assert(!minimap_decode_bounds(bytes,15,bounds));
    assert(!minimap_decode_bounds(bytes,17,bounds));
    unsigned char bad[16]={0};assert(!minimap_decode_bounds(bad,16,bounds));
    bad[0]=0x7f;bad[1]=0x80;assert(!minimap_decode_bounds(bad,16,bounds));
    /* Snapshot freshness: shown only while Link's Execute ran in the current full step. */
    minimap_freshness f={0,0,0,0};
    assert(!minimap_snapshot_current(&f,10));            /* no capture yet */
    for(unsigned long long step=11;step<14;++step){      /* 30 fps: one pass per step */
        minimap_note_capture(&f);assert(minimap_snapshot_current(&f,step));
    }
    assert(!minimap_snapshot_current(&f,14));            /* paused: Execute skipped, hide */
    assert(!minimap_snapshot_current(&f,15));
    assert(!minimap_snapshot_current(&f,15));            /* repeated draw of a paused step */
    minimap_note_capture(&f);assert(minimap_snapshot_current(&f,16)); /* unpaused */
    /* True 60 half passes repeat the full step; Link at 30 Hz (sailing) skips them: no flicker. */
    assert(minimap_snapshot_current(&f,16));
    minimap_note_capture(&f);assert(minimap_snapshot_current(&f,17));
    assert(minimap_snapshot_current(&f,17));
    /* Link at 60 Hz captures in both passes. */
    minimap_note_capture(&f);assert(minimap_snapshot_current(&f,18));
    minimap_note_capture(&f);assert(minimap_snapshot_current(&f,18));
    /* Paused after a half-pass capture: the next full step still hides. */
    assert(!minimap_snapshot_current(&f,19));
    assert(!minimap_snapshot_current(&f,19));
    const float dungeon_extents[4]={0,0,100,200};
    minimap_point edge=dungeon_project(102,-4,dungeon_extents);
    assert(edge.valid&&edge.clipped&&near(edge.x,1.02f)&&near(edge.z,-.02f));
    edge=dungeon_project(50,100,dungeon_extents);
    assert(edge.valid&&!edge.clipped&&near(edge.x,.5f)&&near(edge.z,.5f));
    unsigned char ready[24]={'G','C','R','1',0,0,0,1,'T','e','s','t',0,0,0,0,0,0,0,1,0,0,0,2};
    const char stage[9]="Test";
    assert(dungeon_ready(ready,24,stage,0));
    assert(dungeon_ready(ready,24,stage,33));
    assert(!dungeon_ready(ready,24,stage,1));
    assert(!dungeon_ready(ready,23,stage,0));
    assert(!dungeon_ready(ready,24,"Elsewhere",0));
    assert(!dungeon_ready(ready,24,stage,64));
    unsigned char dungeon[48] = {'G','C','R','1',0,0,0,2,0,0,0,127};
    /* Bounds 0,0,32,64 for B1 and 1F, stored as network IEEE floats. */
    dungeon[20]=0x42; dungeon[21]=0; dungeon[24]=0x42; dungeon[25]=0x80;
    dungeon[31]=129;
    dungeon[40]=0x42; dungeon[41]=0; dungeon[44]=0x42; dungeon[45]=0x80;
    assert(dungeon_bounds(dungeon,48,128,bounds)==127);
    assert(near(bounds[2],32)&&near(bounds[3],64));
    assert(dungeon_bounds(dungeon,48,123,bounds)==127);
    assert(dungeon_bounds(dungeon,48,132,bounds)==129);
    assert(!dungeon_bounds(dungeon,47,128,bounds));
    assert(!dungeon_bounds(dungeon,48,122,bounds));
    dungeon[31]=127; assert(!dungeon_bounds(dungeon,48,128,bounds));
    return 0;
}
