#include "zscriptversion.h"
#include "zelda.h"
#include "link.h"

extern LinkClass Link;

int (*ZScriptVersion::Interpreter)(const uint8_t, const uint16_t, const uint8_t) = &run_script;
void (*ZScriptVersion::onScrolling)(int, int, int, int, bool) = &ScrollingScript;
int    ZScriptVersion::CurrentVersion = V_FFSCRIPT; //Set to current version by default

void ZScriptVersion::ScrollingScript(int scrolldir, int cx, int sx, int sy, bool end_frames)
{
   Link.run_scrolling_script(scrolldir, cx, sx, sy, end_frames);
}
