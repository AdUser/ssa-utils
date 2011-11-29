/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef _SSA_H
#define _SSA_H

/* this should be reasonable values */
#define MAX_FIELDS 24 /* (in ssa styles v4+) */

#define SSA_DEFAULT_FONT "Sans"

#define SSA_E_SORTED   0x01

#define MEDIA_UNKNOWN  0x0
#define MEDIA_HEADER   0x1
#define MEDIA_UUE_LINE 0x2
#define MEDIA_UUE_TAIL 0x3

typedef enum ssa_version
  {
    ssa_unknown,
    ssa_v3, /* legacy ssa format  */
    ssa_v4, /* common ssa format  */
    ssa_v4p /* advanced ssa (ASS) */
  } ssa_version;

typedef struct ssa_style
  {
    struct ssa_style *next;
    char *name;
    char *fontname;
    float fontsize;

    uint32_t pr_color;
    uint32_t se_color;
    uint32_t tr_color;
    uint32_t bg_color;

    /* true as -1, false as 0 */
    int8_t bold;
    int8_t italic;
    int8_t underlined;
    int8_t strikeout;

    /* 100 by default */
    uint8_t scale_x;
    uint8_t scale_y;

    uint8_t spacing;
    float angle; /* values from -360 to 360 */
    uint8_t brd_style;
    uint8_t outline;
    uint8_t shadow;
    uint8_t alignment;
/*  ^   ,---------.          ,---------.     *\
 *      |5   6   7|(+4)      |7   8   9|(+6) *
 *  ssa:|9  10  11|(+8) ass: |4   5   6|(+3) *
 *      |1   2   3|(+0)      |1   2   3|(+0) *
\*      `---------'          `---------'     */

    uint16_t margin_l;  /*  | ML subs MR |  */
    uint16_t margin_r;  /*  |     MB     |  */
    uint16_t margin_v;  /*  `------------'  */
    uint8_t a_level;    /* not used neither in ssa, nor in ass */
    uint8_t codepage;   /* 204 - russian */
  } ssa_style;

/* Defined order & names below matches ssa_v4+ *
 * For use with ssa_v4 - rename #6, throw away *
 * ##10-15,23 and update value in #4-7 and 19  */

#define STYLE_NAME     1 /* Name            */
#define STYLE_FONTNAME 2 /* Fontname        */
#define STYLE_FONTSIZE 3 /* Fontsize        */
#define STYLE_PCOLOR   4 /* PrimaryColour   */
#define STYLE_SCOLOR   5 /* SecondaryColour */
#define STYLE_TCOLOR   6 /* OutlineColour   *//* in ssa_v4 - Tertiary */
#define STYLE_BCOLOR   7 /* BackColour      */
#define STYLE_BOLD     8 /* Bold            */
#define STYLE_ITALIC   9 /* Italic          */
#define STYLE_UNDER   10 /* Underline       */ /* ssa_v4+ only */
#define STYLE_STRIKE  11 /* Strikeout       */ /* ssa_v4+ only */
#define STYLE_SCALEX  12 /* ScaleX          */ /* ssa_v4+ only */
#define STYLE_SCALEY  13 /* ScaleY          */ /* ssa_v4+ only */
#define STYLE_SPACING 14 /* Spacing         */ /* ssa_v4+ only */
#define STYLE_ANGLE   15 /* Angle           */ /* ssa_v4+ only */
#define STYLE_BORDER  16 /* BorderStyle     */
#define STYLE_OUTLINE 17 /* Outline         */
#define STYLE_SHADOW  18 /* Shadow          */
#define STYLE_ALIGN   19 /* Alignment       */ /* see scheme above */
#define STYLE_MARGINL 20 /* MarginL         */
#define STYLE_MARGINR 21 /* MarginR         */
#define STYLE_MARGINV 22 /* MarginV         */
#define STYLE_ALPHA   23 /* AlphaLevel      */ /* ssa_v4 only */
#define STYLE_ENC     24 /* Encoding        */

/* for output */
#define SSA_STYLE_V4_FORMAT  "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding"
#define SSA_STYLE_V4P_FORMAT "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"

typedef enum ssa_event_type
  {
    DIALOGUE = 1,
    COMMENT,
    COMMAND,
    MOVIE,
    PICTURE,
    SOUND
  } ssa_event_type;

