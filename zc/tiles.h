#ifndef _ZC_TILES_H_
#define _ZC_TILES_H_

#define UNPACKSIZE 256

#include "zdefs.h"

extern tiledata *newtilebuf;
extern newcombo *combobuf;
extern uint16_t animated_combo_table[MAXCOMBOS][2];             //[0]=position in act2, [1]=original tile
extern uint16_t animated_combo_table4[MAXCOMBOS][2];            //[0]=combo, [1]=clock
extern uint16_t animated_combos;
extern uint16_t animated_combo_table2[MAXCOMBOS][2];             //[0]=position in act2, [1]=original tile
extern uint16_t animated_combo_table24[MAXCOMBOS][2];            //[0]=combo, [1]=clock
extern uint16_t animated_combos2;
extern bool blank_tile_table[NEWMAXTILES];                  //keeps track of blank tiles
extern bool blank_tile_quarters_table[NEWMAXTILES * 4];     //keeps track of blank tile quarters

// in tiles.cc
extern uint8_t unpackbuf[UNPACKSIZE];
extern comboclass   *combo_class_buf;

void register_blank_tiles();
void setup_combo_animations();
void setup_combo_animations2();
void reset_combo_animations();
void reset_combo_animations2();
void animate_combos();
bool isonline(long x1, long y1, long x2, long y2, long x3, long y3);
void reset_tile(tiledata *buf, int t, int format);
void clear_tiles(tiledata *buf);
void overlay_tile(tiledata *buf, int dest, int src, int cs, bool backwards);
bool copy_tile(tiledata *buf, int src, int dest, bool swap);
void unpack_tile(tiledata *buf, int tile, int flip, bool force);

void pack_tile(tiledata *buf, uint8_t *src, int tile);
void pack_tiledata(uint8_t *dest, uint8_t *src, uint8_t format);
int rotate_value(int flip);

void puttile8(BITMAP *dest, int tile, int x, int y, int cset, int flip);
void oldputtile8(BITMAP *dest, int tile, int x, int y, int cset, int flip);
void overtile8(BITMAP *dest, int tile, int x, int y, int cset, int flip);
void puttile16(BITMAP *dest, int tile, int x, int y, int cset, int flip);
void oldputtile16(BITMAP *dest, int tile, int x, int y, int cset, int flip);
void overtile16(BITMAP *dest, int tile, int x, int y, int cset, int flip);

void putblock8(BITMAP *dest, int tile, int x, int y, int csets[], int flip, int mask);
void oldputblock8(BITMAP *dest, int tile, int x, int y, int csets[], int flip, int mask);
void overblock8(BITMAP *dest, int tile, int x, int y, int csets[], int flip, int mask);

int combo_tile(const newcombo &c, int x, int y);
int combo_tile(int cmbdat, int x, int y);

void putcombo(BITMAP *dest, int x, int y, int cmbdat, int cset);
void overcombo(BITMAP *dest, int x, int y, int cmbdat, int cset);
void overcomboblock(BITMAP *dest, int x, int y, int cmbdat, int cset, int w, int h);
void overcombo2(BITMAP *dest, int x, int y, int cmbdat, int cset);

void puttiletranslucent8(BITMAP *dest, int tile, int x, int y, int cset, int flip, int opacity);
void overtiletranslucent8(BITMAP *dest, int tile, int x, int y, int cset, int flip, int opacity);
void puttiletranslucent16(BITMAP *dest, int tile, int x, int y, int cset, int flip, int opacity);
void overtiletranslucent16(BITMAP *dest, int tile, int x, int y, int cset, int flip, int opacity);
void overtilecloaked16(BITMAP *dest, int tile, int x, int y, int flip);

void overblocktranslucent8(BITMAP *dest, int tile, int x, int y, int csets[], int flip, int mask, int opacity);

void overcombotranslucent(BITMAP *dest, int x, int y, int cmbdat, int cset, int opacity);
void overcomboblocktranslucent(BITMAP *dest, int x, int y, int cmbdat, int cset, int w, int h, int opacity);

int tilesize(uint8_t format);
int comboa_lmasktotal(uint8_t layermask);
#endif                                                      // _ZC_TILES_H_
