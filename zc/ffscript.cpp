#include <string>
#include <math.h>
#include <cstdio>

#include "zcarray.h"
#include "ffscript.h"
#include "zelda.h"
#include "link.h"
#include "guys.h"
#include "title.h"
#include "zscriptversion.h"

#define zc_max(a,b)  ((a)>(b)?(a):(b))
#define zc_min(a,b)  ((a)<(b)?(a):(b))

using std::string;

extern sprite_list particles;
extern LinkClass Link;
extern char *guy_string[];
extern int skipcont;
extern std::map<int, std::pair<string, string> > ffcmap;

//We gain some speed by not passing as arguments
int32_t sarg1 = 0;
int32_t sarg2 = 0;
refInfo *ri = NULL;
ffscript *curscript = NULL;

//Global script data
refInfo globalScriptData;
uint16_t g_doscript = 1;
bool global_wait = false;

//Item script data
refInfo itemScriptData;

//The stacks
int32_t(*stack)[256] = NULL;
int32_t ffc_stack[32][256];
int32_t global_stack[256];
int32_t item_stack[256];

static bool scriptCanSave = true;

void clear_ffc_stack(const uint8_t i)
{
   memset(ffc_stack[i], 0, 256 * sizeof(int32_t));
}

void clear_global_stack()
{
   memset(global_stack, 0, 256 * sizeof(int32_t));
}

//ScriptHelper
class SH
{

   ///-----------------------------//
   //           Errors             //
   ///-----------------------------//

public:

   enum __Error
   {
      _NoError, //OK!
      _Overflow, //script array too small
      _InvalidPointer, //passed NULL pointer or similar
      _OutOfBounds, //library array out of bounds
      _InvalidSpriteUID //bad npc, ffc, etc.
   };

#define INVALIDARRAY localRAM[0]  //localRAM[0] is never used

   //only if the player is messing with their pointers...
   static ZScriptArray &InvalidError(const int32_t ptr)
   {
      Z_scripterrlog("Invalid pointer (%i) passed to array (don't change the values of your array pointers)", ptr);
      return INVALIDARRAY;
   }

   static void write_stack(const int stackoffset, const int32_t value)
   {
      if (stackoffset == 0)
      {
         Z_scripterrlog("Stack over or underflow, stack pointer = %ld", stackoffset);
         return;
      }

      (*stack)[stackoffset] = value;
   }

   static int32_t read_stack(const int stackoffset)
   {
      if (stackoffset == 0)
      {
         Z_scripterrlog("Stack over or underflow, stack pointer = %ld", stackoffset);
         return -10000;
      }

      return (*stack)[stackoffset];
   }

   static inline int32_t get_arg(int32_t arg, bool v)
   {
      return v ? arg : get_register(arg);
   }
};

///----------------------------//
//           Misc.             //
///----------------------------//

//Miscellaneous Helper
class MiscH : public SH
{

public:

};

uint8_t flagpos;
int ornextflag(bool flag)
{
   int f = (flag ? 1 : 0) << flagpos;
   flagpos++;
   return f;
}

int32_t get_screenflags(mapscr *m, int flagset)
{
   int f = 0;
   flagpos = 0;

   switch (flagset)
   {
      case 0: // Room Type
         f = ornextflag(m->flags6 & 1)  | ornextflag(m->flags6 & 2) | ornextflag(m->flags7 & 8);
         break;

      case 1: // View
         f = ornextflag(m->flags3 & 8)  | ornextflag(m->flags7 & 16) | ornextflag(m->flags3 & 16)
             | ornextflag(m->flags3 & 64) | ornextflag(m->flags7 & 2)  | ornextflag(m->flags7 & 1)
             | ornextflag(m->flags & 4);
         break;

      case 2: // Secrets
         f = ornextflag(m->flags & 1)   | ornextflag(m->flags5 & 16) | ornextflag(m->flags6 & 4)
             | ornextflag(m->flags6 & 32);
         break;

      case 3: // Warp
         f = ornextflag(m->flags5 & 4)  | ornextflag(m->flags5 & 8)  | ornextflag(m->flags & 64)
             | ornextflag(m->flags8 & 64) | ornextflag(m->flags3 & 32);
         break;

      case 4: // Item
         f = ornextflag(m->flags3 & 1)  | ornextflag(m->flags7 & 4);
         break;

      case 5: // Combo
         f = ornextflag((m->flags2 >> 4) & 2) | ornextflag(m->flags3 & 2) | ornextflag(m->flags5 & 2)
             | ornextflag(m->flags6 & 64);
         break;

      case 6: // Save
         f = ornextflag(m->flags4 & 64) | ornextflag(m->flags4 & 128) | ornextflag(m->flags6 & 8)
             | ornextflag(m->flags6 & 16);
         break;

      case 7: // FFC
         f = ornextflag(m->flags6 & 128) | ornextflag(m->flags5 & 128);
         break;

      case 8: // Whistle
         f = ornextflag(m->flags & 16)  | ornextflag(m->flags7 & 64)  | ornextflag(m->flags7 & 128);
         break;

      case 9: // Misc
         f = ornextflag(m->flags & 32)  | ornextflag(m->flags5 & 64)  | m->flags8 << 2;
         break;
   }

   return f * 10000;
}

int32_t get_screeneflags(mapscr *m, int flagset)
{
   int f = 0;
   flagpos = 0;

   switch (flagset)
   {
      case 0:
         f = m->enemyflags & 0x1F;
         break;

      case 1:
         f = ornextflag(m->enemyflags & 32) | ornextflag(m->enemyflags & 64) | ornextflag(m->flags3 & 4)
             | ornextflag(m->enemyflags & 128) | ornextflag((m->flags2 >> 4) & 4);
         break;

      case 2:
         f = ornextflag(m->flags3 & 128)    | ornextflag(m->flags & 2)       | ornextflag((m->flags2 >> 4) & 8)
             | ornextflag(m->flags4 & 16);
         break;
   }

   return f * 10000;
}


///------------------------------------------------//
//           Bounds Checking Functions             //
///------------------------------------------------//

//Bounds Checker
class BC : public SH
{
public:

   static inline int checkMapID(const int32_t ID, const char *const str)
   {
      //return checkBounds(ID, 0, map_count-1, str);
      if (ID < 0 || ID > map_count - 1)
      {
         Z_scripterrlog("Invalid value (%i) passed to '%s'", ID + 1, str);
         return _OutOfBounds;
      }

      return _NoError;
   }

   static inline int checkDMapID(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, MAXDMAPS - 1, str);
   }

   static inline int checkComboPos(const int32_t pos, const char *const str)
   {
      return checkBounds(pos, 0, 175, str);
   }

   static inline int checkTile(const int32_t pos, const char *const str)
   {
      return checkBounds(pos, 0, NEWMAXTILES - 1, str);
   }

   static inline int checkCombo(const int32_t pos, const char *const str)
   {
      return checkBounds(pos, 0, MAXCOMBOS - 1, str);
   }

   static inline int checkMisc(const int32_t a, const char *const str)
   {
      return checkBounds(a, 0, 15, str);
   }

   static inline int checkMessage(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, msg_strings_size - 1, str);
   }

   static inline int checkLayer(const int32_t layer, const char *const str)
   {
      return checkBounds(layer, 0, 6, str);
   }

   static inline int checkFFC(const int32_t ffc, const char *const str)
   {
      return checkBounds(ffc, 0, MAXFFCS - 1, str);
   }

   static inline int checkGuyIndex(const int32_t index, const char *const str)
   {
      return checkBounds(index, 0, guys.Count() - 1, str);
   }

   static inline int checkItemIndex(const int32_t index, const char *const str)
   {
      return checkBounds(index, 0, items.Count() - 1, str);
   }

   static inline int checkEWeaponIndex(const int32_t index, const char *const str)
   {
      return checkBounds(index, 0, Ewpns.Count() - 1, str);
   }

   static inline int checkLWeaponIndex(const int32_t index, const char *const str)
   {
      return checkBounds(index, 0, Lwpns.Count() - 1, str);
   }

   static inline int checkGuyID(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, MAXGUYS - 1, str);
   }

   static inline int checkItemID(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, ITEMCNT - 1, str);
   }

   static inline int checkWeaponID(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, WPNCNT - 1, str);
   }

   static inline int checkWeaponMiscSprite(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, MAXWPNS - 1, str);
   }

   static inline int checkSFXID(const int32_t ID, const char *const str)
   {
      return checkBounds(ID, 0, SFX_COUNT - 1, str);
   }

   static inline int checkBounds(const int32_t n, const int32_t boundlow, const int32_t boundup, const char *const funcvar)
   {
      if (n < boundlow || n > boundup)
      {
         Z_scripterrlog("Invalid value (%i) passed to '%s'", n, funcvar);
         return _OutOfBounds;
      }

      return _NoError;
   }

   static inline int checkUserArrayIndex(const int32_t index, const uint32_t size)
   {
      if (index < 0 || index >= int32_t(size))
      {
         Z_scripterrlog("Invalid index (%ld) to local array of size %ld", index, size);
         return _OutOfBounds;
      }

      return _NoError;
   }
};

///------------------------------------------------//
//           Pointer Handling Functions          //
///------------------------------------------------//
//MUST call AND check load functions before trying to use other functions

//Guy Helper
class GuyH : public SH
{

public:
   static int loadNPC(const int32_t eid, const char *const funcvar)
   {
      tempenemy = (enemy *) guys.getByUID(eid);

      if (tempenemy == NULL)
      {
         Z_scripterrlog("Invalid NPC with UID %ld passed to %s\nNPCs on screen have UIDs ", eid, funcvar);

         for (uint16_t i = 0; i < guys.Count(); i++)
            Z_scripterrlog("%ld ", guys.spr(i)->getUID());

         return _InvalidSpriteUID;
      }

      return _NoError;
   }

   static inline enemy *getNPC()
   {
      return tempenemy;
   }

   // Currently only used in a context where the enemy is known to be valid,
   // so there's no need to print an error
   static int getNPCIndex(const int32_t eid)
   {
      for (uint16_t i = 0; i < guys.Count(); i++)
      {
         if (guys.spr(i)->getUID() == eid)
            return i;
      }

      return -1;
   }

   static int32_t getNPCDMisc(const uint8_t a)
   {
      switch (a)
      {
         case 0:
            return tempenemy->dmisc1;

         case 1:
            return tempenemy->dmisc2;

         case 2:
            return tempenemy->dmisc3;

         case 3:
            return tempenemy->dmisc4;

         case 4:
            return tempenemy->dmisc5;

         case 5:
            return tempenemy->dmisc6;

         case 6:
            return tempenemy->dmisc7;

         case 7:
            return tempenemy->dmisc8;

         case 8:
            return tempenemy->dmisc9;

         case 9:
            return tempenemy->dmisc10;

         case 10:
            return tempenemy->dmisc11;

         case 11:
            return tempenemy->dmisc12;
      }

      return 0;
   }

   static bool hasLink()
   {
      if (tempenemy->family == eeWALLM)
         return ((eWallM *) tempenemy)->haslink;

      if (tempenemy->family == eeWALK)
         return ((eStalfos *) tempenemy)->haslink;

      return false;
   }

   static int32_t getMFlags()
   {
      flagpos = 5;
      // Must be in the same order as in the Enemy Editor pane
      return (tempenemy->flags & 0x1F)
             | ornextflag(tempenemy->flags & (lens_only))
             | ornextflag(tempenemy->flags2 & (guy_flashing))
             | ornextflag(tempenemy->flags2 & (guy_blinking))
             | ornextflag(tempenemy->flags2 & (guy_transparent))
             | ornextflag(tempenemy->flags & (inv_front))
             | ornextflag(tempenemy->flags & (inv_left))
             | ornextflag(tempenemy->flags & (inv_right))
             | ornextflag(tempenemy->flags & (inv_back))
             | ornextflag(tempenemy->flags & (guy_bkshield));
   }

   static inline void clearTemp()
   {
      tempenemy = NULL;
   }

private:

   static enemy *tempenemy;
};

enemy *GuyH::tempenemy = NULL;

//Item Helper
class ItemH : public SH
{

public:
   static int loadItem(const int32_t iid, const char *const funcvar)
   {
      tempitem = (item *) items.getByUID(iid);

      if (tempitem == NULL)
      {
         Z_scripterrlog("Invalid item with UID %ld passed to %s\nItems on screen have UIDs ", iid, funcvar);

         for (uint16_t i = 0; i < items.Count(); i++)
            Z_scripterrlog("%ld ", items.spr(i)->getUID());

         return _InvalidSpriteUID;
      }

      return _NoError;
   }

   static inline item *getItem()
   {
      return tempitem;
   }

   static inline void clearTemp()
   {
      tempitem = NULL;
   }

private:

   static item *tempitem;
};

item *ItemH::tempitem = NULL;

//LWeapon Helper
class LwpnH : public SH
{

public:
   static int loadWeapon(const int32_t wid, const char *const funcvar)
   {
      tempweapon = (weapon *) Lwpns.getByUID(wid);

      if (tempweapon == NULL)
      {
         Z_scripterrlog("Invalid lweapon with UID %ld passed to %s\nLWeapons on screen have UIDs ", wid, funcvar);

         for (uint16_t i = 0; i < Lwpns.Count(); i++)
            Z_scripterrlog("%ld ", Lwpns.spr(i)->getUID());

         return _InvalidSpriteUID;
      }

      return _NoError;
   }

   static inline weapon *getWeapon()
   {
      return tempweapon;
   }

   static inline void clearTemp()
   {
      tempweapon = NULL;
   }

private:

   static weapon *tempweapon;
};

weapon *LwpnH::tempweapon = NULL;

//EWeapon Helper
class EwpnH : public SH
{

public:
   static int loadWeapon(const int32_t wid, const char *const funcvar)
   {
      tempweapon = (weapon *) Ewpns.getByUID(wid);

      if (tempweapon == NULL)
      {
         Z_scripterrlog("Invalid eweapon with UID %ld passed to %s\nEWeapons on screen have UIDs ", wid, funcvar);

         for (uint16_t i = 0; i < Ewpns.Count(); i++)
            Z_scripterrlog("%ld ", Ewpns.spr(i)->getUID());

         return _InvalidSpriteUID;
      }

      return _NoError;
   }

   static inline weapon *getWeapon()
   {
      return tempweapon;
   }

   static inline void clearTemp()
   {
      tempweapon = NULL;
   }

private:

   static weapon *tempweapon;
};

weapon *EwpnH::tempweapon = NULL;

void clearScriptHelperData()
{
   GuyH::clearTemp();
   ItemH::clearTemp();
   LwpnH::clearTemp();
   EwpnH::clearTemp();
}

///---------------------------------------------//
//           Array Helper Functions           //
///---------------------------------------------//

//Array Helper
class ArrayH : public SH
{
public:

   //Returns a reference to the correct array based on pointer passed
   static ZScriptArray &getArray(const int32_t ptr)
   {
      if (ptr <= 0)
         return InvalidError(ptr);

      if (ptr >= MAX_ZCARRAY_SIZE) //Then it's a global
      {
         uint32_t gptr = ptr - MAX_ZCARRAY_SIZE;

         if (gptr > game->globalRAM.size())
            return InvalidError(ptr);

         return game->globalRAM[gptr];
      }
      else
      {
         if (localRAM[ptr].Size() == 0)
            return InvalidError(ptr);

         return localRAM[ptr];
      }
   }

   static size_t getSize(const int32_t ptr)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return size_t(-1);

      return a.Size();
   }

   //Can't you get the std::string and then check its length?
   static int strlen(const int32_t ptr)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return -1;

      uint16_t count;

      for (count = 0; BC::checkUserArrayIndex(count, a.Size()) == _NoError && a[count] != '\0'; count++) ;

      return count;
   }

   //Returns values of a zscript array as an std::string.
   static void getString(const int32_t ptr, string &str, uint16_t num_chars = 256)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
      {
         str.clear();
         return;
      }

      str.clear();

      for (uint16_t i = 0; BC::checkUserArrayIndex(i, a.Size()) == _NoError && a[i] != '\0' && num_chars != 0; i++)
      {
         str += char(a[i] / 10000);
         num_chars--;
      }
   }

   //Like getString but for an array of longs instead of chars. *(arrayPtr is not checked for validity)
   static void getValues(const int32_t ptr, int32_t *arrayPtr, uint16_t num_values)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return;

      for (uint16_t i = 0; BC::checkUserArrayIndex(i, a.Size()) == _NoError && num_values != 0; i++)
      {
         arrayPtr[i] = (a[i] / 10000);
         num_values--;
      }
   }

   //Get element from array
   static inline int32_t getElement(const int32_t ptr, const int32_t offset)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return -10000;

      if (BC::checkUserArrayIndex(offset, a.Size()) == _NoError)
         return a[offset];
      else
         return -10000;
   }

   //Set element in array
   static inline void setElement(const int32_t ptr, const int32_t offset, const int32_t value)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return;

      if (BC::checkUserArrayIndex(offset, a.Size()) == _NoError)
         a[offset] = value;
   }

   //Puts values of a zscript array into a client <type> array. returns 0 on success. Overloaded
   template <typename T>
   static int getArray(const int32_t ptr, T *refArray)
   {
      return getArray(ptr, getArray(ptr).Size(), 0, 0, 0, refArray);
   }

   template <typename T>
   static int getArray(const int32_t ptr, const uint16_t size, T *refArray)
   {
      return getArray(ptr, size, 0, 0, 0, refArray);
   }

   template <typename T>
   static int getArray(const int32_t ptr, const uint16_t size, uint16_t userOffset, const uint16_t userStride,
                       const uint16_t refArrayOffset, T *refArray)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return _InvalidPointer;

      uint16_t j = 0, k = userStride;

      for (uint16_t i = 0; j < size; i++)
      {
         if (i >= a.Size())
            return _Overflow;

         if (userOffset-- > 0)
            continue;

         if (k > 0)
            k--;
         else if (BC::checkUserArrayIndex(i, a.Size()) == _NoError)
         {
            refArray[j + refArrayOffset] = T(a[i]);
            k = userStride;
            j++;
         }
      }

      return _NoError;
   }


   static int setArray(const int32_t ptr, const string s2)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return _InvalidPointer;

      uint16_t i;

      for (i = 0; i < s2.size(); i++)
      {
         if (i >= a.Size())
         {
            a.Back() = '\0';
            return _Overflow;
         }

         if (BC::checkUserArrayIndex(i, a.Size()) == _NoError)
            a[i] = s2[i] * 10000;
      }

      if (BC::checkUserArrayIndex(i, a.Size()) == _NoError)
         a[i] = '\0';

      return _NoError;
   }

   //Puts values of a client <type> array into a zscript array. returns 0 on success. Overloaded
   template <typename T>
   static int setArray(const int32_t ptr, const uint16_t size, T *refArray)
   {
      return setArray(ptr, size, 0, 0, 0, refArray);
   }

   template <typename T>
   static int setArray(const int32_t ptr, const uint16_t size, uint16_t userOffset, const uint16_t userStride,
                       const uint16_t refArrayOffset, T *refArray)
   {
      ZScriptArray &a = getArray(ptr);

      if (a == INVALIDARRAY)
         return _InvalidPointer;

      uint16_t j = 0, k = userStride;

      for (uint16_t i = 0; j < size; i++)
      {
         if (i >= a.Size())
            return _Overflow; //Resize?

         if (userOffset-- > 0)
            continue;

         if (k > 0)
            k--;
         else if (BC::checkUserArrayIndex(i, a.Size()) == _NoError)
         {
            a[i] = int32_t(refArray[j + refArrayOffset]) * 10000;
            k = userStride;
            j++;
         }
      }

      return _NoError;
   }
};

// Called when leaving a screen; deallocate arrays created by FFCs that aren't carried over
void deallocateArray(const int32_t ptrval)
{
   if (ptrval <= 0 || ptrval >= MAX_ZCARRAY_SIZE)
      Z_scripterrlog("Script tried to deallocate memory at invalid address %ld", ptrval);
   else
   {
      arrayOwner[ptrval] = 255;

      if (localRAM[ptrval].Size() == 0)
         Z_scripterrlog("Script tried to deallocate memory that was not allocated at address %ld", ptrval);
      else
         localRAM[ptrval].Clear();
   }
}

item *checkItem(int32_t iid)
{
   item *s = (item *)items.getByUID(iid);

   if (s == NULL)
   {
      Z_eventlog("Script attempted to reference a nonexistent item!");
      Z_eventlog("You were trying to reference an item with UID = %ld; Items on screen are UIDs ", iid);

      for (int i = 0; i < items.Count(); i++)
         Z_eventlog("%ld ", items.spr(i)->getUID());

      return NULL;
   }

   return s;
}

weapon *checkLWpn(int32_t eid, const char *what)
{
   weapon *s = (weapon *)Lwpns.getByUID(eid);

   if (s == NULL)
   {

      Z_eventlog("Script attempted to reference a nonexistent LWeapon!");
      Z_eventlog("You were trying to reference the %s of an LWeapon with UID = %ld; LWeapons on screen are UIDs ", what, eid);

      for (int i = 0; i < Lwpns.Count(); i++)
         Z_eventlog("%ld ", Lwpns.spr(i)->getUID());

      return NULL;
   }

   return s;
}

weapon *checkEWpn(int32_t eid, const char *what)
{
   weapon *s = (weapon *)Ewpns.getByUID(eid);

   if (s == NULL)
   {

      Z_eventlog("Script attempted to reference a nonexistent EWeapon!");
      Z_eventlog("You were trying to reference the %s of an EWeapon with UID = %ld; EWeapons on screen are UIDs ", what, eid);

      for (int i = 0; i < Ewpns.Count(); i++)
         Z_eventlog("%ld ", Ewpns.spr(i)->getUID());

      return NULL;
   }

   return s;
}


