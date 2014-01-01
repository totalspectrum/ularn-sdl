/* =============================================================================
 * PROGRAM:  ularn
 * FILENAME: ularn_sdl.c
 *
 * DESCRIPTION:
 * This module contains all operating system dependant code for input and
 * display update.
 * Each version of ularn should provide a different implementation of this
 * module.
 *
 * This is the SDL window display and input module.
 *
 * =============================================================================
 * EXPORTED VARIABLES
 *
 * nonap         : Set to true if no time delays are to be used.
 * nosignal      : Set if ctrl-C is to be trapped to prevent exit.
 * enable_scroll : Probably superfluous
 * yrepcount     : Repeat count for input commands.
 *
 * =============================================================================
 * EXPORTED FUNCTIONS
 *
 * init_app               : Initialise the app
 * close_app              : Close the app and free resources
 * get_normal_input       : Get the next command input
 * get_prompt_input       : Get input in response to a question
 * get_password_input     : Get a password
 * get_num_input          : Geta number
 * get_dir_input          : Get a direction
 * set_display            : Set the display mode
 * UpdateStatus           : Update the status display
 * UpdateEffects          : Update the effects display
 * UpdateStatusAndEffects : Update both status and effects display
 * ClearText              : Clear the text output area
 * UlarnBeep              : Make a beep
 * Cursor                 : Set the cursor location
 * Printc                 : Print a single character
 * Print                  : Print a string
 * Printf                 : Print a formatted string
 * Standout               : Print a string is standout format
 * SetFormat              : Set the output text format
 * ClearEOL               : Clear to end of line
 * ClearEOPage            : Clear to end of page
 * show1cell              : Show 1 cell on the map
 * showplayer             : Show the player on the map
 * showcell               : Show the area around the player
 * drawscreen             : Redraw the screen
 * draws                  : Redraw a section of the screen
 * mapeffect              : Draw a directional effect
 * magic_effect_frames    : Get the number of animation frames in a magic fx
 * magic_effect           : Draw a frame in a magic fx
 * nap                    : Delay for a specified number of milliseconds
 * GetUser                : Get teh username and user id.
 *
 * =============================================================================
 */

#include <SDL/SDL.h>
#include <SDL/SDL_keyboard.h>
#include <SDL/SDL_keysym.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include <stdarg.h>

#include "header.h"
#include "ularn_game.h"

#include "config.h"
#include "dungeon.h"
#include "player.h"
#include "ularn_win.h"
#include "monster.h"
#include "itm.h"

// Default size of the ularn window in characters
#define WINDOW_WIDTH    80
#define WINDOW_HEIGHT   25
#define SEPARATOR_WIDTH   8
#define SEPARATOR_HEIGHT  8
#define BORDER_SIZE       8

/* =============================================================================
 * Exported variables
 */

int nonap = 0;
int nosignal = 0;

char enable_scroll = 0;

int yrepcount = 0;

/* =============================================================================
 * Local variables
 */

#define M_NONE 0
#define M_SHIFT 1
#define M_CTRL  2
#define M_ASCII 255

#define MAX_KEY_BINDINGS 3

struct KeyCodeType
{
  int VirtKey;
  int ModKey;
};

#define NUM_DIRS 8
static ActionType DirActions[NUM_DIRS] =
{
  ACTION_MOVE_WEST,
  ACTION_MOVE_EAST,
  ACTION_MOVE_SOUTH,
  ACTION_MOVE_NORTH,
  ACTION_MOVE_NORTHEAST,
  ACTION_MOVE_NORTHWEST,
  ACTION_MOVE_SOUTHEAST,
  ACTION_MOVE_SOUTHWEST
};

