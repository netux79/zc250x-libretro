#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <map>
#include <ctype.h>

#include "zelda.h"
#include "tiles.h"
#include "pal.h"
#include "qst.h"
#include "zcsys.h"
#include "subscr.h"
#include "maps.h"
#include "sprite.h"
#include "guys.h"
#include "link.h"
#include "title.h"
#include "particles.h"

static int sfx_voice[SFX_COUNT];

extern LinkClass Link;
extern sprite_list  guys, items, Ewpns, Lwpns, Sitems, chainlinks, decorations, particles;

//----------------------------------------------------------------

uint64_t trianglelines[16] =
{
   0x0000000000000000ULL,
   0xFD00000000000000ULL,
   0xFDFD000000000000ULL,
   0xFDFDFD0000000000ULL,
   0xFDFDFDFD00000000ULL,
   0xFDFDFDFDFD000000ULL,
   0xFDFDFDFDFDFD0000ULL,
   0xFDFDFDFDFDFDFD00ULL,
   0xFDFDFDFDFDFDFDFDULL,
   0x00FDFDFDFDFDFDFDULL,
   0x0000FDFDFDFDFDFDULL,
   0x000000FDFDFDFDFDULL,
   0x00000000FDFDFDFDULL,
   0x0000000000FDFDFDULL,
   0x000000000000FDFDULL,
   0x00000000000000FDULL,
};

uint16_t screen_triangles[28][32];

uint64_t triangles[4][16][8] = //[direction][value][line]
{
   {
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFD00000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL,
         0xFD00000000000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFD000000000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFD0000000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFD00000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFD000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFD0000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFD00ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      }
   },
   {
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x00000000000000FDULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x000000000000FDFDULL,
         0x00000000000000FDULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL,
         0x00000000000000FDULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL,
         0x00000000000000FDULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL,
         0x00000000000000FDULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL,
         0x00000000000000FDULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL,
         0x00000000000000FDULL,
         0x0000000000000000ULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL,
         0x00000000000000FDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL,
         0x000000000000FDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x0000000000FDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x00000000FDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x000000FDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      }
   },
   {
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0xFD00000000000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL
      },
      {
         0x0000000000000000ULL,
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL
      },
      {
         0xFD00000000000000ULL,
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFD000000000000ULL,
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFD0000000000ULL,
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFD00000000ULL,
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFD000000ULL,
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFD0000ULL,
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFD00ULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      }
   },
   {
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x00000000000000FDULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x00000000000000FDULL,
         0x000000000000FDFDULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x00000000000000FDULL,
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x00000000000000FDULL,
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x00000000000000FDULL,
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL
      },
      {
         0x0000000000000000ULL,
         0x0000000000000000ULL,
         0x00000000000000FDULL,
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL
      },
      {
         0x0000000000000000ULL,
         0x00000000000000FDULL,
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL
      },
      {
         0x00000000000000FDULL,
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0x000000000000FDFDULL,
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0x0000000000FDFDFDULL,
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0x00000000FDFDFDFDULL,
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0x000000FDFDFDFDFDULL,
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0x0000FDFDFDFDFDFDULL,
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0x00FDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      },
      {
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL,
         0xFDFDFDFDFDFDFDFDULL
      }
   }
};

int black_opening_count = 0;
int black_opening_x, black_opening_y;
int black_opening_shape;

int choose_opening_shape()
{
   // First, count how many bits are set
   int numBits = 0;
   int bitCounter;

   for (int i = 0; i < 4; i++)
   {
      if (COOLSCROLL & (1 << i))
         numBits++;
   }

   // Shouldn't happen...
   if (numBits == 0)
      return bosCIRCLE;

   // Pick a bit
   bitCounter = rand() % numBits + 1;

   for (int i = 0; i < 4; i++)
   {
      // If this bit is set, decrement the bit counter
      if (COOLSCROLL & (1 << i))
         bitCounter--;

      // When the counter hits 0, return a value based on
      // which bit it stopped on.
      // Reminder: enum {bosCIRCLE=0, bosOVAL, bosTRIANGLE, bosSMAS, bosMAX};
      if (bitCounter == 0)
         return i;
   }

   // Shouldn't be necessary, but the compiler might complain, at least
   return bosCIRCLE;
}

void close_black_opening(int x, int y, bool wait)
{
   black_opening_shape = choose_opening_shape();

   int w = 256, h = 224;
   int blockrows = 28, blockcolumns = 32;
   int xoffset = (x - (w / 2)) / 8, yoffset = (y - (h / 2)) / 8;

   for (int blockrow = 0; blockrow < blockrows; ++blockrow) //30
   {
      for (int blockcolumn = 0; blockcolumn < blockcolumns; ++blockcolumn) //40
         screen_triangles[blockrow][blockcolumn] = zc_max(abs(int(double(blockcolumns - 1) / 2 - blockcolumn + xoffset)),
               abs(int(double(blockrows - 1) / 2 - blockrow + yoffset))) | 0x0100 | ((blockrow - yoffset < blockrows / 2) ? 0 : 0x8000)
               | ((blockcolumn - xoffset < blockcolumns / 2) ? 0x4000 : 0);
   }

   black_opening_count = 66;
   black_opening_x = x;
   black_opening_y = y;
   lensclk = 0;
   //black_opening_shape=(black_opening_shape+1)%bosMAX;

   if (wait)
   {
      for (int i = 0; i < 66; i++)
      {
         draw_screen(tmpscr);
         put_passive_subscr(framebuf, &QMisc, 0, 0, false, sspUP);
         advanceframe(true);

         if (zc_state)
            break;
      }
   }
}

void open_black_opening(int x, int y, bool wait)
{
   black_opening_shape = choose_opening_shape();

   int w = 256, h = 224;
   int blockrows = 28, blockcolumns = 32;
   int xoffset = (x - (w / 2)) / 8, yoffset = (y - (h / 2)) / 8;

   for (int blockrow = 0; blockrow < blockrows; ++blockrow) //30
   {
      for (int blockcolumn = 0; blockcolumn < blockcolumns; ++blockcolumn) //40
         screen_triangles[blockrow][blockcolumn] = zc_max(abs(int(double(blockcolumns - 1) / 2 - blockcolumn + xoffset)),
               abs(int(double(blockrows - 1) / 2 - blockrow + yoffset))) | 0x0100 | ((blockrow - yoffset < blockrows / 2) ? 0 : 0x8000)
               | ((blockcolumn - xoffset < blockcolumns / 2) ? 0x4000 : 0);
   }

   black_opening_count = -66;
   black_opening_x = x;
   black_opening_y = y;
   lensclk = 0;

   if (wait)
   {
      for (int i = 0; i < 66; i++)
      {
         draw_screen(tmpscr);
         put_passive_subscr(framebuf, &QMisc, 0, 0, false, sspUP);
         advanceframe(true);

         if (zc_state)
            break;
      }
   }
}