typedef struct ssa_event
  {
    struct ssa_event *next;
    ssa_event_type type;
    uint32_t layer;
    double start;
    double end;
    char *style;
    char *name;
    int margin_r;
    int margin_l;
    int margin_v;
    char *effect;
    char *text;
  } ssa_event;

/* Defined order & names below matches ssa_v4+ *
 * For use with ssa_v4 - rename #1             */
#define EVENT_LAYER    1 /* Layer   */ /* 'Marked' in ssa_v4 */
#define EVENT_START    2 /* Start   */ /* time in format: 0:00:00.00 */
#define EVENT_END      3 /* End     */ /* same as above */
#define EVENT_STYLE    4 /* Style   */
#define EVENT_NAME     5 /* Name    */
#define EVENT_MARGINL  6 /* MarginL */ /* 4-digit number with leading zeroes */
#define EVENT_MARGINR  7 /* MarginR */
#define EVENT_MARGINV  8 /* MarginV */
#define EVENT_EFFECT   9 /* Effect  */
#define EVENT_TEXT    10 /* Text    */ /* all rest till end of line */

/* for output */
#define SSA_EVENT_V4_FORMAT  "Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"
#define SSA_EVENT_V4P_FORMAT "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"

typedef enum ssa_section
  {
    NONE,
    UNKNOWN,
    HEADER,
    STYLES,
    EVENTS,
    FONTS,
    GRAPHICS
  } ssa_section;

typedef struct ssa_media
  {
    struct ssa_media *next;
    enum
    {
      type_unknown,
      type_font,
      type_image
    } type;
    char *filename; /* "Original filename before embedding" */
    FILE *data;
  } ssa_media;

typedef struct ssa_file
  {
    /*** data section */

    /** numeric fields */
    struct res res;     /* PlayResX & PlayResY    */
    uint16_t depth;     /* PlayDepth              */
    float    timer;     /* Timer                  */
    float    sync;      /* Synch Point (WTF?)     */

    uint8_t  wrap;      /* WrapStyle (>= ASS subs) */
    /* 0: smart wrapping, lines are evenly broken   *
     * 1: end-of-line word wrapping, only \N breaks *
     * 2: no word wrapping, \n \N both breaks       *
     * 3: same as 0, but lower line gets wider.     */

    ssa_version type;   /* Script Type            */

    /** text fields */
    /* standart text fields:  *
     * Title                  *
     * Original Script        *
     * Original Translation   *
     * Original Editing       *
     * Original Timing        *
     * Script Updated By      *
     * Collisions             *
     * + any unrecognized     */
    struct slist *txt_params;

    /*** service section */
    uint16_t flags;

    int8_t style_fields_order[MAX_FIELDS];
    int8_t event_fields_order[MAX_FIELDS];

    ssa_style *styles;
    ssa_event *events;
    ssa_media *fonts;
    ssa_media *images;
  } ssa_file;

  /** function prototypes */

bool init_ssa_file(ssa_file * const);

  /** parse functions */
bool parse_ssa_file(FILE *, ssa_file *);

/** header section */
bool get_ssa_param(char * const, ssa_file * const);

/** styles section */
bool set_style_fields_order(char * const, ssa_version, int8_t *);
bool detect_style_fields_order(char * const, int8_t *);
bool get_ssa_style(char * const, ssa_style **, int8_t *);

/** events section */
bool set_event_fields_order(char * const, ssa_version, int8_t *);
bool detect_event_fields_order(char * const, int8_t *);
bool get_ssa_event (char * const, ssa_event * const, int8_t *);

/** media section */
int8_t detect_media_line_type(char const * const);

/** write functions */
bool write_ssa_file(FILE *, ssa_file *, bool);

bool write_ssa_txt_param(FILE *, char *, char *, bool, bool);
bool write_ssa_header(FILE *, ssa_file   * const, bool);

bool write_ssa_styles(FILE *, ssa_style  * const, ssa_version, bool);
bool write_ssa_style (FILE *, ssa_style  * const, ssa_version);

bool write_ssa_events(FILE *, ssa_event  * const, ssa_version, bool);
bool write_ssa_event (FILE *, ssa_event  * const, ssa_version);

/** other */
uint32_t ssa_color(char * const);
char *ssa_version_tos(ssa_version);
bool  ssa_section_switch(enum ssa_section *, char *);
void ssa_event_append(ssa_event **, ssa_event ***,
                      ssa_event * const, bool);
ssa_style *find_ssa_style_by_name(ssa_file *, char *);

#endif /* _SSA_H */
