#ifndef _ZCSYS_H_
#define _ZCSYS_H_

#include "zdefs.h"

int zc_initsound(void);
void zc_deinitsound(void);

void draw_lens_under(BITMAP *dest, bool layer);
void draw_lens_over();
void advanceframe(bool allow_gfx);
void update_video_frame(bool allow_gfx);
void eat_buttons();

bool DrunkUp();
bool DrunkDown();
bool DrunkLeft();
bool DrunkRight();

bool DrunkcAbtn();
bool DrunkcBbtn();

bool DrunkrAbtn();
bool DrunkrBbtn();

enum {bosCIRCLE = 0, bosOVAL, bosTRIANGLE, bosSMAS, bosMAX};

extern int black_opening_count;
extern int black_opening_x, black_opening_y;
extern int black_opening_shape;

void zapout();
void zapin();
void wavyout(bool showlink);
void wavyin();
void blackscr(int fcnt, bool showsubscr);
void black_opening(BITMAP *dest, int x, int y, int a, int max_a);
void close_black_opening(int x, int y, bool wait);
void open_black_opening(int x, int y, bool wait);
void openscreen();
int  TriforceCount();
void pauseGame();

bool item_disabled(int item);
bool can_use_item(int item_type, int item);
bool has_item(int item_type, int item);
//int high_item(int jmax, int item_type, bool consecutive, int itemcluster, bool usecluster);
int current_item(int item_type);
int current_item(int item_type, bool checkenabled);
int current_item_power(int item_type);
int current_item_id(int item_type, bool checkmagic = true);
int item_tile_mod(bool);

bool play_zcmusic(const char *filename, int track);
void play_zeldamusic(int track, int midi);
void jukebox(int index);
void play_levelmusic();
void play_dmapmusic();
void music_pause();
void music_resume();
void music_stop();
int  sfx_count();
void sfx_cleanup();
bool sfx_init(int index);
void sfx(int index, int pan, bool loop, bool restart = true);
bool sfx_allocated(int index);
void cont_sfx(int index);
void stop_sfx(int index);
void adjust_sfx(int index, int pan, bool loop);
void pause_sfx(int index);
void resume_sfx(int index);
void pause_all_sfx();
void resume_all_sfx();
void stop_sfx(int index);
void stop_item_sfx(int family);
void kill_sfx();
int  pan(int x);

char *time_str_short(uint32_t time);
char *time_str_med(uint32_t time);

int  vbound(int x, int low, int high);
float vbound(float x, float low, float high);
bool isinRect(int x, int y, int rx1, int ry1, int rx2, int ry2);

int decode_file_007(const char *srcfile, const char *destfile, const char *header, int method);

int  get_bit(uint8_t *bitstr, int bit);
void set_bit(uint8_t *bitstr, int bit, uint8_t val);

int anim_3_4(int clk, int speed);

#endif                                                      // _ZCSYS_H_