void black_opening(BITMAP *dest, int x, int y, int a, int max_a)
{
   clear_to_color(tempbuf, BLACK);
   int w = 256, h = 224;

   switch (black_opening_shape)
   {
      case bosOVAL:
      {
         double new_w = (w / 2) + abs(w / 2 - x);
         double new_h = (h / 2) + abs(h / 2 - y);
         double b = sqrt(((new_w * new_w) / 4) + (new_h * new_h));
         ellipsefill(tempbuf, x, y, int(2 * a * b / max_a) / 8 * 8, int(a * b / max_a) / 8 * 8, 0);
         break;
      }

      case bosTRIANGLE:
      {
         double new_w = (w / 2) + abs(w / 2 - x);
         double new_h = (h / 2) + abs(h / 2 - y);
         double r = a * (new_w * sqrt((double)3) + new_h) / max_a;
         double P2 = (PI / 2);
         double P23 = (2 * PI / 3);
         double P43 = (4 * PI / 3);
         double Pa = (-4 * PI * a / (3 * max_a));
         double angle = P2 + Pa;
         double a0 = angle;
         double a2 = angle + P23;
         double a4 = angle + P43;
         triangle(tempbuf, x + int(cos(a0)*r), y - int(sin(a0)*r),
                  x + int(cos(a2)*r), y - int(sin(a2)*r),
                  x + int(cos(a4)*r), y - int(sin(a4)*r),
                  0);
         break;
      }

      case bosSMAS:
      {
         int distance = zc_max(abs(w / 2 - x), abs(h / 2 - y)) / 8;

         for (int blockrow = 0; blockrow < 28; ++blockrow) //30
         {
            for (int linerow = 0; linerow < 8; ++linerow)
            {
               uint64_t *triangleline = (uint64_t *)(tempbuf->line[(blockrow * 8 + linerow)]);

               for (int blockcolumn = 0; blockcolumn < 32; ++blockcolumn) //40
               {
                  *triangleline = triangles[(screen_triangles[blockrow][blockcolumn] & 0xC000) >> 14]
                                  [zc_min(zc_max((((31 + distance) * (max_a - a) / max_a) + ((screen_triangles[blockrow][blockcolumn] & 0x0FFF) - 0x0100)
                                                                      - (15 + distance)), 0), 15)]
                                  [linerow];
                  ++triangleline;

                  if (linerow == 0)
                  {
                  }
               }
            }
         }

         break;
      }

      case bosCIRCLE:
      default:
      {
         double new_w = (w / 2) + abs(w / 2 - x);
         double new_h = (h / 2) + abs(h / 2 - y);
         int r = int(sqrt((new_w * new_w) + (new_h * new_h)) * a / max_a);
         circlefill(tempbuf, x, y, r, 0);
         break;
      }
   }

   masked_blit(tempbuf, dest, 0, 0, 0, 0, w, h);
}

//----------------------------------------------------------------

bool item_disabled(int item)                 //is this item disabled?
{
   return (item >= 0 && game->items_off[item] != 0);
}

bool can_use_item(int item_type, int item)                  //can Link use this item?
{
   if (current_item(item_type, true) >= item)
      return true;

   return false;
}

bool has_item(int item_type, int it)                        //does Link possess this item?
{
   switch (item_type)
   {
      case itype_bomb:
      case itype_sbomb:
      {
         int itemid = getItemID(itemsbuf, item_type, it);

         if (itemid == -1)
            return false;

         return (game->get_item(itemid));
      }

      case itype_clock:
         return Link.getClock() ? 1 : 0;

      case itype_key:
         return (game->get_keys() > 0);

      case itype_magiccontainer:
         return (game->get_maxmagic() >= MAGICPERBLOCK);

      case itype_triforcepiece:                               //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game->lvlitems[i]&liTRIFORCE)
                     return true;
               }

               return false;
               break;
            }

            case -1:
               return (game->lvlitems[dlevel] & liTRIFORCE);
               break;

            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game->lvlitems[it] & liTRIFORCE);

               break;
         }

         return 0;
      }

      case itype_map:                                         //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game->lvlitems[i]&liMAP)
                     return true;
               }

               return false;
            }
            break;

            case -1:
               return (game->lvlitems[dlevel] & liMAP) != 0;
               break;

            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game->lvlitems[it] & liMAP) != 0;

               break;
         }

         return 0;
      }

      case itype_compass:                                     //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game->lvlitems[i]&liCOMPASS)
                     return true;
               }

               return false;
               break;
            }

            case -1:
               return (game->lvlitems[dlevel] & liCOMPASS) != 0;
               break;

            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game->lvlitems[it] & liCOMPASS) != 0;

               break;
         }

         return 0;
      }

      case itype_bosskey:                                     //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game->lvlitems[i]&liBOSSKEY)
                     return true;
               }

               return false;
               break;
            }

            case -1:
               return (game->lvlitems[dlevel] & liBOSSKEY) ? 1 : 0;
               break;

            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game->lvlitems[it] & liBOSSKEY) ? 1 : 0;

               break;
         }

         return 0;
      }

      default:
         int itemid = getItemID(itemsbuf, item_type, it);

         if (itemid == -1)
            return false;

         return game->get_item(itemid);
         break;
   }
}


int current_item(int item_type, bool checkenabled)           //item currently being used
{
   switch (item_type)
   {
      case itype_clock:
         return has_item(itype_clock, 1) ? 1 : 0;
         break;

      case itype_key:
         return game->get_keys();

      case itype_lkey:
         return game->lvlkeys[get_dlevel()];

      case itype_magiccontainer:
         return game->get_maxmagic() / MAGICPERBLOCK;

      case itype_triforcepiece:
      {
         int count = 0;

         for (int i = 0; i < MAXLEVELS; i++)
            count += (game->lvlitems[i] & liTRIFORCE) ? 1 : 0;

         return count;
         break;
      }

      case itype_map:
      {
         int count = 0;

         for (int i = 0; i < MAXLEVELS; i++)
            count += (game->lvlitems[i] & liMAP) ? 1 : 0;

         return count;
         break;
      }

      case itype_compass:
      {
         int count = 0;

         for (int i = 0; i < MAXLEVELS; i++)
            count += (game->lvlitems[i] & liCOMPASS) ? 1 : 0;

         return count;
         break;
      }

      case itype_bosskey:
      {
         int count = 0;

         for (int i = 0; i < MAXLEVELS; i++)
            count += (game->lvlitems[i] & liBOSSKEY) ? 1 : 0;

         return count;
         break;
      }

      default:
         int maxid = getHighestLevelOfFamily(game, itemsbuf, item_type, checkenabled);

         if (maxid == -1)
            return 0;

         return itemsbuf[maxid].fam_type;
         break;
   }
}

int current_item(int item_type)           //item currently being used
{
   return current_item(item_type, true);
}

std::map<int, int> itemcache;

// Not actually used by anything at the moment...
void removeFromItemCache(int itemid)
{
   itemcache.erase(itemid);
}

void flushItemCache()
{
   itemcache.clear();

   //also fix the active subscreen if items were deleted -DD
   if (game != NULL)
   {
      verifyBothWeapons();
      load_Sitems(&QMisc);
   }
}

// This is used often, so it should be as direct as possible.
int current_item_id(int itemtype, bool checkmagic)
{
   if (itemtype != itype_ring) // Rings must always be checked.
   {
      std::map<int, int>::iterator res = itemcache.find(itemtype);

      if (res != itemcache.end())
         return res->second;
   }

   int result = -1;
   int highestlevel = -1;

   for (int i = 0; i < MAXITEMS; i++)
   {
      if (game->get_item(i) && itemsbuf[i].family == itemtype && !item_disabled(i))
      {
         if ((checkmagic || itemtype == itype_ring) && itemtype != itype_magicring)
         {
            if (!checkmagiccost(i))
               continue;
         }

         if (itemsbuf[i].fam_type >= highestlevel)
         {
            highestlevel = itemsbuf[i].fam_type;
            result = i;
         }
      }
   }

   itemcache[itemtype] = result;
   return result;
}

int current_item_power(int itemtype)
{
   int result = current_item_id(itemtype, true);
   return (result < 0) ? 0 : itemsbuf[result].power;
}