int get_screen_d(int32_t index1, int32_t index2)
{
   if (index2 < 0 || index2 > 7)
   {
      Z_scripterrlog("You were trying to reference an out-of-bounds array index for a screen's D[] array (%ld); valid indices are from 0 to 7.",
                     index1);
      return 0;
   }

   return game->screen_d[index1][index2];
}

void set_screen_d(int32_t index1, int32_t index2, int val)
{
   if (index2 < 0 || index2 > 7)
   {
      Z_scripterrlog("You were trying to reference an out-of-bounds array index for a screen's D[] array (%ld); valid indices are from 0 to 7.",
                     index1);
      return;
   }

   game->screen_d[index1][index2] = val;
}

// If scr is currently being used as a layer, return that layer no.
int whichlayer(int32_t scr)
{
   for (int i = 0; i < 6; i++)
   {
      if (scr == (tmpscr->layermap[i] - 1) * MAPSCRS + tmpscr->layerscreen[i])
         return i;
   }

   return -1;
}

sprite *s;



int32_t get_register(const int32_t arg)
{
   int32_t ret = 0;

   switch (arg)
   {
      ///----------------------------------------------------------------------------------------------------//
      //FFC Variables
      case DATA:
         ret = tmpscr->ffdata[ri->ffcref] * 10000;
         break;

      case FFSCRIPT:
         ret = tmpscr->ffscript[ri->ffcref] * 10000;
         break;

      case FCSET:
         ret = tmpscr->ffcset[ri->ffcref] * 10000;
         break;

      case DELAY:
         ret = tmpscr->ffdelay[ri->ffcref] * 10000;
         break;

      case FX:
         ret = tmpscr->ffx[ri->ffcref];
         break;

      case FY:
         ret = tmpscr->ffy[ri->ffcref];
         break;

      case XD:
         ret = tmpscr->ffxdelta[ri->ffcref];
         break;

      case YD:
         ret = tmpscr->ffydelta[ri->ffcref];
         break;

      case XD2:
         ret = tmpscr->ffxdelta2[ri->ffcref];
         break;

      case YD2:
         ret = tmpscr->ffydelta2[ri->ffcref];
         break;

      case FFFLAGSD:
         ret = ((tmpscr->ffflags[ri->ffcref] >> (ri->d[0] / 10000)) & 1) ? 10000 : 0;
         break;

      case FFCWIDTH:
         ret = ((tmpscr->ffwidth[ri->ffcref] & 0x3F) + 1) * 10000;
         break;

      case FFCHEIGHT:
         ret = ((tmpscr->ffheight[ri->ffcref] & 0x3F) + 1) * 10000;
         break;

      case FFTWIDTH:
         ret = ((tmpscr->ffwidth[ri->ffcref] >> 6) + 1) * 10000;
         break;

      case FFTHEIGHT:
         ret = ((tmpscr->ffheight[ri->ffcref] >> 6) + 1) * 10000;
         break;

      case FFLINK:
         ret = (tmpscr->fflink[ri->ffcref]) * 10000;
         break;

      case FFMISCD:
      {
         int a = ri->d[0] / 10000;

         if (BC::checkMisc(a, "ffc->Misc") != SH::_NoError)
            ret = -10000;
         else
            ret = tmpscr->ffmisc[ri->ffcref][a];
      }
      break;

      case FFINITDD:
      {
         int a = ri->d[0] / 10000;

         if (BC::checkBounds(a, 0, 7, "ffc->InitD") != SH::_NoError)
            ret = -10000;
         else
            ret = tmpscr->initd[ri->ffcref][a];
      }
      break;

      ///----------------------------------------------------------------------------------------------------//
      //Link's Variables
      case LINKX:
         ret = int32_t(Link.getX()) * 10000;
         break;

      case LINKY:
         ret = int32_t(Link.getY()) * 10000;
         break;

      case LINKZ:
         ret = int32_t(Link.getZ()) * 10000;
         break;

      case LINKJUMP:
         // -fall/100*10000, but doing it that way screwed up the result
         ret = int32_t(-Link.getFall()) * 100;
         break;

      case LINKDIR:
         ret = (int)(Link.dir) * 10000;
         break;

      case LINKHITDIR:
         ret = (int)(Link.getHitDir()) * 10000;
         break;

      case LINKHP:
         ret = (int)(game->get_life()) * 10000;
         break;

      case LINKMP:
         ret = (int)(game->get_magic()) * 10000;
         break;

      case LINKMAXHP:
         ret = (int)(game->get_maxlife()) * 10000;
         break;

      case LINKMAXMP:
         ret = (int)(game->get_maxmagic()) * 10000;
         break;

      case LINKACTION:
         ret = (int)(Link.getAction()) * 10000;
         break;

      case LINKHELD:
         ret = (int)(Link.getHeldItem()) * 10000;
         break;

      case LINKITEMD:
         ret = game->item[vbound(ri->d[0] / 10000, 0, MAXITEMS - 1)] ? 10000 : 0;
         break;

      case LINKEQUIP:
         ret = ((Awpn & 0xFF) | ((Bwpn & 0xFF) << 8)) * 10000;
         break;

      case LINKINVIS:
         ret = (int)(Link.getDontDraw()) * 10000;
         break;

      case LINKINVINC:
         ret = (int)(Link.scriptcoldet) * 10000;
         break;

      case LINKLADDERX:
         ret = (int)(Link.getLadderX()) * 10000;
         break;

      case LINKLADDERY:
         ret = (int)(Link.getLadderY()) * 10000;
         break;

      case LINKSWORDJINX:
         ret = (int)(Link.getSwordClk()) * 10000;
         break;

      case LINKITEMJINX:
         ret = (int)(Link.getItemClk()) * 10000;
         break;

      case LINKDRUNK:
         ret = (int)(Link.DrunkClock()) * 10000;
         break;

      case LINKMISCD:
         ret = (int)(Link.miscellaneous[vbound(ri->d[0] / 10000, 0, 15)]);
         break;

      case LINKHXOFS:
         ret = (int)(Link.hxofs) * 10000;
         break;

      case LINKHYOFS:
         ret = (int)(Link.hyofs) * 10000;
         break;

      case LINKXOFS:
         ret = (int)(Link.xofs) * 10000;
         break;

      case LINKYOFS:
         ret = (int)(Link.yofs - PLAYFIELD_OFFSET) * 10000;
         break;

      case LINKZOFS:
         ret = (int)(Link.zofs) * 10000;
         break;

      case LINKHXSZ:
         ret = (int)(Link.hxsz) * 10000;
         break;

      case LINKHYSZ:
         ret = (int)(Link.hysz) * 10000;
         break;

      case LINKHZSZ:
         ret = (int)(Link.hzsz) * 10000;
         break;

      case LINKTXSZ:
         ret = (int)(Link.txsz) * 10000;
         break;

      case LINKTYSZ:
         ret = (int)(Link.tysz) * 10000;
         break;

      case LINKTILE:
         ret = (int)(Link.tile) * 10000;
         break;

      case LINKFLIP:
         ret = (int)(Link.flip) * 10000;
         break;

      ///----------------------------------------------------------------------------------------------------//
      //Input States
      case INPUTSTART:
         ret = StartKey ? 10000 : 0;
         break;

      case INPUTMAP:
         ret = MapKey ? 10000 : 0;
         break;

      case INPUTUP:
         ret = UpKey ? 10000 : 0;
         break;

      case INPUTDOWN:
         ret = DownKey ? 10000 : 0;
         break;

      case INPUTLEFT:
         ret = LeftKey ? 10000 : 0;
         break;

      case INPUTRIGHT:
         ret = RightKey ? 10000 : 0;
         break;

      case INPUTA:
         ret = AKey ? 10000 : 0;
         break;

      case INPUTB:
         ret = BKey ? 10000 : 0;
         break;

      case INPUTL:
         ret = LKey ? 10000 : 0;
         break;

      case INPUTR:
         ret = RKey ? 10000 : 0;
         break;

      case INPUTEX1:
         ret = Ex1Key ? 10000 : 0;
         break;

      case INPUTEX2:
         ret = Ex2Key ? 10000 : 0;
         break;

      case INPUTEX3:
         ret = Ex3Key ? 10000 : 0;
         break;

      case INPUTEX4:
         ret = Ex4Key ? 10000 : 0;
         break;

      case INPUTAXISUP:
         ret = UpKey ? 10000 : 0;
         break;

      case INPUTAXISDOWN:
         ret = DownKey ? 10000 : 0;
         break;

      case INPUTAXISLEFT:
         ret = LeftKey ? 10000 : 0;
         break;

      case INPUTAXISRIGHT:
         ret = RightKey ? 10000 : 0;
         break;

      case INPUTMOUSEX:
         ret = MouseX * 10000;
         break;

      case INPUTMOUSEY:
         ret = (MouseY - PLAYFIELD_OFFSET) * 10000;
         break;

      case INPUTMOUSEZ:
         ret = MouseZ * 10000;
         break;

      case INPUTMOUSEB:
         ret = MouseB * 10000;
         break;

      case INPUTPRESSSTART:
         ret = StartKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSMAP:
         ret = MapKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSUP:
         ret = UpKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSDOWN:
         ret = DownKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSLEFT:
         ret = LeftKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSRIGHT:
         ret = RightKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSA:
         ret = AKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSB:
         ret = BKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSL:
         ret = LKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSR:
         ret = RKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSEX1:
         ret = Ex1KeyPress ? 10000 : 0;
         break;

      case INPUTPRESSEX2:
         ret = Ex2KeyPress ? 10000 : 0;
         break;

      case INPUTPRESSEX3:
         ret = Ex3KeyPress ? 10000 : 0;
         break;

      case INPUTPRESSEX4:
         ret = Ex4KeyPress ? 10000 : 0;
         break;

      case INPUTPRESSAXISUP:
         ret = UpKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSAXISDOWN:
         ret = DownKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSAXISLEFT:
         ret = LeftKeyPress ? 10000 : 0;
         break;

      case INPUTPRESSAXISRIGHT:
         ret = RightKeyPress ? 10000 : 0;
         break;


      ///----------------------------------------------------------------------------------------------------//
      //Item Variables
      case ITEMX:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((int)((item *)(s))->x) * 10000;

         break;

      case ITEMY:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((int)((item *)(s))->y) * 10000;

         break;

      case ITEMZ:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((int)((item *)(s))->z) * 10000;

         break;

      case ITEMJUMP:
         if (0 != (s = checkItem(ri->itemref)))
            ret = int32_t(((item *)(s))->fall) * -100.0;

         break;

      case ITEMDRAWTYPE:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->drawstyle * 10000;

         break;

      case ITEMID:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->id * 10000;

         break;

      case ITEMTILE:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->tile * 10000;

         break;

      case ITEMOTILE:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->o_tile * 10000;

         break;

      case ITEMCSET:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->o_cset & 15) * 10000;

         break;

      case ITEMFLASHCSET:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->o_cset >> 4) * 10000;

         break;

      case ITEMFRAMES:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->frames * 10000;

         break;

      case ITEMFRAME:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->aframe * 10000;

         break;

      case ITEMASPEED:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->o_speed * 10000;

         break;

      case ITEMDELAY:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->o_delay * 10000;

         break;

      case ITEMFLIP:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->flip * 10000;

         break;

      case ITEMFLASH:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->flash * 10000;

         break;

      case ITEMHXOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->hxofs) * 10000;

         break;

      case ITEMHYOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->hyofs) * 10000;

         break;

      case ITEMXOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((int)(((item *)(s))->xofs)) * 10000;

         break;

      case ITEMYOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((int)(((item *)(s))->yofs - PLAYFIELD_OFFSET)) * 10000;

         break;

      case ITEMZOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((int)(((item *)(s))->zofs)) * 10000;

         break;

      case ITEMHXSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->hxsz) * 10000;

         break;

      case ITEMHYSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->hysz) * 10000;

         break;

      case ITEMHZSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->hzsz) * 10000;

         break;

      case ITEMTXSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->txsz) * 10000;

         break;

      case ITEMTYSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ret = (((item *)(s))->tysz) * 10000;

         break;

      case ITEMCOUNT:
         ret = (items.Count()) * 10000;
         break;

      case ITEMEXTEND:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->extend * 10000;

         break;

      case ITEMPICKUP:
         if (0 != (s = checkItem(ri->itemref)))
            ret = ((item *)(s))->pickup * 10000;

         break;

      case ITEMMISCD:
         if (0 != (s = checkItem(ri->itemref)))
         {
            int a = vbound(ri->d[0] / 10000, 0, 15);
            ret = (((item *)(s))->miscellaneous[a]);
            break;
         }

      ///----------------------------------------------------------------------------------------------------//
      //Itemdata Variables
      case IDATAFAMILY:
         ret = (itemsbuf[ri->idata].family) * 10000;
         break;

      case IDATALEVEL:
         ret = (itemsbuf[ri->idata].fam_type) * 10000;
         break;

      case IDATAKEEP:
         ret = (itemsbuf[ri->idata].flags & ITEM_GAMEDATA) ? 10000 : 0;
         break;

      case IDATAAMOUNT:
         ret = (itemsbuf[ri->idata].amount) * 10000;
         break;

      case IDATASETMAX:
         ret = (itemsbuf[ri->idata].setmax) * 10000;
         break;

      case IDATAMAX:
         ret = (itemsbuf[ri->idata].max) * 10000;
         break;

      case IDATACOUNTER:
         ret = (itemsbuf[ri->idata].count) * 10000;
         break;

      case IDATAUSESOUND:
         ret = (itemsbuf[ri->idata].usesound) * 10000;
         break;

      case IDATAPOWER:
         ret = (itemsbuf[ri->idata].power) * 10000;
         break;

      case IDATAINITDD:
      {
         int a = ri->d[0] / 10000;

         if (BC::checkBounds(a, 0, 7, "itemdata->InitD") != SH::_NoError)
            ret = -10000;
         else
            ret = itemsbuf[ri->idata].initiald[a];
      }
      break;

         ///----------------------------------------------------------------------------------------------------//
         //NPC Variables

         //Reduces accessing integer members to one line
#define GET_NPC_VAR_INT(member, str) \
{ \
   if(GuyH::loadNPC(ri->guyref, str) != SH::_NoError) \
      ret = -10000; \
   else \
      ret = GuyH::getNPC()->member * 10000; \
}

      case NPCDIR:
         if (GuyH::loadNPC(ri->guyref, "npc->Dir") != SH::_NoError)
            ret = -10000;
         else
            ret = zc_max(GuyH::getNPC()->dir * 10000, 0);

         break;

      case NPCRATE:
         GET_NPC_VAR_INT(rate, "npc->Rate") break;

      case NPCHOMING:
         GET_NPC_VAR_INT(homing, "npc->Homing") break;

      case NPCFRAMERATE:
         GET_NPC_VAR_INT(frate, "npc->ASpeed") break;

      case NPCHALTRATE:
         GET_NPC_VAR_INT(hrate, "npc->HaltRate") break;

      case NPCDRAWTYPE:
         GET_NPC_VAR_INT(drawstyle, "npc->DrawStyle") break;

      case NPCHP:
         GET_NPC_VAR_INT(hp, "npc->HP") break;

      case NPCCOLLDET:
         GET_NPC_VAR_INT(scriptcoldet, "npc->ColDetection") break;

      case NPCSTUN:
         GET_NPC_VAR_INT(stunclk, "npc->Stun") break;

      case NPCHUNGER:
         GET_NPC_VAR_INT(grumble, "npc->Hunger") break;

      case NPCTYPE:
         GET_NPC_VAR_INT(family, "npc->Type") break;

      case NPCDP:
         GET_NPC_VAR_INT(dp, "npc->Damage") break;

      case NPCWDP:
         GET_NPC_VAR_INT(wdp, "npc->WeaponDamage") break;

      case NPCOTILE:
         GET_NPC_VAR_INT(o_tile, "npc->OriginalTile") break;

      case NPCTILE:
         GET_NPC_VAR_INT(tile, "npc->Tile") break;

      case NPCWEAPON:
         GET_NPC_VAR_INT(wpn, "npc->Weapon") break;

      case NPCITEMSET:
         GET_NPC_VAR_INT(item_set, "npc->ItemSet") break;

      case NPCCSET:
         GET_NPC_VAR_INT(cs, "npc->CSet") break;

      case NPCBOSSPAL:
         GET_NPC_VAR_INT(bosspal, "npc->BossPal") break;

      case NPCBGSFX:
         GET_NPC_VAR_INT(bgsfx, "npc->SFX") break;

      case NPCEXTEND:
         GET_NPC_VAR_INT(extend, "npc->Extend") break;

      case NPCHXOFS:
         GET_NPC_VAR_INT(hxofs, "npc->HitXOffset") break;

      case NPCHYOFS:
         GET_NPC_VAR_INT(hyofs, "npc->HitYOffset") break;

      case NPCHXSZ:
         GET_NPC_VAR_INT(hxsz, "npc->HitWidth") break;

      case NPCHYSZ:
         GET_NPC_VAR_INT(hysz, "npc->HitHeight") break;

      case NPCHZSZ:
         GET_NPC_VAR_INT(hzsz, "npc->HitZHeight") break;

      case NPCTXSZ:
         GET_NPC_VAR_INT(txsz, "npc->TileWidth") break;

      case NPCTYSZ:
         GET_NPC_VAR_INT(tysz, "npc->TileHeight") break;

         //And fix