/* Default keymap */
/* Allow up to MAX_KEY_BINDINGS per action */
static struct KeyCodeType KeyMap[ACTION_COUNT][MAX_KEY_BINDINGS] =
{
  { { 0, 0 },         { 0, 0 }, { 0, 0 } },                          // ACTION_NULL
  { { '~', M_ASCII }, { 0, 0 }, { 0, 0 } },                                  // ACTION_DIAG
  { { 'h', M_ASCII }, { SDLK_KP4 ,  M_NONE },  { SDLK_LEFT, M_NONE } },      // ACTION_MOVE_WEST
  { { 'H', M_ASCII }, { SDLK_LEFT,  M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_WEST
  { { 'l', M_ASCII }, { SDLK_KP6,  M_NONE },  { SDLK_RIGHT, M_NONE } },     // ACTION_MOVE_EAST,
  { { 'L', M_ASCII }, { SDLK_RIGHT, M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_EAST,
  { { 'j', M_ASCII }, { SDLK_KP2,  M_NONE },  { SDLK_DOWN, M_NONE } },       // ACTION_MOVE_SOUTH,
  { { 'J', M_ASCII }, { SDLK_DOWN,  M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_SOUTH,
  { { 'k', M_ASCII }, { SDLK_KP8,  M_NONE },  { SDLK_UP, M_NONE } },           // ACTION_MOVE_NORTH,
  { { 'K', M_ASCII }, { SDLK_UP,    M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_NORTH,
  { { 'u', M_ASCII }, { SDLK_KP9,  M_NONE },  { SDLK_PAGEUP, M_NONE } },   // ACTION_MOVE_NORTHEAST,
  { { 'U', M_ASCII }, { SDLK_PAGEUP, M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_NORTHEAST,
  { { 'y', M_ASCII }, { SDLK_KP7,  M_NONE },  { SDLK_HOME, M_NONE } },       // ACTION_MOVE_NORTHWEST,
  { { 'Y', M_ASCII }, { SDLK_HOME,  M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_NORTHWEST,
  { { 'n', M_ASCII }, { SDLK_KP3,  M_NONE },  { SDLK_PAGEDOWN, M_NONE } },  // ACTION_MOVE_SOUTHEAST,
  { { 'N', M_ASCII }, { SDLK_PAGEDOWN,  M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_SOUTHEAST,
  { { 'b', M_ASCII }, { SDLK_KP1,  M_NONE },  { SDLK_END, M_NONE } },         // ACTION_MOVE_SOUTHWEST,
  { { 'B', M_ASCII }, { SDLK_END,   M_SHIFT }, { 0, 0 } },                     // ACTION_RUN_SOUTHWEST,
  { { '.', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_WAIT,
  { { ' ', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_NONE,
  { { 'w', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_WIELD,
  { { 'W', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_WEAR,
  { { 'r', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_READ,
  { { 'q', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_QUAFF,
  { { 'd', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_DROP,
  { { 'c', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_CAST_SPELL,
  { { 'o', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_OPEN_DOOR
  { { 'C', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_CLOSE_DOOR,
  { { 'O', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_OPEN_CHEST
  { { 'i', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_INVENTORY,
  { { 'e', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_EAT_COOKIE,
  { { '\\',M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_LIST_SPELLS,
  { { '?', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_HELP,
  { { 'S', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_SAVE,
  { { 'Z', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_TELEPORT,
  { { '^', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_IDENTIFY_TRAPS,
  { { '_', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_BECOME_CREATOR,
  { { '+', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_CREATE_ITEM,
  { { '-', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_TOGGLE_WIZARD,
  { { '`', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_DEBUG_MODE,
  { { 'T', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_REMOVE_ARMOUR,
  { { 'g', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_PACK_WEIGHT,
  { { 'v', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_VERSION,
  { { 'Q', M_ASCII }, { 0, 0 }, { 0, 0 } },                   // ACTION_QUIT,
  { { 'r', M_CTRL  }, { 0, 0 }, { 0, 0 } },                   // ACTION_REDRAW_SCREEN,
  { { 'P', M_ASCII }, { 0, 0 }, { 0, 0 } }                    // ACTION_SHOW_TAX
};

static struct KeyCodeType RunKeyMap = { SDLK_KP5, M_NONE };


//
// Variables for SDL
//

int ularn_menu_height;

//#define INITIAL_WIDTH 640
//#define INITIAL_HEIGHT 480
#define INITIAL_WIDTH 1024
#define INITIAL_HEIGHT 768

static SDL_Surface* ularn_window = NULL;

static unsigned long white_pixel;
static unsigned long black_pixel;

static SDL_Color LtGrey = { 192, 192, 192};
static SDL_Color MidGrey = { 128, 128, 128};
static SDL_Color DkGrey = {96, 96, 96};
static SDL_Color Red = {255, 0, 0};
static SDL_Color Green = {0, 200, 0};
static SDL_Color Blue = {0, 0, 255};
static SDL_Color White = {255, 255, 255};
static SDL_Color Black = {0, 0, 0};
static Uint32 LtGrey_pixel;
static Uint32 MidGrey_pixel;
static Uint32 DkGrey_pixel;

static TTF_Font *font_info;
char * font_name = "lib/ConsolaMono.ttf";

static SDL_Surface* TilePixmap = NULL;
static SDL_Surface* TilePixmapKeyed = NULL;
static SDL_Surface* MenuPixmap = NULL;

static int CaretActive = 0;

static int TileWidth = 32;
static int TileHeight = 32;
static int CharHeight;
static int CharWidth;
static int CharAscent;
static int LarnWindowWidth = INITIAL_WIDTH;
static int LarnWindowHeight = INITIAL_HEIGHT;
static int MinWindowWidth;
static int MinWindowHeight;

static int Runkey;
static ActionType Event;
static int GotChar;
static char EventChar;

//
// Bitmaps for tiles
//

static char *TileFilename = "lib/ularn_gfx.bmp";
static char *MenuFilename = "lib/ularn_menu.bmp";

/* Tiles for different character classes, (female, male) */
static int PlayerTiles[8][2] =
{
  { 165, 181 }, /* Ogre */
  { 166, 182 }, /* Wizard */
  { 167, 183 }, /* Klingon */
  { 168, 184 }, /* Elf */
  { 169, 185 }, /* Rogue */
  { 170, 186 }, /* Adventurer */
  { 171, 187 }, /* Dwarf */
  { 172, 188 }  /* Rambo */
};

#define TILE_CURSOR1 174
#define TILE_CURSOR2 190
#define WALL_TILES   352

/* Tiles for directional effects */
static int EffectTile[EFFECT_COUNT][9] =
{
  { 191, 198, 196, 194, 192, 195, 193, 197, 199 },
  { 191, 206, 204, 202, 200, 203, 201, 205, 207 },
  { 191, 214, 212, 210, 208, 211, 209, 213, 215 },
  { 191, 222, 220, 218, 216, 219, 217, 221, 223 },
  { 191, 230, 228, 226, 224, 227, 225, 229, 231 }
};

#define MAX_MAGICFX_FRAME 8

struct MagicEffectDataType
{
  int Overlay;                  /* 0 = no overlay, 1 = overlay     */
  int Frames;                   /* Number of frames in the effect  */
  int Tile1[MAX_MAGICFX_FRAME]; /* The primary tile for this frame */
  int Tile2[MAX_MAGICFX_FRAME]; /* Only used for overlay effects   */
};

static struct MagicEffectDataType magicfx_tile[MAGIC_COUNT] =
{
  /* Sparkle */
  { 1, /* Overlay this on current tile */
    8,
    { 240, 241, 242, 243, 244, 245, 246, 247 },
    { 248, 249, 250, 251, 252, 253, 254, 255 }
  },

  /* Sleep */
  {
    0,
    6,
    { 256, 272, 288, 304, 320, 336, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Web */
  {
    0,
    6,
    { 257, 273, 289, 305, 321, 337, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Phantasmal forces */
  {
    0,
    6,
    { 258, 274, 290, 306, 322, 338, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Cloud kill */
  {
    0,
    6,
    { 259, 275, 291, 307, 323, 339, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Vaporize rock */
  {
    0,
    6,
    { 260, 276, 292, 308, 324, 340, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Dehydrate */
  {
    0,
    6,
    { 261, 277, 293, 309, 325, 341, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Drain life */
  {
    0,
    6,
    { 262, 278, 294, 310, 326, 342, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Flood */
  {
    0,
    6,
    { 263, 279, 295, 311, 327, 343, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Finger of death */
  {
    0,
    6,
    { 264, 280, 296, 312, 328, 344, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Teleport away */
  {
    0,
    6,
    { 265, 281, 297, 313, 329, 345, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Magic fire */
  {
    0,
    6,
    { 266, 282, 298, 314, 330, 346, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Make wall */
  {
    0,
    6,
    { 267, 283, 299, 315, 331, 347, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Summon demon */
  {
    0,
    6,
    { 268, 284, 300, 316, 332, 348, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  },

  /* Annihilate (scroll) */
  {
    0,
    6,
    { 269, 285, 301, 317, 333, 349, 0, 0 },
    { 0, 0, 0, 0, 0, 0 ,0 ,0 }
  }
};

//
// Current display mode
//
DisplayModeType CurrentDisplayMode = DISPLAY_TEXT;

//
// Map window position and size
//
SDL_Rect MapRect;
SDL_Rect MapAreaRect;
SDL_Rect MapTileRect;

//
// Status lines window position and size
//
SDL_Rect StatusRect;

//
// Effects window position and size
//
SDL_Rect EffectsRect;

//
// Message window position and size
//
SDL_Rect MessageRect;
SDL_Rect MenuRect;

//
// Text window position, size
//
SDL_Rect TextRect;
static int ShowTextBorder;

// =============================================================================
// Text mode stuff
//

#define LINE_LENGTH 80

typedef char TextLine[LINE_LENGTH + 1];
typedef FormatType FormatLine[LINE_LENGTH + 1];

//
// Messages
//
#define MAX_MSG_LINES    5
static TextLine MessageChr[MAX_MSG_LINES];
static FormatLine MessageFmt[MAX_MSG_LINES];
static FormatType CurrentMsgFormat;
static int MsgCursorX = 1;
static int MsgCursorY = 1;

//
// Text
//
#define MAX_TEXT_LINES 25
#define TEXT_LINE_LENGTH 80
static TextLine TextChr[MAX_TEXT_LINES];
static FormatLine TextFmt[MAX_TEXT_LINES];
static FormatType CurrentTextFormat;
static int TextCursorX = 1;
static int TextCursorY = 1;

//
// Generalised text buffer
// Top left corner is x=1, y=1
//
static TextLine   *Text;
static FormatLine *Format;
static FormatType CurrentFormat;
static int CursorX = 1;
static int CursorY = 1;
static int MaxLine;
static int TTop;
static int TLeft;
static int TWidth;
static int THeight;

//
// The monster to use for showing mimics. Changes every 10 turns.
//
static int mimicmonst = MIMIC;

/* =============================================================================
 * Local functions
 */

/* =============================================================================
 * FUNCTION: calc_scroll
 *
 * DESCRIPTION:
 * Calculate the new scroll position of the map based on the player's current
 * position.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   true if the new scroll position differs from the previous scroll position.
 */
static int calc_scroll(void)
{
  int ox, oy;

  ox = MapTileRect.x;
  oy = MapTileRect.y;

  if (MapTileRect.h < MAXY)
  {
    MapTileRect.y = playery - MapTileRect.h / 2;
    if (MapTileRect.y < 0)
    {
      MapTileRect.y = 0;
    }

    if ((MapTileRect.y + MapTileRect.h) > MAXY)
    {
      MapTileRect.y = MAXY - MapTileRect.h;
    }
  }
  else
  {
    MapTileRect.y = 0;
  }

  if (MapTileRect.w < MAXX)
  {
    MapTileRect.x = playerx - MapTileRect.w / 2;
    if (MapTileRect.x < 0)
    {
      MapTileRect.x = 0;
    }

    if ((MapTileRect.x + MapTileRect.w) > MAXX)
    {
      MapTileRect.x = MAXX - MapTileRect.w;
    }
  }
  else
  {
    MapTileRect.x = 0;
  }

  //
  // return true if the map requires scrolling
  //
  return ((MapTileRect.x != ox) || (MapTileRect.y != oy));
}

/* =============================================================================
 * FUNCTION: CalcMinWindowSize
 *
 * DESCRIPTION:
 * Calculate the minimum window size.
 * The new minimum windows size is stored in MinWindowWidth and MinWindowHeight.
 * If the current window size is smaller than this then it is resized.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void CalcMinWindowSize(void)
{
  int r;

  r = TTF_SizeText(font_info, "W", &CharWidth, NULL);
  (void)r;
  CharHeight = TTF_FontHeight(font_info);
  CharAscent = TTF_FontAscent(font_info);

  MinWindowWidth =
    WINDOW_WIDTH * CharWidth;
  MinWindowHeight =
    WINDOW_HEIGHT * CharHeight  +
    2 * SEPARATOR_HEIGHT +
    ularn_menu_height;

  //
  // Update the window size
  //
  if (MinWindowWidth > LarnWindowWidth)
  {
    LarnWindowWidth = MinWindowWidth;
  }
  if (MinWindowHeight > LarnWindowHeight)
  {
    LarnWindowHeight = MinWindowHeight;
  }
}

/*
 * Repaint flag to force redraw of everything, not just deltas
 */
static int Repaint = 0;


/* =============================================================================
 * FUNCTION: DrawString
 *
 * DESCRIPTION:
 * Draw a string at position x, y
 *
 * PARAMETERS:
 *
 *   x, y = upper left corner of string
 *   str  = string (ASCII encoded)
 *   fg   = foreground color
 *   bg   = background color
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void
DrawString(int x, int y, const char *str, SDL_Color fg, SDL_Color bg)
{
    SDL_Surface *text;

    text = TTF_RenderText_Shaded(font_info, str, fg, bg);
//    text = TTF_RenderText_Solid(font_info, str, fg);
    if (text) {
        SDL_Rect dst;

        dst.x = x; dst.y = y;
        dst.w = text->w; dst.h = text->h;
        SDL_BlitSurface(text, NULL, ularn_window, &dst);
        SDL_FreeSurface(text);
    }
}

static void
FillRectangle(int x, int y, int w, int h, unsigned int color)
{
    SDL_Rect dst;
    dst.x = x; dst.y = y;
    dst.w = w; dst.h = h;
    SDL_FillRect(ularn_window, &dst, color);
}

static void
CopyArea(SDL_Surface *srcsurf, SDL_Surface *dstsurf, int sx, int sy, int sw, int sh,
         int dx, int dy)
{
    SDL_Rect src;
    SDL_Rect dst;
    src.x = sx; src.y = sy;
    src.w = dst.w = sw; src.h = dst.h = sh;
    dst.x = dx; dst.y = dy;

    SDL_BlitSurface(srcsurf, &src, dstsurf, &dst);
//    SDL_UpdateRect(dstsurf, dst.x, dst.y, dst.w, dst.h);
}

/* =============================================================================
 * FUNCTION: PaintStatus
 *
 * DESCRIPTION:
 * Paint the status area.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void PaintStatus(void)
{
  char Line[81];
  char Buf[81];
  int i;

  SDL_FillRect(ularn_window, &StatusRect, white_pixel);


  //
  // Build the top status line
  //
  Line[0] = 0;

  /* Spells */
  if (c[SPELLMAX]>99)
    sprintf(Buf, "Spells:%3ld(%3ld)", c[SPELLS],c[SPELLMAX]);
  else
    sprintf(Buf, "Spells:%3ld(%2ld) ",c[SPELLS],c[SPELLMAX]);

  strcat(Line, Buf);

  /* AC, WC */
  sprintf(Buf, " AC: %-3ld  WC: %-3ld  Level", c[AC], c[WCLASS]);
  strcat(Line, Buf);

  /* Level */
  if (c[LEVEL]>99)
    sprintf(Buf, "%3ld", c[LEVEL]);
  else
    sprintf(Buf, " %-2ld", c[LEVEL]);
  strcat(Line, Buf);

  /* Exp, class */
  sprintf(Buf, " Exp: %-9ld %s", c[EXPERIENCE], class[c[LEVEL]-1]);
  strcat(Line, Buf);

  DrawString(StatusRect.x, StatusRect.y, Line, Black, White);

  //
  // Format the second line of the status
  //
  sprintf(Buf, "%ld (%ld)", c[HP], c[HPMAX]);

  sprintf(Line, "HP: %11s STR=%-2ld INT=%-2ld WIS=%-2ld CON=%-2ld DEX=%-2ld CHA=%-2ld LV:",
      Buf,
      c[STRENGTH]+c[STREXTRA],
      c[INTELLIGENCE],
      c[WISDOM],
      c[CONSTITUTION],
      c[DEXTERITY],
      c[CHARISMA]);

  if ((level==0) || (wizard))
    c[TELEFLAG]=0;

  if (c[TELEFLAG])
    strcat(Line, " ?");
  else
  strcat(Line, levelname[level]);

  sprintf(Buf, "  Gold: %-8ld", c[GOLD]);
  strcat(Line, Buf);

  DrawString(StatusRect.x, StatusRect.y + CharHeight, Line, Black, White);

  //
  // Mark all character values as displayed.
  //
  c[TMP] = c[STRENGTH]+c[STREXTRA];
  for (i=0; i<100; i++)
    cbak[i]=c[i];

}

/* Effects strings */
static struct bot_side_def
{
  int typ;
  char *string;
} bot_data[] =
{
  { STEALTH,        "Stealth   " },
  { UNDEADPRO,      "Undead Pro" },
  { SPIRITPRO,      "Spirit Pro" },
  { CHARMCOUNT,     "Charm     " },
  { TIMESTOP,       "Time Stop " },
  { HOLDMONST,      "Hold Monst" },
  { GIANTSTR,       "Giant Str " },
  { FIRERESISTANCE, "Fire Resit" },
  { DEXCOUNT,       "Dexterity " },
  { STRCOUNT,       "Strength  " },
  { SCAREMONST,     "Scare     " },
  { HASTESELF,      "Haste Self" },
  { CANCELLATION,   "Cancel    " },
  { INVISIBILITY,   "Invisible " },
  { ALTPRO,         "Protect 3 " },
  { PROTECTIONTIME, "Protect 2 " },
  { WTW,            "Wall-Walk " }
};

/* =============================================================================
 * FUNCTION: PaintEffects
 *
 * DESCRIPTION:
 * Paint the effects display.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void PaintEffects(void)
{
  int i, idx;
  int WasSet;
  int IsSet;

  if (Repaint)
    {
      SDL_FillRect(ularn_window, &EffectsRect, white_pixel);
    }

  for (i=0; i < 17; i++)
  {
    idx = bot_data[i].typ;
    WasSet = (cbak[idx] != 0);
    IsSet  = (c[idx] != 0);

    if ((Repaint) || (IsSet != WasSet))
    {
      if (IsSet)
      {
          DrawString(EffectsRect.x, EffectsRect.y + i * CharHeight, bot_data[i].string,
                     Black, White);
      }
      else
      {
          SDL_Rect tmp;
          tmp.x = EffectsRect.x;
          tmp.y = EffectsRect.y + i * CharHeight;
          tmp.w = EffectsRect.w;
          tmp.h = CharHeight;
          SDL_FillRect(ularn_window, &tmp, white_pixel);
      }
    }

    cbak[idx] = c[idx];
  }

}

/* =============================================================================
 * FUNCTION: GetTile
 *
 * DESCRIPTION:
 * Get the tile to be displayed for a location on the map.
 *
 * PARAMETERS:
 *
 *   x      : The x coordinate for the tile
 *
 *   y      : The y coordiante for the tile
 *
 *   TileId : This is set to the tile to be displayed for (x, y).
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void GetTile(int x, int y, int *TileId)
{
  MonsterIdType k;

  if ((x == playerx) && (y == playery) && (c[BLINDCOUNT] == 0))
  {
    //
    // This is the square containing the player and the players isn't
    // blind, so return the player tile.
    //
    *TileId = PlayerTiles[class_num][(int) sex];
    return;
  }

  //
  // Work out what is here
  //
  if (know[x][y] == OUNKNOWN)
  {
    //
    // The player doesn't know what is at this position.
    //
    *TileId = objtilelist[OUNKNOWN];
  }
  else
  {
    k = mitem[x][y].mon;
    if (k != 0)
    {
      if ((c[BLINDCOUNT] == 0) &&
          (((stealth[x][y] & STEALTH_SEEN) != 0) ||
           ((stealth[x][y] & STEALTH_AWAKE) != 0)))
      {
        //
        // There is a monster here and the player is not blind and the
        // monster is seen or awake.
        //
        if (k == MIMIC)
        {
          if ((gtime % 10) == 0)
          {
            while ((mimicmonst = rnd(MAXMONST))==INVISIBLESTALKER);
          }

          *TileId = monsttilelist[mimicmonst];
        }
        else if ((k==INVISIBLESTALKER) && (c[SEEINVISIBLE]==0))
        {
          *TileId = objtilelist[(int) know[x][y]];
        }
        else if ((k>=DEMONLORD) && (k<=LUCIFER) && (c[EYEOFLARN]==0))
        {
          /* demons are invisible if not have the eye */
          *TileId = objtilelist[(int) know[x][y]];
        }
        else
        {
          *TileId = monsttilelist[k];
        }

      } /* can see monster */
      else
      {
        /*
         * The monster at this location is not known to the player, so show
         * the tile for the item at this location
         */
        *TileId = objtilelist[(int) know[x][y]];
      }
    } /* monster here */
    else
    {
      k = know[x][y];
      *TileId = objtilelist[k];
    }
  }
  
  /* Handle walls */
  if (*TileId == objtilelist[OWALL])
  {
    *TileId = WALL_TILES + iarg[x][y];
  }
}

/* =============================================================================
 * FUNCTION: PaintMap
 *
 * DESCRIPTION:
 * Repaint the map.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void PaintMap(void)
{
  int x, y;
  int sx, sy;
  int mx, my;
  int TileId;
  int TileX;
  int TileY;


  if (Repaint)
    {
      SDL_FillRect(ularn_window, &MapAreaRect, black_pixel);
    }


  mx = MapTileRect.x + MapTileRect.w;
  my = MapTileRect.y + MapTileRect.h;

  if (my > MAXY)
  {
    my = MAXY;
  }

  if (mx > MAXX)
  {
    mx = MAXX;
  }

  sx = 0;
  for (x = MapTileRect.x ; x < mx ; x++)
  {
    sy = 0;
    for (y = MapTileRect.y ; y < my ; y++)
    {
      GetTile(x, y, &TileId);

      TileX = (TileId % 16) * TileWidth;
      TileY = (TileId / 16) * TileHeight;

      CopyArea(TilePixmap, ularn_window, TileX, TileY, TileWidth, TileHeight,
               MapRect.x + sx * TileWidth,
               MapRect.y + sy * TileHeight);
      sy++;
    }

    sx++;
  }

  sx = playerx - MapTileRect.x;
  sy = playery - MapTileRect.y;

  if ((sx >= 0) && (sx < MapTileRect.w) &&
      (sy >= 0) && (sy < MapTileRect.h))
    {
      TileId = TILE_CURSOR2;
      TileX = (TileId % 16) * TileWidth;
      TileY = (TileId / 16) * TileHeight;
      CopyArea(TilePixmapKeyed, ularn_window,
		TileX, TileY,
		TileWidth, TileHeight,
		MapRect.x + sx*TileWidth, 
		MapRect.y + sy*TileHeight);
    }
}

/* =============================================================================
 * FUNCTION: PaintTextWindow
 *
 * DESCRIPTION:
 * Repaint the window in text mode.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void PaintTextWindow(void)
{
  int sx, ex, y;
  FormatType Fmt;
  int FillX, FillY;
  int FillWidth, FillHeight;
  SDL_Color fgcolor, bgcolor;

  FillX = TLeft;
  FillY  = TTop;
  FillWidth = TWidth;
  FillHeight = THeight;

  if (CurrentDisplayMode == DISPLAY_TEXT)
  {
    if (ShowTextBorder)
    {
      //
      // Clear the drawable area
      //
      FillX = 0;
      FillY = ularn_menu_height;
      FillWidth = LarnWindowWidth;
      FillHeight = LarnWindowHeight - ularn_menu_height;

      FillRectangle(FillX, FillY, FillWidth, FillHeight, black_pixel);

#if 0
      XSetForeground(display, ularn_gc, white_pixel);
      XSetBackground(display, ularn_gc, black_pixel);

      values.line_width = 2;
      XChangeGC(display, ularn_gc, GCLineWidth, &values);

      XDrawArc(display, ularn_window, ularn_gc,
	       TLeft - 8, TTop - 8, 16, 16, 
	       90 * 64, 90 * 64);

      XDrawArc(display, ularn_window, ularn_gc,
	       TLeft - 8, TTop + THeight - 8, 16, 16, 
	       180 * 64, 90 * 64);

      XDrawArc(display, ularn_window, ularn_gc,
	       TLeft + TWidth - 8, TTop - 8, 16, 16, 
	       0 * 64, 90 * 64);

      XDrawArc(display, ularn_window, ularn_gc,
	       TLeft + TWidth - 8, TTop + THeight - 8, 16, 16, 
	       270 * 64, 90 * 64);

      XDrawLine(display, ularn_window, ularn_gc,
		TLeft, TTop - 8, 
		TLeft + TWidth, TTop - 8);

      XDrawLine(display, ularn_window, ularn_gc,
		TLeft, TTop + THeight +  8, 
		TLeft + TWidth, TTop + THeight + 8);

      XDrawLine(display, ularn_window, ularn_gc,
		TLeft - 8 , TTop, 
		TLeft - 8, TTop + THeight);

      XDrawLine(display, ularn_window, ularn_gc,
		TLeft + TWidth + 8, TTop, 
		TLeft + TWidth + 8, TTop + THeight);

      values.line_width = 0;
      XChangeGC(display, ularn_gc, GCLineWidth, &values);
#endif
      FillX = TLeft;
      FillY  = TTop;
      FillWidth = TWidth;
      FillHeight = THeight;

    }
    else
    {
      //
      // Not enough space around the text area for a border
      // Fill the entire drawable area
      //
      FillX = 0;
      FillY = ularn_menu_height;
      FillWidth = LarnWindowWidth;
      FillHeight = LarnWindowHeight - ularn_menu_height;
      
    }
  }

  FillRectangle(FillX, FillY, FillWidth, FillHeight, white_pixel);

  fgcolor = Black;
  bgcolor = White;

  for (y = 0 ; y < MaxLine ; y++)
    {
      sx = 0;
      char sbuf[LINE_LENGTH+1];

      while (sx < LINE_LENGTH)
	{
	  Fmt = Format[y][sx];
	  ex = sx;
	  
	  while ((ex < LINE_LENGTH) && (Format[y][ex] == Fmt)) ex++;
	  
	  switch (Fmt)
	    {
	    case FORMAT_NORMAL:
              fgcolor = Black;
	      break;
	    case FORMAT_STANDOUT:
	      fgcolor = Red;
	      break;
	    case FORMAT_STANDOUT2:
	      fgcolor = Green;
	      break;
	    case FORMAT_STANDOUT3:
	      fgcolor = Blue;
	      break;
	    default:
	      break;
	    }

          strncpy(sbuf, Text[y]+sx, ex - sx);
          sbuf[ex - sx] = 0;

	  DrawString( TLeft + sx * CharWidth,
		      TTop + y * CharHeight,
		      sbuf, fgcolor, bgcolor );
	  
	  sx = ex;
	}
    }
  
}

/* =============================================================================
 * FUNCTION: PaintMapWindow
 *
 * DESCRIPTION:
 * Repaint the window in map mode
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void PaintMapWindow(void)
{

  //
  // Draw separators between the different window areas
  //

  //
  // Message area
  //
  FillRectangle( MessageRect.x, MessageRect.y - SEPARATOR_HEIGHT,
		 MessageRect.w, 2, LtGrey_pixel);

  FillRectangle( MessageRect.x, MessageRect.y - SEPARATOR_HEIGHT + 2,
		 MessageRect.w, 4, MidGrey_pixel);

  FillRectangle( MessageRect.x, MessageRect.y - SEPARATOR_HEIGHT + 6,
		 MessageRect.w, 2, DkGrey_pixel);

  //
  // Status area
  //
  FillRectangle( StatusRect.x, StatusRect.y - SEPARATOR_HEIGHT,
		 StatusRect.w, 2, LtGrey_pixel);

  FillRectangle( StatusRect.x, StatusRect.y - SEPARATOR_HEIGHT + 2,
                  StatusRect.w, 4, MidGrey_pixel);

  FillRectangle( StatusRect.x, StatusRect.y - SEPARATOR_HEIGHT + 6,
		 StatusRect.w, 2, DkGrey_pixel);

  //
  // Effects area
  //
  FillRectangle( EffectsRect.x - SEPARATOR_WIDTH, EffectsRect.y,
		 2, EffectsRect.h, LtGrey_pixel);

  FillRectangle( EffectsRect.x - SEPARATOR_WIDTH + 2, EffectsRect.y,
		 4, EffectsRect.h + 2, MidGrey_pixel);

  FillRectangle( EffectsRect.x - SEPARATOR_WIDTH + 6, EffectsRect.y,
                 2, EffectsRect.h, DkGrey_pixel);

  PaintStatus();
  PaintEffects();
  PaintMap();
  PaintTextWindow();
}

/* =============================================================================
 * FUNCTION: PaintWindow
 *
 * DESCRIPTION:
 * Repaint the window.
 *
 * PARAMETERS:
 *
 *   None.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void PaintWindow(void)
{
  
  Repaint = 1;

  SDL_BlitSurface(MenuPixmap, NULL, ularn_window, &MenuRect);
  if (CurrentDisplayMode == DISPLAY_MAP)
  {
    PaintMapWindow();
  }
  else
  {
    PaintTextWindow();
  }

  Repaint = 0;
  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);
}

/* =============================================================================
 * FUNCTION: Resize
 *
 * DESCRIPTION:
 * This procedure handles resizing the window in response to any event that
 * requires the sub-window size and position to be recalculated.
 *
 * PARAMETERS:
 *
 *   w,h: new window width and height
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void Resize(int newW, int newH)
{
  int ClientWidth;
  int ClientHeight;

  LarnWindowWidth = newW;
  LarnWindowHeight = newH;

  ClientWidth = LarnWindowWidth;
  ClientHeight = LarnWindowHeight;

  //
  // Calculate the message window size and position
  //
  MessageRect.w = ClientWidth;
  MessageRect.h = CharHeight * MAX_MSG_LINES;
  MessageRect.x = 0;
  MessageRect.y = ClientHeight - MessageRect.h - 1;

  //
  // Calculate the Status window size and position
  //
  StatusRect.x = 0;
  StatusRect.y = (MessageRect.y - SEPARATOR_HEIGHT) - CharHeight * 2;
  StatusRect.w = ClientWidth;
  StatusRect.h = CharHeight * 2;

  //
  // Calculate the Effects window size and position
  //
  EffectsRect.x = ClientWidth - CharWidth * 10;
  EffectsRect.y = ularn_menu_height;
  EffectsRect.w = CharWidth * 10;
  EffectsRect.h = StatusRect.y - SEPARATOR_HEIGHT - EffectsRect.y;

  //
  // Calculate the size and position of the map window
  //
  MapAreaRect.x = 0;
  MapAreaRect.y = ularn_menu_height;

  MapRect.x = 0;
  MapRect.y = MapAreaRect.y;
  MapRect.w = EffectsRect.x - SEPARATOR_WIDTH;
  MapRect.h = StatusRect.y - SEPARATOR_HEIGHT - MapRect.y;

  MapAreaRect.w = MapRect.w;
  MapAreaRect.h = MapRect.h;

  MapTileRect = MapRect;
  MapTileRect.w = MapRect.w / TileWidth;
  MapTileRect.h = MapRect.h / TileHeight;

  //
  // Calculate the size and position of the text window
  //

  TextRect.w = CharWidth * LINE_LENGTH;
  TextRect.h = CharHeight * MAX_TEXT_LINES;

  TextRect.x = (ClientWidth - TextRect.w) / 2;
  TextRect.y =
      ularn_menu_height +
      (ClientHeight - TextRect.h) / 2;

  //
  // Check if should draw a border around the text page when it is displayed
  //
  ShowTextBorder = (TextRect.x >= BORDER_SIZE) && 
    (TextRect.y >= ularn_menu_height + BORDER_SIZE);


  //
  // If the map window is bigger than required to display the map, then centre
  // the map in the window.
  //

  if (MapTileRect.w > MAXX)
  {
    MapTileRect.w = MAXX;
    MapRect.x = (MapRect.w - MapTileRect.w * TileWidth) / 2;
  }

  if (MapTileRect.h > MAXY)
  {
    MapTileRect.h = MAXY;
    MapRect.y = MapAreaRect.y + (MapRect.h - MapTileRect.h * TileHeight) / 2;
  }

  if (CurrentDisplayMode == DISPLAY_MAP)
  {
    TLeft = MessageRect.x;
    TTop = MessageRect.y;
    TWidth = MessageRect.w;
    THeight = MessageRect.h;
  }
  else
  {
    TLeft = TextRect.x;
    TTop = TextRect.y;
    TWidth = TextRect.w;
    THeight = TextRect.h;
  }

  //
  // calculate the map scroll position for the current player position
  //

  calc_scroll();

  //
  // Force the window to redraw
  //
  PaintWindow();

  if (CaretActive)
    {
#ifdef NOTFINISHED
      XSetForeground(display, ularn_gc, black_pixel);
      XSetClipOrigin(display, ularn_gc, 
		     TLeft + (CursorX - 1) * CharWidth, 
		     TTop + (CursorY - 1) * CharHeight + CharAscent);
      XSetClipMask(display, ularn_gc, CursorPixmap);
      XCopyPlane(display, CursorPixmap, ularn_window, ularn_gc,
		 0, 0,
		 cursor_width, cursor_height,
		 TLeft + (CursorX - 1) * CharWidth, 
		 TTop + (CursorY - 1) * CharHeight + CharAscent, 1);
      
      XSetClipOrigin(display, ularn_gc, 0, 0);
      XSetClipMask(display, ularn_gc, None);
#endif
    }

}

/*
 * handle a click in the menu area
 */
typedef struct MenuStruct {
    int w, h;        // size of button
    int ch;          // character to press
    ActionType act;  // action to emulate
} MenuItem;

MenuItem menus[] = {
    { 42, 28, '\e', ACTION_NONE },
    { 40, 28, '\r', ACTION_NONE },
    { 36, 28, 'i', ACTION_INVENTORY },
    { 51, 28, 'd', ACTION_DROP },
    { 57, 28, 'W', ACTION_WEAR },
    { 58, 28, 'w', ACTION_WIELD },
    { 57, 28, 'q', ACTION_QUAFF },
    { 83, 28, 'T', ACTION_REMOVE_ARMOUR },
    { 49, 28, '?', ACTION_HELP },
    { 45, 28, 'Q', ACTION_QUIT },
};

#define NUM_MENU_ITEMS (sizeof(menus)/sizeof(menus[0]))
#define MENU_BORDER 4

static void
handle_menu(int bx, int by)
{
    int i;
    int x = 0;

    for (i = 0; i < NUM_MENU_ITEMS; i++) {
        x += menus[i].w;
        if (bx < x) {
            Event = menus[i].act;
            GotChar = 0;
            if (Event == ACTION_NONE) {
                GotChar = 1;
                EventChar = menus[i].ch;
            }
            return;
        }
    }
}

/* =============================================================================
 * FUNCTION: handle_mouse
 *
 * DESCRIPTION:
 * This procedure handles the processing for X events.
 *
 * PARAMETERS:
 *
 *   button : The SDL MouseButtonEvent to process.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void
handle_mouse(SDL_MouseButtonEvent *button)
{
    int bx, by;
    int deltax, deltay;
    static ActionType ClickActions[9] = {
        ACTION_MOVE_NORTHWEST,
        ACTION_MOVE_NORTH,
        ACTION_MOVE_NORTHEAST,
        ACTION_MOVE_WEST,
        ACTION_WAIT,
        ACTION_MOVE_EAST,
        ACTION_MOVE_SOUTHWEST,
        ACTION_MOVE_SOUTH,
        ACTION_MOVE_SOUTHEAST
    };

    if (button->type == SDL_MOUSEBUTTONUP)
        return;
    bx = button->x;
    by = button->y;

    if ( (bx >= MenuRect.x && (bx - MenuRect.x) < MenuRect.w)
         && (by >= MenuRect.y && (by - MenuRect.y) < MenuRect.h - MENU_BORDER) )
    {
        handle_menu(bx - MenuRect.x, by - MenuRect.y);
    }
    else if ( CurrentDisplayMode == DISPLAY_MAP
         && (bx >= MapRect.x && (bx - MapRect.x) < MapRect.w)
         && (by >= MapRect.y && (by - MapRect.y) < MapRect.h) )
    {
        bx = ((bx - MapRect.x) / TileWidth) + MapTileRect.x;
        by = ((by - MapRect.y) / TileHeight) + MapTileRect.y;
        deltax = deltay = 1;
        if (bx < playerx)
            deltax = 0;
        else if (bx > playerx)
            deltax = 2;
        if (by < playery)
            deltay = 0;
        else if (by > playery)
            deltay = 2;

        Event = ClickActions[deltay * 3 + deltax];
        GotChar = 0;
    }
    else if ( ( bx >= TLeft && (bx - TLeft) < TWidth )
              && (by >= TTop && (by - TTop) < THeight ) )
    {
        int ch;

        bx = (bx - TLeft) / CharWidth;
        by = (by - TTop) / CharHeight;

        ch = Text[by][bx];
        if (ch == '(' && bx < LINE_LENGTH-1)
            ch = Text[by][bx+1];
        else if (ch == ')' && bx > 0)
            ch = Text[by][bx-1];
        
        EventChar = ch;
        GotChar = 1;
        return;
    }
}

/* =============================================================================
 * FUNCTION: handle_event
 *
 * DESCRIPTION:
 * This procedure handles the processing for X events.
 *
 * PARAMETERS:
 *
 *   event : The SDL event to process.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void handle_event(SDL_Event *event)
{
  //
  // What is the message
  //
  switch (event->type)
    {
    case SDL_VIDEOEXPOSE:      
	{
	  PaintWindow();
	}
      break;
      
    case SDL_VIDEORESIZE:
      Resize(event->resize.w, event->resize.h);
      break;

    case SDL_QUIT:
        Event = ACTION_QUIT;
        break;
    case SDL_MOUSEBUTTONDOWN:
        handle_mouse(&event->button);
        break;
    case SDL_KEYDOWN:
    {
      ActionType Action;
      int ModKey = 0;
      int Found = 0;
      SDL_keysym *keysym;
      char KeyString[40];
      int i;


      keysym = &event->key.keysym;

      if ((keysym->mod & (KMOD_LSHIFT|KMOD_RSHIFT)) != 0)
	{
	  ModKey |= M_SHIFT;
	}
      
      if ((keysym->mod & (KMOD_LCTRL|KMOD_RCTRL)) != 0)
	{
	  ModKey |= M_CTRL;
	}

      /* Get ASCII character */
 
      EventChar = keysym->unicode;
      GotChar = (EventChar != 0);

      /* Decode key press as a ULarn Action */

      Action = ACTION_NULL;

      /* Check virtual key bindings */

      while ((Action < ACTION_COUNT) && (!Found))
	{
	  for (i = 0 ; i < MAX_KEY_BINDINGS ; i++)
	    {
	      
	      if (KeyMap[Action][i].ModKey != M_ASCII)
		{
		  /* Virtual key binding */
		  if ((keysym->sym == KeyMap[Action][i].VirtKey) &&
		      (KeyMap[Action][i].ModKey == ModKey))
		    {
		      Found = 1;
		    }
		}
	    }

	  if (!Found)
	    {
	      Action++;
	    }
	}

      /* 
       * Check ASCII key bindings if no virtual key matches and 
       * got a valid ASCII char 
       */

      if (!Found && GotChar)
	{
	  Action = ACTION_NULL;

	  while ((Action < ACTION_COUNT) && (!Found))
	    {
	      for (i = 0 ; i < MAX_KEY_BINDINGS ; i++)
		{
		  if (KeyMap[Action][i].ModKey == M_ASCII)
		    {
		      /* ASCII key binding */
		      if (EventChar == KeyMap[Action][i].VirtKey)
			{
			  Found = 1;
			}
		    }	      
		}
	
	      if (!Found)
		{
		  Action++;
		}
	    }
	}
      
      if (Found)
	{
	  Event = Action;
	}
      else
	{
	  /* check run key */
	  if ((keysym->sym == RunKeyMap.VirtKey) &&
	      (RunKeyMap.ModKey == ModKey))
	    {
	      Runkey = 1;
	    }
	}
      
      break;
    }
    
    default:
      break;

  } 
}

#if 0
#define MASK_TILES 9
static int MaskTiles[MASK_TILES][2] =
{
  { 240, 248 },
  { 241, 249 },
  { 242, 250 },
  { 243, 251 },
  { 244, 252 },
  { 245, 253 },
  { 246, 254 },
  { 247, 255 },
  { TILE_CURSOR1, TILE_CURSOR2 }
};
#endif

/* =============================================================================
 * Exported functions
 */

/* =============================================================================
 * FUNCTION: init_app
 */
int init_app(void)
{
  int x, y;
  int rc;
  char *LoadingText = "Loading data...";
  char *UlarnText = "UVLarn";

  if (SDL_Init( SDL_INIT_EVERYTHING ) == -1)
  {
      fprintf(stderr, "Error: Cannot initialize SDL\n");
      return 0;
  }
  if ( TTF_Init() == -1 )
  {
      fprintf(stderr, "Error: Cannot initialize SDL_ttf library\n");
      return 0;
  }

  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  ularn_window = SDL_SetVideoMode( INITIAL_WIDTH, INITIAL_HEIGHT, 32, SDL_SWSURFACE );
  if (ularn_window == NULL)
  {
      fprintf(stderr, "Error: Cannot initialize window\n");
      return 0;
  }

  white_pixel = SDL_MapRGB(ularn_window->format, 255, 255, 255);
  black_pixel = SDL_MapRGB(ularn_window->format, 0, 0, 0);
  LtGrey_pixel = SDL_MapRGB(ularn_window->format, LtGrey.r, LtGrey.g, LtGrey.b);
  DkGrey_pixel = SDL_MapRGB(ularn_window->format, DkGrey.r, DkGrey.g, DkGrey.b);
  MidGrey_pixel = SDL_MapRGB(ularn_window->format, MidGrey.r, MidGrey.g, MidGrey.b);

  /* Set the window name */
  SDL_WM_SetCaption( "UVLarn", NULL );

  /* Set up font */
  font_info = TTF_OpenFont(font_name, 12);
  if (!font_info)
    {
      fprintf(stderr, "Error: TTF_OpenFont: failed loading font '%s'\n", font_name);
      return 0;
    }

  TilePixmap = SDL_LoadBMP(TileFilename);
  TilePixmapKeyed = SDL_LoadBMP(TileFilename);

  if (TilePixmap == NULL || TilePixmapKeyed == NULL)
    {
        fprintf(stderr, "Error reading pixmap: %s\n", TileFilename);
        return 0;
    }

  SDL_SetColorKey(TilePixmapKeyed, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);

  MenuPixmap = SDL_LoadBMP(MenuFilename);
  if (MenuPixmap == NULL)
    {
        fprintf(stderr, "Error reading pixmap: %s\n", MenuFilename);
        return 0;
    }

  MenuRect.x = 0;
  MenuRect.y = 0;
  MenuRect.w = MenuPixmap->w;
  MenuRect.h = MenuPixmap->h;
  ularn_menu_height = MenuRect.h;

  //
  // Clear the text buffers
  //
  for (y = 0 ; y < MAX_MSG_LINES ; y++)
  {
    for (x = 0 ; x < LINE_LENGTH ; x++)
    {
      MessageChr[y][x] = ' ';
      MessageFmt[y][x] = FORMAT_NORMAL;
    }

    MessageChr[y][LINE_LENGTH] = 0;
  }

  for (y = 0 ; y < MAX_TEXT_LINES ; y++)
  {
    for (x = 0 ; x < LINE_LENGTH ; x++)
    {
      TextChr[y][x] = ' ';
      TextFmt[y][x] = FORMAT_NORMAL;
    }

    TextChr[y][LINE_LENGTH] = 0;
  }

  //
  // Set the initial window size
  //
  CalcMinWindowSize();

  Resize(LarnWindowWidth, LarnWindowHeight);

  return 1;
}

/* =============================================================================
 * FUNCTION: close_app
 */
void close_app(void)
{

  if (TilePixmap != NULL)
    {
      SDL_FreeSurface(TilePixmap);
      TilePixmap = NULL;
    }

    SDL_Quit();
}

/* =============================================================================
 * FUNCTION: get_normal_input
 */
ActionType get_normal_input(void)
{
  SDL_Event xevent;       // The X event
  int idx;
  int got_dir;

  Event = ACTION_NULL;
  Runkey = 0;

  while (Event == ACTION_NULL)
    {
      SDL_UpdateRect(ularn_window, 0, 0, 0, 0);
      SDL_WaitEvent(&xevent);

      handle_event(&xevent);
      
      //
      // Clear enhanced interface events in enhanced interface is not active
      //
      if (!enhance_interface)
	{
	  if ((Event == ACTION_OPEN_DOOR) ||
	      (Event == ACTION_OPEN_CHEST)) 
	    {
	      Event = ACTION_NULL;
	    }
	}
    }
  
  if (Runkey)
    {
      idx = 0;
      got_dir = 0;
      
      while ((idx < NUM_DIRS) && (!got_dir))
	{
	  if (DirActions[idx] == Event)
	    {
	      got_dir = 1;
	    }
	  else
	    {
	      idx++;
	    }
	}
      
    if (got_dir)
      {
	/* modify into a run event */
	Event = Event + 1;
      }
    }
  
  return Event;
}

/* =============================================================================
 * FUNCTION: get_prompt_input
 */
char get_prompt_input(char *prompt, char *answers, int ShowCursor)
{
  SDL_Event xevent;       // The X event
  char *ch;

  Print(prompt);

  if (ShowCursor)
    {
#if 0      
      XSetForeground(display, ularn_gc, black_pixel);
      XSetClipOrigin(display, ularn_gc, 
		     TLeft + (CursorX - 1) * CharWidth, 
		     TTop + (CursorY - 1) * CharHeight + CharAscent);
      XSetClipMask(display, ularn_gc, CursorPixmap);
      XCopyPlane(display, CursorPixmap, ularn_window, ularn_gc,
		 0, 0,
		 cursor_width, cursor_height,
		 TLeft + (CursorX - 1) * CharWidth, 
		 TTop + (CursorY - 1) * CharHeight + CharAscent, 1);

      XSetClipOrigin(display, ularn_gc, 0, 0);
      XSetClipMask(display, ularn_gc, None);
#endif      
      CaretActive = 1;
    }

  //
  // Process events until a character in answers has been pressed.
  //
  GotChar = 0;
  while (!GotChar)
    {
      SDL_WaitEvent(&xevent);
      
      handle_event(&xevent);

      if (GotChar)
	{

	  //
	  // Search for the input character in the answers string
	  //
	  ch = strchr(answers, EventChar);

	  if (ch == NULL)
	    {
	      //
	      // Not an answer we want
	      //
	      GotChar = 0;
	    }
	} 
    }

  if (ShowCursor)
    {
#if 0      
      XSetForeground(display, ularn_gc, white_pixel);
      XSetClipOrigin(display, ularn_gc, 
		     TLeft + (CursorX - 1) * CharWidth, 
		     TTop + (CursorY - 1) * CharHeight + CharAscent);
      XSetClipMask(display, ularn_gc, CursorPixmap);
      XCopyPlane(display, CursorPixmap, ularn_window, ularn_gc,
		 0, 0,
		 cursor_width, cursor_height,
		 TLeft + (CursorX - 1) * CharWidth, 
		 TTop + (CursorY - 1) * CharHeight + CharAscent, 1);

      XSetClipOrigin(display, ularn_gc, 0, 0);
      XSetClipMask(display, ularn_gc, None);
#endif      
      CaretActive = 0;
    }

  return EventChar;
}

/* =============================================================================
 * FUNCTION: get_password_input
 */
void get_password_input(char *password, int Len)
{
  char ch;
  char inputchars[256];
  int Pos;
  int value;

  /* get the printable characters on this system */
  Pos = 0;
  for (value = 0 ; value < 256 ; value++)
  {
    if (isprint(value))
    {
      inputchars[Pos] = (char) value;
      Pos++;
    }
  }

  /* add CR, BS and null terminator */
  inputchars[Pos++] = '\010';
  inputchars[Pos++] = '\015';
  inputchars[Pos] = '\0';

  Pos = 0;
  do
  {
    ch = get_prompt_input("", inputchars, 1);

    if (isprint((int) ch) && (Pos < Len))
    {
      password[Pos] = ch;
      Pos++;
      Printc('*');
    }
    else if (ch == '\010')
    {
      //
      // Backspace
      //

      if (Pos > 0)
      {
        CursorX--;
        Printc(' ');
        CursorX--;
        Pos--;
      }
    }

  } while (ch != '\015');

  password[Pos] = 0;


}

/* =============================================================================
 * FUNCTION: get_num_input
 */
int get_num_input(int defval)
{
  char ch;
  int Pos = 0;
  int value = 0;
  int neg = 0;

  do
  {
    ch = get_prompt_input("", "-*0123456789\010\015", 1);

    if ((ch == '-') && (Pos == 0))
    {
      //
      // Minus
      //
      neg = 1;
      Printc(ch);
      Pos++;
    }
    if (ch == '*')
    {
      return defval;
    }
    else if (ch == '\010')
    {
      //
      // Backspace
      //

      if (Pos > 0)
      {
        if ((Pos == 1) && neg)
        {
          neg = 0;
        }
        else
        {
          value = value / 10;
        }

        CursorX--;
        Printc(' ');
        CursorX--;
        Pos--;
      }
    }
    else if ((ch >= '0') && (ch <= '9'))
    {
      //
      // digit
      //
      value = value * 10 + (ch - '0');
      Printc(ch);
      Pos++;
    }

  } while (ch != '\015');

  if (Pos == 0)
  {
    return defval;
  }
  else
  {
    if (neg) value = -value;

    return value;
  }
}

/* =============================================================================
 * FUNCTION: get_dir_input
 */
ActionType get_dir_input(char *prompt, int ShowCursor)
{
  SDL_Event xevent;
  int got_dir;
  int idx;

  //
  // Display the prompt at the current position
  //
  Print(prompt);

  //
  // Show the cursor if required
  //
  if (ShowCursor)
    {
#if 0      
      XSetForeground(display, ularn_gc, black_pixel);
      XSetClipOrigin(display, ularn_gc, 
		     TLeft + (CursorX - 1) * CharWidth, 
		     TTop + (CursorY - 1) * CharHeight + CharAscent);
      XSetClipMask(display, ularn_gc, CursorPixmap);
      XCopyPlane(display, CursorPixmap, ularn_window, ularn_gc,
		 0, 0,
		 cursor_width, cursor_height,
		 TLeft + (CursorX - 1) * CharWidth, 
		 TTop + (CursorY - 1) * CharHeight + CharAscent, 1);
      
      XSetClipOrigin(display, ularn_gc, 0, 0);
      XSetClipMask(display, ularn_gc, None);
#endif     
      CaretActive = 1;
  }

  Event = ACTION_NULL;
  got_dir = 0;

  while (!got_dir)
  {
    SDL_WaitEvent(&xevent);
    
    handle_event(&xevent);
     
    idx = 0;

    while ((idx < NUM_DIRS) && (!got_dir))
    {
      if (DirActions[idx] == Event)
      {
        got_dir = 1;
      }
      else
      {
        idx++;
      }
    }

  }

  if (ShowCursor)
    {
#if 0      
      XSetForeground(display, ularn_gc, white_pixel);
      XSetClipOrigin(display, ularn_gc, 
		     TLeft + (CursorX - 1) * CharWidth, 
		     TTop + (CursorY - 1) * CharHeight + CharAscent);
      XSetClipMask(display, ularn_gc, CursorPixmap);
      XCopyPlane(display, CursorPixmap, ularn_window, ularn_gc,
		 0, 0,
		 cursor_width, cursor_height,
		 TLeft + (CursorX - 1) * CharWidth, 
		 TTop + (CursorY - 1) * CharHeight + CharAscent, 1);
      
      XSetClipOrigin(display, ularn_gc, 0, 0);
      XSetClipMask(display, ularn_gc, None);
    CaretActive = 0;
#endif
  }

  return Event;

}

/* =============================================================================
 * FUNCTION: UpdateStatus
 */
void UpdateStatus(void)
{
  if (CurrentDisplayMode == DISPLAY_TEXT)
  {
    /* Don't redisplay if in text mode */
    return;
  }

  PaintStatus();
  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);
}

/* =============================================================================
 * FUNCTION: UpdateEffects
 */
void UpdateEffects(void)
{
  if (CurrentDisplayMode == DISPLAY_TEXT)
  {
    /* Don't redisplay if in text mode */
    return;
  }

  PaintEffects();
  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);

}

/* =============================================================================
 * FUNCTION: UpdateStatusAndEffects
 */
void UpdateStatusAndEffects(void)
{
  if (CurrentDisplayMode == DISPLAY_TEXT)
  {
    /* Don't redisplay if in text mode */
    return;
  }
  
  //
  // Do effects first as update status will mark all effects as current
  //
  PaintEffects();
  PaintStatus();

}

/* =============================================================================
 * FUNCTION: set_display
 */
void set_display(DisplayModeType Mode)
{
  //
  // Save the current settings
  //
  if (CurrentDisplayMode == DISPLAY_MAP)
  {
    MsgCursorX = CursorX;
    MsgCursorY = CursorY;
    CurrentMsgFormat = CurrentFormat;
  }
  else if (CurrentDisplayMode == DISPLAY_TEXT)
  {
    TextCursorX = CursorX;
    TextCursorY = CursorY;
    CurrentTextFormat = CurrentFormat;
  }

  CurrentDisplayMode = Mode;

  //
  // Set the text buffer settings for the new display mode
  //
  if (CurrentDisplayMode == DISPLAY_MAP)
  {
    CursorX = MsgCursorX;
    CursorY = MsgCursorY;
    CurrentFormat = CurrentMsgFormat;

    Text = MessageChr;
    Format = MessageFmt;
    MaxLine = MAX_MSG_LINES;

    TLeft = MessageRect.x;
    TTop = MessageRect.y;
    TWidth = MessageRect.w;
    THeight = MessageRect.h;

  }
  else if (CurrentDisplayMode == DISPLAY_TEXT)
  {
    CursorX = TextCursorX;
    CursorY = TextCursorY;
    CurrentFormat = CurrentTextFormat;

    Text = TextChr;
    Format = TextFmt;
    MaxLine = MAX_TEXT_LINES;

    TLeft = TextRect.x;
    TTop = TextRect.y;
    TWidth = TextRect.w;
    THeight = TextRect.h;

  }

  PaintWindow();
  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);
}

/* =============================================================================
 * FUNCTION: IncCursorY
 *
 * DESCRIPTION:
 * Increae the cursor y position, scrolling the text window if requried.
 *
 * PARAMETERS:
 *
 *   Count : The number of lines to increase the cursor y position
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void IncCursorY(int Count)
{
  int Scroll;
  int inc;
  int Line;
  int x;

  inc = Count;
  Scroll = 0;

  while (inc > 0)
  {
    CursorY = CursorY + 1;

    if (CursorY > MaxLine)
    {
      Scroll = 1;
      for (Line = 0 ; Line < (MaxLine - 1) ; Line++)
      {
        for (x = 0 ; x < LINE_LENGTH ; x++)
        {
          Text[Line][x] = Text[Line+1][x];
          Format[Line][x] = Format[Line+1][x];
        }
      }
      CursorY--;

      for (x = 0 ; x < LINE_LENGTH ; x++)
      {
        Text[MaxLine-1][x] = ' ';
        Format[MaxLine-1][x] = FORMAT_NORMAL;
      }
    }

    inc--;
  }

  if (Scroll)
  {
    PaintTextWindow();
  }
}

/* =============================================================================
 * FUNCTION: IncCursorX
 *
 * DESCRIPTION:
 * Increase the cursor x position, handling line wrap.
 *
 * PARAMETERS:
 *
 *   Count : The amount to increase the cursor x position.
 *
 * RETURN VALUE:
 *
 *   None.
 */
static void IncCursorX(int Count)
{
  CursorX = CursorX + Count;
  if (CursorX > LINE_LENGTH)
  {
    CursorX = 1;
    IncCursorY(1);
  }
}

/* =============================================================================
 * FUNCTION: ClearText
 */
void ClearText(void)
{
  int x, y;

  //
  // Clear the text buffer
  //

  for (y = 0 ; y < MaxLine ; y++)
  {
    for (x = 0 ; x < LINE_LENGTH ; x++)
    {
      Text[y][x] = ' ';
      Format[y][x] = FORMAT_NORMAL;
    }

    Text[y][LINE_LENGTH] = 0;
  }

  CursorX = 1;
  CursorY = 1;

  //
  // Clear the text area
  //
  PaintTextWindow();

  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);

}

/* =============================================================================
 * FUNCTION: UlarnBeep
 */
void UlarnBeep(void)
{
#if 0
  //
  // Middle C, 1/4 second
  //
  if (!nobeep)
    {
      XBell(display, 100);
    }
#endif
}

/* =============================================================================
 * FUNCTION: Cursor
 */
void MoveCursor(int x, int y)
{
  CursorX = x;
  CursorY = y;
}

/* =============================================================================
 * FUNCTION: Printc
 */
void Printc(char c)
{
  int incx;
  char lc;
  SDL_Color fgcolor, bgcolor;
  char cbuf[2];

  switch (c)
    {
    case '\t':
      incx = ((((CursorX - 1) / 8) + 1) * 8 + 1) - CursorX;
      IncCursorX(incx);
      break;

    case '\n':
      CursorX = 1;
      IncCursorY(1);
      break;
      
    case '\015':
      break;
      
    default:

      lc = Text[CursorY-1][CursorX-1];

      if (lc != c)
	{
	  /* Erase the char that was here */
	  FillRectangle( TLeft + (CursorX - 1) * CharWidth,
			 TTop + (CursorY - 1) * CharHeight,
			 CharWidth, CharHeight, white_pixel);
	}
      
      Text[CursorY-1][CursorX-1] = c;
      Format[CursorY-1][CursorX-1] = CurrentFormat;

      fgcolor = Black;
      bgcolor = White;

      switch (CurrentFormat)
	{
        case FORMAT_NORMAL:
	  fgcolor = Black;
          break;
        case FORMAT_STANDOUT:
	  fgcolor = Red;
          break;
        case FORMAT_STANDOUT2:
          fgcolor = Green;
          break;
        case FORMAT_STANDOUT3:
	  fgcolor = Blue;
          break;
        default:
          break;
	}

      cbuf[0] = c;
      cbuf[1] = 0;
      DrawString( TLeft + (CursorX - 1) * CharWidth,
		  TTop + (CursorY - 1) * CharHeight,
		  cbuf, fgcolor, bgcolor);

      IncCursorX(1);
      break;
    }
}

/* =============================================================================
 * FUNCTION: Print
 */
void Print(char *string)
{
  int Len;
  int pos;

  if (string == NULL) return;

  Len = strlen(string);

  if (Len == 0) return;

  for (pos = 0 ; pos < Len ; pos++)
  {
    Printc(string[pos]);
  }

  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);

}

/* =============================================================================
 * FUNCTION: Printf
 */
void Printf(char *fmt, ...)
{
  char buf[2048];
  va_list argptr;

  va_start(argptr, fmt);
  vsprintf(buf, fmt, argptr);
  va_end(argptr);

  Print(buf);
}

/* =============================================================================
 * FUNCTION: Standout
 */
void Standout(char *String)
{
  CurrentFormat = FORMAT_STANDOUT;

  Print(String);

  CurrentFormat = FORMAT_NORMAL;
}

/* =============================================================================
 * FUNCTION: SetFormat
 */
void SetFormat(FormatType format)
{
  CurrentFormat = format;
}

/* =============================================================================
 * FUNCTION: ClearToEOL
 */
void ClearToEOL(void)
{
  int x;

  for (x = CursorX ; x <= LINE_LENGTH ; x++)
  {
    Text[CursorY-1][x-1] = ' ';
    Format[CursorY-1][x-1] = FORMAT_NORMAL;
  }


  FillRectangle( TLeft + (CursorX - 1) * CharWidth,
		 TTop + (CursorY - 1) * CharHeight,
		 ((LINE_LENGTH - CursorX) + 1) * CharWidth,
		 CharHeight, 
                 white_pixel );

}

/* =============================================================================
 * FUNCTION: ClearToEOPage
 */
void ClearToEOPage(int x, int y)
{
  int tx, ty;

  for (tx = x ; tx <= LINE_LENGTH ; tx++)
  {
    Text[y-1][tx-1] = ' ';
    Format[y-1][tx-1] = FORMAT_NORMAL;
  }

  FillRectangle( TLeft + (x - 1) * CharWidth,
		 TTop + (y - 1) * CharHeight,
		 ((LINE_LENGTH - x) + 1) * CharWidth,
		 CharHeight, white_pixel);

  for (ty = y+1 ; ty <= MaxLine ; ty++)
  {
    for (tx = 1 ; tx <= LINE_LENGTH ; tx++)
    {
      Text[ty-1][tx-1] = ' ';
      Format[ty-1][tx-1] = FORMAT_NORMAL;
    }

    FillRectangle( TLeft,
		   TTop + (ty - 1) * CharHeight,
		   LINE_LENGTH * CharWidth,
		   CharHeight, white_pixel);

  }

}

/* =============================================================================
 * FUNCTION: show1cell
 */
void show1cell(int x, int y)
{
  int TileId;
  int sx, sy;
  int TileX, TileY;

  /* see nothing if blind		*/
	if (c[BLINDCOUNT]) return;

  /* we end up knowing about it */
	know[x][y] = item[x][y];
  if (mitem[x][y].mon != MONST_NONE)
  {
    stealth[x][y] |= STEALTH_SEEN;
  }

  sx = x - MapTileRect.x;
  sy = y - MapTileRect.y;

  if ((sx < 0) || (sx >= MapTileRect.w) ||
      (sy < 0) || (sy >= MapTileRect.h))
  {
    //
    // Tile is not currently in the visible part of the map,
    // so don't draw anything
    //
    return;
  }

  GetTile(x, y, &TileId);

  TileX = (TileId % 16) * TileWidth;
  TileY = (TileId / 16) * TileHeight;

  CopyArea(TilePixmap, ularn_window,
	    TileX, TileY,
	    TileWidth, TileHeight,
	    MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);
}

/* =============================================================================
 * FUNCTION: showplayer
 */
void showplayer(void)
{
  int sx, sy;
  int TileId;
  int TileX, TileY;
  int scroll;

  //
  // Determine if we need to scroll the map
  //
  scroll = calc_scroll();

  if (scroll)
  {
    PaintMap();
  }
  else
  {
    sx = playerx - MapTileRect.x;
    sy = playery - MapTileRect.y;

    if ((sx >= 0) && (sx < MapTileRect.w) &&
	(sy >= 0) && (sy < MapTileRect.h))
      {
	if (c[BLINDCOUNT] == 0)
	  {
	    TileId = PlayerTiles[class_num][(int) sex];
	  }
	else
	  {
	    GetTile(playerx, playery, &TileId);
	  }
	
	TileX = (TileId % 16) * TileWidth;
	TileY = (TileId / 16) * TileHeight;
	
	CopyArea(TilePixmap, ularn_window,
		  TileX, TileY,
		  TileWidth, TileHeight,
		  MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);
	
	TileId = TILE_CURSOR2;
	TileX = (TileId % 16) * TileWidth;
	TileY = (TileId / 16) * TileHeight;
	
	CopyArea(TilePixmapKeyed, ularn_window,
		  TileX, TileY,
		  TileWidth, TileHeight,
		  MapRect.x + sx*TileWidth, 
		  MapRect.y + sy*TileHeight);
      } /* If player on visible map area */
  }
}

/* =============================================================================
 * FUNCTION: showcell
 */
void showcell(int x, int y)
{
  int minx, maxx;
  int miny, maxy;
  int mx, my;
  int sx, sy;
  int TileX, TileY;
  int TileId;
  int scroll;

  //
  // Determine if we need to scroll the map
  //
  scroll = calc_scroll();

  /*
   * Decide how much the player knows about around him/her.
   */
  if (c[AWARENESS])
    {
      minx = x-3;
      maxx = x+3;
      miny = y-3;
      maxy = y+3;
    }
  else
    {
      minx = x-1;
      maxx = x+1;
      miny = y-1;
      maxy = y+1;
    }
  
  if (c[BLINDCOUNT])
    {
      minx = x;
      maxx = x;
      miny = y;
      maxy = y;
      
      //
      // Redraw the last player position to remove the cursor
      //
      
      if (!scroll)
	{
	  //
	  // Only redraw if the map is not going to be completely redrawn.
	  //
	  sx = lastpx - MapTileRect.x;
	  sy = lastpy - MapTileRect.y;
	  
	  if ((sx >= 0) && (sx < MapTileRect.w) &&
	      (sy >= 0) && (sy < MapTileRect.h))
	    {
	      //
	      // Tile is currently visible, so draw it
	      //
	      
	      GetTile(lastpx, lastpy, &TileId);
	      
	      TileX = (TileId % 16) * TileWidth;
	      TileY = (TileId / 16) * TileHeight;
	      
	      CopyArea(TilePixmap, ularn_window,
			TileX, TileY,
			TileWidth, TileHeight,
			MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);
	    }
	  
	}
      
    }
  
  /*
   * Limit the area to the map extents
   */
  if (minx < 0) minx = 0;
  if (maxx > MAXX-1) maxx = MAXX-1;
  if (miny < 0) miny=0;
  if (maxy > MAXY-1) maxy = MAXY-1;
  
  for (my = miny; my <= maxy; my++)
    {
      for (mx = minx; mx <= maxx; mx++)
	{
	  if ((mx == playerx) && (my == playery))
	    {
	      know[mx][my] = item[mx][my];
	      if (!scroll)
		{
		  //
		  // Only draw if the entire map is not going to be scrolled
		  //
		  showplayer();
		}
	    }
	  else if ((know[mx][my] != item[mx][my]) ||       /* item changed    */
		   ((mx == lastpx) && (my == lastpy)) ||   /* last player pos */
		   ((mitem[mx][my].mon != MONST_NONE) &&   /* unseen monster  */
		    ((stealth[mx][my] & STEALTH_SEEN) == 0)))
	    {
	      //
	      // Only draw areas not already known (and hence displayed)
	      //
	      know[mx][my] = item[mx][my];
	      if (mitem[mx][my].mon != MONST_NONE)
		{
		  stealth[mx][my] |= STEALTH_SEEN;
		}
	      
	      if (!scroll)
		{
		  //
		  // Only draw the tile if the map is not going to be scrolled
		  //
		  sx = mx - MapTileRect.x;
		  sy = my - MapTileRect.y;
		  
		  if ((sx >= 0) && (sx < MapTileRect.w) &&
		      (sy >= 0) && (sy < MapTileRect.h))
		    {
		      //
		      // Tile is currently visible, so draw it
		      //
		      
		      GetTile(mx, my, &TileId);
		      
		      TileX = (TileId % 16) * TileWidth;
		      TileY = (TileId / 16) * TileHeight;
		      
		      CopyArea(TilePixmap, ularn_window,
				TileX, TileY,
				TileWidth, TileHeight,
				MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);
		      
		    }
		  
		}
	      
	    } // if not known
	  
	}
    }
  
  if (scroll)
    {
      /* scrolling the map window, so repaint everything and return */
      PaintMap();
    }

  SDL_UpdateRect(ularn_window, 0, 0, 0, 0);
}

/* =============================================================================
 * FUNCTION: drawscreen
 */
void drawscreen(void)
{
  PaintWindow();
}

/* =============================================================================
 * FUNCTION: draws
 */
void draws(int minx, int miny, int maxx, int maxy)
{
  PaintWindow();
}

/* =============================================================================
 * FUNCTION: mapeffect
 */
void mapeffect(int x, int y, DirEffectsType effect, int dir)
{
  int TileId;
  int sx, sy;
  int TileX, TileY;

  /* see nothing if blind		*/
	if (c[BLINDCOUNT]) return;

  sx = x - MapTileRect.x;
  sy = y - MapTileRect.y;

  if ((sx < 0) || (sx >= MapTileRect.w) ||
      (sy < 0) || (sy >= MapTileRect.h))
  {
    //
    // Tile is not currently in the visible part of the map,
    // so don't draw anything
    //
    return;
  }

  TileId = EffectTile[effect][dir];

  TileX = (TileId % 16) * TileWidth;
  TileY = (TileId / 16) * TileHeight;

  CopyArea(TilePixmap, ularn_window,
	    TileX, TileY,
	    TileWidth, TileHeight,
	    MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);
}

/* =============================================================================
 * FUNCTION: magic_effect_frames
 */
int magic_effect_frames(MagicEffectsType fx)
{
  return magicfx_tile[fx].Frames;
}

/* =============================================================================
 * FUNCTION: magic_effect
 */
void magic_effect(int x, int y, MagicEffectsType fx, int frame)
{
  int TileId;
  int sx, sy;
  int TileX, TileY;

  if (frame > magicfx_tile[fx].Frames)
  {
    return;
  }

  /*
   * draw the tile that is at this location
   */

  /* see nothing if blind		*/
	if (c[BLINDCOUNT]) return;

  sx = x - MapTileRect.x;
  sy = y - MapTileRect.y;

  if ((sx < 0) || (sx >= MapTileRect.w) ||
      (sy < 0) || (sy >= MapTileRect.h))
  {
    //
    // Tile is not currently in the visible part of the map,
    // so don't draw anything
    //
    return;
  }

  if (magicfx_tile[fx].Overlay)
  {
    GetTile(x, y, &TileId);

    TileX = (TileId % 16) * TileWidth;
    TileY = (TileId / 16) * TileHeight;

    CopyArea(TilePixmap, ularn_window,
	      TileX, TileY,
	      TileWidth, TileHeight,
	      MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);
 
    TileId = magicfx_tile[fx].Tile1[frame];
    TileX = (TileId % 16) * TileWidth;
    TileY = (TileId / 16) * TileHeight;

#if 0
    XSetClipOrigin(display, ularn_gc, 
		   MapRect.x + sx*TileWidth - TileX, 
		   MapRect.y + sy*TileHeight - TileY);
    XSetClipMask(display, ularn_gc, TilePShape);
#endif
    CopyArea(TilePixmap, ularn_window,
	      TileX, TileY,
	      TileWidth, TileHeight,
	      MapRect.x + sx*TileWidth, 
	      MapRect.y + sy*TileHeight);

      
  }
  else
  {
    TileId = magicfx_tile[fx].Tile1[frame];
    TileX = (TileId % 16) * TileWidth;
    TileY = (TileId / 16) * TileHeight;

    CopyArea(TilePixmap, ularn_window,
	      TileX, TileY,
	      TileWidth, TileHeight,
	      MapRect.x + sx*TileWidth, MapRect.y + sy*TileHeight);

  }
}

/* =============================================================================
 * FUNCTION: nap
 */
void nap(int delay)
{
  SDL_Flip(ularn_window);
  SDL_Delay(delay);
}

/* =============================================================================
 * FUNCTION: GetUser
 */
void GetUser(char *username, int *uid)
{

  *uid = getuid();
  
  strcpy(username, getenv("USER"));
}