int item_tile_mod(bool)
{
   int tile = 0;
   int ret = 0;

   ret = game->get_bombs();

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         if (current_item_id(itype_bomb, false) >= 0)
            ret = itemsbuf[current_item_id(itype_bomb, false)].ltm;
         else
            ret = 0;

         break;

   }

   tile += ret;

   ret = game->get_sbombs();

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         if (current_item_id(itype_sbomb, false) >= 0)
            ret = itemsbuf[current_item_id(itype_sbomb, false)].ltm;
         else
            ret = 0;

         break;
   }

   tile += ret;

   ret = current_item(itype_clock);

   switch (ret)
   {
      case 1:
         ret = itemsbuf[iClock].ltm;
         break;

      default:
         ret = 0;
         break;
   }

   tile += ret;

   ret = current_item(itype_key);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iKey].ltm;
         break;
   }

   tile += ret;

   ret = current_item(itype_lkey);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iLevelKey].ltm;
         break;
   }

   tile += ret;

   ret = current_item(itype_map);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iMap].ltm;
         break;
   }

   tile += ret;

   ret = current_item(itype_compass);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iCompass].ltm;
         break;
   }

   tile += ret;

   ret = current_item(itype_bosskey);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iBossKey].ltm;
         break;
   }

   tile += ret;

   ret = current_item(itype_magiccontainer);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iMagicC].ltm;
         break;
   }

   tile += ret;

   ret = current_item(itype_triforcepiece);

   switch (ret)
   {
      case 0:
         ret = 0;
         break;

      default:
         ret = itemsbuf[iTriforce].ltm;
         break;
   }

   tile += ret;

   for (int i = 0; i < itype_max; i++)
   {
      ret = current_item_id(i, false);

      if (ret >= 0)
         tile += itemsbuf[ret].ltm;
   }

   return tile;
}