#define GET_NPC_VAR_FIX(member, str) \
   { \
      if(GuyH::loadNPC(ri->guyref, str) != SH::_NoError) \
         ret = -10000; \
      else \
         ret = (int32_t(GuyH::getNPC()->member) * 10000); \
   }

      case NPCX:
         GET_NPC_VAR_FIX(x, "npc->X") break;

      case NPCY:
         GET_NPC_VAR_FIX(y, "npc->Y") break;

      case NPCZ:
         GET_NPC_VAR_FIX(z, "npc->Z") break;

      case NPCXOFS:
         GET_NPC_VAR_FIX(xofs, "npc->DrawXOffset") break;

      case NPCYOFS:
         GET_NPC_VAR_FIX(yofs, "npc->DrawYOffset") ret -= PLAYFIELD_OFFSET * 10000;
         break;

      case NPCZOFS:
         GET_NPC_VAR_FIX(zofs, "npc->DrawZOffset") break;

      //These variables are all different to the templates (casting for jump and step is slightly non-standard)
      case NPCJUMP:
         if (GuyH::loadNPC(ri->guyref, "npc->Jump") != SH::_NoError)
            ret = -10000;
         else
            ret = (int32_t(GuyH::getNPC()->fall) * -100.0);

         break;

      case NPCSTEP:
         if (GuyH::loadNPC(ri->guyref, "npc->Step") != SH::_NoError)
            ret = -10000;
         else
            ret = int32_t(GuyH::getNPC()->step * fix(100.0)) * 10000;

         break;

      case NPCID:
         if (GuyH::loadNPC(ri->guyref, "npc->ID") != SH::_NoError)
            ret = -10000;
         else
            ret = (GuyH::getNPC()->id & 0xFFF) * 10000;

         break;

      case NPCMFLAGS:
         if (GuyH::loadNPC(ri->guyref, "npc->MiscFlags") != SH::_NoError)
            ret = -10000;
         else
            ret = GuyH::getMFlags() * 10000;

         break;

      //Indexed (two checks)
      case NPCDEFENSED:
      {
         int a = ri->d[0] / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Defense") != SH::_NoError ||
               BC::checkBounds(a, 0, edefSCRIPT, "npc->Defense") != SH::_NoError)
            ret = -10000;
         else
            ret = GuyH::getNPC()->defense[a] * 10000;
      }
      break;

      case NPCMISCD:
      {
         int a = ri->d[0] / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Misc") != SH::_NoError ||
               BC::checkMisc(a, "npc->Misc") != SH::_NoError)
            ret = -10000;
         else
            ret = GuyH::getNPC()->miscellaneous[a];
      }
      break;

      case NPCDD:
      {
         int a = ri->d[0] / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Attributes") != SH::_NoError ||
               BC::checkBounds(a, 0, 11, "npc->Attributes") != SH::_NoError)
            ret = -10000;
         else
            ret = GuyH::getNPCDMisc(a) * 10000;
      }
      break;


      ///----------------------------------------------------------------------------------------------------//
      //LWeapon Variables
      case LWPNX:
         if (0 != (s = checkLWpn(ri->lwpn, "X")))
            ret = ((int)((weapon *)(s))->x) * 10000;

         break;

      case LWPNY:
         if (0 != (s = checkLWpn(ri->lwpn, "Y")))
            ret = ((int)((weapon *)(s))->y) * 10000;

         break;

      case LWPNZ:
         if (0 != (s = checkLWpn(ri->lwpn, "Z")))
            ret = ((int)((weapon *)(s))->z) * 10000;

         break;

      case LWPNJUMP:
         if (0 != (s = checkLWpn(ri->lwpn, "Jump")))
            ret = int32_t(((weapon *)(s))->fall) * -100.0;

         break;

      case LWPNDIR:
         if (0 != (s = checkLWpn(ri->lwpn, "Dir")))
            ret = ((weapon *)(s))->dir * 10000;

         break;

      case LWPNSTEP:
         if (0 != (s = checkLWpn(ri->lwpn, "Step")))
            ret = (int)((float)((weapon *)s)->step * 1000000.0);

         break;

      case LWPNANGLE:
         if (0 != (s = checkLWpn(ri->lwpn, "Angle")))
            ret = (int)(((weapon *)(s))->angle * 10000);

         break;

      case LWPNANGULAR:
         if (0 != (s = checkLWpn(ri->lwpn, "Angular")))
            ret = ((weapon *)(s))->angular * 10000;

         break;

      case LWPNBEHIND:
         if (0 != (s = checkLWpn(ri->lwpn, "Behind")))
            ret = ((weapon *)(s))->behind * 10000;

         break;

      case LWPNDRAWTYPE:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawStyle")))
            ret = ((weapon *)(s))->drawstyle * 10000;

         break;

      case LWPNPOWER:
         if (0 != (s = checkLWpn(ri->lwpn, "Damage")))
            ret = ((weapon *)(s))->power * 10000;

         break;

      case LWPNDEAD:
         if (0 != (s = checkLWpn(ri->lwpn, "DeadState")))
            ret = ((weapon *)(s))->dead * 10000;

         break;

      case LWPNID:
         if (0 != (s = checkLWpn(ri->lwpn, "ID")))
            ret = ((weapon *)(s))->id * 10000;

         break;

      case LWPNTILE:
         if (0 != (s = checkLWpn(ri->lwpn, "Tile")))
            ret = ((weapon *)(s))->tile * 10000;

         break;

      case LWPNCSET:
         if (0 != (s = checkLWpn(ri->lwpn, "CSet")))
            ret = ((weapon *)(s))->cs * 10000;

         break;

      case LWPNFLASHCSET:
         if (0 != (s = checkLWpn(ri->lwpn, "FlashCSet")))
            ret = (((weapon *)(s))->o_cset >> 4) * 10000;

         break;

      case LWPNFRAMES:
         if (0 != (s = checkLWpn(ri->lwpn, "NumFrames")))
            ret = ((weapon *)(s))->frames * 10000;

         break;

      case LWPNFRAME:
         if (0 != (s = checkLWpn(ri->lwpn, "Frame")))
            ret = ((weapon *)(s))->aframe * 10000;

         break;

      case LWPNASPEED:
         if (0 != (s = checkLWpn(ri->lwpn, "ASpeed")))
            ret = ((weapon *)(s))->o_speed * 10000;

         break;

      case LWPNFLASH:
         if (0 != (s = checkLWpn(ri->lwpn, "Flash")))
            ret = ((weapon *)(s))->flash * 10000;

         break;

      case LWPNFLIP:
         if (0 != (s = checkLWpn(ri->lwpn, "Flip")))
            ret = ((weapon *)(s))->flip * 10000;

         break;

      case LWPNCOUNT:
         ret = Lwpns.Count() * 10000;
         break;

      case LWPNEXTEND:
         if (0 != (s = checkLWpn(ri->lwpn, "Extend")))
            ret = ((weapon *)(s))->extend * 10000;

         break;

      case LWPNOTILE:
         if (0 != (s = checkLWpn(ri->lwpn, "OriginalTile")))
            ret = ((weapon *)(s))->o_tile * 10000;

         break;

      case LWPNOCSET:
         if (0 != (s = checkLWpn(ri->lwpn, "OriginalCSet")))
            ret = (((weapon *)(s))->o_cset & 15) * 10000;

         break;

      case LWPNHXOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "HitXOffset")))
            ret = (((weapon *)(s))->hxofs) * 10000;

         break;

      case LWPNHYOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "HitYOffset")))
            ret = (((weapon *)(s))->hyofs) * 10000;

         break;

      case LWPNXOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawXOffset")))
            ret = ((int)(((weapon *)(s))->xofs)) * 10000;

         break;

      case LWPNYOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawYOffset")))
            ret = ((int)(((weapon *)(s))->yofs - PLAYFIELD_OFFSET)) * 10000;

         break;

      case LWPNZOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawZOffset")))
            ret = ((int)(((weapon *)(s))->zofs)) * 10000;

         break;

      case LWPNHXSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "HitWidth")))
            ret = (((weapon *)(s))->hxsz) * 10000;

         break;

      case LWPNHYSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "HitHeight")))
            ret = (((weapon *)(s))->hysz) * 10000;

         break;

      case LWPNHZSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "HitZHeight")))
            ret = (((weapon *)(s))->hzsz) * 10000;

         break;

      case LWPNTXSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "TileWidth")))
            ret = (((weapon *)(s))->txsz) * 10000;

         break;

      case LWPNTYSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "TileHeight")))
            ret = (((weapon *)(s))->tysz) * 10000;

         break;

      case LWPNMISCD:
         if (0 != (s = checkLWpn(ri->lwpn, "Misc")))
         {
            int a = vbound(ri->d[0] / 10000, 0, 15);
            ret = (((weapon *)(s))->miscellaneous[a]);
         }

         break;

      case LWPNCOLLDET:
         if (0 != (s = checkLWpn(ri->lwpn, "CollDetection")))
            ret = (((weapon *)(s))->scriptcoldet) * 10000;

         break;

      ///----------------------------------------------------------------------------------------------------//
      //EWeapon Variables
      case EWPNX:
         if (0 != (s = checkEWpn(ri->ewpn, "X")))
            ret = ((int)((weapon *)(s))->x) * 10000;

         break;

      case EWPNY:
         if (0 != (s = checkEWpn(ri->ewpn, "Y")))
            ret = ((int)((weapon *)(s))->y) * 10000;

         break;

      case EWPNZ:
         if (0 != (s = checkEWpn(ri->ewpn, "Z")))
            ret = ((int)((weapon *)(s))->z) * 10000;

         break;

      case EWPNJUMP:
         if (0 != (s = checkEWpn(ri->ewpn, "Jump")))
            ret = int32_t(((weapon *)(s))->fall) * -100.0;

         break;

      case EWPNDIR:
         if (0 != (s = checkEWpn(ri->ewpn, "Dir")))
            ret = ((weapon *)(s))->dir * 10000;

         break;

      case EWPNSTEP:
         if (0 != (s = checkEWpn(ri->ewpn, "Step")))
            ret = (int)((float)((weapon *)s)->step * 1000000.0);

         break;

      case EWPNANGLE:
         if (0 != (s = checkEWpn(ri->ewpn, "Angle")))
            ret = (int)(((weapon *)(s))->angle * 10000);

         break;

      case EWPNANGULAR:
         if (0 != (s = checkEWpn(ri->ewpn, "Angular")))
            ret = ((weapon *)(s))->angular * 10000;

         break;

      case EWPNBEHIND:
         if (0 != (s = checkEWpn(ri->ewpn, "Behind")))
            ret = ((weapon *)(s))->behind * 10000;

         break;

      case EWPNDRAWTYPE:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawStyle")))
            ret = ((weapon *)(s))->drawstyle * 10000;

         break;

      case EWPNPOWER:
         if (0 != (s = checkEWpn(ri->ewpn, "Damage")))
            ret = ((weapon *)(s))->power * 10000;

         break;

      case EWPNDEAD:
         if (0 != (s = checkEWpn(ri->ewpn, "DeadState")))
            ret = ((weapon *)(s))->dead * 10000;

         break;

      case EWPNID:
         if (0 != (s = checkEWpn(ri->ewpn, "ID")))
            ret = ((weapon *)(s))->id * 10000;

         break;

      case EWPNTILE:
         if (0 != (s = checkEWpn(ri->ewpn, "Tile")))
            ret = ((weapon *)(s))->tile * 10000;

         break;

      case EWPNCSET:
         if (0 != (s = checkEWpn(ri->ewpn, "CSet")))
            ret = ((weapon *)(s))->cs * 10000;

         break;

      case EWPNFLASHCSET:
         if (0 != (s = checkEWpn(ri->ewpn, "FlashCSet")))
            ret = (((weapon *)(s))->o_cset >> 4) * 10000;

         break;

      case EWPNFRAMES:
         if (0 != (s = checkEWpn(ri->ewpn, "NumFrames")))
            ret = ((weapon *)(s))->frames * 10000;

         break;

      case EWPNFRAME:
         if (0 != (s = checkEWpn(ri->ewpn, "Frame")))
            ret = ((weapon *)(s))->aframe * 10000;

         break;

      case EWPNASPEED:
         if (0 != (s = checkEWpn(ri->ewpn, "ASpeed")))
            ret = ((weapon *)(s))->o_speed * 10000;

         break;

      case EWPNFLASH:
         if (0 != (s = checkEWpn(ri->ewpn, "Flash")))
            ret = ((weapon *)(s))->flash * 10000;

         break;

      case EWPNFLIP:
         if (0 != (s = checkEWpn(ri->ewpn, "Flip")))
            ret = ((weapon *)(s))->flip * 10000;

         break;

      case EWPNCOUNT:
         ret = Ewpns.Count() * 10000;
         break;

      case EWPNEXTEND:
         if (0 != (s = checkEWpn(ri->ewpn, "Extend")))
            ret = ((weapon *)(s))->extend * 10000;

         break;

      case EWPNOTILE:
         if (0 != (s = checkEWpn(ri->ewpn, "OriginalTile")))
            ret = ((weapon *)(s))->o_tile * 10000;

         break;

      case EWPNOCSET:
         if (0 != (s = checkEWpn(ri->ewpn, "OriginalCSet")))
            ret = (((weapon *)(s))->o_cset & 15) * 10000;

         break;

      case EWPNHXOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "HitXOffset")))
            ret = (((weapon *)(s))->hxofs) * 10000;

         break;

      case EWPNHYOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "HitYOffset")))
            ret = (((weapon *)(s))->hyofs) * 10000;

         break;

      case EWPNXOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawXOffset")))
            ret = ((int)(((weapon *)(s))->xofs)) * 10000;

         break;

      case EWPNYOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawYOffset")))
            ret = ((int)(((weapon *)(s))->yofs - PLAYFIELD_OFFSET)) * 10000;

         break;

      case EWPNZOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawZOffset")))
            ret = ((int)(((weapon *)(s))->zofs)) * 10000;

         break;

      case EWPNHXSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "HitWidth")))
            ret = (((weapon *)(s))->hxsz) * 10000;

         break;

      case EWPNHYSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "HitHeight")))
            ret = (((weapon *)(s))->hysz) * 10000;

         break;

      case EWPNHZSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "HitZHeight")))
            ret = (((weapon *)(s))->hzsz) * 10000;

         break;

      case EWPNTXSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "TileWidth")))
            ret = (((weapon *)(s))->txsz) * 10000;

         break;

      case EWPNTYSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "TileHeight")))
            ret = (((weapon *)(s))->tysz) * 10000;

         break;

      case EWPNMISCD:
         if (0 != (s = checkEWpn(ri->ewpn, "Misc")))
         {
            int a = vbound(ri->d[0] / 10000, 0, 15);
            ret = (((weapon *)(s))->miscellaneous[a]);
         }

         break;

      case EWPNCOLLDET:
         if (0 != (s = checkEWpn(ri->ewpn, "CollDetection")))
            ret = (((weapon *)(s))->scriptcoldet) * 10000;

         break;

      ///----------------------------------------------------------------------------------------------------//
      //Game Info
      case ZELDAVERSION:
         ret = ZC_VERSION; //Do *not* multiply by 10,000!
         break;

      case GAMEDEATHS:
         ret = game->get_deaths() * 10000;
         break;

      case GAMECHEAT:
         ret = game->get_cheat() * 10000;
         break;

      case GAMETIME:
         ret = game->get_time();
         break;// Can't multiply by 10000 or the maximum result is too big

      case GAMETIMEVALID:
         ret = game->get_timevalid() ? 10000 : 0;
         break;

      case GAMEHASPLAYED:
         ret = game->get_hasplayed() ? 10000 : 0;
         break;

      case GAMESTANDALONE:
         ret = 0;
         break;

      case GAMEGUYCOUNT:
      {
         int mi = (currmap * MAPSCRSNORMAL) + (ri->d[0] / 10000);
         ret = game->guys[mi] * 10000;
      }
      break;

      case GAMECONTSCR:
         ret = game->get_continue_scrn() * 10000;
         break;

      case GAMECONTDMAP:
         ret = game->get_continue_dmap() * 10000;
         break;

      case GAMEENTRSCR:
         ret = lastentrance * 10000;
         break;

      case GAMEENTRDMAP:
         ret = lastentrance_dmap * 10000;
         break;

      case GAMECOUNTERD:
         ret = game->get_counter((ri->d[0]) / 10000) * 10000;
         break;

      case GAMEMCOUNTERD:
         ret = game->get_maxcounter((ri->d[0]) / 10000) * 10000;
         break;

      case GAMEDCOUNTERD:
         ret = game->get_dcounter((ri->d[0]) / 10000) * 10000;
         break;

      case GAMEGENERICD:
         ret = game->get_generic((ri->d[0]) / 10000) * 10000;
         break;

      case GAMEITEMSD:
         ret = (game->item[(ri->d[0]) / 10000] ? 10000 : 0);
         break;

      case GAMELITEMSD:
         ret = game->lvlitems[(ri->d[0]) / 10000] * 10000;
         break;

      case GAMELKEYSD:
         ret = game->lvlkeys[(ri->d[0]) / 10000] * 10000;
         break;

      case SCREENSTATED:
      {
         int mi = (currmap * MAPSCRSNORMAL) + currscr;
         ret = ((game->maps[mi] >> ((ri->d[0] / 10000))) & 1) ? 10000 : 0;
      }
      break;

      case SCREENSTATEDD:
      {
         // Gah! >:(  Screen state is stored in game->maps, which uses 128 screens per map,
         // but the compiler multiplies the map number by 136, so it has to be corrected here.
         // Yeah, the compiler could be fixed, but that wouldn't cover existing quests...
         int mi = ri->d[0] / 10000;
         mi -= 8 * ((ri->d[0] / 10000) / MAPSCRS);

         if (BC::checkMapID(mi >> 7, "Game->GetScreenState") == SH::_NoError)
            ret = (game->maps[mi] >> (ri->d[1] / 10000) & 1) ? 10000 : 0;
         else
            ret = 0;

         break;
      }

      case GAMEGUYCOUNTD:
         ret = game->guys[(currmap * MAPSCRSNORMAL) + (ri->d[0] / 10000)] * 10000;
         break;

      case CURMAP:
         ret = (1 + currmap) * 10000;
         break;

      case CURSCR:
         ret = currscr * 10000;
         break;

      case GETMIDI:
         ret = (sel_music - (MID_COUNT - 1)) * 10000;
         break;

      case CURDSCR:
      {
         int di = (get_currscr() - DMaps[get_currdmap()].xoff);
         ret = (DMaps[get_currdmap()].type == dmOVERW ? currscr : di) * 10000;
      }
      break;

      case CURDMAP:
         ret = currdmap * 10000;
         break;

      case CURLEVEL:
         ret = DMaps[get_currdmap()].level * 10000;
         break;

      case GAMECLICKFREEZE:
         ret = 10000;
         break;


         ///----------------------------------------------------------------------------------------------------//
         //DMap Information

#define GET_DMAP_VAR(member, str) \
   { \
      int ID = ri->d[0] / 10000; \
      if(BC::checkDMapID(ID, str) != SH::_NoError) \
         ret = -10000; \
      else \
         ret = DMaps[ID].member * 10000; \
   }

      case DMAPFLAGSD:
         GET_DMAP_VAR(flags,   "Game->DMapFlags")    break;

      case DMAPLEVELD:
         GET_DMAP_VAR(level,   "Game->DMapLevel")    break;

      case DMAPCOMPASSD:
         GET_DMAP_VAR(compass, "Game->DMapCompass")  break;

      case DMAPCONTINUED:
         GET_DMAP_VAR(cont,    "Game->DMapContinue") break;

      case DMAPOFFSET:
         GET_DMAP_VAR(xoff,    "Game->DMapOffset")   break;

      case DMAPMAP:
      {
         int ID = ri->d[0] / 10000;

         if (BC::checkDMapID(ID, "Game->DMapMap") != SH::_NoError)
            ret = -10000;
         else
            ret = (DMaps[ID].map + 1) * 10000;

         break;
      }

      case DMAPMIDID:
      {
         int ID = ri->d[0] / 10000;

         if (BC::checkDMapID(ID, "Game->DMapMIDI") == SH::_NoError)
         {
            switch (DMaps[ID].midi)
            {
               case 2:
                  ret = -60000;
                  break; // Dungeon

               case 3:
                  ret = -30000;
                  break; // Level 9

               case 1:
                  ret = -20000;
                  break; // Overworld

               case 0:
                  ret = 0;
                  break; // None

               default:
                  ret = (DMaps[ID].midi - 3) * 10000;
            }
         }
         else
            ret = -10000; // Which is valid, but whatever.

         break;
      }

         ///----------------------------------------------------------------------------------------------------//
         //Screen->ComboX
#define GET_COMBO_VAR(member, str) \
{ \
   int pos = ri->d[0] / 10000; \
   if(BC::checkComboPos(pos, str) != SH::_NoError) \
      ret = -10000; \
   else \
      ret = tmpscr->member[pos]*10000; \
}

      case COMBODD:
         GET_COMBO_VAR(data,  "Screen->ComboD") break;

      case COMBOCD:
         GET_COMBO_VAR(cset,  "Screen->ComboC") break;

      case COMBOFD:
         GET_COMBO_VAR(sflag, "Screen->ComboF") break;

#define GET_COMBO_VAR_BUF(member, str) \
   { \
      int pos = ri->d[0] / 10000; \
      if(BC::checkComboPos(pos, str) != SH::_NoError) \
         ret = -10000; \
      else \
         ret = combobuf[tmpscr->data[pos]].member * 10000; \
   }

      case COMBOTD:
         GET_COMBO_VAR_BUF(type, "Screen->ComboT") break;

      case COMBOID:
         GET_COMBO_VAR_BUF(flag, "Screen->ComboI") break;

      case COMBOSD:
      {
         int pos = ri->d[0] / 10000;

         if (BC::checkComboPos(pos, "Screen->ComboS") != SH::_NoError)
            ret = -10000;
         else
            ret = (combobuf[tmpscr->data[pos]].walk & 0xF) * 10000;
      }
      break;

      ///----------------------------------------------------------------------------------------------------//
      //Game->GetComboX
      case COMBODDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);
         int layr = whichlayer(scr);

         if (pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)
         {
            if (scr == (currmap * MAPSCRS + currscr))
               ret = tmpscr->data[pos] * 10000;
            else if (layr > -1)
               ret = tmpscr2[layr].data[pos] * 10000;
            else ret = TheMaps[scr].data[pos] * 10000;
         }
         else
            ret = -10000;
      }
      break;

      case COMBOCDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);
         int layr = whichlayer(scr);

         if (pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)
         {
            if (scr == (currmap * MAPSCRS + currscr))
               ret = tmpscr->cset[pos] * 10000;
            else if (layr > -1)
               ret = tmpscr2[layr].cset[pos] * 10000;
            else ret = TheMaps[scr].cset[pos] * 10000;
         }
         else
            ret = -10000;
      }
      break;

      case COMBOFDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);
         int layr = whichlayer(scr);

         if (pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)
         {
            if (scr == (currmap * MAPSCRS + currscr))
               ret = tmpscr->sflag[pos] * 10000;
            else if (layr > -1)
               ret = tmpscr2[layr].sflag[pos] * 10000;
            else ret = TheMaps[scr].sflag[pos] * 10000;
         }
         else
            ret = -10000;
      }
      break;

      case COMBOTDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);
         int layr = whichlayer(scr);

         if (pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)
         {
            if (scr == (currmap * MAPSCRS + currscr))
               ret = combobuf[tmpscr->data[pos]].type * 10000;
            else if (layr > -1)
               ret = combobuf[tmpscr2[layr].data[pos]].type * 10000;
            else ret = combobuf[
                             TheMaps[scr].data[pos]].type * 10000;
         }
         else
            ret = -10000;
      }
      break;

      case COMBOIDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);
         int layr = whichlayer(scr);

         if (pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)
         {
            if (scr == (currmap * MAPSCRS + currscr))
               ret = combobuf[tmpscr->data[pos]].flag * 10000;
            else if (layr > -1)
               ret = combobuf[tmpscr2[layr].data[pos]].flag * 10000;
            else ret = combobuf[TheMaps[scr].data[pos]].flag * 10000;
         }
         else
            ret = -10000;
      }
      break;

      case COMBOSDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);
         int layr = whichlayer(scr);

         if (pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)
         {
            if (scr == (currmap * MAPSCRS + currscr))
               ret = (combobuf[tmpscr->data[pos]].walk & 15) * 10000;
            else if (layr > -1)
               ret = (combobuf[tmpscr2[layr].data[pos]].walk & 15) * 10000;
            else ret = (combobuf[TheMaps[scr].data[pos]].walk & 15) * 10000;
         }
         else
            ret = -10000;
      }
      break;

      ///----------------------------------------------------------------------------------------------------//
      //Screen Information
      case SDD:
      {
         int di = ((get_currdmap()) << 7) + get_currscr() - (DMaps[get_currdmap()].type == dmOVERW ? 0 :
                  DMaps[get_currdmap()].xoff);
         ret = get_screen_d(di, ri->d[0] / 10000);
      }
      break;

      case SDDD:
         ret = get_screen_d((ri->d[0]) / 10000 + ((get_currdmap()) << 7), ri->d[1] / 10000);
         break;

      case SDDDD:
         ret = get_screen_d(ri->d[1] / 10000 + ((ri->d[0] / 10000) << 7), ri->d[2] / 10000);
         break;

      case SCRDOORD:
         ret = tmpscr->door[ri->d[0] / 10000] * 10000;
         break;

      case LIT:
         ret = darkroom ? 0 : 10000;
         break;

      case WAVY:
         ret = wavy * 10000;
         break;

      case QUAKE:
         ret = quakeclk * 10000;
         break;

      case SCREENFLAGSD:
         ret = get_screenflags(tmpscr, vbound(ri->d[0] / 10000, 0, 9));
         break;

      case SCREENEFLAGSD:
         ret = get_screeneflags(tmpscr, vbound(ri->d[0] / 10000, 0, 2));
         break;

      case NPCCOUNT:
         ret = guys.Count() * 10000;
         break;

      case ROOMDATA:
         ret = tmpscr->catchall * 10000;
         break;

      case ROOMTYPE:
         ret = tmpscr->room * 10000;
         break;

      case PUSHBLOCKX:
         ret = blockmoving ? int(mblock2.x) * 10000 : -10000;
         break;

      case PUSHBLOCKY:
         ret = blockmoving ? int(mblock2.y) * 10000 : -10000;
         break;

      case PUSHBLOCKCOMBO:
         ret = mblock2.bcombo * 10000;
         break;

      case PUSHBLOCKCSET:
         ret = mblock2.cs * 10000;
         break;

      case UNDERCOMBO:
         ret = tmpscr->undercombo * 10000;
         break;

      case UNDERCSET:
         ret = tmpscr->undercset * 10000;
         break;

      ///----------------------------------------------------------------------------------------------------//
      //Misc./Internal
      case REFFFC:
         ret = ri->ffcref * 10000;
         break;

      case REFITEM:
         ret = ri->itemref;
         break;

      case REFITEMCLASS:
         ret = ri->idata;
         break;

      case REFLWPN:
         ret = ri->lwpn;
         break;

      case REFEWPN:
         ret = ri->ewpn;
         break;

      case REFNPC:
         ret = ri->guyref;
         break;

      case SP:
         ret = ri->sp * 10000;
         break;

      case SCRIPTRAM:
      case GLOBALRAM:
         ret = ArrayH::getElement(ri->d[0] / 10000, ri->d[1] / 10000);
         break;

      case SCRIPTRAMD:
      case GLOBALRAMD:
         ret = ArrayH::getElement(ri->d[0] / 10000, 0);
         break;

      case GDD://Doesn't work like this =(
         ret = game->global_d[ri->d[0] / 10000];
         break;

      ///----------------------------------------------------------------------------------------------------//
      //Most of this is deprecated I believe ~Joe123
      default:
      {
         if (arg >= D(0) && arg <= D(7))       ret = ri->d[arg - D(0)];
         else if (arg >= A(0) && arg <= A(1))     ret = ri->a[arg - A(0)];
         else if (arg >= GD(0) && arg <= GD(255)) ret = game->global_d[arg - GD(0)];

         break;
      }
   }

   return ret;
}



