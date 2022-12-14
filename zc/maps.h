#ifndef _MAPS_H_
#define _MAPS_H_
#include "zdefs.h"

#define DRIEDLAKE ((tmpscr->flags7 & fWHISTLEWATER) && (whistleclk>=88))
#define COMBOPOS(x,y) (((y)&0xF0)+((x)>>4))
int isdungeon(int dmap = -1, int scr = -1);
int MAPCOMBO(int x, int y);
int MAPFFCOMBO(int x, int y);
int MAPCSET(int x, int y);
int MAPFLAG(int x, int y);
int MAPCOMBOFLAG(int x, int y);
int MAPFFCOMBOFLAG(int x, int y);
int COMBOTYPE(int x, int y);
int FFCOMBOTYPE(int x, int y);
int MAPCOMBO2(int layer, int x, int y);
int MAPFLAG2(int layer, int x, int y);
int MAPCOMBOFLAG2(int layer, int x, int y);
int getFFCAt(int x, int y);
void eventlog_mapflags();
void setmapflag(int mi2, int flag);
void setmapflag(int flag = 32); // 32 = mBELOW
void unsetmapflag(int mi2, int flag, bool anyflag = false);
void unsetmapflag(int flag = 32);
bool getmapflag(int flag = 32); // 32 = mBELOW
int WARPCODE(int dmap, int scr, int dw);
void update_combo_cycling();
bool iswater(int combo);
bool iswater_type(int type);
bool isGrassType(int type);
bool isFlowersType(int type);
bool isBushType(int type);
bool isCuttableNextType(int type);
bool isTouchyType(int type);
bool isCuttableType(int type);
bool isCuttableItemType(int type);
bool isstepable(int combo);                                 //can use ladder on it
bool ishookshottable(int bx, int by);
bool hiddenstair(int tmp, bool redraw);                      // tmp = index of tmpscr[]
bool remove_lockblocks(int tmp);                // tmp = index of tmpscr[]
bool remove_bosslockblocks(int tmp);            // tmp = index of tmpscr[]
bool remove_chests(int tmp);                    // tmp = index of tmpscr[]
bool remove_lockedchests(int tmp);              // tmp = index of tmpscr[]
bool remove_bosschests(int tmp);                // tmp = index of tmpscr[]
bool overheadcombos(mapscr *s);
void delete_fireball_shooter(mapscr *s, int i);
void hidden_entrance(int tmp, bool refresh, bool high16only = false, int single = -1);
void update_freeform_combos();
bool findentrance(int x, int y, int flag, bool setflag);
bool hitcombo(int x, int y, int combotype);
int nextscr(int dir);
void bombdoor(int x, int y);
void do_scrolling_layer(BITMAP *bmp, int type, mapscr *layer, int x, int y, bool scrolling, int tempscreen);
void do_layer(BITMAP *bmp, int type, mapscr *layer, int x, int y, int tempscreen, bool scrolling = false,
              bool drawprimitives = false);
void do_primitives(BITMAP *bmp, int type, mapscr *layer, int x, int y);
void draw_screen(mapscr *this_screen, bool showlink = true);
void put_door(BITMAP *dest, int t, int pos, int side, int type, bool redraw, bool even_walls = false);
void over_door(BITMAP *dest, int t, int pos, int side);
void putdoor(BITMAP *dest, int t, int side, int door, bool redraw = true, bool even_walls = false);
void showbombeddoor(BITMAP *dest, int side);
void openshutters();
void loadscr(int tmp, int destdmap, int scr, int ldir, bool overlay);
void putscr(BITMAP *dest, int x, int y, mapscr *screen);
void putscrdoors(BITMAP *dest, int x, int y, mapscr *screen);
bool _walkflag(int x, int y, int cnt);
bool water_walkflag(int x, int y, int cnt);
bool hit_walkflag(int x, int y, int cnt);
void map_bkgsfx(bool on);

/****  View Map  ****/
extern int mapres;
bool displayOnMap(int x, int y);
void ViewMap();
void onViewMap();

#endif