// Hints are drawn on a separate layer to combo reveals.
void draw_lens_under(BITMAP *dest, bool layer)
{
   bool hints = (layer && get_bit(quest_rules, qr_LENSHINTS));

   int strike_hint_table[11] =
   {
      mfARROW, mfBOMB, mfBRANG, mfWANDMAGIC,
      mfSWORD, mfREFMAGIC, mfHOOKSHOT,
      mfREFFIREBALL, mfHAMMER, mfSWORDBEAM, mfWAND
   };

   //  int page = tmpscr->cpage;
   {
      int blink_rate = 1;
      //    int temptimer=0;
      int tempitem, tempweapon = 0;
      strike_hint = strike_hint_table[strike_hint_counter];

      if (strike_hint_timer > 32)
      {
         strike_hint_timer = 0;
         strike_hint_counter = ((strike_hint_counter + 1) % 11);
      }

      ++strike_hint_timer;

      for (int i = 0; i < 176; i++)
      {
         int x = (i & 15) << 4;
         int y = (i & 0xF0) + PLAYFIELD_OFFSET;
         int tempitemx = -16, tempitemy = -16;
         int tempweaponx = -16, tempweapony = -16;

         for (int iter = 0; iter < 2; ++iter)
         {
            int checkflag = 0;

            if (iter == 0)
               checkflag = combobuf[tmpscr->data[i]].flag;
            else
               checkflag = tmpscr->sflag[i];

            if (checkflag == mfSTRIKE)
            {
               if (!hints)
                  putcombo(dest, x, y, tmpscr->secretcombo[sSTRIKE], tmpscr->secretcset[sSTRIKE]);
               else
                  checkflag = strike_hint;
            }

            switch (checkflag)
            {
               case 0:
               case mfZELDA:
               case mfPUSHED:
               case mfENEMY0:
               case mfENEMY1:
               case mfENEMY2:
               case mfENEMY3:
               case mfENEMY4:
               case mfENEMY5:
               case mfENEMY6:
               case mfENEMY7:
               case mfENEMY8:
               case mfENEMY9:
               case mfSINGLE:
               case mfSINGLE16:
               case mfNOENEMY:
               case mfTRAP_H:
               case mfTRAP_V:
               case mfTRAP_4:
               case mfTRAP_LR:
               case mfTRAP_UD:
               case mfNOGROUNDENEMY:
               case mfNOBLOCKS:
               case mfSCRIPT1:
               case mfSCRIPT2:
               case mfSCRIPT3:
               case mfSCRIPT4:
               case mfSCRIPT5:
                  break;

               case mfPUSHUD:
               case mfPUSHLR:
               case mfPUSH4:
               case mfPUSHU:
               case mfPUSHD:
               case mfPUSHL:
               case mfPUSHR:
               case mfPUSHUDNS:
               case mfPUSHLRNS:
               case mfPUSH4NS:
               case mfPUSHUNS:
               case mfPUSHDNS:
               case mfPUSHLNS:
               case mfPUSHRNS:
               case mfPUSHUDINS:
               case mfPUSHLRINS:
               case mfPUSH4INS:
               case mfPUSHUINS:
               case mfPUSHDINS:
               case mfPUSHLINS:
               case mfPUSHRINS:
                  if (!hints && (lensclk & 16))
                     putcombo(dest, x, y, tmpscr->undercombo, tmpscr->undercset);

                  if (lensclk & blink_rate)
                  {
                     if (hints)
                     {
                        switch (combobuf[tmpscr->data[i]].type)
                        {
                           case cPUSH_HEAVY:
                           case cPUSH_HW:
                              tempitem = getItemIDPower(itemsbuf, itype_bracelet, 1);
                              tempitemx = x, tempitemy = y;

                              if (tempitem > -1)
                                 putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);

                              break;

                           case cPUSH_HEAVY2:
                           case cPUSH_HW2:
                              tempitem = getItemIDPower(itemsbuf, itype_bracelet, 2);
                              tempitemx = x, tempitemy = y;

                              if (tempitem > -1)
                                 putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);

                              break;
                        }
                     }
                  }

                  break;

               case mfWHISTLE:
                  if (hints)
                  {
                     tempitem = getItemID(itemsbuf, itype_whistle, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               //Why is this here?
               case mfFAIRY:
               case mfMAGICFAIRY:
               case mfALLFAIRY:
                  if (hints)
                  {
                     tempitem = getItemID(itemsbuf, itype_fairy, 1); //iFairyMoving;

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfBCANDLE:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sBCANDLE], tmpscr->secretcset[sBCANDLE]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_candle, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfRCANDLE:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sRCANDLE], tmpscr->secretcset[sRCANDLE]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_candle, 2);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfWANDFIRE:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sWANDFIRE], tmpscr->secretcset[sWANDFIRE]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_wand, 1);

                     if (tempitem < 0) break;

                     tempweapon = wFire;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }
                     else
                     {
                        tempweaponx = x;
                        tempweapony = y;
                     }

                     putweapon(dest, tempweaponx, tempweapony, tempweapon, 0, up, lens_hint_weapon[tempweapon][0],
                               lens_hint_weapon[tempweapon][1], -1);
                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfDINSFIRE:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sDINSFIRE], tmpscr->secretcset[sDINSFIRE]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_dinsfire, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfARROW:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sARROW], tmpscr->secretcset[sARROW]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_arrow, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfSARROW:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sSARROW], tmpscr->secretcset[sSARROW]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_arrow, 2);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfGARROW:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sGARROW], tmpscr->secretcset[sGARROW]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_arrow, 3);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfBOMB:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sBOMB], tmpscr->secretcset[sBOMB]);
                  else
                  {
                     //tempitem=getItemID(itemsbuf,itype_bomb,1);
                     tempweapon = wLitBomb;

                     //if (tempitem<0) break;
                     if (lensclk & blink_rate)
                     {
                        tempweaponx = x;
                        tempweapony = y;
                     }

                     putweapon(dest, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4], tempweapon, 0, up,
                               lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1], -1);
                  }

                  break;

               case mfSBOMB:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sSBOMB], tmpscr->secretcset[sSBOMB]);
                  else
                  {
                     //tempitem=getItemID(itemsbuf,itype_sbomb,1);
                     //if (tempitem<0) break;
                     tempweapon = wLitSBomb;

                     if (lensclk & blink_rate)
                     {
                        tempweaponx = x;
                        tempweapony = y;
                     }

                     putweapon(dest, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4], tempweapon, 0, up,
                               lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1], -1);
                  }

                  break;

               case mfARMOS_SECRET:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sSTAIRS], tmpscr->secretcset[sSTAIRS]);

                  break;

               case mfBRANG:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sBRANG], tmpscr->secretcset[sBRANG]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_brang, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfMBRANG:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sMBRANG], tmpscr->secretcset[sMBRANG]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_brang, 2);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfFBRANG:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sFBRANG], tmpscr->secretcset[sFBRANG]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_brang, 3);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfWANDMAGIC:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sWANDMAGIC], tmpscr->secretcset[sWANDMAGIC]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_wand, 1);

                     if (tempitem < 0) break;

                     tempweapon = itemsbuf[tempitem].wpn3;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }
                     else
                     {
                        tempweaponx = x;
                        tempweapony = y;
                        --lens_hint_weapon[wMagic][4];

                        if (lens_hint_weapon[wMagic][4] < -8)
                           lens_hint_weapon[wMagic][4] = 8;
                     }

                     putweapon(dest, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4], tempweapon, 0, up,
                               lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1], -1);
                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfREFMAGIC:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sREFMAGIC], tmpscr->secretcset[sREFMAGIC]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_shield, 3);

                     if (tempitem < 0) break;

                     tempweapon = ewMagic;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }
                     else
                     {
                        tempweaponx = x;
                        tempweapony = y;

                        if (lens_hint_weapon[ewMagic][2] == up)
                           --lens_hint_weapon[ewMagic][4];
                        else
                           ++lens_hint_weapon[ewMagic][4];

                        if (lens_hint_weapon[ewMagic][4] > 8)
                           lens_hint_weapon[ewMagic][2] = up;

                        if (lens_hint_weapon[ewMagic][4] <= 0)
                           lens_hint_weapon[ewMagic][2] = down;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                     putweapon(dest, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4], tempweapon, 0, lens_hint_weapon[ewMagic][2],
                               lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1], -1);
                  }

                  break;

               case mfREFFIREBALL:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sREFFIREBALL], tmpscr->secretcset[sREFFIREBALL]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_shield, 3);

                     if (tempitem < 0) break;

                     tempweapon = ewFireball;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                        tempweaponx = x;
                        tempweapony = y;
                        ++lens_hint_weapon[ewFireball][3];

                        if (lens_hint_weapon[ewFireball][3] > 8)
                        {
                           lens_hint_weapon[ewFireball][3] = -8;
                           lens_hint_weapon[ewFireball][4] = 8;
                        }

                        if (lens_hint_weapon[ewFireball][3] > 0)
                           ++lens_hint_weapon[ewFireball][4];
                        else
                           --lens_hint_weapon[ewFireball][4];
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                     putweapon(dest, tempweaponx + lens_hint_weapon[tempweapon][3], tempweapony + lens_hint_weapon[ewFireball][4],
                               tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1], -1);
                  }

                  break;

               case mfSWORD:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sSWORD], tmpscr->secretcset[sSWORD]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfWSWORD:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sWSWORD], tmpscr->secretcset[sWSWORD]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 2);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfMSWORD:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sMSWORD], tmpscr->secretcset[sMSWORD]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 3);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfXSWORD:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sXSWORD], tmpscr->secretcset[sXSWORD]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 4);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfSWORDBEAM:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sSWORDBEAM], tmpscr->secretcset[sSWORDBEAM]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 1);
                  }

                  break;

               case mfWSWORDBEAM:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sWSWORDBEAM], tmpscr->secretcset[sWSWORDBEAM]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 2);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 2);
                  }

                  break;

               case mfMSWORDBEAM:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sMSWORDBEAM], tmpscr->secretcset[sMSWORDBEAM]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 3);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 3);
                  }

                  break;

               case mfXSWORDBEAM:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sXSWORDBEAM], tmpscr->secretcset[sXSWORDBEAM]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_sword, 4);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 4);
                  }

                  break;

               case mfHOOKSHOT:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sHOOKSHOT], tmpscr->secretcset[sHOOKSHOT]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_hookshot, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfWAND:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sWAND], tmpscr->secretcset[sWAND]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_wand, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfHAMMER:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[sHAMMER], tmpscr->secretcset[sHAMMER]);
                  else
                  {
                     tempitem = getItemID(itemsbuf, itype_hammer, 1);

                     if (tempitem < 0) break;

                     if (lensclk & blink_rate)
                     {
                        tempitemx = x;
                        tempitemy = y;
                     }

                     putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  }

                  break;

               case mfARMOS_ITEM:
               case mfDIVE_ITEM:
                  if (!getmapflag())
                     //          putitem2(dest,x,y,tmpscr->catchall);
                     putitem2(dest, x, y, tmpscr->catchall, lens_hint_item[tmpscr->catchall][0], lens_hint_item[tmpscr->catchall][1], 0);

                  break;

               case 16:
               case 17:
               case 18:
               case 19:
               case 20:
               case 21:
               case 22:
               case 23:
               case 24:
               case 25:
               case 26:
               case 27:
               case 28:
               case 29:
               case 30:
               case 31:
                  if (!hints)
                     putcombo(dest, x, y, tmpscr->secretcombo[checkflag - 16 + 4],
                              tmpscr->secretcset[checkflag - 16 + 4]);

                  break;

               default:
                  if (layer && ((checkflag != mfRAFT && checkflag != mfRAFT_BRANCH && checkflag != mfRAFT_BOUNCE)
                                || get_bit(quest_rules, qr_RAFTLENS)))
                  {
                     if (lensclk & 1)
                        rectfill(dest, x, y, x + 15, y + 15, WHITE);
                  }

                  break;
            }
         }
      }

      if (layer)
      {
         if (tmpscr->door[0] == dWALK)
            rectfill(dest, 120, 16 + PLAYFIELD_OFFSET, 135, 31 + PLAYFIELD_OFFSET, WHITE);

         if (tmpscr->door[1] == dWALK)
            rectfill(dest, 120, 144 + PLAYFIELD_OFFSET, 135, 159 + PLAYFIELD_OFFSET, WHITE);

         if (tmpscr->door[2] == dWALK)
            rectfill(dest, 16, 80 + PLAYFIELD_OFFSET, 31, 95 + PLAYFIELD_OFFSET, WHITE);

         if (tmpscr->door[3] == dWALK)
            rectfill(dest, 224, 80 + PLAYFIELD_OFFSET, 239, 95 + PLAYFIELD_OFFSET, WHITE);

         if (tmpscr->door[0] == dBOMB)
            showbombeddoor(dest, 0);

         if (tmpscr->door[1] == dBOMB)
            showbombeddoor(dest, 1);

         if (tmpscr->door[2] == dBOMB)
            showbombeddoor(dest, 2);

         if (tmpscr->door[3] == dBOMB)
            showbombeddoor(dest, 3);
      }

      if (tmpscr->stairx + tmpscr->stairy)
      {
         if (!hints)
            putcombo(dest, tmpscr->stairx, tmpscr->stairy + PLAYFIELD_OFFSET, tmpscr->secretcombo[sSTAIRS],
                     tmpscr->secretcset[sSTAIRS]);
         else
         {
            if (tmpscr->flags & fWHISTLE)
            {
               tempitem = getItemID(itemsbuf, itype_whistle, 1);
               int tempitemx = -16;
               int tempitemy = -16;

               if (lensclk & (blink_rate / 4))
               {
                  tempitemx = tmpscr->stairx;
                  tempitemy = tmpscr->stairy + PLAYFIELD_OFFSET;
               }

               putitem2(dest, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
            }
         }
      }
   }
}