void set_register(const int32_t arg, const int32_t value)
{

   switch (arg)
   {
      ///----------------------------------------------------------------------------------------------------//
      //FFC Variables
      case DATA:
         tmpscr->ffdata[ri->ffcref] = vbound(value / 10000, 0, MAXCOMBOS - 1);
         break;

      case FFSCRIPT:
         for (int32_t i = 1; i < MAX_ZCARRAY_SIZE; i++)
         {
            if (arrayOwner[i] == ri->ffcref)
               deallocateArray(i);
         }

         tmpscr->ffscript[ri->ffcref] = vbound(value / 10000, 0, NUMSCRIPTFFC - 1);

         for (int i = 0; i < 16; i++)
            tmpscr->ffmisc[ri->ffcref][i] = 0;

         for (int i = 0; i < 2; i++)
            tmpscr->inita[ri->ffcref][i] = 0;

         for (int i = 0; i < 8; i++)
            tmpscr->initd[ri->ffcref][i] = 0;

         tmpscr->scriptData[ri->ffcref].Clear();
         tmpscr->initialized[ri->ffcref] = false;
         break;

      case FCSET:
         tmpscr->ffcset[ri->ffcref] = (value / 10000) & 15;
         break;

      case DELAY:
         tmpscr->ffdelay[ri->ffcref] = value / 10000;
         break;

      case FX:
         tmpscr->ffx[ri->ffcref] = value;
         break;

      case FY:
         tmpscr->ffy[ri->ffcref] = value;
         break;

      case XD:
         tmpscr->ffxdelta[ri->ffcref] = value;
         break;

      case YD:
         tmpscr->ffydelta[ri->ffcref] = value;
         break;

      case XD2:
         tmpscr->ffxdelta2[ri->ffcref] = value;
         break;

      case YD2:
         tmpscr->ffydelta2[ri->ffcref] = value;
         break;

      case FFFLAGSD:
         value ? tmpscr->ffflags[ri->ffcref] |=   1 << ((ri->d[0]) / 10000)
               : tmpscr->ffflags[ri->ffcref] &= ~(1 << ((ri->d[0]) / 10000));
         break;

      case FFCWIDTH:
         tmpscr->ffwidth[ri->ffcref] = (tmpscr->ffwidth[ri->ffcref] & ~63) | (((value / 10000) - 1) & 63);
         break;

      case FFCHEIGHT:
         tmpscr->ffheight[ri->ffcref] = (tmpscr->ffheight[ri->ffcref] & ~63) | (((value / 10000) - 1) & 63);
         break;

      case FFTWIDTH:
         tmpscr->ffwidth[ri->ffcref] = (tmpscr->ffwidth[ri->ffcref] & 63) | ((((value / 10000) - 1) & 3) << 6);
         break;

      case FFTHEIGHT:
         tmpscr->ffheight[ri->ffcref] = (tmpscr->ffheight[ri->ffcref] & 63) | ((((value / 10000) - 1) & 3) << 6);
         break;

      case FFLINK:
         (tmpscr->fflink[ri->ffcref]) = vbound(value / 10000, 0, 32); //Needs to be 0 to be able to clear it.
         break;

      case FFMISCD:
      {
         int a = vbound(ri->d[0] / 10000, 0, 15);
         (tmpscr->ffmisc[ri->ffcref][a]) = value;
         break;
      }

      case FFINITDD:
         (tmpscr->initd[ri->ffcref][vbound(ri->d[0] / 10000, 0, 7)]) = value;
         break;


      ///----------------------------------------------------------------------------------------------------//
      //Link's Variables
      case LINKX:
         Link.setX(value / 10000);
         break;

      case LINKY:
         Link.setY(value / 10000);
         break;

      case LINKZ:
         Link.setZ(value / 10000);
         break;

      case LINKJUMP:
         Link.setFall(fix((-value * (100.0)) / 10000.0));
         break;

      case LINKDIR:
      {
         //Link.setDir() calls reset_hookshot(), which removes the sword sprite.. O_o
         if (Link.getAction() == attacking) Link.dir = (value / 10000);
         else Link.setDir(value / 10000);

         break;
      }

      case LINKHITDIR:
         Link.setHitDir(value / 10000);
         break;

      case LINKHP:
         game->set_life(zc_max(0, zc_min(value / 10000, game->get_maxlife())));
         break;

      case LINKMP:
         game->set_magic(zc_max(0, zc_min(value / 10000, game->get_maxmagic())));
         break;

      case LINKMAXHP:
         game->set_maxlife(value / 10000);
         break;

      case LINKMAXMP:
         game->set_maxmagic(value / 10000);
         break;

      case LINKACTION:
         Link.setAction((actiontype)(value / 10000));
         break;

      case LINKHELD:
         Link.setHeldItem(vbound(value / 10000, 0, MAXITEMS - 1));
         break;

      case LINKITEMD:
      {
         int itemID = vbound(ri->d[0] / 10000, 0, MAXITEMS - 1);

         // If the Cane of Byrna is being removed, cancel its effect.
         if (value == 0 && itemID == current_item_id(itype_cbyrna))
            stopCaneOfByrna();

         bool settrue = (value != 0);
         //Sanity check to prevent setting the item if the value would be the same. -Z
         if (game->item[itemID] != settrue) game->set_item(itemID, (value != 0));

         if ((get_bit(quest_rules, qr_OVERWORLDTUNIC) != 0) || (currscr < 128 || dlevel)) ringcolor(false);
      }
      break;

      /*case LINKEQUIP:
        {

       int setb = ((value/10000)&0xFF00)>>8, seta = (value/10000)&0xFF;
       if(seta && get_bit(quest_rules,qr_SELECTAWPN) && game->item[seta]){

       }
       if(setb && game->item[setb]){
       }
        }
       break;*/
      case LINKINVIS:
         Link.setDontDraw(value / 10000);
         break;

      case LINKINVINC:
         Link.scriptcoldet = (value / 10000);
         break;

      case LINKSWORDJINX:
         Link.setSwordClk(value / 10000);
         break;

      case LINKITEMJINX:
         Link.setItemClk(value / 10000);
         break;

      case LINKDRUNK:
         Link.setDrunkClock(value / 10000);
         break;

      case LINKMISCD:
         Link.miscellaneous[vbound(ri->d[0] / 10000, 0, 15)] = value;
         break;

      case LINKHXOFS:
         (Link.hxofs) = (fix)(value / 10000);
         break;

      case LINKHYOFS:
         (Link.hyofs) = (fix)(value / 10000);
         break;

      case LINKXOFS:
         (Link.xofs) = (fix)(value / 10000);
         break;

      case LINKYOFS:
         (Link.yofs) = (fix)(value / 10000) + PLAYFIELD_OFFSET;
         break;

      case LINKZOFS:
         (Link.zofs) = (fix)(value / 10000);
         break;

      case LINKHXSZ:
         (Link.hxsz) = (fix)(value / 10000);
         break;

      case LINKHYSZ:
         (Link.hysz) = (fix)(value / 10000);
         break;

      case LINKHZSZ:
         (Link.hzsz) = (fix)(value / 10000);
         break;

      case LINKTXSZ:
         (Link.txsz) = (fix)(value / 10000);
         break;

      case LINKTYSZ:
         (Link.tysz) = (fix)(value / 10000);
         break;

      case LINKTILE:
         (Link.tile) = (fix)(value / 10000);
         break;

      case LINKFLIP:
         (Link.flip) = (fix)(value / 10000);
         break;

      ///----------------------------------------------------------------------------------------------------//
      //Input States
      case INPUTSTART:
         StartKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTMAP:
         MapKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTUP:
         UpKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTDOWN:
         DownKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTLEFT:
         LeftKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTRIGHT:
         RightKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTA:
         AKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTB:
         BKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTL:
         LKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTR:
         RKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTEX1:
         Ex1Key = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTEX2:
         Ex2Key = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTEX3:
         Ex3Key = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTEX4:
         Ex4Key = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTAXISUP:
         UpKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTAXISDOWN:
         DownKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTAXISLEFT:
         LeftKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTAXISRIGHT:
         RightKey = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSSTART:
         StartKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSMAP:
         MapKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSUP:
         UpKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSDOWN:
         DownKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSLEFT:
         LeftKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSRIGHT:
         RightKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSA:
         AKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSB:
         BKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSL:
         LKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSR:
         RKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSEX1:
         Ex1KeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSEX2:
         Ex2KeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSEX3:
         Ex3KeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSEX4:
         Ex4KeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSAXISUP:
         UpKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSAXISDOWN:
         DownKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSAXISLEFT:
         LeftKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTPRESSAXISRIGHT:
         RightKeyPress = ((value / 10000) != 0) ? TRUE : FALSE;
         break;

      case INPUTMOUSEX:
         MouseX = value / 10000;
         break;

      case INPUTMOUSEY:
         MouseY = (value / 10000) + PLAYFIELD_OFFSET;
         break;

      case INPUTMOUSEZ:
         MouseZ = value / 10000;
         break;

      ///----------------------------------------------------------------------------------------------------//
      //Item Variables
      case ITEMX:
         if (0 != (s = checkItem(ri->itemref)))
         {
            (s->x) = (fix)(value / 10000);

            // Move the Fairy enemy as well.
            if (itemsbuf[((item *)(s))->id].family == itype_fairy && itemsbuf[((item *)(s))->id].misc3)
               movefairy2(((item *)(s))->x, ((item *)(s))->y, ((item *)(s))->misc);
         }

         break;

      case ITEMY:
         if (0 != (s = checkItem(ri->itemref)))
         {
            (s->y) = (fix)(value / 10000);

            // Move the Fairy enemy as well.
            if (itemsbuf[((item *)(s))->id].family == itype_fairy && itemsbuf[((item *)(s))->id].misc3)
               movefairy2(((item *)(s))->x, ((item *)(s))->y, ((item *)(s))->misc);
         }

         break;

      case ITEMZ:
         if (0 != (s = checkItem(ri->itemref)))
         {
            (s->z) = (fix)(value / 10000);

            if (s->z < 0)
               s->z = 0;
         }

         break;

      case ITEMJUMP:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->fall) = -value * 100.0 / 10000.0;

         break;

      case ITEMDRAWTYPE:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->drawstyle) = value / 10000;

         break;

      case ITEMID:
         if (0 != (s = checkItem(ri->itemref)))
         {
            (((item *)s)->id) = value / 10000;
            flushItemCache();
         }

         break;

      case ITEMTILE:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->tile) = vbound(value / 10000, 0, NEWMAXTILES - 1);

         break;

      case ITEMOTILE:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->o_tile) = vbound(value / 10000, 0, NEWMAXTILES - 1);

         break;

      case ITEMCSET:
         if (0 != (s = checkItem(ri->itemref)))
         {
            (((item *)s)->o_cset) = (((item *)s)->o_cset & ~15) | ((value / 10000) & 15);
            (((item *)s)->cs) = (((item *)s)->o_cset & 15);
         }

         break;

      case ITEMFLASHCSET:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->o_cset) = ((value / 10000) << 4) | (((item *)s)->o_cset & 15);

         break;

      case ITEMFRAMES:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->frames) = value / 10000;

         break;

      case ITEMFRAME:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->aframe) = value / 10000;

         break;

      case ITEMASPEED:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->o_speed) = value / 10000;

         break;

      case ITEMDELAY:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->o_delay) = value / 10000;

         break;

      case ITEMFLIP:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->flip) = value / 10000;

         break;

      case ITEMFLASH:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->flash) = (value / 10000) ? 1 : 0;

         break;

      case ITEMEXTEND:
         if (0 != (s = checkItem(ri->itemref)))
            (((item *)s)->extend) = value / 10000;

         break;

      case ITEMHXOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->hxofs = value / 10000;

         break;

      case ITEMHYOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->hyofs = value / 10000;

         break;

      case ITEMXOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->xofs = (fix)(value / 10000);

         break;

      case ITEMYOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->yofs = (fix)(value / 10000) + PLAYFIELD_OFFSET;

         break;

      case ITEMZOFS:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->zofs = (fix)(value / 10000);

         break;

      case ITEMHXSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->hxsz = value / 10000;

         break;

      case ITEMHYSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->hysz = value / 10000;

         break;

      case ITEMHZSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->hzsz = value / 10000;

         break;

      case ITEMTXSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->txsz = vbound((value / 10000), 1, 20);

         break;

      case ITEMTYSZ:
         if (0 != (s = checkItem(ri->itemref)))
            ((item *)(s))->tysz = vbound((value / 10000), 1, 20);

         break;

      case ITEMPICKUP:
         if (0 != (s = checkItem(ri->itemref)))
         {
            int newpickup = value / 10000;
            // Values that the questmaker should not use, ever
            newpickup &= ~(ipBIGRANGE | ipCHECK | ipMONEY | ipBIGTRI | ipNODRAW | ipFADE);

            // If making an item timeout, set its timer
            if (newpickup & ipFADE)
               (((item *)(s))->clk2) = 512;

            // If making it a carried item,
            // alter hasitem and set an itemguy.
            if ((((item *)(s))->pickup & ipENEMY) < (newpickup & ipENEMY))
            {
               hasitem |= 2;
               bool hasitemguy = false;

               for (int i = 0; i < guys.Count(); i++)
               {
                  if (((enemy *)guys.spr(i))->itemguy)
                     hasitemguy = true;
               }

               if (!hasitemguy && guys.Count() > 0)
                  ((enemy *)guys.spr(guys.Count() - 1))->itemguy = true;
            }
            // If unmaking it a carried item,
            // alter hasitem if there are no more carried items.
            else if ((((item *)(s))->pickup & ipENEMY) > (newpickup & ipENEMY))
            {
               // Move it back onscreen!
               if (get_bit(quest_rules, qr_HIDECARRIEDITEMS))
               {
                  for (int i = 0; i < guys.Count(); i++)
                  {
                     if (((enemy *)guys.spr(i))->itemguy)
                     {
                        ((item *)(s))->x = ((enemy *)guys.spr(i))->x;
                        ((item *)(s))->y = ((enemy *)guys.spr(i))->y;
                        ((item *)(s))->z = ((enemy *)guys.spr(i))->z;
                        break;
                     }
                  }
               }

               if (more_carried_items() <= 1) // 1 includes this own item.
                  hasitem &= ~2;
            }

            ((item *)(s))->pickup = value / 10000;
         }

         break;

      case ITEMMISCD:
         if (0 != (s = checkItem(ri->itemref)))
         {
            int a = vbound(ri->d[0] / 10000, 0, 15);
            (((item *)(s))->miscellaneous[a]) = value;
         }

         break;

      ///----------------------------------------------------------------------------------------------------//
      //Itemdata Variables
      case IDATAFAMILY:
         (itemsbuf[ri->idata].family) = value / 10000;
         flushItemCache();
         break;

      case IDATALEVEL:
         (itemsbuf[ri->idata].fam_type) = value / 10000;
         flushItemCache();
         break;

      case IDATAKEEP:
         (itemsbuf[ri->idata].flags) |= (value / 10000) ? ITEM_GAMEDATA : 0;
         break;

      case IDATAAMOUNT:
         (itemsbuf[ri->idata].amount) = value / 10000;
         break;

      case IDATASETMAX:
         (itemsbuf[ri->idata].setmax) = value / 10000;
         break;

      case IDATAMAX:
         (itemsbuf[ri->idata].max) = value / 10000;
         break;

      case IDATAPOWER:
         (itemsbuf[ri->idata].power) = value / 10000;
         break;

      case IDATACOUNTER:
         (itemsbuf[ri->idata].count) = value / 10000;
         break;

      case IDATAUSESOUND:
         (itemsbuf[ri->idata].usesound) = value / 10000;
         break;

      case IDATAINITDD:
      {
         int a = ri->d[0] / 10000;

         if (BC::checkBounds(a, 0, 7, "itemdata->InitD") == SH::_NoError)
            itemsbuf[ri->idata].initiald[a] = value;
      }
      break;

      ///----------------------------------------------------------------------------------------------------//
      //LWeapon Variables
      case LWPNX:
         if (0 != (s = checkLWpn(ri->lwpn, "X")))
            ((weapon *)s)->x = (fix)(value / 10000);

         break;

      case LWPNY:
         if (0 != (s = checkLWpn(ri->lwpn, "Y")))
            ((weapon *)s)->y = (fix)(value / 10000);

         break;

      case LWPNZ:
         if (0 != (s = checkLWpn(ri->lwpn, "Z")))
            ((weapon *)s)->z = zc_max((fix)(value / 10000), (fix)0);

         break;

      case LWPNJUMP:
         if (0 != (s = checkLWpn(ri->lwpn, "Jump")))
            ((weapon *)s)->fall = ((-value * 100.0) / 10000.0);

         break;

      case LWPNDIR:
         if (0 != (s = checkLWpn(ri->lwpn, "Dir")))
            ((weapon *)s)->dir = (value / 10000);

         break;

      case LWPNSTEP:
         if (0 != (s = checkLWpn(ri->lwpn, "Step")))
            ((weapon *)s)->step = (value / 10000) / 100.0;

         break;

      case LWPNANGLE:
         if (0 != (s = checkLWpn(ri->lwpn, "Angle")))
            ((weapon *)s)->angle = (double)(value / 10000.0);

         break;

      case LWPNANGULAR:
         if (0 != (s = checkLWpn(ri->lwpn, "Angular")))
            ((weapon *)s)->angular = (value / 10000) != 0;

         break;

      case LWPNBEHIND:
         if (0 != (s = checkLWpn(ri->lwpn, "Behind")))
            ((weapon *)s)->behind = (value / 10000) != 0;

         break;

      case LWPNDRAWTYPE:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawStyle")))
            ((weapon *)s)->drawstyle = (value / 10000);

         break;

      case LWPNPOWER:
         if (0 != (s = checkLWpn(ri->lwpn, "Damage")))
            ((weapon *)s)->power = (value / 10000);

         break;

      case LWPNDEAD:
         if (0 != (s = checkLWpn(ri->lwpn, "DeadState")))
            ((weapon *)s)->dead = (value / 10000);

         break;

      case LWPNID:
         if (0 != (s = checkLWpn(ri->lwpn, "ID")))
            ((weapon *)s)->id = (value / 10000);

         break;

      case LWPNTILE:
         if (0 != (s = checkLWpn(ri->lwpn, "Tile")))
            ((weapon *)s)->tile = (value / 10000);

         break;

      case LWPNCSET:
         if (0 != (s = checkLWpn(ri->lwpn, "CSet")))
            ((weapon *)s)->cs = (value / 10000) & 15;

         break;

      case LWPNFLASHCSET:
         if (0 != (s = checkLWpn(ri->lwpn, "FlashCSet")))
            (((weapon *)s)->o_cset) |= (value / 10000) << 4;

         break;

      case LWPNFRAMES:
         if (0 != (s = checkLWpn(ri->lwpn, "NumFrames")))
            ((weapon *)s)->frames = (value / 10000);

         break;

      case LWPNFRAME:
         if (0 != (s = checkLWpn(ri->lwpn, "Frame")))
            ((weapon *)s)->aframe = (value / 10000);

         break;

      case LWPNASPEED:
         if (0 != (s = checkLWpn(ri->lwpn, "ASpeed")))
            ((weapon *)s)->o_speed = (value / 10000);

         break;

      case LWPNFLASH:
         if (0 != (s = checkLWpn(ri->lwpn, "Flash")))
            ((weapon *)s)->flash = (value / 10000);

         break;

      case LWPNFLIP:
         if (0 != (s = checkLWpn(ri->lwpn, "Flip")))
            ((weapon *)s)->flip = (value / 10000);

         break;

      case LWPNEXTEND:
         if (0 != (s = checkLWpn(ri->lwpn, "Extend")))
            ((weapon *)s)->extend = (value / 10000);

         break;

      case LWPNOTILE:
         if (0 != (s = checkLWpn(ri->lwpn, "OriginalTile")))
            ((weapon *)s)->o_tile = (value / 10000);

         break;

      case LWPNOCSET:
         if (0 != (s = checkLWpn(ri->lwpn, "OriginalCSet")))
            (((weapon *)s)->o_cset) |= (value / 10000) & 15;

         break;

      case LWPNHXOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "HitXOffset")))
            (((weapon *)s)->hxofs) = (value / 10000);

         break;

      case LWPNHYOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "HitYOffset")))
            (((weapon *)s)->hyofs) = (value / 10000);

         break;

      case LWPNXOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawXOffset")))
            (((weapon *)s)->xofs) = (fix)(value / 10000);

         break;

      case LWPNYOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawYOffset")))
            (((weapon *)s)->yofs) = (fix)(value / 10000) + PLAYFIELD_OFFSET;

         break;

      case LWPNZOFS:
         if (0 != (s = checkLWpn(ri->lwpn, "DrawZOffset")))
            (((weapon *)s)->zofs) = (fix)(value / 10000);

         break;

      case LWPNHXSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "HitWidth")))
            (((weapon *)s)->hxsz) = (value / 10000);

         break;

      case LWPNHYSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "HitHeight")))
            (((weapon *)s)->hysz) = (value / 10000);

         break;

      case LWPNHZSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "HitZHeight")))
            (((weapon *)s)->hzsz) = (value / 10000);

         break;

      case LWPNTXSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "TileWidth")))
            (((weapon *)s)->txsz) = vbound((value / 10000), 1, 20);

         break;

      case LWPNTYSZ:
         if (0 != (s = checkLWpn(ri->lwpn, "TileHeight")))
            (((weapon *)s)->tysz) = vbound((value / 10000), 1, 20);

         break;

      case LWPNMISCD:
         if (0 != (s = checkLWpn(ri->lwpn, "Misc")))
         {
            int a = vbound(ri->d[0] / 10000, 0, 15);
            (((weapon *)(s))->miscellaneous[a]) = value;
         }

         break;

      case LWPNCOLLDET:
         if (0 != (s = checkLWpn(ri->lwpn, "CollDetection")))
            (((weapon *)(s))->scriptcoldet) = value / 10000;

         break;

      ///----------------------------------------------------------------------------------------------------//
      //EWeapon Variables
      case EWPNX:
         if (0 != (s = checkEWpn(ri->ewpn, "X")))
            ((weapon *)s)->x = (fix)(value / 10000);

         break;

      case EWPNY:
         if (0 != (s = checkEWpn(ri->ewpn, "Y")))
            ((weapon *)s)->y = (fix)(value / 10000);

         break;

      case EWPNZ:
         if (0 != (s = checkEWpn(ri->ewpn, "Z")))
            ((weapon *)s)->z = zc_max((fix)(value / 10000), (fix)0);

         break;

      case EWPNJUMP:
         if (0 != (s = checkEWpn(ri->ewpn, "Jump")))
            ((weapon *)s)->fall = (-value * 100.0 / 10000.0);

         break;

      case EWPNDIR:
         if (0 != (s = checkEWpn(ri->ewpn, "Dir")))
            ((weapon *)s)->dir = (value / 10000);

         break;

      case EWPNSTEP:
         if (0 != (s = checkEWpn(ri->ewpn, "Step")))
            ((weapon *)s)->step = (value / 10000) / 100.0;

         break;

      case EWPNANGLE:
         if (0 != (s = checkEWpn(ri->ewpn, "Angle")))
            ((weapon *)s)->angle = (double)(value / 10000.0);

         break;

      case EWPNANGULAR:
         if (0 != (s = checkEWpn(ri->ewpn, "Angular")))
            ((weapon *)s)->angular = (value / 10000) != 0;

         break;

      case EWPNBEHIND:
         if (0 != (s = checkEWpn(ri->ewpn, "Behind")))
            ((weapon *)s)->behind = (value / 10000) != 0;

         break;

      case EWPNDRAWTYPE:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawStyle")))
            ((weapon *)s)->drawstyle = (value / 10000);

         break;

      case EWPNPOWER:
         if (0 != (s = checkEWpn(ri->ewpn, "Damage")))
            ((weapon *)s)->power = (value / 10000);

         break;

      case EWPNDEAD:
         if (0 != (s = checkEWpn(ri->ewpn, "DeadState")))
            ((weapon *)s)->dead = (value / 10000);

         break;

      case EWPNID:
         if (0 != (s = checkEWpn(ri->ewpn, "ID")))
            ((weapon *)s)->id = (value / 10000);

         break;

      case EWPNTILE:
         if (0 != (s = checkEWpn(ri->ewpn, "Tile")))
            ((weapon *)s)->tile = (value / 10000);

         break;

      case EWPNCSET:
         if (0 != (s = checkEWpn(ri->ewpn, "CSet")))
            ((weapon *)s)->cs = (value / 10000) & 15;

         break;

      case EWPNFLASHCSET:
         if (0 != (s = checkEWpn(ri->ewpn, "FlashCSet")))
            (((weapon *)s)->o_cset) |= (value / 10000) << 4;

         break;

      case EWPNFRAMES:
         if (0 != (s = checkEWpn(ri->ewpn, "NumFrames")))
            ((weapon *)s)->frames = (value / 10000);

         break;

      case EWPNFRAME:
         if (0 != (s = checkEWpn(ri->ewpn, "Frame")))
            ((weapon *)s)->aframe = (value / 10000);

         break;

      case EWPNASPEED:
         if (0 != (s = checkEWpn(ri->ewpn, "ASpeed")))
            ((weapon *)s)->o_speed = (value / 10000);

         break;

      case EWPNFLASH:
         if (0 != (s = checkEWpn(ri->ewpn, "Flash")))
            ((weapon *)s)->flash = (value / 10000);

         break;

      case EWPNFLIP:
         if (0 != (s = checkEWpn(ri->ewpn, "Flip")))
            ((weapon *)s)->flip = (value / 10000);

         break;

      case EWPNEXTEND:
         if (0 != (s = checkEWpn(ri->ewpn, "Extend")))
            ((weapon *)s)->extend = (value / 10000);

         break;

      case EWPNOTILE:
         if (0 != (s = checkEWpn(ri->ewpn, "OriginalTile")))
            ((weapon *)s)->o_tile = (value / 10000);

         break;

      case EWPNOCSET:
         if (0 != (s = checkEWpn(ri->ewpn, "OriginalCSet")))
            (((weapon *)s)->o_cset) |= (value / 10000) & 15;

         break;

      case EWPNHXOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "HitXOffset")))
            (((weapon *)s)->hxofs) = (value / 10000);

         break;

      case EWPNHYOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "HitYOffset")))
            (((weapon *)s)->hyofs) = (value / 10000);

         break;

      case EWPNXOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawXOffset")))
            (((weapon *)s)->xofs) = (fix)(value / 10000);

         break;

      case EWPNYOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawYOffset")))
            (((weapon *)s)->yofs) = (fix)(value / 10000) + PLAYFIELD_OFFSET;

         break;

      case EWPNZOFS:
         if (0 != (s = checkEWpn(ri->ewpn, "DrawZOffset")))
            (((weapon *)s)->zofs) = (fix)(value / 10000);

         break;

      case EWPNHXSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "HitWidth")))
            (((weapon *)s)->hxsz) = (value / 10000);

         break;

      case EWPNHYSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "HitHeight")))
            (((weapon *)s)->hysz) = (value / 10000);

         break;

      case EWPNHZSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "HitZHeight")))
            (((weapon *)s)->hzsz) = (value / 10000);

         break;

      case EWPNTXSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "TileWidth")))
            (((weapon *)s)->txsz) = vbound((value / 10000), 1, 20);

         break;

      case EWPNTYSZ:
         if (0 != (s = checkEWpn(ri->ewpn, "TileHeight")))
            (((weapon *)s)->tysz) = vbound((value / 10000), 1, 20);

         break;

      case EWPNMISCD:
         if (0 != (s = checkEWpn(ri->ewpn, "Misc")))
         {
            int a = vbound(ri->d[0] / 10000, 0, 15);
            (((weapon *)(s))->miscellaneous[a]) = value;
         }

         break;

      case EWPNCOLLDET:
         if (0 != (s = checkEWpn(ri->ewpn, "CollDetection")))
            (((weapon *)(s))->scriptcoldet) = value / 10000;

         break;

      ///----------------------------------------------------------------------------------------------------//
      //NPC Variables

      //Fixs are all a bit different
      case NPCX:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->X") == SH::_NoError)
         {
            GuyH::getNPC()->x = fix(value / 10000);

            if (GuyH::hasLink())
               Link.setX(fix(value / 10000));
         }
      }
      break;

      case NPCY:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->Y") == SH::_NoError)
         {
            fix oldy = GuyH::getNPC()->y;
            GuyH::getNPC()->y = fix(value / 10000);
            GuyH::getNPC()->floor_y += ((value / 10000) - oldy);

            if (GuyH::hasLink())
               Link.setY(fix(value / 10000));
         }
      }
      break;

      case NPCZ:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->Z") == SH::_NoError)
         {
            if (!never_in_air(GuyH::getNPC()->id))
            {
               if (value < 0)
                  GuyH::getNPC()->z = fix(0);
               else
                  GuyH::getNPC()->z = fix(value / 10000);

               if (GuyH::hasLink())
                  Link.setZ(fix(value / 10000));
            }
         }
      }
      break;

      case NPCJUMP:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->Jump") == SH::_NoError)
         {
            if (canfall(GuyH::getNPC()->id))
               GuyH::getNPC()->fall = -fix(value * 100.0 / 10000.0);

            if (GuyH::hasLink())
               Link.setFall(value / fix(10000.0));
         }
      }
      break;

      case NPCSTEP:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->Step") == SH::_NoError)
            GuyH::getNPC()->step = fix(value / 10000) / fix(100.0);
      }
      break;

      case NPCXOFS:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->DrawXOffset") == SH::_NoError)
            GuyH::getNPC()->xofs = fix(value / 10000);
      }
      break;

      case NPCYOFS:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->DrawYOffset") == SH::_NoError)
            GuyH::getNPC()->yofs = fix(value / 10000) + PLAYFIELD_OFFSET;
      }
      break;

      case NPCZOFS:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->DrawZOffset") == SH::_NoError)
            GuyH::getNPC()->zofs = fix(value / 10000);
      }
      break;

#define SET_NPC_VAR_INT(member, str) \
{ \
   if(GuyH::loadNPC(ri->guyref, str) == SH::_NoError) \
      GuyH::getNPC()->member = value / 10000; \
}

      case NPCDIR:
         SET_NPC_VAR_INT(dir, "npc->Dir") break;

      case NPCRATE:
         SET_NPC_VAR_INT(rate, "npc->Rate") break;

      case NPCHOMING:
         SET_NPC_VAR_INT(homing, "npc->Homing") break;

      case NPCFRAMERATE:
         SET_NPC_VAR_INT(frate, "npc->ASpeed") break;

      case NPCHALTRATE:
         SET_NPC_VAR_INT(hrate, "npc->HaltRate") break;

      case NPCDRAWTYPE:
         SET_NPC_VAR_INT(drawstyle, "npc->DrawStyle") break;

      case NPCHP:
         SET_NPC_VAR_INT(hp, "npc->HP") break;

      //case NPCID:        SET_NPC_VAR_INT(id, "npc->ID") break; ~Disallowed
      case NPCDP:
         SET_NPC_VAR_INT(dp, "npc->Damage") break;

      case NPCWDP:
         SET_NPC_VAR_INT(wdp, "npc->WeaponDamage") break;

      case NPCITEMSET:
         SET_NPC_VAR_INT(item_set, "npc->ItemSet") break;

      case NPCBOSSPAL:
         SET_NPC_VAR_INT(bosspal, "npc->BossPal") break;

      case NPCBGSFX:
         if (GuyH::loadNPC(ri->guyref, "npc->SFX") == SH::_NoError)
         {
            enemy *en = GuyH::getNPC();
            int newSFX = value / 10000;

            // Stop the old sound and start the new one
            if (en->bgsfx != newSFX)
            {
               en->stop_bgsfx(GuyH::getNPCIndex(ri->guyref));
               cont_sfx(newSFX);
               en->bgsfx = newSFX;
            }
         }

         break;

      case NPCEXTEND:
         SET_NPC_VAR_INT(extend, "npc->Extend") break;

      case NPCHXOFS:
         SET_NPC_VAR_INT(hxofs, "npc->HitXOffset") break;

      case NPCHYOFS:
         SET_NPC_VAR_INT(hyofs, "npc->HitYOffset") break;

      case NPCHXSZ:
         SET_NPC_VAR_INT(hxsz, "npc->HitWidth") break;

      case NPCHYSZ:
         SET_NPC_VAR_INT(hysz, "npc->HitHeight") break;

      case NPCHZSZ:
         SET_NPC_VAR_INT(hzsz, "npc->HitZHeight") break;

      case NPCCOLLDET:
         SET_NPC_VAR_INT(scriptcoldet, "npc->CollDetection") break;

      case NPCSTUN:
         SET_NPC_VAR_INT(stunclk, "npc->Stun") break;

      case NPCHUNGER:
         SET_NPC_VAR_INT(grumble, "npc->Hunger") break;

      case NPCCSET:
      {
         if (GuyH::loadNPC(ri->guyref, "npc->CSet") == SH::_NoError)
            GuyH::getNPC()->cs = (value / 10000) & 0xF;
      }
      break;

      //Bounds on value
      case NPCTXSZ:
      {
         int32_t height = value / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->TileWidth") == SH::_NoError &&
               BC::checkBounds(height, 0, 20, "npc->TileWidth") == SH::_NoError)
            GuyH::getNPC()->txsz = height;
      }
      break;

      case NPCTYSZ:
      {
         int32_t width = value / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->TileHeight") == SH::_NoError &&
               BC::checkBounds(width, 0, 20, "npc->TileHeight") == SH::_NoError)
            GuyH::getNPC()->tysz = width;
      }
      break;

      case NPCOTILE:
      {
         int32_t tile = value / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->OriginalTile") == SH::_NoError &&
               BC::checkTile(tile, "npc->OriginalTile") == SH::_NoError)
            GuyH::getNPC()->o_tile = tile;
      }
      break;

      case NPCTILE:
      {
         int32_t tile = value / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Tile") == SH::_NoError &&
               BC::checkTile(tile, "npc->Tile") == SH::_NoError)
            GuyH::getNPC()->tile = tile;
      }
      break;

      case NPCWEAPON:
      {
         int32_t weapon = value / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Weapon") == SH::_NoError &&
               BC::checkBounds(weapon, 0, MAXWPNS - 1, "npc->Weapon") == SH::_NoError)
            GuyH::getNPC()->wpn = weapon;
      }
      break;

      //Indexed
      case NPCDEFENSED:
      {
         int32_t a = ri->d[0] / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Defense") == SH::_NoError &&
               BC::checkBounds(a, 0, edefSCRIPT, "npc->Defense") == SH::_NoError)
            GuyH::getNPC()->defense[a] = value / 10000;
      }
      break;

      case NPCMISCD:
      {
         int32_t a = ri->d[0] / 10000;

         if (GuyH::loadNPC(ri->guyref, "npc->Misc") == SH::_NoError &&
               BC::checkMisc(a, "npc->Misc") == SH::_NoError)
            GuyH::getNPC()->miscellaneous[a] = value;

      }

      break;

      ///----------------------------------------------------------------------------------------------------//
      //Game Information
      case GAMEDEATHS:
         game->set_deaths(value / 10000);
         break;

      case GAMECHEAT:
         game->set_cheat(value / 10000);
         break;

      case GAMETIME:
         game->set_time(value);
         break; // Can't multiply by 10000 or the maximum result is too big

      case GAMETIMEVALID:
         game->set_timevalid((value / 10000) ? 1 : 0);
         break;

      case GAMEHASPLAYED:
         game->set_hasplayed((value / 10000) ? 1 : 0);
         break;

      case GAMEGUYCOUNT:
      {
         int mi2 = (currmap * MAPSCRSNORMAL) + (ri->d[0] / 10000);
         game->guys[mi2] = value / 10000;
      }
      break;

      case GAMECONTSCR:
         game->set_continue_scrn(value / 10000);
         break;

      case GAMECONTDMAP:
         game->set_continue_dmap(value / 10000);
         break;

      case GAMEENTRSCR:
         lastentrance = value / 10000;
         break;

      case GAMEENTRDMAP:
         lastentrance_dmap = value / 10000;
         break;

      case GAMECOUNTERD:
         game->set_counter(value / 10000, (ri->d[0]) / 10000);
         break;

      case GAMEMCOUNTERD:
         game->set_maxcounter(value / 10000, (ri->d[0]) / 10000);
         break;

      case GAMEDCOUNTERD:
         game->set_dcounter(value / 10000, (ri->d[0]) / 10000);
         break;

      case GAMEGENERICD:
         game->set_generic(value / 10000, (ri->d[0]) / 10000);
         break;

      case GAMEITEMSD:
         game->set_item((ri->d[0]) / 10000, (value != 0));
         break;

      case GAMELITEMSD:
         game->lvlitems[(ri->d[0]) / 10000] = vbound(value / 10000, 0, 255);
         break;

      case GAMELKEYSD:
         game->lvlkeys[(ri->d[0]) / 10000] = vbound(value / 10000, 0, 255);
         break;

      case SCREENSTATED:
      {
         int mi2 = (currmap * MAPSCRSNORMAL) + currscr;
         (value) ? setmapflag(mi2, 1 << ((ri->d[0]) / 10000)) : unsetmapflag(mi2, 1 << ((ri->d[0]) / 10000));
      }
      break;

      case SCREENSTATEDD:
      {
         int mi2 = ri->d[0] / 10000;
         mi2 -= 8 * (mi2 / MAPSCRS);

         if (BC::checkMapID(mi2 >> 7, "Game->SetScreenState") == SH::_NoError)
            (value) ? setmapflag(mi2, 1 << (ri->d[1] / 10000)) : unsetmapflag(mi2, 1 << (ri->d[1] / 10000), true);
      }
      break;

      case GAMEGUYCOUNTD:
         game->guys[(currmap * MAPSCRSNORMAL) + (ri->d[0] / 10000)] = value / 10000;
         break;

      case GAMECLICKFREEZE:
         //No longer used
         break;

         ///----------------------------------------------------------------------------------------------------//
         //DMap Information