void draw_lens_over()
{
   static int last_width = -1;
   int width = itemsbuf[current_item_id(itype_lens, true)].misc1;

   // Only redraw the circle if the size has changed
   if (width != last_width)
   {
      if (lens_scr == NULL)
         lens_scr = create_bitmap(2 * 288, 2 * (240 - PLAYFIELD_OFFSET));

      clear_to_color(lens_scr, BLACK);
      circlefill(lens_scr, 288, 240 - PLAYFIELD_OFFSET, width, 0);
      circle(lens_scr, 288, 240 - PLAYFIELD_OFFSET, width + 2, 0);
      circle(lens_scr, 288, 240 - PLAYFIELD_OFFSET, width + 5, 0);
      last_width = width;
   }

   masked_blit(lens_scr, framebuf, 288 - (Link.getX() + 8), 240 - PLAYFIELD_OFFSET - (Link.getY() + 8), 0,
               PLAYFIELD_OFFSET, 256, 168);
}

//----------------------------------------------------------------

void draw_wavy(BITMAP *buffer, int amplitude)
{
   clear_to_color(tempbuf, 0);
   blit(buffer, tempbuf, 0, PLAYFIELD_OFFSET, 16, 0, 256, 224 - PLAYFIELD_OFFSET);

   amplitude = zc_min(2048, amplitude); // some arbitrary limit to prevent crashing
   int amp2 = 168;
   int i = frame % amp2;

   for (int j = 0; j < 168; j++)
   {
      int ofs = -int(sin((double(i + j) * 2 * PI / amp2)) * amplitude);

      if (ofs)
      {
         for (int k = 0; k < 256; k++)
            buffer->line[j + PLAYFIELD_OFFSET][k] = tempbuf->line[j][k + ofs + 16];
      }
   }
}

void draw_quake(BITMAP *buffer, int qclock)
{
   /* Calculate the quake offset to use. */
   int quakeofs = (int)(sin((double)(qclock * 2 - frame)) * 4);

   clear_to_color(tempbuf, 0);
   blit(buffer, tempbuf, 0, PLAYFIELD_OFFSET, 0, 3 + quakeofs, 256, 224 - PLAYFIELD_OFFSET);
   blit(tempbuf, buffer, 0, 0, 0, PLAYFIELD_OFFSET, 256, 224 - PLAYFIELD_OFFSET);
}

void draw_fuzzy(int fuzz)
// draws from right half of scrollbuf to framebuf
{
   int firstx, firsty, xstep, ystep, i, y, dx, dy;
   uint8_t *start, *si, *di;

   if (fuzz < 1)
      fuzz = 1;

   xstep = 128 % fuzz;

   if (xstep > 0)
      xstep = fuzz - xstep;

   ystep = 112 % fuzz;

   if (ystep > 0)
      ystep = fuzz - ystep;

   firsty = 1;

   for (y = 0; y < 224;)
   {
      start = &(scrollbuf->line[y][256]);

      for (dy = 0; dy < ystep && dy + y < 224; dy++)
      {
         si = start;
         di = &(framebuf->line[y + dy][0]);
         i = xstep;
         firstx = 1;

         for (dx = 0; dx < 256; dx++)
         {
            *(di++) = *si;

            if (++i >= fuzz)
            {
               if (!firstx)
                  si += fuzz;
               else
               {
                  si += fuzz - xstep;
                  firstx = 0;
               }

               i = 0;
            }
         }
      }

      if (!firsty)
         y += fuzz;
      else
      {
         y += ystep;
         ystep = fuzz;
         firsty = 0;
      }
   }
}

void update_video_frame(bool allow_gfx)
{
   if (!playing)
      black_opening_count = 0;

   if (black_opening_count < 0) //shape is opening up
   {
      black_opening(framebuf, black_opening_x, black_opening_y, (66 + black_opening_count), 66);
      ++black_opening_count;
   }
   else if (black_opening_count > 0) //shape is closing
   {
      black_opening(framebuf, black_opening_x, black_opening_y, black_opening_count, 66);
      --black_opening_count;
   }

   if (zc_sync_pal)
   {
      create_rgb_table(&rgb_table, zc_palette);
      create_zc_trans_table(&trans_table, zc_palette, 128, 128, 128);
      memcpy(&trans_table2, &trans_table, sizeof(COLOR_MAP));

      for (int q = 0; q < PAL_SIZE; q++)
      {
         trans_table2.data[0][q] = q;
         trans_table2.data[q][q] = q;
      }
   }

   if (playing && allow_gfx)
   {
      if (wavy)
      {
         // Wavy was set by a script.
         draw_wavy(framebuf, wavy);
         wavy--; // Decrement it
      }
      else if (DMaps[currdmap].flags & dmfWAVY)
      {
         // Wavy was set by a DMap flag.
         draw_wavy(framebuf, 4);
      }

      if (quakeclk)
      {
         draw_quake(framebuf, quakeclk);
         quakeclk--;
      }
   }

   /*if(!(msgdisplaybuf->clip) && playing && msgpos && !screenscrolling)
   {
       masked_blit(msgdisplaybuf,framebuf,0,0,0,PLAYFIELD_OFFSET,256,168);
   }*/

   bool nosubscr = (tmpscr->flags3 & fNOSUBSCR && !(tmpscr->flags3 & fNOSUBSCROFFSET));

   /* Use tempbuf as buffer when we want to display panorama screen */
   if (nosubscr)
   {
      rectfill(tempbuf, 0, 0, 255, (SUBSCREEN_HEIGHT / 2) - 1, 0);
      rectfill(tempbuf, 0, 168 + SUBSCREEN_HEIGHT / 2, 255, 168 + SUBSCREEN_HEIGHT - 1, 0);
      blit(framebuf, tempbuf, 0, PLAYFIELD_OFFSET, 0, SUBSCREEN_HEIGHT / 2, 256, 224 - SUBSCREEN_HEIGHT);
   }

   zc_canvas = nosubscr ? tempbuf : framebuf;
}

//----------------------------------------------------------------

void zc_action(int state)
{
   music_pause();
   pause_all_sfx();
   zc_state = state;
}

// 99*360 + 59*60
#define MAXTIME  21405240

void advanceframe(bool allow_gfx)
{
   if (zc_state)
      return;

   if (playing && game->get_time() < MAXTIME)
      game->change_time(1);

   ++frame;

   /* process midi and zcmusic */
   midi_fill_buffer();
   zcmusic_poll();

   /* prepare the video frame */
   update_video_frame(allow_gfx);

   /* signal main thread to continue
    * and wait for it to ask for the
    * next iteration. */
   scond_signal(cond);
   scond_wait(cond, mutex);

   sfx_cleanup();
}

void pauseGame()
{
   music_pause();
   pause_all_sfx();
   playing = false; /* need this to stop the game time running */

   /* put message in the middle of the screen */
   textout_centre_ex(framebuf, nfont, "PAUSED", 128, 136, WHITE, BLACK);

   do
   {
      advanceframe(false);
   }
   while (!SelectKeyPress && !zc_state);
   
   music_resume();
   resume_all_sfx();
   playing = true;
}

void zapout()
{
   set_clip_rect(scrollbuf, 0, 0, scrollbuf->w, scrollbuf->h);
   blit(framebuf, scrollbuf, 0, 0, 256, 0, 256, 224);

   script_drawing_commands.Clear();

   // zap out
   for (int i = 1; i <= 24; i++)
   {
      draw_fuzzy(i);
      advanceframe(true);

      if (zc_state)
         break;
   }
}

void zapin()
{
   draw_screen(tmpscr);
   set_clip_rect(scrollbuf, 0, 0, scrollbuf->w, scrollbuf->h);
   put_passive_subscr(framebuf, &QMisc, 0, 0, false, sspUP);
   blit(framebuf, scrollbuf, 0, 0, 256, 0, 256, 224);

   // zap out
   for (int i = 24; i >= 1; i--)
   {
      draw_fuzzy(i);
      advanceframe(true);

      if (zc_state)
         break;
   }
}