#define SET_DMAP_VAR(member, str) \
   { \
      int ID = ri->d[0] / 10000; \
      if(BC::checkDMapID(ID, str) == SH::_NoError) \
         DMaps[ID].member = value / 10000; \
   }

      case DMAPFLAGSD:
         SET_DMAP_VAR(flags, "Game->DMapFlags") break;

      case DMAPLEVELD:
         SET_DMAP_VAR(level, "Game->DMapLevel") break;

      case DMAPCOMPASSD:
         SET_DMAP_VAR(compass, "Game->DMapCompass") break;

      case DMAPCONTINUED:
         SET_DMAP_VAR(cont, "Game->DMapContinue") break;

      case DMAPMIDID:
      {
         int ID = ri->d[0] / 10000;

         if (BC::checkDMapID(ID, "Game->DMapMIDI") == SH::_NoError)
         {
            switch (value / 10000)
            {
               case -6:
                  DMaps[ID].midi = 2;
                  break; // Dungeon

               case -3:
                  DMaps[ID].midi = 3;
                  break; // Level 9

               case -2:
                  DMaps[ID].midi = 1;
                  break; // Overworld

               case 0:
                  DMaps[ID].midi = 0;
                  break; // None

               default:
                  DMaps[ID].midi = value / 10000 + 3;
            }
         }

         break;
      }

      ///----------------------------------------------------------------------------------------------------//
      //Screen->ComboX
      case COMBODD:
      {
         int pos = (ri->d[0]) / 10000;

         if (pos >= 0 && pos < 176)
         {
            screen_combo_modify_preroutine(tmpscr, pos);
            tmpscr->data[pos] = (value / 10000);
            screen_combo_modify_postroutine(tmpscr, pos);
         }
      }
      break;

      case COMBOCD:
      {
         int pos = (ri->d[0]) / 10000;

         if (pos >= 0 && pos < 176)
         {
            screen_combo_modify_preroutine(tmpscr, pos);
            tmpscr->cset[pos] = (value / 10000) & 15;
            screen_combo_modify_postroutine(tmpscr, pos);
         }
      }
      break;

      case COMBOFD:
      {
         int pos = (ri->d[0]) / 10000;

         if (pos >= 0 && pos < 176)
            tmpscr->sflag[pos] = (value / 10000);
      }
      break;

      case COMBOTD:
      {
         int pos = (ri->d[0]) / 10000;

         if (pos >= 0 && pos < 176)
         {
            // Preprocess each instance of the combo on the screen
            for (int i = 0; i < 176; i++)
            {
               if (tmpscr->data[i] == tmpscr->data[pos])
                  screen_combo_modify_preroutine(tmpscr, i);
            }

            combobuf[tmpscr->data[pos]].type = value / 10000;

            for (int i = 0; i < 176; i++)
            {
               if (tmpscr->data[i] == tmpscr->data[pos])
                  screen_combo_modify_postroutine(tmpscr, i);
            }
         }
      }
      break;

      case COMBOID:
      {
         int pos = (ri->d[0]) / 10000;

         if (pos >= 0 && pos < 176)
            combobuf[tmpscr->data[pos]].flag = value / 10000;
      }
      break;

      case COMBOSD:
      {
         int pos = (ri->d[0]) / 10000;

         if (pos >= 0 && pos < 176)
            combobuf[tmpscr->data[pos]].walk = (value / 10000) & 15;
      }
      break;

      ///----------------------------------------------------------------------------------------------------//
      //Game->SetComboX
      case COMBODDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);

         if (!(pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)) break;

         if (scr == (currmap * MAPSCRS + currscr))
            screen_combo_modify_preroutine(tmpscr, pos);

         TheMaps[scr].data[pos] = value / 10000;

         if (scr == (currmap * MAPSCRS + currscr))
         {
            tmpscr->data[pos] = value / 10000;
            screen_combo_modify_postroutine(tmpscr, pos);
         }

         int layr = whichlayer(scr);

         if (layr > -1)
         {
            //if (layr==(currmap*MAPSCRS+currscr))
            //  screen_combo_modify_preroutine(tmpscr,pos);
            tmpscr2[layr].data[pos] = value / 10000;
            //if (layr==(currmap*MAPSCRS+currscr))
            //  screen_combo_modify_postroutine(tmpscr,pos);
         }
      }
      break;

      case COMBOCDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);

         if (!(pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)) break;

         TheMaps[scr].cset[pos] = (value / 10000) & 15;

         if (scr == (currmap * MAPSCRS + currscr))
            tmpscr->cset[pos] = value / 10000;

         int layr = whichlayer(scr);

         if (layr > -1)
            tmpscr2[layr].cset[pos] = (value / 10000) & 15;
      }
      break;

      case COMBOFDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);

         if (!(pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count)) break;

         TheMaps[scr].sflag[pos] = value / 10000;

         if (scr == (currmap * MAPSCRS + currscr))
            tmpscr->sflag[pos] = value / 10000;

         int layr = whichlayer(scr);

         if (layr > -1)
            tmpscr2[layr].sflag[pos] = value / 10000;
      }
      break;

      case COMBOTDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);

         if (!(pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count))
            break;

         int cdata = TheMaps[scr].data[pos];

         // Preprocess the screen's combos in case the combo changed is present on the screen. -L
         for (int i = 0; i < 176; i++)
         {
            if (tmpscr->data[i] == cdata)
               screen_combo_modify_preroutine(tmpscr, i);
         }

         combobuf[cdata].type = value / 10000;

         for (int i = 0; i < 176; i++)
         {
            if (tmpscr->data[i] == cdata)
               screen_combo_modify_postroutine(tmpscr, i);
         }
      }
      break;

      case COMBOIDM:
      {
         int pos = (ri->d[0]) / 10000;
         int sc = (ri->d[2] / 10000);
         int m = zc_max((ri->d[1] / 10000) - 1, 0);
         int32_t scr = zc_max(m * MAPSCRS + sc, 0);

         if (!(pos >= 0 && pos < 176 && scr >= 0 && sc < MAPSCRS && m < map_count))
            break;

         combobuf[TheMaps[scr].data[pos]].flag = value / 10000;
      }
      break;

      case COMBOSDM:
      {
         int pos = (ri->d[0]) / 10000;
         int32_t scr = (ri->d[1] / 10000) * MAPSCRS + (ri->d[2] / 10000);

         if (pos < 0 || pos >= 176 || scr < 0) break;

         combobuf[TheMaps[scr].data[pos]].walk = (value / 10000) & 15;
      }
      break;

      ///----------------------------------------------------------------------------------------------------//
      //Screen Variables
      case SDD:
      {
         {
            int di2 = ((get_currdmap()) << 7) + get_currscr() - (DMaps[get_currdmap()].type == dmOVERW ? 0 :
                      DMaps[get_currdmap()].xoff);
            set_screen_d(di2, ri->d[0] / 10000, value);
            break;
         }
      }

      case GDD:
         game->global_d[ri->d[0] / 10000] = value;
         break;

      case SDDD:
         set_screen_d((ri->d[0]) / 10000 + ((get_currdmap()) << 7), ri->d[1] / 10000, value);
         break;

      case SDDDD:
         set_screen_d(ri->d[1] / 10000 + ((ri->d[0] / 10000) << 7), ri->d[2] / 10000, value);
         break;

      case SCRDOORD:
         tmpscr->door[ri->d[0] / 10000] = value / 10000;
         putdoor(scrollbuf, 0, ri->d[0] / 10000, value / 10000, true, true);
         break;

      case LIT:
         naturaldark = !value;
         lighting(false, false);
         break;

      case WAVY:
         wavy = value / 10000;
         break;

      case QUAKE:
         quakeclk = value / 10000;
         break;

      //case ROOMTYPE:
      //  tmpscr->room=value/10000; break; //this probably doesn't work too well...
      case ROOMDATA:
         tmpscr->catchall = value / 10000;
         break;

      case PUSHBLOCKCOMBO:
         mblock2.bcombo = value / 10000;
         break;

      case PUSHBLOCKCSET:
         mblock2.cs = value / 10000;
         mblock2.oldcset = value / 10000;
         break;

      case UNDERCOMBO:
         tmpscr->undercombo = value / 10000;
         break;

      case UNDERCSET:
         tmpscr->undercset = value / 10000;
         break;

      ///----------------------------------------------------------------------------------------------------//
      //Misc./Internal
      case SP:
         ri->sp = value / 10000;
         break;

      case SCRIPTRAM:
      case GLOBALRAM:
         ArrayH::setElement(ri->d[0] / 10000, ri->d[1] / 10000, value);
         break;

      case SCRIPTRAMD:
      case GLOBALRAMD:
         ArrayH::setElement(ri->d[0] / 10000, 0, value);
         break;

      case REFFFC:
         ri->ffcref = value / 10000;
         break;

      case REFITEM:
         ri->itemref = value;
         break;

      case REFITEMCLASS:
         ri->idata = value;
         break;

      case REFLWPN:
         ri->lwpn = value;
         break;

      case REFEWPN:
         ri->ewpn = value;
         break;

      case REFNPC:
         ri->guyref = value;
         break;

      default:
      {
         if (arg >= D(0) && arg <= D(7))       ri->d[arg - D(0)] = value;
         else if (arg >= A(0) && arg <= A(1))     ri->a[arg - A(0)] = value;
         else if (arg >= GD(0) && arg <= GD(255)) game->global_d[arg - GD(0)] = value;

         break;
      }
   }
}

///----------------------------------------------------------------------------------------------------//
//                                       ASM Functions                                                 //
///----------------------------------------------------------------------------------------------------//


///----------------------------------------------------------------------------------------------------//
//Internal (to ZScript)

void do_set(const bool v, uint8_t whichFFC)
{
   // Trying to change the current script?
   if (sarg1 == FFSCRIPT && ri->ffcref == whichFFC)
      return;

   int32_t temp = SH::get_arg(sarg2, v);
   set_register(sarg1, temp);
}

void do_push(const bool v)
{
   const int32_t value = SH::get_arg(sarg1, v);
   ri->sp--;
   SH::write_stack(ri->sp, value);
}

void do_pop()
{
   const int32_t value = SH::read_stack(ri->sp);
   ri->sp++;
   set_register(sarg1, value);
}

void do_loadi()
{
   const int32_t stackoffset = get_register(sarg2) / 10000;
   const int32_t value = SH::read_stack(stackoffset);
   set_register(sarg1, value);
}

void do_storei()
{
   const int32_t stackoffset = get_register(sarg2) / 10000;
   const int32_t value = get_register(sarg1);
   SH::write_stack(stackoffset, value);
}

void do_enqueue(const bool)
{
}
void do_dequeue(const bool)
{
}

void do_comp(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);

   if (temp2 >= temp)   ri->scriptflag |= MOREFLAG;
   else                ri->scriptflag &= ~MOREFLAG;

   if (temp2 == temp)   ri->scriptflag |= TRUEFLAG;
   else                ri->scriptflag &= ~TRUEFLAG;
}

void do_allocatemem(const bool v, const bool local, const uint8_t i)
{
   const int32_t size = SH::get_arg(sarg2, v) / 10000;
   uint32_t ptrval;

   if (size <= 0)
   {
      Z_scripterrlog("Array initialized to invalid size of %d", size);
      set_register(sarg1, 0); //Pass back NULL
      return;
   }

   if (local)
   {
      //localRAM[0] is used as an invalid container, so 0 can be the NULL pointer in ZScript
      for (ptrval = 1; localRAM[ptrval].Size() != 0; ptrval++) ;

      if (ptrval >= MAX_ZCARRAY_SIZE)
      {
         Z_scripterrlog("%d local arrays already in use, no more can be allocated", MAX_ZCARRAY_SIZE - 1);
         ptrval = 0;
      }
      else
      {
         ZScriptArray &a = localRAM[ptrval]; //marginally faster for large arrays if we use a reference

         a.Resize(size);

         for (uint32_t j = 0; j < (uint32_t)size; j++)
            a[j] = 0; //initialize array

         // Keep track of which FFC created the array so we know which to deallocate when changing screens
         arrayOwner[ptrval] = i;
      }
   }
   else
   {
      //Globals are only allocated here at first play, otherwise in init_game
      for (ptrval = 0; game->globalRAM[ptrval].Size() != 0; ptrval++) ;

      if (ptrval >= game->globalRAM.size())
      {
         zc_message("Invalid pointer value of %d passed to global allocate", ptrval);
         //this shouldn't happen, unless people are putting ALLOCATEGMEM in their ZASM scripts where they shouldn't be
      }

      ZScriptArray &a = game->globalRAM[ptrval];

      a.Resize(size);

      for (uint32_t j = 0; j < (uint32_t)size; j++)
         a[j] = 0;

      ptrval += MAX_ZCARRAY_SIZE; //so each pointer has a unique value
   }


   set_register(sarg1, ptrval * 10000);
}

void do_deallocatemem()
{
   const int32_t ptrval = get_register(sarg1) / 10000;

   deallocateArray(ptrval);
}

void do_loada(const uint8_t a)
{
   if (ri->a[a] == 0)
   {
      Z_eventlog("Global scripts currently have no A registers");
      return;
   }

   int32_t ffcref = (ri->a[a] / 10000) - 1; //FFC 2

   if (BC::checkFFC(ffcref, "LOAD%i") != SH::_NoError)
      return;

   int32_t reg = get_register(sarg2); //Register in FFC 2

   if (reg >= D(0) || reg <= D(7))
      set_register(sarg1, tmpscr->scriptData[ffcref].d[reg - D(0)]); //get back the info into *sarg1
   else if (reg == A(0) || reg == A(1))
      set_register(sarg1, tmpscr->scriptData[ffcref].a[reg - A(0)]);
   else if (reg == SP)
      set_register(sarg1, tmpscr->scriptData[ffcref].sp * 10000);

   //Can get everything else using REFFFC
}

void do_seta(const uint8_t a)
{
   if (ri->a[a] == 0)
   {
      Z_eventlog("Global scripts currently have no A registers");
      return;
   }

   int32_t ffcref = (ri->a[a] / 10000) - 1; //FFC 2

   if (BC::checkFFC(ffcref, "SETA%i") != SH::_NoError)
      return;

   int32_t reg = get_register(sarg2); //Register in FFC 2

   if (reg >= D(0) || reg <= D(7))
      tmpscr->scriptData[ffcref].d[reg - D(0)] = get_register(sarg1); //Set it to *sarg1
   else if (reg == A(0) || reg == A(1))
      tmpscr->scriptData[ffcref].a[reg - A(0)] = get_register(sarg1);
   else if (reg == SP)
      tmpscr->scriptData[ffcref].sp = get_register(sarg1) / 10000;
}

///----------------------------------------------------------------------------------------------------//
//Mathematical

void do_add(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);

   set_register(sarg1, temp2 + temp);
}

void do_sub(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);

   set_register(sarg1, temp2 - temp);
}

void do_mult(const bool v)
{
   long long temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);

   set_register(sarg1, int32_t((temp * temp2) / 10000));
}

void do_div(const bool v)
{
   long long temp = SH::get_arg(sarg2, v);
   long long temp2 = get_register(sarg1);

   if (temp == 0)
   {
      Z_scripterrlog("Script attempted to divide %ld by zero!", temp2);
      set_register(sarg1, int32_t(sign(temp2) * INT_MAX));
   }
   else
      set_register(sarg1, int32_t((temp2 * 10000) / temp));
}

void do_mod(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);

   if (temp == 0)
   {
      Z_scripterrlog("Script attempted to modulo %ld by zero!", temp2);
      temp = 1;
   }

   set_register(sarg1, temp2 % temp);
}

void do_trig(const bool v, const uint8_t type)
{
   double rangle = (SH::get_arg(sarg2, v) / 10000.0) * PI / 180.0;

   switch (type)
   {
      case 0:
         set_register(sarg1, int32_t(sin(rangle) * 10000.0));
         break;

      case 1:
         set_register(sarg1, int32_t(cos(rangle) * 10000.0));
         break;

      case 2:
         set_register(sarg1, int32_t(tan(rangle) * 10000.0));
         break;
   }
}

void do_asin(const bool v)
{
   double temp = double(SH::get_arg(sarg2, v)) / 10000.0;

   if (temp >= -1 && temp <= 1)
      set_register(sarg1, int32_t(asin(temp) * 10000.0));
   else
   {
      Z_scripterrlog("Script attempted to pass %ld into ArcSin!", temp);
      set_register(sarg1, -10000);
   }
}

void do_acos(const bool v)
{
   double temp = double(SH::get_arg(sarg2, v)) / 10000.0;

   if (temp >= -1 && temp <= 1)
      set_register(sarg1, int32_t(acos(temp) * 10000.0));
   else
   {
      Z_scripterrlog("Script attempted to pass %ld into ArcCos!", temp);
      set_register(sarg1, -10000);
   }
}

void do_arctan()
{
   double xpos = ri->d[0] / 10000.0;
   double ypos = ri->d[1] / 10000.0;

   set_register(sarg1, int32_t(atan2(ypos, xpos) * 10000.0));
}

void do_abs(const bool v)
{
   int32_t temp = SH::get_arg(sarg1, v);
   set_register(sarg1, abs(temp));
}

void do_log10(const bool v)
{
   double temp = double(SH::get_arg(sarg1, v)) / 10000.0;

   if (temp > 0)
      set_register(sarg1, int32_t(log10(temp) * 10000.0));
   else if (temp == 0)
   {
      Z_eventlog("Script tried to calculate log of 0");
      set_register(sarg1, -INT_MAX);
   }
   else
   {
      Z_eventlog("Script tried to calculate log of %f", temp / 10000.0);
      set_register(sarg1, 0);
   }
}

void do_naturallog(const bool v)
{
   double temp = double(SH::get_arg(sarg1, v)) / 10000.0;

   if (temp > 0)
      set_register(sarg1, int32_t(log(temp) * 10000.0));
   else if (temp == 0)
   {
      Z_eventlog("Script tried to calculate ln of 0");
      set_register(sarg1, -INT_MAX);
   }
   else
   {
      Z_eventlog("Script tried to calculate ln of %f", temp / 10000.0);
      set_register(sarg1, 0);
   }
}

void do_min(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);
   set_register(sarg1, zc_min(temp2, temp));
}

void do_max(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   int32_t temp2 = get_register(sarg1);

   set_register(sarg1, zc_max(temp2, temp));
}


void do_rnd(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;

   if (temp > 0)
      set_register(sarg1, (rand() % temp) * 10000);
   else if (temp < 0)
      set_register(sarg1, (rand() % (-temp)) * -10000);
   else
      set_register(sarg1, 0); // Just return 0. (Do not log an error)
}

void do_factorial(const bool v)
{
   int32_t temp;

   if (v)
      return;  //must factorial a register, not a value (why is this exactly? ~Joe123)
   else
   {
      temp = get_register(sarg1) / 10000;

      if (temp < 2)
      {
         set_register(sarg1, temp >= 0 ? 10000 : 00000);
         return;
      }
   }

   int32_t temp2 = 1;

   for (int32_t temp3 = temp; temp > 1; temp--)
      temp2 *= temp3;

   set_register(sarg1, temp2 * 10000);
}

void do_power(const bool v)
{
   double temp = double(SH::get_arg(sarg2, v)) / 10000.0;
   double temp2 = double(get_register(sarg1)) / 10000.0;

   if (temp == 0 && temp2 == 0)
   {
      Z_scripterrlog("Script attempted to calculate 0 to the power 0!");
      set_register(sarg1, 1);
      return;
   }

   set_register(sarg1, int32_t(pow(temp2, temp) * 10000.0));
}

void do_ipower(const bool v)
{
   double temp = 10000.0 / double(SH::get_arg(sarg2, v));
   double temp2 = double(get_register(sarg1)) / 10000.0;

   if (temp == 0 && temp2 == 0)
   {
      Z_scripterrlog("Script attempted to calculate 0 to the power 0!");
      set_register(sarg1, 1);
      return;
   }

   set_register(sarg1, int32_t(pow(temp2, temp) * 10000.0));
}

void do_sqroot(const bool v)
{
   double temp = double(SH::get_arg(sarg2, v)) / 10000.0;

   if (temp < 0)
   {
      Z_scripterrlog("Script attempted to calculate square root of %ld!", temp);
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, int32_t(sqrt(temp) * 10000.0));
}

///----------------------------------------------------------------------------------------------------//
//Bitwise

void do_and(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (temp2 & temp) * 10000);
}

void do_or(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (temp2 | temp) * 10000);
}

void do_xor(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (temp2 ^ temp) * 10000);
}

void do_nand(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (~(temp2 & temp)) * 10000);
}

void do_nor(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (~(temp2 | temp)) * 10000);
}

void do_xnor(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (~(temp2 ^ temp)) * 10000);
}

void do_not(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v);
   set_register(sarg1, !temp);
}

void do_bitwisenot(const bool v)
{
   int32_t temp = SH::get_arg(sarg1, v) / 10000;
   set_register(sarg1, (~temp) * 10000);
}

void do_lshift(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (temp2 << temp) * 10000);
}

void do_rshift(const bool v)
{
   int32_t temp = SH::get_arg(sarg2, v) / 10000;
   int32_t temp2 = get_register(sarg1) / 10000;
   set_register(sarg1, (temp2 >> temp) * 10000);
}

///----------------------------------------------------------------------------------------------------//
//Gameplay functions

void do_warp(bool v)
{
   tmpscr->sidewarpdmap[0] = SH::get_arg(sarg1, v) / 10000;
   tmpscr->sidewarpscr[0]  = SH::get_arg(sarg2, v) / 10000;
   tmpscr->sidewarptype[0] = wtIWARP;
   Link.ffwarp = true;
}

void do_pitwarp(bool v)
{
   tmpscr->sidewarpdmap[0] = SH::get_arg(sarg1, v) / 10000;
   tmpscr->sidewarpscr[0]  = SH::get_arg(sarg2, v) / 10000;
   tmpscr->sidewarptype[0] = wtIWARP;
   Link.ffwarp = true;
   Link.ffpit = true;
}

void do_breakshield()
{
   int32_t UID = get_register(sarg1);

   for (int j = 0; j < guys.Count(); j++)
      if (guys.spr(j)->getUID() == UID)
      {
         ((enemy *)guys.spr(j))->break_shield();
         return;
      }
}

void do_showsavescreen()
{
   bool saved = save_game(false, 0);
   set_register(sarg1, saved ? 10000 : 0);
}

void do_selectweapon(bool v, bool Abtn)
{
   if (Abtn && !get_bit(quest_rules, qr_SELECTAWPN))
      return;

   uint8_t dir = (uint8_t)(SH::get_arg(sarg1, v) / 10000);

   // Selection directions don't match the normal ones...
   switch (dir)
   {
      case 0:
         dir = SEL_UP;
         break;

      case 1:
         dir = SEL_DOWN;
         break;

      case 2:
         dir = SEL_LEFT;
         break;

      case 3:
         dir = SEL_RIGHT;
         break;

      default:
         return;
   }

   if (Abtn)
      selectNextAWpn(dir);
   else
      selectNextBWpn(dir);
}

///----------------------------------------------------------------------------------------------------//
//Screen Information

void do_issolid()
{
   int x = int(ri->d[0] / 10000);
   int y = int(ri->d[1] / 10000);

   set_register(sarg1, (_walkflag(x, y, 1) ? 10000 : 0));
}

void do_setsidewarp()
{
   int32_t warp   = SH::read_stack(ri->sp + 3) / 10000;
   int32_t scrn = SH::read_stack(ri->sp + 2) / 10000;
   int32_t dmap   = SH::read_stack(ri->sp + 1) / 10000;
   int32_t type   = SH::read_stack(ri->sp + 0) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->SetSideWarp") != SH::_NoError ||
         BC::checkBounds(scrn, -1, 0x87, "Screen->SetSideWarp") != SH::_NoError ||
         BC::checkBounds(dmap, -1, MAXDMAPS - 1, "Screen->SetSideWarp") != SH::_NoError ||
         BC::checkBounds(type, -1, wtMAX - 1, "Screen->SetSideWarp") != SH::_NoError)
      return;

   if (scrn > -1)
      tmpscr->sidewarpscr[warp] = scrn;

   if (dmap > -1)
      tmpscr->sidewarpdmap[warp] = dmap;

   if (type > -1)
      tmpscr->sidewarptype[warp] = type;
}

void do_settilewarp()
{
   int32_t warp   = SH::read_stack(ri->sp + 3) / 10000;
   int32_t scrn = SH::read_stack(ri->sp + 2) / 10000;
   int32_t dmap   = SH::read_stack(ri->sp + 1) / 10000;
   int32_t type   = SH::read_stack(ri->sp + 0) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->SetTileWarp") != SH::_NoError ||
         BC::checkBounds(scrn, -1, 0x87, "Screen->SetTileWarp") != SH::_NoError ||
         BC::checkBounds(dmap, -1, MAXDMAPS - 1, "Screen->SetTileWarp") != SH::_NoError ||
         BC::checkBounds(type, -1, wtMAX - 1, "Screen->SetTileWarp") != SH::_NoError)
      return;

   if (scrn > -1)
      tmpscr->tilewarpscr[warp] = scrn;

   if (dmap > -1)
      tmpscr->tilewarpdmap[warp] = dmap;

   if (type > -1)
      tmpscr->tilewarptype[warp] = type;
}