void wavyout(bool showlink)
{
   draw_screen(tmpscr, showlink);
   put_passive_subscr(framebuf, &QMisc, 0, 0, false, sspUP);

   BITMAP *wavebuf = create_bitmap(288, 224);
   clear_to_color(wavebuf, 0);
   blit(framebuf, wavebuf, 0, 0, 16, 0, 256, 224);

   PALETTE wavepal;

   /* use this as main palette
    * temporary for this process */
   zc_palette = wavepal;

   int ofs;
   int amplitude = 8;

   int wavelength = 4;
   double palpos = 0, palstep = 4, palstop = 126;

   for (int i = 0; i < 168; i += wavelength)
   {
      for (int l = 0; l < 256; l++)
      {
         wavepal[l].r = vbound(int(RAMpal[l].r + ((palpos / palstop) * (63 - RAMpal[l].r))), 0, 63);
         wavepal[l].g = vbound(int(RAMpal[l].g + ((palpos / palstop) * (63 - RAMpal[l].g))), 0, 63);
         wavepal[l].b = vbound(int(RAMpal[l].b + ((palpos / palstop) * (63 - RAMpal[l].b))), 0, 63);
      }

      palpos += palstep;

      for (int j = 0; j + PLAYFIELD_OFFSET < 224; j++)
      {
         for (int k = 0; k < 256; k++)
         {
            ofs = 0;

            if ((j < i) && (j & 1))
               ofs = int(sin((double(i + j) * 2 * PI / 168.0)) * amplitude);

            framebuf->line[j + PLAYFIELD_OFFSET][k] = wavebuf->line[j + PLAYFIELD_OFFSET][k + ofs + 16];
         }
      }

      /* ensure changes get applied */
      zc_sync_pal = true;
      advanceframe(true);

      if (zc_state)
         break;
   }

   destroy_bitmap(wavebuf);

   /* restore system palette */
   zc_palette = RAMpal;
   zc_sync_pal = true;
}

void wavyin()
{
   draw_screen(tmpscr);
   put_passive_subscr(framebuf, &QMisc, 0, 0, false, sspUP);

   BITMAP *wavebuf = create_bitmap(288, 224);
   clear_to_color(wavebuf, 0);
   blit(framebuf, wavebuf, 0, 0, 16, 0, 256, 224);

   PALETTE wavepal;

   /* use this as main palette
    * temporary for this process */
   zc_palette = wavepal;

   int ofs;
   int amplitude = 8;
   int wavelength = 4;
   double palpos = 168, palstep = 4, palstop = 126;

   for (int i = 0; i < 168; i += wavelength)
   {
      for (int l = 0; l < 256; l++)
      {
         wavepal[l].r = vbound(int(RAMpal[l].r + ((palpos / palstop) * (63 - RAMpal[l].r))), 0, 63);
         wavepal[l].g = vbound(int(RAMpal[l].g + ((palpos / palstop) * (63 - RAMpal[l].g))), 0, 63);
         wavepal[l].b = vbound(int(RAMpal[l].b + ((palpos / palstop) * (63 - RAMpal[l].b))), 0, 63);
      }

      palpos -= palstep;

      for (int j = 0; j + PLAYFIELD_OFFSET < 224; j++)
      {
         for (int k = 0; k < 256; k++)
         {
            ofs = 0;

            if ((j < (167 - i)) && (j & 1))
               ofs = int(sin((double(i + j) * 2 * PI / 168.0)) * amplitude);

            framebuf->line[j + PLAYFIELD_OFFSET][k] = wavebuf->line[j + PLAYFIELD_OFFSET][k + ofs + 16];
         }
      }

      /* ensure the palette change gets applied */
      zc_sync_pal = true;
      advanceframe(true);

      if (zc_state)
         break;
   }

   destroy_bitmap(wavebuf);

   /* restore system palette */
   zc_palette = RAMpal;
   zc_sync_pal = true;
}

void blackscr(int fcnt, bool showsubscr)
{
   reset_pal_cycling();
   script_drawing_commands.Clear();

   while (fcnt > 0)
   {
      clear_bitmap(framebuf);

      if (showsubscr)
         put_passive_subscr(framebuf, &QMisc, 0, 0, false, sspUP);

      advanceframe(true);

      if (zc_state)
         break;

      --fcnt;
   }
}

void openscreen()
{
   reset_pal_cycling();
   black_opening_count = 0;

   if (COOLSCROLL)
   {
      open_black_opening(Link.getX() + 8, (Link.getY() - Link.getZ()) + 8 + PLAYFIELD_OFFSET, true);
      return;
   }
   else
   {
      Link.setDontDraw(true);
      show_subscreen_dmap_dots = false;
      show_subscreen_numbers = false;
      show_subscreen_life = false;
   }

   for (int i = 0; i < 80; i++)
   {
      draw_screen(tmpscr);
      int x = 128 - (((i * 128 / 80) / 8) * 8);

      if (x > 0)
      {
         rectfill(framebuf, 0, PLAYFIELD_OFFSET, x, 167 + PLAYFIELD_OFFSET, 0);
         rectfill(framebuf, 256 - x, PLAYFIELD_OFFSET, 255, 167 + PLAYFIELD_OFFSET, 0);
      }

      advanceframe(true);

      if (zc_state)
         break;
   }

   Link.setDontDraw(false);
   show_subscreen_items = true;
   show_subscreen_dmap_dots = true;
}

int TriforceCount()
{
   int c = 0;

   for (int i = 1; i <= 8; i++)
      if (game->lvlitems[i]&liTRIFORCE)
         ++c;

   return c;
}

void music_pause()
{
   zcmusic_pause(TRUE);
   midi_pause();
}

void music_resume()
{
   zcmusic_pause(FALSE);
   midi_resume();
}

void music_stop()
{
   zcmusic_stop();
   zcmusic_unload_file(zcmusic);
   zcmusic = NULL;
   midi_stop();
   sel_music = 0;
}

/*****************************/
/**** Custom Sound System ****/
/*****************************/

inline int mixvol(int v1, int v2)
{
   return (zc_min(v1, 255) * zc_min(v2, 255)) >> 8;
}

bool play_zcmusic(const char *filename, int track)
{
   ZCMUSIC *newzcmusic = NULL;

   if (use_nsf_dat && strcmp(filename, ZELDA_NSF_FILE) == 0)
      newzcmusic = (ZCMUSIC *)zcmusic_load_zeldansf_df(&data[ZELDA_NSF]);
   else
   {
      char musicpath[MAX_STRLEN];
      replace_filename(musicpath, qst_path, filename);
      newzcmusic = (ZCMUSIC *)zcmusic_load_file(musicpath);
   }

   if (newzcmusic != NULL)
   {
      zcmusic_stop();
      zcmusic_unload_file(zcmusic);
      midi_stop();

      zcmusic = newzcmusic;
      zcmusic_play(zcmusic);
      update_music_volume();

      if (track > 0)
         zcmusic_change_track(track);

      return true;
   }

   return false;
}

// Run an NSF, or a MIDI if the NSF fails.
void play_zeldamusic(int track, int midi)
{
   if (play_zcmusic(ZELDA_NSF_FILE, track))
      return;

   jukebox(midi);
}

void jukebox(int index)
{
   if (index < 0) index = MAXMIDIS - 1;
   if (index >= MAXMIDIS) index = 0;

   /* do nothing if it's already playing */
   if (index == sel_music && midi_isplaying())
      return;

   music_stop();

   sel_music = index;
   update_music_volume();
   midi_play(tunes[sel_music].data, tunes[sel_music].loop);

   if (tunes[sel_music].start > 0)
      midi_fastforward(tunes[sel_music].start);

   midi_loopend(tunes[sel_music].loop_end);
   midi_loopstart(tunes[sel_music].loop_start);
}