void do_getsidewarpdmap(const bool v)
{
   int32_t warp = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->GetSideWarpDMap") != SH::_NoError)
   {
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, tmpscr->sidewarpdmap[warp] * 10000);
}

void do_getsidewarpscr(const bool v)
{
   int32_t warp = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->GetSideWarpScreen") != SH::_NoError)
   {
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, tmpscr->sidewarpscr[warp] * 10000);
}

void do_getsidewarptype(const bool v)
{
   int32_t warp = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->GetSideWarpType") != SH::_NoError)
   {
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, tmpscr->sidewarptype[warp] * 10000);
}

void do_gettilewarpdmap(const bool v)
{
   int32_t warp = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->GetTileWarpDMap") != SH::_NoError)
   {
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, tmpscr->tilewarpdmap[warp] * 10000);
}

void do_gettilewarpscr(const bool v)
{
   int32_t warp = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->GetTileWarpScreen") != SH::_NoError)
   {
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, tmpscr->tilewarpscr[warp] * 10000);
}

void do_gettilewarptype(const bool v)
{
   int32_t warp = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(warp, -1, 3, "Screen->GetTileWarpType") != SH::_NoError)
   {
      set_register(sarg1, -10000);
      return;
   }

   set_register(sarg1, tmpscr->tilewarptype[warp] * 10000);
}

void do_layerscreen()
{
   int32_t layer = (get_register(sarg2) / 10000) - 1;

   if (BC::checkBounds(layer, 0, 5, "Screen->LayerScreen") != SH::_NoError ||
         tmpscr->layermap[layer] == 0)
      set_register(sarg1, -10000);
   else
      set_register(sarg1, tmpscr->layerscreen[layer] * 10000);
}

void do_layermap()
{
   int32_t layer = (get_register(sarg2) / 10000) - 1;

   if (BC::checkBounds(layer, 0, 5, "Screen->LayerMap") != SH::_NoError ||
         tmpscr->layermap[layer] == 0)
      set_register(sarg1, -10000);
   else
      set_register(sarg1, tmpscr->layermap[layer] * 10000);
}

void do_triggersecrets()
{
   hidden_entrance(0, true, false, -4);
}

void do_getscreenflags()
{
   int32_t map     = (ri->d[2] / 10000) - 1;
   int32_t scrn  = ri->d[1] / 10000;
   int32_t flagset = ri->d[0] / 10000;

   if (BC::checkMapID(map, "Game->GetScreenFlags") != SH::_NoError ||
         BC::checkBounds(scrn, 0, 0x87, "Game->GetScreenFlags") != SH::_NoError ||
         BC::checkBounds(flagset, 0, 9, "Game->GetScreenFlags") != SH::_NoError)
      return;

   set_register(sarg1, get_screenflags(&TheMaps[map * MAPSCRS + scrn], flagset));
}

void do_getscreeneflags()
{
   int32_t map     = (ri->d[2] / 10000) - 1;
   int32_t scrn  = ri->d[1] / 10000;
   int32_t flagset = ri->d[0] / 10000;

   if (BC::checkMapID(map, "Game->GetScreenEFlags") != SH::_NoError ||
         BC::checkBounds(scrn, 0, 0x87, "Game->GetScreenEFlags") != SH::_NoError ||
         BC::checkBounds(flagset, 0, 9, "Game->GetScreenEFlags") != SH::_NoError)
      return;

   set_register(sarg1, get_screeneflags(&TheMaps[map * MAPSCRS + scrn], flagset));
}

///----------------------------------------------------------------------------------------------------//
//Pointer handling

void do_isvaliditem()
{
   int32_t IID = get_register(sarg1);

   for (int j = 0; j < items.Count(); j++)
      if (items.spr(j)->getUID() == IID)
      {
         set_register(sarg1, 10000);
         return;
      }

   set_register(sarg1, 0);
}

void do_isvalidnpc()
{
   int32_t UID = get_register(sarg1);

   for (int j = 0; j < guys.Count(); j++)
      if (guys.spr(j)->getUID() == UID)
      {
         set_register(sarg1, 10000);
         return;
      }

   set_register(sarg1, 0);
}

void do_isvalidlwpn()
{
   int32_t WID = get_register(sarg1);

   for (int j = 0; j < Lwpns.Count(); j++)
      if (Lwpns.spr(j)->getUID() == WID)
      {
         set_register(sarg1, 10000);
         return;
      }

   set_register(sarg1, 0);
}

void do_isvalidewpn()
{
   int32_t WID = get_register(sarg1);

   for (int j = 0; j < Ewpns.Count(); j++)
      if (Ewpns.spr(j)->getUID() == WID)
      {
         set_register(sarg1, 10000);
         return;
      }

   set_register(sarg1, 0);
}

void do_lwpnusesprite(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkWeaponMiscSprite(ID, "lweapon->UseSprite") != SH::_NoError)
      return;

   if (LwpnH::loadWeapon(ri->lwpn, "lweapon->UseSprite") == SH::_NoError)
      LwpnH::getWeapon()->LOADGFX(ID);
}

void do_ewpnusesprite(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkWeaponMiscSprite(ID, "eweapon->UseSprite") != SH::_NoError)
      return;

   if (EwpnH::loadWeapon(ri->ewpn, "eweapon->UseSprite") == SH::_NoError)
      EwpnH::getWeapon()->LOADGFX(ID);
}

void do_clearsprites(const bool v)
{
   int32_t spritelist = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkBounds(spritelist, 0, 5, "Screen->ClearSprites") != SH::_NoError)
      return;

   switch (spritelist)
   {
      case 0:
         guys.clear();
         break;

      case 1:
         items.clear();
         break;

      case 2:
         Ewpns.clear();
         break;

      case 3:
         Lwpns.clear();
         Link.reset_hookshot();
         break;

      case 4:
         decorations.clear();
         break;

      case 5:
         particles.clear();
         break;
   }
}

void do_loadlweapon(const bool v)
{
   int32_t index = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkLWeaponIndex(index, "Screen->LoadLWeapon") != SH::_NoError)
      ri->lwpn = INT_MAX;
   else
   {
      ri->lwpn = Lwpns.spr(index)->getUID();
   }
}

void do_loadeweapon(const bool v)
{
   int32_t index = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkEWeaponIndex(index, "Screen->LoadEWeapon") != SH::_NoError)
      ri->ewpn = INT_MAX;
   else
   {
      ri->ewpn = Ewpns.spr(index)->getUID();
   }
}

void do_loaditem(const bool v)
{
   int32_t index = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkItemIndex(index, "Screen->LoadItem") != SH::_NoError)
      ri->itemref = INT_MAX;
   else
   {
      ri->itemref = items.spr(index)->getUID();
   }
}

void do_loaditemdata(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;

   //I *think* this is the right check ~Joe
   if (BC::checkItemID(ID, "Game->LoadItemData") != SH::_NoError)
      return;

   ri->idata = ID;
}

void do_loadnpc(const bool v)
{
   int32_t index = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkGuyIndex(index, "Screen->LoadNPC") != SH::_NoError)
      ri->guyref = INT_MAX;
   else
   {
      ri->guyref = guys.spr(index)->getUID();
   }
}

void addLwpn(int x, int y, int z, int id, int type, int power, int dir, int parentid)
{
   Lwpns.add(new weapon((fix)x, (fix)y, (fix)z, id, type, power, dir, -1, parentid));
}

void do_createlweapon(const bool v)
{
   const int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkWeaponID(ID, "Screen->CreateLWeapon") != SH::_NoError)
      return;

   addLwpn(0, 0, 0, ID, 0, 0, 0, Link.getUID());

   if (Lwpns.Count() < 1)
   {
      ri->lwpn = INT_MAX;
      Z_scripterrlog("Couldn't create lweapon %ld, screen lweapon limit reached", ID);
   }
   else
   {
      ri->lwpn = Lwpns.spr(Lwpns.Count() - 1)->getUID();
      Z_eventlog("Script created lweapon %ld with UID = %ld", ID, ri->lwpn);
   }
}

void do_createeweapon(const bool v)
{
   const int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkWeaponID(ID, "Screen->CreateEWeapon") != SH::_NoError)
      return;

   addEwpn(0, 0, 0, ID, 0, 0, 0, -1);

   if (Ewpns.Count() < 1)
   {
      ri->ewpn = INT_MAX;
      Z_scripterrlog("Couldn't create eweapon %ld, screen eweapon limit reached", ID);
   }
   else
   {
      ri->ewpn = Ewpns.spr(Ewpns.Count() - 1)->getUID();
      Z_eventlog("Script created eweapon %ld with UID = %ld", ID, ri->ewpn);
   }
}

void do_createitem(const bool v)
{
   const int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkItemID(ID, "Screen->CreateItem") != SH::_NoError)
      return;

   additem(0, (get_bit(quest_rules, qr_NOITEMOFFSET) ? 1 : 0), ID, ipBIGRANGE);

   if (items.Count() < 1)
   {
      ri->itemref = INT_MAX;
      Z_scripterrlog("Couldn't create item \"%s\", screen item limit reached", item_string[ID]);
   }
   else
   {
      ri->itemref = items.spr(items.Count() - 1)->getUID();
      Z_eventlog("Script created item \"%s\" with UID = %ld", item_string[ID], ri->itemref);
   }
}

void do_createnpc(const bool v)
{
   const int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkGuyID(ID, "Screen->CreateNPC") != SH::_NoError)
      return;

   //If we make a segmented enemy there'll be more than one sprite created
   uint16_t numcreated = addenemy(0, 0, ID, -10);

   if (numcreated == 0)
   {
      ri->guyref = INT_MAX;
      Z_scripterrlog("Couldn't create NPC \"%s\", screen NPC limit reached", guy_string[ID]);
   }
   else
   {
      uint16_t index = guys.Count() - numcreated; //Get the main enemy, not a segment
      ri->guyref = guys.spr(index)->getUID();

      for (; index < guys.Count(); index++)
         ((enemy *)guys.spr(index))->script_spawned = true;

      Z_eventlog("Script created NPC \"%s\" with UID = %ld", guy_string[ID], ri->guyref);
   }
}

///----------------------------------------------------------------------------------------------------//
//Drawing & Sound

void do_message(const bool v)
{
   const int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkMessage(ID, "Screen->Message") != SH::_NoError)
      return;

   if (ID == 0)
   {
      dismissmsg();
      msgfont = zfont;
      blockpath = false;
      Link.finishedmsg();
   }
   else
      donewmsg(ID);
}

inline void set_drawing_command_args(const int j, const uint16_t numargs)
{
   for (int k = 1; k <= numargs; k++)
      script_drawing_commands[j][k] = SH::read_stack(ri->sp + (numargs - k));
}

void do_drawing_command(const int script_command)
{
   int j = script_drawing_commands.GetNext();

   if (j == -1) //out of drawing command space
   {
      Z_scripterrlog("Max draw primitive limit reached");
      return;
   }

   script_drawing_commands[j][0] = script_command;
   script_drawing_commands[j][18] = zscriptDrawingRenderTarget->GetCurrentRenderTarget(); // no fixed bs.

   switch (script_command)
   {
      case RECTR:
         set_drawing_command_args(j, 12);
         break;

      case CIRCLER:
         set_drawing_command_args(j, 11);
         break;

      case ARCR:
         set_drawing_command_args(j, 14);
         break;

      case ELLIPSER:
         set_drawing_command_args(j, 12);
         break;

      case LINER:
         set_drawing_command_args(j, 11);
         break;

      case PUTPIXELR:
         set_drawing_command_args(j, 8);
         break;

      case DRAWTILER:
         set_drawing_command_args(j, 15);
         break;

      case DRAWCOMBOR:
         set_drawing_command_args(j, 16);
         break;

      case FASTTILER:
         set_drawing_command_args(j, 6);
         break;

      case FASTCOMBOR:
         set_drawing_command_args(j, 6);
         break;

      case DRAWCHARR:
         set_drawing_command_args(j, 10);
         break;

      case DRAWINTR:
         set_drawing_command_args(j, 11);
         break;

      case SPLINER:
         set_drawing_command_args(j, 11);
         break;

      case QUADR:
         set_drawing_command_args(j, 15);
         break;

      case TRIANGLER:
         set_drawing_command_args(j, 13);
         break;

      case BITMAPR:
         set_drawing_command_args(j, 12);
         break;

      case DRAWLAYERR:
         set_drawing_command_args(j, 8);
         break;

      case DRAWSCREENR:
         set_drawing_command_args(j, 6);
         break;

      case QUAD3DR:
      {
         std::vector<int32_t> *v = script_drawing_commands.GetVector();
         v->resize(26, 0);

         int32_t *pos = &v->at(0);
         int32_t *uv = &v->at(12);
         int32_t *col = &v->at(20);
         int32_t *size = &v->at(24);

         set_drawing_command_args(j, 8);
         ArrayH::getValues(script_drawing_commands[j][2] / 10000, pos, 12);
         ArrayH::getValues(script_drawing_commands[j][3] / 10000, uv, 8);
         ArrayH::getValues(script_drawing_commands[j][4] / 10000, col, 4);
         ArrayH::getValues(script_drawing_commands[j][5] / 10000, size, 2);

         script_drawing_commands[j].SetVector(v);
      }
      break;

      case TRIANGLE3DR:
      {
         std::vector<int32_t> *v = script_drawing_commands.GetVector();
         v->resize(20, 0);

         int32_t *pos = &v->at(0);
         int32_t *uv = &v->at(9);
         int32_t *col = &v->at(15);
         int32_t *size = &v->at(18);

         set_drawing_command_args(j, 8);
         ArrayH::getValues(script_drawing_commands[j][2] / 10000, pos, 8);
         ArrayH::getValues(script_drawing_commands[j][3] / 10000, uv, 6);
         ArrayH::getValues(script_drawing_commands[j][4] / 10000, col, 3);
         ArrayH::getValues(script_drawing_commands[j][5] / 10000, size, 2);

         script_drawing_commands[j].SetVector(v);
      }
      break;

      case DRAWSTRINGR:
      {
         set_drawing_command_args(j, 9);
         // Unused
         //const int index = script_drawing_commands[j][19] = j;

         string *str = script_drawing_commands.GetString();
         ArrayH::getString(script_drawing_commands[j][8] / 10000, *str);
         script_drawing_commands[j].SetString(str);
      }
      break;
   }
}

void do_set_rendertarget(bool)
{
   int target = int(SH::read_stack(ri->sp) / 10000);
   zscriptDrawingRenderTarget->SetCurrentRenderTarget(target);
}

void do_sfx(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkSFXID(ID, "Game->PlaySound") != SH::_NoError)
      return;

   sfx(ID);
}

void do_midi(bool v)
{
   int32_t MIDI = SH::get_arg(sarg1, v) / 10000;

   if (MIDI == 0)
      music_stop();
   else
      jukebox(MIDI + (MID_COUNT - 1));
}

void do_enh_music(bool v)
{
   int32_t arrayptr = SH::get_arg(sarg1, v) / 10000;
   int32_t track = (SH::get_arg(sarg2, v) / 10000) - 1;

   if (arrayptr == 0)
      music_stop();
   else // Pointer to a string..
   {
      string filename_str;
      char filename_char[256];
      bool ret;
      ArrayH::getString(arrayptr, filename_str, 256);
      strncpy(filename_char, filename_str.c_str(), 255);
      filename_char[255] = '\0';
      ret = play_zcmusic(filename_char, track);
      set_register(sarg2, ret ? 10000 : 0);
   }
}

void do_get_enh_music_filename(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;
   int32_t arrayptr = get_register(sarg2) / 10000;

   if (BC::checkDMapID(ID, "Game->GetDMapMusicFilename") != SH::_NoError)
      return;

   if (ArrayH::setArray(arrayptr, string(DMaps[ID].tmusic)) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'Game->GetDMapMusicFilename' not large enough");
}

void do_get_enh_music_track(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkDMapID(ID, "Game->GetDMapMusicTrack") != SH::_NoError)
      return;

   set_register(sarg1, (DMaps[ID].tmusictrack + 1) * 10000);
}

void do_set_dmap_enh_music(const bool v)
{
   int32_t ID   = SH::read_stack(ri->sp + 2) / 10000;
   int32_t arrayptr = SH::read_stack(ri->sp + 1) / 10000;
   int32_t track = (SH::read_stack(ri->sp + 0) / 10000) - 1;
   string filename_str;

   if (BC::checkDMapID(ID, "Game->SetDMapEnhancedMusic") != SH::_NoError)
      return;


   ArrayH::getString(arrayptr, filename_str, sizeof(DMaps[ID].tmusic));
   strncpy(DMaps[ID].tmusic, filename_str.c_str(), sizeof(DMaps[ID].tmusic));
   DMaps[ID].tmusic[sizeof(DMaps[ID].tmusic) - 1] = '\0';
   DMaps[ID].tmusictrack = track;
}

///----------------------------------------------------------------------------------------------------//
//Tracing

void do_trace(bool v)
{
   int32_t temp = SH::get_arg(sarg1, v);

   char tmp[100];
   sprintf(tmp, (temp < 0 ? "%06d" : "%05d"), temp);
   string s2(tmp);
   s2 = s2.substr(0, s2.size() - 4) + "." + s2.substr(s2.size() - 4, 4);
   zc_message("%s", s2.c_str());
}

void do_tracebool(const bool v)
{
   int32_t temp = SH::get_arg(sarg1, v);

   zc_message("%s", temp ? "true" : "false");
}

void do_tracestring()
{
   int32_t arrayptr = get_register(sarg1) / 10000;
   string str;
   ArrayH::getString(arrayptr, str, 512);
   zc_message("%s", str.c_str());
}

void do_tracenl()
{
   zc_message("");
}

void do_cleartrace()
{
   /* zc_trace_clear();*/
}

string inttobase(uint16_t base, int32_t x, uint16_t mindigits)
{
   static const char coeff[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

   string s2;
   uint16_t digits = zc_max(mindigits - 1, uint16_t(floor(log(double(x)) / log(double(base)))));

   for (int i = digits; i >= 0; i--)
      s2 += coeff[uint16_t(floor(x / pow(double(base), i))) % base];

   return s2;
}

void do_tracetobase()
{
   int32_t x = SH::read_stack(ri->sp + 2) / 10000;
   uint32_t base = vbound(SH::read_stack(ri->sp + 1) / 10000, 2, 36);
   uint32_t mindigits = zc_max(1, SH::read_stack(ri->sp) / 10000);

   string s2 = x < 0 ? "-" : "";

   switch (base)
   {
      case 8:
         s2 += '0';
         break;

      case 16:
         s2 += "0x";
         break;
   }

   s2 += inttobase(base, int(fabs(double(x))), mindigits);

   switch (base)
   {
      case 8:
      case 10:
      case 16:
         break;

      case 2:
         s2 += 'b';
         break;

      default:
         char tmp[32];
         sprintf(tmp, " (Base %d)", base);
         s2 += tmp;
         break;
   }

   zc_message("%s", s2.c_str());
}

///----------------------------------------------------------------------------------------------------//
//Array & string related

void do_arraysize()
{
   int32_t arrayptr = get_register(sarg1) / 10000;
   set_register(sarg1, ArrayH::getSize(arrayptr) * 10000);
}

void do_getsavename()
{
   int32_t arrayptr = get_register(sarg1) / 10000;

   if (ArrayH::setArray(arrayptr, string(game->get_name())) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'Game->GetSaveName' not large enough");
}

void do_setsavename()
{
   int32_t arrayptr = get_register(sarg1) / 10000;

   string str;
   ArrayH::getString(arrayptr, str);
   uint8_t j;

   for (j = 0; str[j] != '\0'; j++)
   {
      if (j >= 8)
      {
         Z_scripterrlog("String supplied to 'Game->GetSaveName' too large");
         break;
      }

      game->get_name()[j] = str[j];
   }

   game->get_name()[j] = '\0';
}

void do_getmessage(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;
   int32_t arrayptr = get_register(sarg2) / 10000;

   if (BC::checkMessage(ID, "Game->GetMessage") != SH::_NoError)
      return;

   if (ArrayH::setArray(arrayptr, string(MsgStrings[ID].s)) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'Game->GetMessage' not large enough");
}

void do_getdmapname(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;
   int32_t arrayptr = get_register(sarg2) / 10000;

   if (BC::checkDMapID(ID, "Game->GetDMapName") != SH::_NoError)
      return;

   if (ArrayH::setArray(arrayptr, string(DMaps[ID].name)) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'Game->GetDMapName' not large enough");
}

void do_getdmaptitle(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;
   int32_t arrayptr = get_register(sarg2) / 10000;

   if (BC::checkDMapID(ID, "Game->GetDMapTitle") != SH::_NoError)
      return;

   if (ArrayH::setArray(arrayptr, string(DMaps[ID].title)) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'Game->GetDMapTitle' not large enough");
}

void do_getdmapintro(const bool v)
{
   int32_t ID = SH::get_arg(sarg1, v) / 10000;
   int32_t arrayptr = get_register(sarg2) / 10000;

   if (BC::checkDMapID(ID, "Game->GetDMapIntro") != SH::_NoError)
      return;

   if (ArrayH::setArray(arrayptr, string(DMaps[ID].intro)) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'Game->GetDMapIntro' not large enough");
}

void do_getitemname()
{
   int32_t arrayptr = get_register(sarg1) / 10000;

   if (ArrayH::setArray(arrayptr, item_string[ri->idata]) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'itemdata->GetName' not large enough");
}

void do_getnpcname()
{
   int32_t arrayptr = get_register(sarg1) / 10000;

   if (GuyH::loadNPC(ri->guyref, "npc->GetName") != SH::_NoError)
      return;

   uint16_t ID = (GuyH::getNPC()->id & 0xFFF);

   if (ArrayH::setArray(arrayptr, guy_string[ID]) == SH::_Overflow)
      Z_scripterrlog("Array supplied to 'npc->GetName' not large enough");
}

void do_getffcscript()
{
   int32_t arrayptr = get_register(sarg1) / 10000;
   string name;
   int num = -1;
   ArrayH::getString(arrayptr, name, 256); // What's the limit on name length?

   for (int i = 0; i < 512; i++)
   {
      if (strcmp(name.c_str(), ffcmap[i].second.c_str()) == 0)
      {
         num = i + 1;
         break;
      }
   }

   set_register(sarg1, num * 10000);
}

///----------------------------------------------------------------------------------------------------//
//Tile Manipulation

void do_copytile(const bool v, const bool v2)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;
   int32_t tile2 = SH::get_arg(sarg2, v2) / 10000;

   copy_tile(newtilebuf, tile, tile2, false);
}

void do_swaptile(const bool v, const bool v2)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;
   int32_t tile2 = SH::get_arg(sarg2, v2) / 10000;

   copy_tile(newtilebuf, tile, tile2, true);
}

void do_overlaytile(const bool v, const bool v2)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;
   int32_t tile2 = SH::get_arg(sarg2, v2) / 10000;

   if (BC::checkTile(tile, "OverlayTile") != SH::_NoError ||
         BC::checkTile(tile2, "OverlayTile") != SH::_NoError)
      return;

   //Could add an arg for the CSet or something instead of just passing 0, currently only 8-bit is supported
   overlay_tile(newtilebuf, tile, tile2, 0, false);
}

void do_fliprotatetile(const bool v, const bool v2)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;
   int32_t tile2 = SH::get_arg(sarg2, v2) / 10000;

   if (BC::checkTile(tile, "FlipRotateTile") != SH::_NoError ||
         BC::checkTile(tile2, "FlipRotateTile") != SH::_NoError)
      return;

   //fliprotatetile
}

void do_settilepixel(const bool v)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkTile(tile, "SetTilePixel") != SH::_NoError)
      return;

   //settilepixel
}

void do_gettilepixel(const bool v)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkTile(tile, "GetTilePixel") != SH::_NoError)
      return;

   //gettilepixel
}

void do_shifttile(const bool v, const bool v2)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;
   int32_t tile2 = SH::get_arg(sarg2, v2) / 10000;

   if (BC::checkTile(tile, "ShiftTile") != SH::_NoError ||
         BC::checkTile(tile2, "ShiftTile") != SH::_NoError)
      return;

   //shifttile
}

void do_cleartile(const bool v)
{
   int32_t tile = SH::get_arg(sarg1, v) / 10000;

   if (BC::checkTile(tile, "ClearTile") != SH::_NoError)
      return;

   reset_tile(newtilebuf, tile, newtilebuf[tile].format);
}

void do_combotile(const bool v)
{
   int32_t combo = SH::get_arg(sarg2, v) / 10000;

   if (BC::checkCombo(combo, "Game->ComboTile") != SH::_NoError)
      return;

   set_register(sarg1, combobuf[combo].tile * 10000);
}


///----------------------------------------------------------------------------------------------------//
//                                       Run the script                                                //
///----------------------------------------------------------------------------------------------------//

// Let's do this
int run_script(const uint8_t type, const uint16_t script, const uint8_t i)
{
   switch (type)
   {
      case SCRIPT_FFC:
      {
         ri = &(tmpscr->scriptData[i]);

         curscript = ffscripts[script];
         stack = &(ffc_stack[i]);

         if (!tmpscr->initialized[i])
         {
            memcpy(ri->d, tmpscr->initd[i], 8 * sizeof(int32_t));
            memcpy(ri->a, tmpscr->inita[i], 2 * sizeof(int32_t));
         }

         ri->ffcref = i; //'this' pointer
      }
      break;

      case SCRIPT_ITEM:
      {
         ri = &itemScriptData;
         ri->Clear(); //Only runs for one frame so we just zero it out

         curscript = itemscripts[script];
         stack = &item_stack;
         memset(stack, 0, 256 * sizeof(int32_t)); //zero here too

         memcpy(ri->d, itemsbuf[i].initiald, 8 * sizeof(int32_t));
         memcpy(ri->a, itemsbuf[i].initiala, 2 * sizeof(int32_t));

         ri->idata = i; //'this' pointer

      }
      break;

      case SCRIPT_GLOBAL:
      {
         ri = &globalScriptData;

         curscript = globalscripts[script];
         stack = &global_stack;
      }
      break;

      default:
         zc_message("No other scripts are currently supported");
         return 1;
         break;
   }

   uint32_t pc = ri->pc; //this is (marginally) quicker than dereferencing ri each time
   uint16_t scommand = curscript[pc].command;
   sarg1 = curscript[pc].arg1;
   sarg2 = curscript[pc].arg2;

   bool increment = true;

   while (scommand != 0xFFFF && scommand != WAITFRAME && scommand != WAITDRAW)
   {
      switch (scommand)
      {
         case QUIT:
            scommand = 0xFFFF;
            break;

         case GOTO:
            pc = sarg1;
            increment = false;
            break;

         case GOTOR:
         {
            pc = (get_register(sarg1) / 10000) - 1;
            increment = false;
         }
         break;

         case GOTOTRUE:
            if (ri->scriptflag & TRUEFLAG)
            {
               pc = sarg1;
               increment = false;
            }

            break;

         case GOTOFALSE:
            if (!(ri->scriptflag & TRUEFLAG))
            {
               pc = sarg1;
               increment = false;
            }

            break;

         case GOTOMORE:
            if (ri->scriptflag & MOREFLAG)
            {
               pc = sarg1;
               increment = false;
            }

            break;

         case GOTOLESS:
            if (!(ri->scriptflag & MOREFLAG) || (!get_bit(quest_rules, qr_GOTOLESSNOTEQUAL) && (ri->scriptflag & TRUEFLAG)))
            {
               pc = sarg1;
               increment = false;
            }

            break;

         case LOOP:
         {
            if (get_register(sarg2) > 0)
            {
               pc = sarg1;
               increment = false;
            }
            else
               set_register(sarg1, sarg1 - 1);
         }
         break;

         case SETTRUE:
            set_register(sarg1, (ri->scriptflag & TRUEFLAG) ? 1 : 0);
            break;

         case SETFALSE:
            set_register(sarg1, (ri->scriptflag & TRUEFLAG) ? 0 : 1);
            break;

         case SETMORE:
            set_register(sarg1, (ri->scriptflag & MOREFLAG) ? 1 : 0);
            break;

         case SETLESS:
            set_register(sarg1, (!(ri->scriptflag & MOREFLAG)
                                 || (ri->scriptflag & TRUEFLAG)) ? 1 : 0);
            break;

         case NOT:
            do_not(false);
            break;

         case COMPAREV:
            do_comp(true);
            break;

         case COMPARER:
            do_comp(false);
            break;

         case SETV:
            do_set(true, type == SCRIPT_FFC ? i : -1);
            break;

         case SETR:
            do_set(false, type == SCRIPT_FFC ? i : -1);
            break;

         case PUSHR:
            do_push(false);
            break;

         case PUSHV:
            do_push(true);
            break;

         case POP:
            do_pop();
            break;

         case LOADI:
            do_loadi();
            break;

         case STOREI:
            do_storei();
            break;

         case LOAD1:
            do_loada(0);
            break;

         case LOAD2:
            do_loada(1);
            break;

         case SETA1:
            do_seta(0);
            break;

         case SETA2:
            do_seta(1);
            break;

         case ALLOCATEGMEMR:
            if (type == SCRIPT_GLOBAL) do_allocatemem(false, false, type == SCRIPT_FFC ? i : 255);

            break;

         case ALLOCATEGMEMV:
            if (type == SCRIPT_GLOBAL) do_allocatemem(true, false, type == SCRIPT_FFC ? i : 255);

            break;

         case ALLOCATEMEMR:
            do_allocatemem(false, true, type == SCRIPT_FFC ? i : 255);
            break;

         case ALLOCATEMEMV:
            do_allocatemem(true, true, type == SCRIPT_FFC ? i : 255);
            break;

         case DEALLOCATEMEMR:
            do_deallocatemem();
            break;

         case ARRAYSIZE:
            do_arraysize();
            break;

         case GETFFCSCRIPT:
            do_getffcscript();
            break;


         case ADDV:
            do_add(true);
            break;

         case ADDR:
            do_add(false);
            break;

         case SUBV:
            do_sub(true);
            break;

         case SUBR:
            do_sub(false);
            break;

         case MULTV:
            do_mult(true);
            break;

         case MULTR:
            do_mult(false);
            break;

         case DIVV:
            do_div(true);
            break;

         case DIVR:
            do_div(false);
            break;

         case MODV:
            do_mod(true);
            break;

         case MODR:
            do_mod(false);
            break;

         case SINV:
            do_trig(true, 0);
            break;

         case SINR:
            do_trig(false, 0);
            break;

         case COSV:
            do_trig(true, 1);
            break;

         case COSR:
            do_trig(false, 1);
            break;

         case TANV:
            do_trig(true, 2);
            break;

         case TANR:
            do_trig(false, 2);
            break;

         case ARCSINR:
            do_asin(false);
            break;

         case ARCCOSR:
            do_acos(false);
            break;

         case ARCTANR:
            do_arctan();
            break;

         case ABSR:
            do_abs(false);
            break;

         case MINR:
            do_min(false);
            break;

         case MINV:
            do_min(true);
            break;

         case MAXR:
            do_max(false);
            break;

         case MAXV:
            do_max(true);
            break;

         case RNDR:
            do_rnd(false);
            break;

         case RNDV:
            do_rnd(true);
            break;

         case FACTORIAL:
            do_factorial(false);
            break;

         case SQROOTV:
            do_sqroot(true);
            break;

         case SQROOTR:
            do_sqroot(false);
            break;

         case POWERR:
            do_power(false);
            break;

         case POWERV:
            do_power(true);
            break;

         case IPOWERR:
            do_ipower(false);
            break;

         case IPOWERV:
            do_ipower(true);
            break;

         case LOG10:
            do_log10(false);
            break;

         case LOGE:
            do_naturallog(false);
            break;

         case ANDR:
            do_and(false);
            break;

         case ANDV:
            do_and(true);
            break;

         case ORR:
            do_or(false);
            break;

         case ORV:
            do_or(true);
            break;

         case XORR:
            do_xor(false);
            break;

         case XORV:
            do_xor(true);
            break;

         case NANDR:
            do_nand(false);
            break;

         case NANDV:
            do_nand(true);
            break;

         case NORR:
            do_nor(false);
            break;

         case NORV:
            do_nor(true);
            break;

         case XNORR:
            do_xnor(false);
            break;

         case XNORV:
            do_xnor(true);
            break;

         case BITNOT:
            do_bitwisenot(false);
            break;

         case LSHIFTR:
            do_lshift(false);
            break;

         case LSHIFTV:
            do_lshift(true);
            break;

         case RSHIFTR:
            do_rshift(false);
            break;

         case RSHIFTV:
            do_rshift(true);
            break;

         case TRACER:
            do_trace(false);
            break;

         case TRACEV:
            do_trace(true);
            break;

         case TRACE2R:
            do_tracebool(false);
            break;

         case TRACE2V:
            do_tracebool(true);
            break;

         case TRACE3:
            do_tracenl();
            break;

         case TRACE4:
            do_cleartrace();
            break;

         case TRACE5:
            do_tracetobase();
            break;

         case TRACE6:
            do_tracestring();
            break;

         case WARP:
            do_warp(true);
            break;

         case WARPR:
            do_warp(false);
            break;

         case PITWARP:
            do_pitwarp(true);
            break;

         case PITWARPR:
            do_pitwarp(false);
            break;

         case BREAKSHIELD:
            do_breakshield();
            break;

         case SELECTAWPNV:
            do_selectweapon(true, true);
            break;

         case SELECTAWPNR:
            do_selectweapon(false, true);
            break;

         case SELECTBWPNV:
            do_selectweapon(true, false);
            break;

         case SELECTBWPNR:
            do_selectweapon(false, false);
            break;

         case PLAYSOUNDR:
            do_sfx(false);
            break;

         case PLAYSOUNDV:
            do_sfx(true);
            break;

         case PLAYMIDIR:
            do_midi(false);
            break;

         case PLAYMIDIV:
            do_midi(true);
            break;

         case PLAYENHMUSIC:
            do_enh_music(false);
            break;

         case GETMUSICFILE:
            do_get_enh_music_filename(false);
            break;

         case GETMUSICTRACK:
            do_get_enh_music_track(false);
            break;

         case SETDMAPENHMUSIC:
            do_set_dmap_enh_music(false);
            break;

         case MSGSTRR:
            do_message(false);
            break;

         case MSGSTRV:
            do_message(true);
            break;

         case ITEMNAME:
            do_getitemname();
            break;

         case NPCNAME:
            do_getnpcname();
            break;

         case GETSAVENAME:
            do_getsavename();
            break;

         case SETSAVENAME:
            do_setsavename();
            break;

         case GETMESSAGE:
            do_getmessage(false);
            break;

         case GETDMAPNAME:
            do_getdmapname(false);
            break;

         case GETDMAPTITLE:
            do_getdmaptitle(false);
            break;

         case GETDMAPINTRO:
            do_getdmapintro(false);
            break;

         case LOADLWEAPONR:
            do_loadlweapon(false);
            break;

         case LOADLWEAPONV:
            do_loadlweapon(true);
            break;

         case LOADEWEAPONR:
            do_loadeweapon(false);
            break;

         case LOADEWEAPONV:
            do_loadeweapon(true);
            break;

         case LOADITEMR:
            do_loaditem(false);
            break;

         case LOADITEMV:
            do_loaditem(true);
            break;

         case LOADITEMDATAR:
            do_loaditemdata(false);
            break;

         case LOADITEMDATAV:
            do_loaditemdata(true);
            break;

         case LOADNPCR:
            do_loadnpc(false);
            break;

         case LOADNPCV:
            do_loadnpc(true);
            break;

         case CREATELWEAPONR:
            do_createlweapon(false);
            break;

         case CREATELWEAPONV:
            do_createlweapon(true);
            break;

         case CREATEEWEAPONR:
            do_createeweapon(false);
            break;

         case CREATEEWEAPONV:
            do_createeweapon(true);
            break;

         case CREATEITEMR:
            do_createitem(false);
            break;

         case CREATEITEMV:
            do_createitem(true);
            break;

         case CREATENPCR:
            do_createnpc(false);
            break;

         case CREATENPCV:
            do_createnpc(true);
            break;

         case ISVALIDITEM:
            do_isvaliditem();
            break;

         case ISVALIDNPC:
            do_isvalidnpc();
            break;

         case ISVALIDLWPN:
            do_isvalidlwpn();
            break;

         case ISVALIDEWPN:
            do_isvalidewpn();
            break;

         case LWPNUSESPRITER:
            do_lwpnusesprite(false);
            break;

         case LWPNUSESPRITEV:
            do_lwpnusesprite(true);
            break;

         case EWPNUSESPRITER:
            do_ewpnusesprite(false);
            break;

         case EWPNUSESPRITEV:
            do_ewpnusesprite(true);
            break;

         case CLEARSPRITESR:
            do_clearsprites(false);
            break;

         case CLEARSPRITESV:
            do_clearsprites(true);
            break;

         case ISSOLID:
            do_issolid();
            break;

         case SETSIDEWARP:
            do_setsidewarp();
            break;

         case SETTILEWARP:
            do_settilewarp();
            break;

         case GETSIDEWARPDMAP:
            do_getsidewarpdmap(false);
            break;

         case GETSIDEWARPSCR:
            do_getsidewarpscr(false);
            break;

         case GETSIDEWARPTYPE:
            do_getsidewarptype(false);
            break;

         case GETTILEWARPDMAP:
            do_gettilewarpdmap(false);
            break;

         case GETTILEWARPSCR:
            do_gettilewarpscr(false);
            break;

         case GETTILEWARPTYPE:
            do_gettilewarptype(false);
            break;

         case LAYERSCREEN:
            do_layerscreen();
            break;

         case LAYERMAP:
            do_layermap();
            break;

         case SECRETS:
            do_triggersecrets();
            break;

         case GETSCREENFLAGS:
            do_getscreenflags();
            break;

         case GETSCREENEFLAGS:
            do_getscreeneflags();
            break;

         case COMBOTILE:
            do_combotile(false);
            break;

         case RECTR:
         case CIRCLER:
         case ARCR:
         case ELLIPSER:
         case LINER:
         case PUTPIXELR:
         case DRAWTILER:
         case DRAWCOMBOR:
         case DRAWCHARR:
         case DRAWINTR:
         case QUADR:
         case TRIANGLER:
         case QUAD3DR:
         case TRIANGLE3DR:
         case FASTTILER:
         case FASTCOMBOR:
         case DRAWSTRINGR:
         case SPLINER:
         case BITMAPR:
         case DRAWLAYERR:
         case DRAWSCREENR:
            do_drawing_command(scommand);
            break;

         case COPYTILEVV:
            do_copytile(true, true);
            break;

         case COPYTILEVR:
            do_copytile(true, false);
            break;

         case COPYTILERV:
            do_copytile(false, true);
            break;

         case COPYTILERR:
            do_copytile(false, false);
            break;

         case SWAPTILEVV:
            do_swaptile(true, true);
            break;

         case SWAPTILEVR:
            do_swaptile(true, false);
            break;

         case SWAPTILERV:
            do_swaptile(false, true);
            break;

         case SWAPTILERR:
            do_swaptile(false, false);
            break;

         case CLEARTILEV:
            do_cleartile(true);
            break;

         case CLEARTILER:
            do_cleartile(false);
            break;

         case OVERLAYTILEVV:
            do_overlaytile(true, true);
            break;

         case OVERLAYTILEVR:
            do_overlaytile(true, false);
            break;

         case OVERLAYTILERV:
            do_overlaytile(false, true);
            break;

         case OVERLAYTILERR:
            do_overlaytile(false, false);
            break;

         case FLIPROTTILEVV:
            do_fliprotatetile(true, true);
            break;

         case FLIPROTTILEVR:
            do_fliprotatetile(true, false);
            break;

         case FLIPROTTILERV:
            do_fliprotatetile(false, true);
            break;

         case FLIPROTTILERR:
            do_fliprotatetile(false, false);
            break;

         case GETTILEPIXELV:
            do_gettilepixel(true);
            break;

         case GETTILEPIXELR:
            do_gettilepixel(false);
            break;

         case SETTILEPIXELV:
            do_settilepixel(true);
            break;

         case SETTILEPIXELR:
            do_settilepixel(false);
            break;

         case SHIFTTILEVV:
            do_shifttile(true, true);
            break;

         case SHIFTTILEVR:
            do_shifttile(true, false);
            break;

         case SHIFTTILERV:
            do_shifttile(false, true);
            break;

         case SHIFTTILERR:
            do_shifttile(false, false);
            break;

         case SETRENDERTARGET:
            do_set_rendertarget(true);
            break;

         case GAMEEND:
            zc_state = ZC_QUIT;
            skipcont = 1;
            scommand = 0xFFFF;
            break;

         case SAVE:
            if (scriptCanSave)
            {
               save_game(false);
               scriptCanSave = false;
            }
            break;

         case SAVESCREEN:
            do_showsavescreen();
            break;

         case SAVEQUITSCREEN:
            save_game(false, 1);
            break;

         //Not Implemented
         case ELLIPSE2:
         case FLOODFILL:
            break;

         case SETCOLORB:
         case SETDEPTHB:
         case GETCOLORB:
         case GETDEPTHB:
            break;

         case ENQUEUER:
            do_enqueue(false);
            break;

         case ENQUEUEV:
            do_enqueue(true);
            break;

         case DEQUEUE:
            do_dequeue(false);
            break;

         default:
            Z_scripterrlog("Invalid ZASM command %ld reached", scommand);
            break;
      }

      if (increment)   pc++;
      else         increment = true;

      if (scommand != 0xFFFF)
      {
         scommand = curscript[pc].command;
         sarg1 = curscript[pc].arg1;
         sarg2 = curscript[pc].arg2;
      }
   }

   if (!scriptCanSave)
      scriptCanSave = true;

   if (scommand == WAITDRAW)
   {
      switch (type)
      {
         case SCRIPT_GLOBAL:
            global_wait = true;
            break;

         default:
            Z_scripterrlog("Waitdraw can only be used in the active global script");
            break;
      }
   }

   if (scommand == 0xFFFF) //Quit/command list end reached/bad command
   {
      switch (type)
      {
         case SCRIPT_FFC:
            tmpscr->ffscript[i] = 0;
            break;

         case SCRIPT_GLOBAL:
            g_doscript = 0;
            break;

         case SCRIPT_ITEM:
            break; //item scripts aren't gonna go again anyway
      }
   }
   else
      pc++;

   ri->pc = pc; //Put it back where we got it from

   return 0;
}

int ffscript_engine(const bool preload)
{
   for (uint8_t i = 0; i < MAXFFCS; i++)
   {
      if (tmpscr->ffscript[i] == 0)
         continue;

      if (preload && !(tmpscr->ffflags[i]&ffPRELOAD))
         continue;

      if ((tmpscr->ffflags[i]&ffIGNOREHOLDUP) == 0 && Link.getHoldClk() > 0)
         continue;

      ZScriptVersion::RunScript(SCRIPT_FFC, tmpscr->ffscript[i], i);
      tmpscr->initialized[i] = true;
   }

   return 0;
}