void play_midimusic(int m)
{
   switch (m)
   {
      case 1:
         jukebox(MID_OVERWORLD);
         break;

      case 2:
         jukebox(MID_DUNGEON);
         break;

      case 3:
         jukebox(MID_LEVEL9);
         break;

      default:
         if (m >= 4 && m < 4 + MAXCUSTOMMIDIS)
            jukebox(m - 4 + MID_COUNT);
         else
            music_stop();
   }
}

void play_dmapmusic()
{
   if (DMaps[currdmap].tmusic[0] != 0)
   {
      if (zcmusic == NULL || strcmp(zcmusic->filename, DMaps[currdmap].tmusic) != 0 ||
            (zcmusic->type == STREAM_GME && zcmusic->track != DMaps[currdmap].tmusictrack))
      {
         if (play_zcmusic(DMaps[currdmap].tmusic, DMaps[currdmap].tmusictrack))
            return;
      }
      else
         return; /* No need to change current song */
   }

   /* If couldn't play ZCMUSIC, try MIDI */
   play_midimusic(DMaps[currdmap].midi);
}

void play_levelmusic()
{
   int m = tmpscr->screen_midi;

   switch (m)
   {
      case -2:
         music_stop();
         break;

      case -1:
         play_dmapmusic();
         break;

      default:
         play_midimusic(m);
   }
}

void update_music_volume(void)
{
   int i = MIN(MAX(sel_music, 0), MAXMUSIC - 1);
   int vol = mixvol(tunes[i].volume, music_vol);
   midi_set_volume(vol);
   zcmusic_set_volume(music_vol);
}

/*****************/
/*****  SFX  *****/
/*****************/

// array of voices, one for each sfx sample in the data file
// 0+ = voice #
// -1 = voice not allocated
int zc_initsound(void)
{
   char temp[MAX_STRLEN];

   /* Init mixer */
   if (!mixer_init(sampling_rate / TIMING_FPS, sampling_rate, mix_quality, MIXER_MAX_SFX))
      return false;

   /* calculate the Sound Font path to load with the midi engine */
   sprintf(temp, "%s%c" ZC_SYS_DIR "%c" ZC_SF2_DIR "%c%s.sf2", system_path, 
           OTHER_PATH_SEPARATOR, OTHER_PATH_SEPARATOR, OTHER_PATH_SEPARATOR, sf2_file);
   
   if (!file_exists(temp))
      return false;

   /* Setup the midi processor */
   if (!midi_init(sampling_rate, 1 / TIMING_FPS, temp))
      return false;

   if (!zcmusic_init(1 / TIMING_FPS))
      return false;

   /* Apply master volume */
   mixer_set_volume(master_vol);

   for (int i = 0; i < SFX_COUNT; i++)
      sfx_voice[i] = -1;

   for (int i = 0; i < MID_COUNT; i++)
      tunes[i].data = mididata[i].dat;

   for (int j = 0; j < MAXCUSTOMMIDIS; j++)
      tunes[MID_COUNT + j].data = NULL;

   return true;
}

void zc_deinitsound(void)
{
   midi_deinit();
   zcmusic_exit();
   mixer_exit();
}

// returns number of voices currently allocated
int sfx_count()
{
   int c = 0;

   for (int i = 0; i < SFX_COUNT; i++)
      if (sfx_voice[i] != -1)
         ++c;

   return c;
}

// clean up finished samples
void sfx_cleanup()
{
   for (int i = 0; i < SFX_COUNT; i++)
      if (sfx_voice[i] != -1 && voice_get_position(sfx_voice[i]) < 0)
      {
         deallocate_voice(sfx_voice[i]);
         sfx_voice[i] = -1;
      }
}

void update_sfx_volume(void)
{
   for (int i = 0; i < SFX_COUNT; i++)
      if (sfx_voice[i] != -1 && voice_get_position(sfx_voice[i]) >= 0)
         voice_set_volume(sfx_voice[i], sfx_vol);
}

// allocates a voice for the sample "wav_index" (index into zelda.dat)
// if a voice is already allocated (and/or playing), then it just returns true
// Returns true:  voice is allocated
//         false: unsuccessful
bool sfx_init(int index)
{
   // check index
   if (index <= 0 || index >= SFX_COUNT)
      return false;

   if (sfx_voice[index] == -1)
   {
      if (use_qst_sfx && qst_has_sfx)
         sfx_voice[index] = allocate_voice(&customsfxdata[index]);
      else
         sfx_voice[index] = allocate_voice((SAMPLE *)sfxdata[(index < SFX_Z35) ? index : SFX_Z35].dat);

      voice_set_volume(sfx_voice[index], sfx_vol);
   }

   return sfx_voice[index] != -1;
}

// plays an sfx sample
void sfx(int index, int pan, bool loop, bool restart)
{
   if (!sfx_init(index))
      return;

   voice_set_playmode(sfx_voice[index], loop ? PLAYMODE_LOOP : PLAYMODE_PLAY);
   voice_set_pan(sfx_voice[index], pan);

   int pos = voice_get_position(sfx_voice[index]);

   if (restart) voice_set_position(sfx_voice[index], 0);

   if (pos <= 0)
      voice_start(sfx_voice[index]);
}

// true if sfx is allocated
bool sfx_allocated(int index)
{
   return (index > 0 && index < SFX_COUNT && sfx_voice[index] != -1);
}

// start it (in loop mode) if it's not already playing,
// otherwise adjust it to play in loop mode -DD
void cont_sfx(int index)
{
   if (!sfx_init(index))
      return;

   if (voice_get_position(sfx_voice[index]) <= 0)
   {
      voice_set_position(sfx_voice[index], 0);
      voice_set_playmode(sfx_voice[index], PLAYMODE_LOOP);
      voice_start(sfx_voice[index]);
   }
   else
      adjust_sfx(index, 128, true);
}

// adjust parameters while playing
void adjust_sfx(int index, int pan, bool loop)
{
   if (index <= 0 || index >= SFX_COUNT || sfx_voice[index] == -1)
      return;

   voice_set_playmode(sfx_voice[index], loop ? PLAYMODE_LOOP : PLAYMODE_PLAY);
   voice_set_pan(sfx_voice[index], pan);
}

// pauses a voice
void pause_sfx(int index)
{
   if (index > 0 && index < SFX_COUNT && sfx_voice[index] != -1)
      voice_stop(sfx_voice[index]);
}

// resumes a voice
void resume_sfx(int index)
{
   if (index > 0 && index < SFX_COUNT && sfx_voice[index] != -1)
      voice_start(sfx_voice[index]);
}

// pauses all active voices
void pause_all_sfx()
{
   for (int i = 0; i < SFX_COUNT; i++)
      if (sfx_voice[i] != -1)
         voice_stop(sfx_voice[i]);
}

// resumes all paused voices
void resume_all_sfx()
{
   for (int i = 0; i < SFX_COUNT; i++)
      if (sfx_voice[i] != -1)
         voice_start(sfx_voice[i]);
}

// stops an sfx and deallocates the voice
void stop_sfx(int index)
{
   if (index <= 0 || index >= SFX_COUNT)
      return;

   if (sfx_voice[index] != -1)
   {
      deallocate_voice(sfx_voice[index]);
      sfx_voice[index] = -1;
   }
}

// Stops SFX played by Link's item of the given family
void stop_item_sfx(int family)
{
   int id = current_item_id(family);

   if (id < 0)
      return;

   stop_sfx(itemsbuf[id].usesound);
}

void kill_sfx()
{
   for (int i = 0; i < SFX_COUNT; i++)
      if (sfx_voice[i] != -1)
      {
         deallocate_voice(sfx_voice[i]);
         sfx_voice[i] = -1;
      }
}

int pan(int x)
{
   switch (pan_style)
   {
      case 0:
         return 128;

      case 1:
         return vbound((x >> 1) + 68, 0, 255);

      case 2:
         return vbound(((x * 3) >> 2) + 36, 0, 255);
   }

   return vbound(x, 0, 255);
}

/*******************************/
/******* Input Handlers ********/
/*******************************/

int rButton(int button, int &flag)
{
   if (!button)
      flag = FALSE;
   else if (!flag)
   {
      flag = TRUE;
      return TRUE;
   }

   return FALSE;
}

bool drunk()
{
   return ((!(frame % ((rand() % 100) + 1))) && (rand() % MAXDRUNKCLOCK < Link.DrunkClock()));
}

bool DrunkUp()
{
   return drunk() ? (rand() % 2) ? 0 : !UpKey : UpKey;
}
bool DrunkDown()
{
   return drunk() ? (rand() % 2) ? 0 : !DownKey : DownKey;
}
bool DrunkLeft()
{
   return drunk() ? (rand() % 2) ? 0 : !LeftKey : LeftKey;
}
bool DrunkRight()
{
   return drunk() ? (rand() % 2) ? 0 : !RightKey : RightKey;
}
bool DrunkcAbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !AKey : AKey;
}
bool DrunkcBbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !BKey : BKey;
}

bool DrunkrAbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !AKeyPress : AKeyPress;
}
bool DrunkrBbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !BKeyPress : BKeyPress;
}

/* Here set all buttons to FALSE to avoid reacting to button
   pressed in the rest of the frame iteration until a button is released
   and pressed again. */
void eat_buttons()
{
   UpKeyPress = FALSE;
   DownKeyPress = FALSE;
   LeftKeyPress = FALSE;
   RightKeyPress = FALSE;

   AKeyPress = FALSE;
   BKeyPress = FALSE;
   MapKeyPress = FALSE;
   ModKeyPress = FALSE;
   LKeyPress = FALSE;
   RKeyPress = FALSE;

   Ex1KeyPress = FALSE;
   Ex2KeyPress = FALSE;
   Ex3KeyPress = FALSE;
   Ex4KeyPress = FALSE;

   SelectKeyPress = FALSE;
   StartKeyPress = FALSE;
}

char *time_str_med(uint32_t time)
{
   static char s[16];

   uint32_t secs = (time / 60) % 60;
   uint32_t mins = (time / 3600) % 60;
   uint32_t hours = time / 216000;

   sprintf(s, "%d:%02d:%02d", hours, mins, secs);
   return s;
}

char *time_str_short(uint32_t time)
{
   static char s[16];

   uint32_t mins = (time / 3600) % 60;
   uint32_t hours = time / 216000;

   sprintf(s, "%02d%s%02d", hours, (time % 60) < 30 ? ":" : ";", mins);
   return s;
}

int vbound(int x, int low, int high)
{
   if (x < low) return low;

   if (x > high) return high;

   return x;
}

float vbound(float x, float low, float high)
{
   if (x < low) return low;

   if (x > high) return high;

   return x;
}

void set_bit(uint8_t *bitstr, int bit, uint8_t val)
{
   bitstr += bit >> 3;
   uint8_t mask = 1 << (bit & 7);

   if (val)
      *bitstr |= mask;
   else
      *bitstr &= ~mask;
}

int get_bit(uint8_t *bitstr, int bit)
{
   bitstr += bit >> 3;
   return ((*bitstr) >> (bit & 7)) & 1;
}

int anim_3_4(int clk, int speed)
{
   clk /= speed;

   switch (clk & 3)
   {
      case 0:
      case 2:
         clk = 0;
         break;

      case 1:
         clk = 1;
         break;

      case 3:
         clk = 2;
         break;
   }

   return clk;
}

/**********  Encryption Stuff  *****************/

//#define MASK 0x4C358938
static int seed = 0;
//#define MASK 0x91B2A2D1
//static int seed = 7351962;
static unsigned int enc_mask[ENC_METHOD_MAX] = {0x4C358938, 0x91B2A2D1, 0x4A7C1B87, 0xF93941E6, 0xFD095E94};
static int pvalue[ENC_METHOD_MAX] = {0x62E9, 0x7D14, 0x1A82, 0x02BB, 0xE09C};
static int qvalue[ENC_METHOD_MAX] = {0x3619, 0xA26B, 0xF03C, 0x7B12, 0x4E8F};

static int rand_007(int method)
{
   short BX = seed >> 8;
   short CX = (seed & 0xFF) << 8;
   signed char AL = seed >> 24;
   signed char C = AL >> 7;
   signed char D = BX >> 15;
   AL <<= 1;
   BX = (BX << 1) | C;
   CX = (CX << 1) | D;
   CX += seed & 0xFFFF;
   BX += (seed >> 16) + C;
   //  CX += 0x62E9;
   //  BX += 0x3619 + D;
   CX += pvalue[method];
   BX += qvalue[method] + D;
   seed = (BX << 16) + CX;
   return (CX << 16) + BX;
}

//
// RETURNS:
//   0 - OK
//   1 - srcfile not opened
//   2 - destfile not opened
//   3 - scrfile too small
//   4 - srcfile EOF
//   5 - checksum mismatch
//   6 - header mismatch
//
int decode_file_007(const char *srcfile, const char *destfile, const char *header, int method)
{
   FILE *src = NULL, *dest = NULL;
   int tog = 0, c, r = 0, err;
   long size, i;
   short c1 = 0, c2 = 0, check1, check2;

   // open files
   size = file_size(srcfile);

   if (size < 1)
      return 1;

   size -= 8;   // get actual data size, minus key and checksums

   if (size < 1)
      return 3;

   src = fopen(srcfile, "rb");

   if (!src)
      return 1;


   dest = fopen(destfile, "wb");

   if (!dest)
   {
      fclose(src);
      return 2;
   }

   // read the header
   err = 4;

   if (header)
   {
      for (i = 0; header[i]; i++)
      {
         if ((c = fgetc(src)) == EOF)
            goto error;

         if ((c & 255) != header[i])
         {
            err = 6;
            goto error;
         }

         --size;
      }
   }

   // read the key
   if ((c = fgetc(src)) == EOF)
      goto error;

   seed = c << 24;

   if ((c = fgetc(src)) == EOF)
      goto error;

   seed += (c & 255) << 16;

   if ((c = fgetc(src)) == EOF)
      goto error;

   seed += (c & 255) << 8;

   if ((c = fgetc(src)) == EOF)
      goto error;

   seed += c & 255;
   seed ^= enc_mask[method];

   // decode the data
   for (i = 0; i < size; i++)
   {
      if ((c = fgetc(src)) == EOF)
         goto error;

      if (tog)
         c -= r;
      else
      {
         r = rand_007(method);
         c ^= r;
      }

      tog ^= 1;

      c &= 255;
      c1 += c;
      c2 = (c2 << 4) + (c2 >> 12) + c;

      fputc(c, dest);
   }

   // read checksums
   if ((c = fgetc(src)) == EOF)
      goto error;

   check1 = c << 8;

   if ((c = fgetc(src)) == EOF)
      goto error;

   check1 += c & 255;

   if ((c = fgetc(src)) == EOF)
      goto error;

   check2 = c << 8;

   if ((c = fgetc(src)) == EOF)
      goto error;

   check2 += c & 255;

   // verify checksums
   r = rand_007(method);
   check1 ^= r;
   check2 -= r;
   check1 &= 0xFFFF;
   check2 &= 0xFFFF;

   if (check1 != c1 || check2 != c2)
   {
      err = 5;
      goto error;
   }

   fclose(src);
   fclose(dest);
   return 0;

error:
   fclose(src);
   fclose(dest);
   delete_file(destfile);
   return err;
}
