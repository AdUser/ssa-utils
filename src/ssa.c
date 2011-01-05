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

#include "common.h"
#include "ssa.h"

#define MSG_W_WRONGFORDER   _("Wrong fields order in %s.")
#define MSG_W_UNRECFIELD    _("Unrecognized field '%s'.")
#define MSG_W_UNRECPARAM    _("Unrecognized parameter '%s' at line '%u'.")
#define MSG_W_CANTDETECT    _("Can't detect fields order for %s. Default order assumed.")
#define MSG_W_TOOMANYFIELDS _("Too many fields in %s.")
#define MSG_W_CURRSECTION   _("Line %i: %s section.")
#define MSG_W_SKIPEPARAM    _("Skipping parameter '%s' with empty value at line '%u'.")

/* variables */
extern uint32_t line_num;
extern struct unicode_test BOMs[6];
extern struct options opts;

/* to use from outer space :-) */
  int8_t fields_order[MAX_FIELDS] = { 0 };

/* templates of normal fields order, zero is list-terminator */
int8_t style_fields_order_ssa_v4[MAX_FIELDS] = \
  { 1,  2,  3,  4,  5,  6,  7,  8,  9,
   16, 17, 18, 19, 20, 21, 22, 23, 24, 0 };
int8_t style_fields_order_ssa_v4p[MAX_FIELDS] = \
  { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,
   13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24, 0 };

int8_t event_fields_normal_order[MAX_FIELDS] = \
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0 };

/* order is important */
struct unicode_test uc_t_ssa[5] =
{
  { UTF32BE, { 0x00, 0x00, 0x00,  '['}, 4 },
  { UTF32LE, { '[',  0x00, 0x00, 0x00}, 4 },
  { UTF16BE, { 0x00,  '[', 0x00, 0x00}, 3 },
  { UTF16LE, { '[',  0x00, 0x00, 0x00}, 2 },
  { 0,       { 0x00, 0x00, 0x00, 0x00}, 0 } /* this entry acts as list-terminator */
};

/* structs */
ssa_file ssa_file_template =
  {
    /** data section */
    /* text fields */
    "",       /* Title                */
    "",       /* Original Script      */
    "",       /* Original Translation */
    "",       /* Original Editing     */
    "",       /* Original Timing      */
    "",       /* Script Updated By    */
    "Normal", /* Collisions           */

    /* numeric fields */
    { 0, 0 }, /* PlayResX & PlayResY  */
    0,        /* PlayDepth            */
    100.0,    /* Timer                */
    0,        /* Synch Point          */
    0,        /* WrapStyle            */
    /* 0: smart wrapping, lines are evenly broken   *
     * 1: end-of-line word wrapping, only \N breaks *
     * 2: no word wrapping, \n \N both breaks       *
     * 3: same as 0, but lower line gets wider.     */

    unknown,  /* Script Type          */

    /** service section */
    0,        /* flags: 0x00000000 */

    { 0 }, /* style field order */
    { 0 }, /* event field order */

    (ssa_style *) 0, /* styles list */
    (ssa_event *) 0  /* events list */
  };

ssa_style ssa_style_template =
  {
    (ssa_style *) 0, /* pointer to next style */
    "Default", /* style name   */
    "Arial",   /* font name    */
    24.0,      /* font size    */
    16761538,  /* primary    - 00FFC2C2 */
    65535,     /* secondary  - 0000FFFF */
    3735552,   /* outline    - 00390000 */
    0,         /* background - 00000000 */
    0,         /* bold         */
    0,         /* italic       */
    0,         /* underlined   */
    0,         /* strikeout    */
    100,       /* scale x      */
    100,       /* scale y      */
    0,         /* spacing (px) */
    0.0,       /* angle        */
    1,         /* border style: 1 - shadow, 3 - bg under subs  */
    2,         /* outline, if 'border style' equals to: *
                * 1, then specifies shadow width in px  *
                * 3, then specifies size of bg box      */
    0,         /* shadow depth (offset) */
    2,         /* alignment (see scheme in header) */
    10,        /* margin left   */
    10,        /* margin right  */
    20,        /* margin bottom */
    0,         /* alpha level */
    204        /* 0 - ansi, 204 - russian */
  };

ssa_event ssa_event_template =
{
    (ssa_event *) 0,
    0,         /* layer    */
    0.0,       /* start    */
    0.0,       /* end      */
    "Default", /* style    */
    "",        /* name     */
    0,         /* margin_r */
    0,         /* margin_l */
    0,         /* margin_l */
    "",        /* effect   */
    ""         /* text     */
};

/* top-level functions */
bool
init_ssa_file(ssa_file * const file)
  {
    if (!file) return false;

    memcpy(file, &ssa_file_template, sizeof(ssa_file));
    return true;
  }

bool
parse_ssa_file(FILE *infile, ssa_file *file)
  {
    bool get_styles = true; /* skip or not styles in file */
    char line[MAXLINE] = "";
    ssa_event *e = NULL;
    ssa_event **elist_tail  = &file->events;

    ssa_section section = NONE;

    while (!feof(infile))
      {
        memset(line, 0, MAXLINE);
        fgets(line, MAXLINE, infile);
        line_num++;

        /* unicode handle */
        if (line_num == 1)
          opts.i_chs_type = unicode_check(line, uc_t_ssa);

        trim_newline(line);
        log_msg(raw, "%s", line);
        trim_spaces(line, LINE_START | LINE_END);

        if (line[0] == ';' || is_empty_line(line))
          continue;

        if (ssa_section_switch(&section, line) || section == UNKNOWN)
          continue;

        switch (section)
          {
            case HEADER :
              log_msg(debug, MSG_W_CURRSECTION, line_num, _("header"));
              get_ssa_option(line, file);
              break;
            case STYLES :
              log_msg(debug, MSG_W_CURRSECTION, line_num, _("styles"));
              if      (*line == 'F' || *line == 'f')
                get_styles = set_style_fields_order(line,
                    file->type, file->style_fields_order);
              else if (get_styles && (*line == 'S' || *line == 's'))
                get_ssa_style(line, &file->styles, file->style_fields_order);
              break;
            case EVENTS :
              log_msg(debug, MSG_W_CURRSECTION, line_num, _("events"));
              if      (*line == 'F' || *line == 'f')
                set_event_fields_order(line,
                    file->type, file->event_fields_order);
              else if (*line == 'D' || *line == 'd')
                {
                  if ((e = calloc(1, sizeof(ssa_event))) == NULL)
                    log_msg(error, MSG_M_OOM);
                  if (get_ssa_event(line, e, file->event_fields_order) != false)
                    ssa_event_append(&file->events, &elist_tail, e, opts.i_sort);
                  else free(e);
                }
              break;
            case FONTS :
              log_msg(debug, MSG_W_CURRSECTION, line_num, _("fonts"));
              break;
            case GRAPHICS :
              log_msg(debug, MSG_W_CURRSECTION, line_num, _("graphics"));
              break;
            case UNKNOWN :
              log_msg(debug, MSG_W_CURRSECTION, line_num, _("unknown"));
              break;
            case NONE :
            default :
              log_msg(warn, _("Skipping line %i: not in any ssa section."), line_num);
              break;
          }
        }

      /* some checks and fixes */
      if (file->type == unknown)
        log_msg(error, _("Missing 'Script Type' line in input file."));

      if (file->timer == 0)
        {
          log_msg(warn, _("Undefined or zero 'Timer' value. Default value assumed."));
          file->timer = 100;
        }

      if ((get_styles && file->styles == (ssa_style *) 0) || !get_styles)
        {
          log_msg(warn, _("No styles was defined. Default style assumed."));
          file->styles = calloc(1, sizeof(ssa_style));
          if (file->styles)
            memcpy(file->styles, &ssa_style_template, sizeof(ssa_style));
        }

      return true;
  }


/** low-level parse functions */
/** this functions should return:
 *  true - if all went fine
 * false - if line stays unhandled */

bool
get_ssa_option(char * const line, ssa_file * const h)
  {
    /* line: "Param: Value" */
    char first_char; /* for optimize blocks of strcmp */
    char *value;
    char param[MAX_HEADER_LINE];
    int v_len = 0;
    int param_len = 0;

    if ((value = strchr(line, ':')) == NULL)
      {
        log_msg(warn, _("Can't get parameter value at line '%u'."), line_num);
        return false;
      } /* else -> "Param|: Value" */

      param_len = value - line; /* "[Param]|: Value" */
      value = value + 1; /* "Param|: Value" -> "Param:| Value"  */

      first_char = tolower(line[0]);
      strncpy(param, line, param_len); /* "[Param]:|Value" */
      *(param + param_len) = '\0';
      string_lowercase(line, param_len); /* -> "[param]:|Value" */
      trim_spaces(value, LINE_START | LINE_END);
      v_len = strlen(value);

      switch (first_char)
        {
        case 'c' :
          if (strncmp("collisions", line, param_len) == 0 && v_len)
            strncpy(h->collisions, value, MAX_HEADER_LINE),
                  h->flags |= SSA_H_HAVE_COLLS;
              break;
        case 'o' : /* all 'Original*' lines */
          if      (strncmp("original script", line, param_len) == 0)
            {
              if (v_len)
                strncpy(h->o_script, value, v_len), h->flags |= SSA_H_HAVE_OSCRIPT;
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          else if (strncmp("original translation", line, param_len) == 0)
            {
              if (v_len)
                strncpy(h->o_trans, value, v_len), h->flags |= SSA_H_HAVE_OTRANS;
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          else if (strncmp("original editing", line, param_len) == 0)
            {
              if (v_len)
                strncpy(h->o_edit, value, v_len), h->flags |= SSA_H_HAVE_OEDIT;
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          else if (strncmp("original timing", line, param_len) == 0)
            {
              if (v_len)
                strncpy(h->o_timing, value, v_len), h->flags |= SSA_H_HAVE_OTIMING;
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          break;
        case 'p' : /* all 'Play*' lines */
          if      (strncmp("playresx",  line, param_len) == 0)
            h->res.width  = atoi(value);
          else if (strncmp("playresy",  line, param_len) == 0)
            h->res.height = atoi(value);
          else if (strncmp("playdepth", line, param_len) == 0)
            h->depth = atoi(value);
          else log_msg(warn, MSG_W_UNRECPARAM, param, line_num);
          break;
        case 's' :
          if      (strncmp("script updated by", line, param_len) == 0)
            {
              if (v_len)
                strncpy(h->updated, value, v_len), h->flags |= SSA_H_HAVE_UPDATED;
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          else if (strncmp("scripttype", line, param_len) == 0)
            {
              string_lowercase(value, v_len);
              if      (strncmp("v4.00+", value, param_len) == 0)
                h->type = ssa_v4p;
              else if (strncmp("v4.00",  value, param_len) == 0)
                h->type = ssa_v4;
              else if (strncmp("v3.00",  value, param_len) == 0)
                h->type = ssa_v3;
              else
                h->type = unknown;
              /* TODO: выяснить насчет остальных типов */
            }
          else if (strncmp("synch point", line, param_len) == 0)
            {
              if (v_len) h->sync = atof(value);
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          else log_msg(warn, MSG_W_UNRECPARAM, param, line_num);
          break;
        case 't' :
          if      (strncmp("timer", line, param_len) == 0)
            h->timer = atof(value);
          else if (strncmp("title", line, param_len) == 0)
            {
              if (v_len)
                strncpy(h->title, value, v_len), h->flags |= SSA_H_HAVE_TITLE;
              else log_msg(info, MSG_W_SKIPEPARAM, line, line_num);
            }
          else log_msg(warn, MSG_W_UNRECPARAM, param, line_num);
          break;
        case 'w' :
          if (strncmp("wrapstyle", line, param_len) == 0)
            h->wrap = atoi(value);
          break;
        default :
          log_msg(warn, MSG_W_UNRECPARAM, param, line_num);
      }
    return true;
  }

bool
set_style_fields_order(char *format, ssa_version v, int8_t *fieldlist)
  {
    bool result = true;
    int len = strlen(format);
    int8_t *fields_order;
    char compare[MAXLINE];

    string_skip_chars(format, " ");
    string_lowercase(format, len);

    switch (v)
      {
        case ssa_v4p :
          fields_order = style_fields_order_ssa_v4p;
          strcpy(compare, SSA_STYLE_V4P_FORMAT);
          break;
        case ssa_v4 :
          fields_order = style_fields_order_ssa_v4;
          strcpy(compare, SSA_STYLE_V4_FORMAT);
          break;
        case ssa_v3:
        case unknown:
        default :
          return false;
          break;
      }

    string_skip_chars(compare, " ");
    string_lowercase(compare, 0);

    if (strcmp(compare,  format) == 0)
      memcpy(fieldlist, fields_order, sizeof(uint8_t) * MAX_FIELDS);
    else if ((result = detect_style_fields_order(format, fieldlist)) == true)
      log_msg(warn, MSG_W_WRONGFORDER, _("styles"));
    else
      {
        log_msg(warn, MSG_W_CANTDETECT, _("styles"));
        memcpy(fieldlist, fields_order, sizeof(uint8_t) * MAX_FIELDS);
      }

    return result;
  }

bool
detect_style_fields_order(char * format, int8_t *fieldlist)
  {
    char *p, *token;
    bool result;
    int i;
    int8_t *field = fieldlist;

    if ((p = strchr(format, ':')) == 0)
      {
        log_msg(error, _("Malformed 'Format:' line."));
        return false;
      }

    /* this saves from of duplicate code below *
     * possible, i should do it with memset()?  */
    *(field + MAX_FIELDS) = 0; /* set list-terminator */
    for (i = 0; i < MAX_FIELDS; i++)
                  *field++ = -1;

    field = fieldlist;

    token = strtok(++p, ",");
    do
      {
        switch(token[0])
          {
            case 'a' :
                if      (strcmp(token, "alignment") == 0)
                  *field = STYLE_ALIGN;
                else if (strcmp(token, "alphalevel") == 0)
                  *field = STYLE_ALPHA;
                else if (strcmp(token, "angle") == 0)
                  *field = STYLE_ANGLE;
                else
                  goto unknown;
              break;
            case 'b' :
                if      (strcmp(token, "backcolour") == 0)
                  *field = STYLE_BCOLOR;
                else if (strcmp(token, "borderstyle") == 0)
                  *field = STYLE_BORDER;
                else if (strcmp(token, "bold") == 0)
                  *field = STYLE_BOLD;
                else
                  goto unknown;
              break;
            case 'e' :
                if      (strcmp(token, "encoding") == 0)
                  *field = STYLE_ENC;
                else
                  goto unknown;
              break;
            case 'f' :
                if      (strcmp(token, "fontname") == 0)
                  *field = STYLE_FONTNAME;
                else if (strcmp(token, "fontsize") == 0)
                  *field = STYLE_FONTSIZE;
                else
                  goto unknown;
              break;
            case 'i' :
                if      (strcmp(token, "italic") == 0)
                  *field = STYLE_ITALIC;
                else
                  goto unknown;
              break;
            case 'm' :
                if      (strcmp(token, "marginl") == 0)
                  *field = STYLE_MARGINL;
                else if (strcmp(token, "marginr") == 0)
                  *field = STYLE_MARGINR;
                else if (strcmp(token, "marginv") == 0)
                  *field = STYLE_MARGINV;
                else
                  goto unknown;
              break;
            case 'n' :
                if      (strcmp(token, "name") == 0)
                  *field = STYLE_NAME;
                else
                  goto unknown;
              break;
            case 'o' :
                if      (strcmp(token, "outlinecolour") == 0)
                    *field = STYLE_TCOLOR;
                else if (strcmp(token, "outline") == 0)
                    *field = STYLE_OUTLINE;
                else
                  goto unknown;
              break;
            case 'p' :
                if      (strcmp(token, "primarycolour") == 0)
                  *field = STYLE_PCOLOR;
                else
                  goto unknown;
              break;
            case 's' :
                if      (strcmp(token, "scalex") == 0)
                  *field = STYLE_SCALEX;
                else if (strcmp(token, "scaley") == 0)
                  *field = STYLE_SCALEY;
                else if (strcmp(token, "shadow") == 0)
                  *field = STYLE_SHADOW;
                else if (strcmp(token, "spacing") == 0)
                  *field = STYLE_SPACING;
                else if (strcmp(token, "strikeout") == 0)
                  *field = STYLE_STRIKE;
                else if (strcmp(token, "secondarycolour") == 0)
                  *field = STYLE_SCOLOR;
                else
                  goto unknown;
              break;
            case 't' :
                if      (strcmp(token, "tertiarycolour") == 0)
                  *field = STYLE_TCOLOR;
                else
                  goto unknown;
              break;
            case 'u' :
                if      (strcmp(token, "underline") == 0)
                  *field = STYLE_UNDER;
                else
                  goto unknown;
              break;
            default :
            unknown :
              log_msg(warn, MSG_W_UNRECFIELD, token);
            result = false;
              break;
          }
        field++;
      }
    while ((token = strtok(NULL, ",")) != 0 && *field != 0);

    if (*field == 0)
      log_msg(error, MSG_W_TOOMANYFIELDS, _("styles"));
    else
      *field = 0; /* set new list terminator after last param */

    return result;
  }

bool
get_ssa_style(char * const line, ssa_style ** style,
              int8_t * fieldlist)
  {
    int8_t *field = fieldlist;
    ssa_style *ptr = *style, **ptr_alloc;
    char *p = line, *delim = ",";
    char token[MAXLINE];
    int len = -1;

    if (ptr != (ssa_style *) 0) /* list has no entries */
      {
        while (ptr->next != (ssa_style *) 0)
          ptr = ptr->next;
        ptr_alloc = &ptr->next;
      }
    else
      ptr_alloc = style;

    if ((p = strchr(line, ':')) == NULL)
      return false;
    else if ((*ptr_alloc = calloc(1, sizeof(ssa_style))) != NULL)
      memcpy(*ptr_alloc, &ssa_style_template, sizeof(ssa_style));
    else
      return false;

    ptr = *ptr_alloc;

    p = p + 1; /* "Style:| " */
    memset(token, 0, MAXLINE);

    while (*field != 0)
      {
        len = _strtok(p, delim);
        if (len < 0)
          return false;
        else if (len > 0 && len < MAXLINE)
          {
            strncpy(token, p, len);
            p += (len + strlen(delim));
            token[len] = '\0';
          }
        else /* len == 0 || len > MAXLINE */
          token[0] = '\0', p++;

        switch (*field)
          {
            case STYLE_NAME :
                strncpy(ptr->name, token, MAX_STYLE_NAME);
                trim_spaces(ptr->name, LINE_START | LINE_END);
              break;
            case STYLE_FONTNAME :
                strncpy(ptr->fontname, token, MAX_STYLE_NAME);
                trim_spaces(ptr->fontname, LINE_START | LINE_END);
              break;
            case STYLE_FONTSIZE :
                ptr->fontsize = atof(token);
              break;
            case STYLE_PCOLOR :
                ptr->primary_color = ssa_color(token);
              break;
            case STYLE_SCOLOR :
                ptr->secondary_color = ssa_color(token);
              break;
            case STYLE_TCOLOR :
                ptr->outline_color = ssa_color(token);
              break;
            case STYLE_BCOLOR :
                ptr->background_color = ssa_color(token);
              break;
            case STYLE_BOLD :
                ptr->bold = atoi(token);
              break;
            case STYLE_ITALIC :
                ptr->italic = atoi(token);
              break;
            case STYLE_UNDER :
                ptr->underlined = atoi(token);
              break;
            case STYLE_STRIKE :
                ptr->strikeout = atoi(token);
              break;
            case STYLE_SCALEX :
                ptr->scale_x = atoi(token);
              break;
            case STYLE_SCALEY :
                ptr->scale_y = atoi(token);
              break;
            case STYLE_SPACING :
                ptr->spacing = atoi(token);
              break;
            case STYLE_ANGLE :
                ptr->angle = atof(token);
              break;
            case STYLE_BORDER :
                ptr->border_style = atoi(token);
              break;
            case STYLE_OUTLINE :
                ptr->outline = atoi(token);
              break;
            case STYLE_SHADOW :
                ptr->shadow = atoi(token);
              break;
            case STYLE_ALIGN :
                ptr->alignment = atoi(token);
              break;
            case STYLE_MARGINL :
                ptr->margin_l = atoi(token);
              break;
            case STYLE_MARGINR :
                ptr->margin_r = atoi(token);
              break;
            case STYLE_MARGINV :
                ptr->margin_v = atoi(token);
              break;
            case STYLE_ALPHA :
                ptr->alpha_level = atoi(token);
              break;
            case STYLE_ENC :
                ptr->codepage = atoi(token);
              break;
            default :
              break;
          }
        field++;
        if (*field == 0) delim = "";
      }

    return true;
  }

bool
set_event_fields_order(char * const format, ssa_version v, int8_t * fieldlist)
  {
    bool result = true;
    int len = strlen(format);
    char compare[MAXLINE];

    string_skip_chars(format, " ");
    string_lowercase(format, len);

    switch (v)
      {
        case ssa_v4p :
          strcpy(compare, SSA_EVENT_V4P_FORMAT);
          break;
        case ssa_v4 :
          strcpy(compare, SSA_EVENT_V4_FORMAT);
          break;
        case ssa_v3:
        case unknown:
        default :
          return false;
          break;
      }

    string_skip_chars(compare, " ");
    string_lowercase(compare, 0);

    if (strcmp(compare,  format) == 0)
      memcpy(fieldlist, event_fields_normal_order, sizeof(uint8_t) * MAX_FIELDS);
    else if ((result = detect_event_fields_order(format, fieldlist)) == true)
      log_msg(warn, MSG_W_WRONGFORDER, _("events"));
    else
      {
        log_msg(warn, MSG_W_CANTDETECT, _("events"));
        memcpy(fieldlist, event_fields_normal_order, sizeof(uint8_t) * MAX_FIELDS);
      }

    return result;
  }

bool
detect_event_fields_order(char *format, int8_t *fieldlist)
  {
    char *p, *token;
    bool result;
    int i;
    int8_t *field = fieldlist;

    if ((p = strchr(format, ':')) == 0)
      log_msg(error, _("Malformed 'Format:' line."));

    /* this saves from of duplicate code below *
     * possible, i should do it with memset()?  */
    *(field + MAX_FIELDS) = 0; /* set list-terminator */
    for (i = 0; i < MAX_FIELDS; i++)
                  *field++ = -1;

    token = strtok(++p, ",");
    do
      {
        switch(token[0])
          {
            case 'e' :
                if      (strcmp(token, "effect") == 0)
                  *field = EVENT_EFFECT;
                else if (strcmp(token, "end") == 0)
                  *field = EVENT_END;
                else
                  goto unknown;
              break;
            case 'l' :
                if      (strcmp(token, "layer") == 0)
                  *field = EVENT_LAYER;
                else
                  goto unknown;
              break;
            case 'm' :
                if      (strcmp(token, "marginl") == 0)
                  *field = EVENT_MARGINL;
                if      (strcmp(token, "marginr") == 0)
                  *field = EVENT_MARGINR;
                if      (strcmp(token, "marginv") == 0)
                  *field = EVENT_MARGINV;
                if      (strcmp(token, "marked") == 0)
                  *field = EVENT_LAYER;
                else
                  goto unknown;
              break;
            case 'n' :
                if      (strcmp(token, "name") == 0)
                  *field = EVENT_NAME;
                else
                  goto unknown;
              break;
            case 's' :
                if      (strcmp(token, "start") == 0)
                  *field = EVENT_START;
                else if (strcmp(token, "style") == 0)
                  *field = EVENT_STYLE;
                else
                  goto unknown;
              break;
            case 't' :
                if      (strcmp(token, "text") == 0)
                  *field = EVENT_TEXT;
                else
                  goto unknown;
              break;
            default :
            unknown :
              log_msg(warn, MSG_W_UNRECFIELD, token);
            result = false;
              break;
          }
        field++;
      }
    while ((token = strtok(NULL, ",")) != 0 && *field != 0);

    if (*field == 0)
      log_msg(error, MSG_W_TOOMANYFIELDS, _("events"));
    else
      *field = 0; /* set new list terminator after last param */

    return result;
  }

bool
get_ssa_event(char * const line, ssa_event * const event, int8_t *fieldlist)
  {
    int8_t *field = fieldlist;
    subtime st = { 0, 0, 0, 0.0 };
    char *p = line, *delim = ",";
    char token[MAXLINE];
    int len = -1;
    double *t;

    if (event == NULL)
      return false;

    if ((p = strchr(line, ':')) == NULL)
      return false;

    p = p + 1; /* "Dialogie:|" */

    while (*field != 0)
      {
        len = _strtok(p, delim);
        if (len < 0)
          return false;
        else if (len > 0 && len < MAXLINE)
          {
            strncpy(token, p, len);
            p += (len + strlen(delim));
            token[len] = '\0';
          }
        else /* len == 0 || len > MAXLINE */
          token[0] = '\0', p++;

        switch (*field)
          {
            case EVENT_LAYER :
              event->layer = atoi(token); /* little hack */
              break;
            case EVENT_START :
            case EVENT_END :
              t = (*field == EVENT_START) ? &event->start : &event->end;
              if (!str2subtime(token, &st))
                {
                  log_msg(warn, _("Can't get timing at line '%u'."), line_num);
                  return false;
                }
              else
                subtime2double(&st, t);
              break;
            case EVENT_STYLE :
              strncpy(event->style, token, MAX_EVENT_NAME);
              break;
            case EVENT_NAME :
              strncpy(event->name, token, MAX_EVENT_NAME);
              break;
            case EVENT_MARGINL :
              event->margin_l = atoi(token);
              break;
            case EVENT_MARGINR :
              event->margin_r = atoi(token);
              break;
            case EVENT_MARGINV :
              event->margin_v = atoi(token);
              break;
            case EVENT_EFFECT :
              strncpy(event->effect, token, MAX_EVENT_NAME);
              break;
            case EVENT_TEXT :
              strncpy(event->text, token, MAXLINE);
              break;
            default :
              break;
          }
        field++;
        if (*field == EVENT_TEXT) delim = "";
      }

    return true;
  }

/* write functions */

bool
write_ssa_file(FILE *outfile, ssa_file *f, bool memfree)
  {
    bool result = true;

    if (!outfile) return false;

    if (fprintf(outfile, "%s\n", "[Script Info]") < 0)
      log_msg(error, MSG_F_WRFAIL);

    fprintf(outfile, "%s: %s %.2f\n", "; Generated by",
              COMMON_PROG_NAME, VERSION);

    if (f->flags & SSA_H_HAVE_TITLE)
      fprintf(outfile, "%s: %s\n", "Title",                f->title);
    if (f->flags & SSA_H_HAVE_OSCRIPT)
      fprintf(outfile, "%s: %s\n", "Original Script",      f->o_script);
    if (f->flags & SSA_H_HAVE_OTRANS)
      fprintf(outfile, "%s: %s\n", "Original Translation", f->o_trans);
    if (f->flags & SSA_H_HAVE_OEDIT)
      fprintf(outfile, "%s: %s\n", "Original Editing",     f->o_edit);
    if (f->flags & SSA_H_HAVE_OTIMING)
      fprintf(outfile, "%s: %s\n", "Original Timing",      f->o_timing);
    if (f->flags & SSA_H_HAVE_UPDATED)
      fprintf(outfile, "%s: %s\n", "Script Updated By",    f->updated);
    if (f->flags & SSA_H_HAVE_COLLS)
      fprintf(outfile, "%s: %s\n", "Collisions",           f->collisions);

    if (f->sync != 0)
      fprintf(outfile, "%s: %.0f\n", "Synch Point", f->sync);

    fprintf(outfile, "%s%s: %i\n", (f->res.width) ? "" : ";",
                        "PlayResX", f->res.width);
    fprintf(outfile, "%s%s: %i\n", (f->res.height) ? "" : ";",
                        "PlayResY", f->res.height);
    fprintf(outfile, "%s%s: %i\n", (f->depth) ? "" : ";",
                        "PlayDepth", f->depth);

    /* theses fields MUST present */
    fprintf(outfile, "%s: %s\n", "ScriptType", ssa_version_tos(f->type));
    fprintf(outfile, "%s: %.4f\n", "Timer",              f->timer);

    if (f->type >= ssa_v4p)
        fprintf(outfile, "%s: %i\n", "WrapStyle",          f->wrap);

    fputc('\n', outfile); /* blank line after header */

    result &= write_ssa_styles(outfile, f->styles, f->type, memfree);
    result &= write_ssa_events(outfile, f->events, f->type, memfree);

    return result;
  }

bool
write_ssa_styles(FILE * outfile, ssa_style  * const style, ssa_version v, bool memfree)
  {
    char *s = "";
    char *format;
    int write = 0;
    bool section_header = true;
    bool format_string = true;
    ssa_style *ptr = style, *prev;

    switch (v)
      {
        case ssa_v4p:
          s = "+";
          format = SSA_STYLE_V4P_FORMAT;
          break;
        case ssa_v4:
          s = "";
          format = SSA_STYLE_V4_FORMAT;
          break;
        case ssa_v3:
          format_string = false;
          section_header = false;
/*        break;*/
        case unknown:
        default :
          return false;
          break;
      }

    if (section_header)
      write = fprintf(outfile, "[V4%s Styles]\n", s);

    if (format_string)
      write = fprintf(outfile, format);

    if (write < 0)
      log_msg(error, MSG_F_WRFAIL);

    putc('\n', outfile);

    while (ptr != NULL)
      {
        write_ssa_style(outfile, ptr, v);
        prev = ptr;
        ptr = ptr->next;
        if (memfree) free(prev);
      }

    fputc('\n', outfile);

    return true;
  }

bool
write_ssa_style(FILE * outfile, ssa_style  * const style, ssa_version v)
  {
    char *color_format;
    bool alphalevel = true;

    fprintf(outfile, "Style: %s,%s,%.0f,", \
              style->name, style->fontname, style->fontsize);

    /* various versions have a different color format representation */
    switch (v)
      {
        case ssa_v4p : /* ssa_v4+ uses more nice-looking hex format */
          color_format = "&H%08X,&H%08X,&H%08X,&H%08X,";
          alphalevel = false;
          break;
        case ssa_v4 : /* all other uses raw integer */
        case ssa_v3 :
        case unknown:
        default :
          color_format = "%i,%i,%i,%i,";
          break;
      }

    fprintf(outfile, color_format, \
              style->primary_color, style->secondary_color, \
              style->outline_color, style->background_color);

    fprintf(outfile, "%i,%i,", style->bold, style->italic);

    /* ssa v4+ specific parameters ^_^ */
    if (v == ssa_v4p)
      fprintf(outfile, "%i,%i,%i,%i,%i,%.0f,", \
                style->underlined, style->strikeout, \
                style->scale_x, style->scale_y, \
                style->spacing, style->angle);

    fprintf(outfile, "%i,%i,%i,%i,",  \
                style->border_style, style->outline, \
                style->shadow,       style->alignment);

    fprintf(outfile, "%i,%i,%i,",  \
                style->margin_l, style->margin_r, style->margin_v);

    if (alphalevel)
        fprintf(outfile, "%i,", style->alpha_level);

    fprintf(outfile, "%i\n", style->codepage);

    return true;
  }

bool
write_ssa_events(FILE * outfile, ssa_event * const events, ssa_version v, bool memfree)
  {
    char *format;
    int write = 0;
    bool section_header = true;
    bool format_string = true;
    ssa_event *ptr = events, *prev;

    switch (v)
      {
        case ssa_v4p:
          format = SSA_EVENT_V4P_FORMAT;
          break;
        case ssa_v4:
          format = SSA_EVENT_V4_FORMAT;
          break;
        case ssa_v3:
          format_string = false;
          section_header = false;
/*        break;*/
        case unknown:
        default :
          return false;
          break;
      }

    if (section_header)
      write = fprintf(outfile, "[Events]\n");

    if (format_string)
      write = fprintf(outfile, "%s\n", format);

    if (write < 0)
      log_msg(error, MSG_F_WRFAIL);

    while (ptr != NULL)
      {
        write_ssa_event(outfile, ptr, v);
        prev = ptr;
        ptr = ptr->next;
        if (memfree) free(prev);
      }

    fputc('\n', outfile);

    return true;
  }

bool
write_ssa_event(FILE *outfile, ssa_event * const event, ssa_version v)
  {
    struct subtime st;
    char *s = "";

    if (v == ssa_v4) s = "Marked=";

    fprintf(outfile, "Dialogue: %s%i,", s, event->layer);

    double2subtime(event->start, &st);
    fprintf(outfile, "%i:%02i:%02i.%02i,",
        st.hrs, st.min, st.sec, (int) round((double) st.msec / 10.0));

    double2subtime(event->end, &st);
    fprintf(outfile, "%i:%02i:%02i.%02i,",
        st.hrs, st.min, st.sec, (int) round((double) st.msec / 10.0));

    fprintf(outfile, "%s,%s,%04i,%04i,%04i,%s,%s", \
                    event->style, event->name,\
                    event->margin_l, event->margin_r, event->margin_v, \
                    event->effect, event->text);
    fputc('\n', outfile);

    return true;
  }
/* other */

uint32_t
ssa_color(char * const line)
  {
    uint32_t color = 0;
    int len = strlen(line);

    if (len < 1) /* empty line */
      return (uint32_t) 0xFFFFFFFF; /* pure white */
    else if (len > 2 && line[0] == '&')
      sscanf(line + 2, "%X", &color);
    else if (isdigit(*line))
      color = atoi(line);

    return color;
  }

char *
ssa_version_tos(ssa_version version)
  {
    switch (version)
      {
        case ssa_v3 :
          return "v3.00";
          break;
        case ssa_v4 :
          return "v4.00";
          break;
        case ssa_v4p :
          return "v4.00+";
          break;
        case unknown :
        default :
          return "";
          break;
      }
  }

/* returns true, if changes section and false otherwise */
bool
ssa_section_switch(enum ssa_section *section, char *line)
  {
    bool result = true;
    if (line[0] == '[')
      {
        string_lowercase(line, MAXLINE);
        if      (strcmp("[script info]", line) == 0)
          *section = HEADER;
        else if (strcmp("[events]", line) == 0)
          *section = EVENTS;
        else if (strcmp(line, "[fonts]") == 0)
          *section = FONTS;
        else if (strcmp(line, "[graphics]") == 0)
          *section = GRAPHICS;
        else if (strstr(line, "styles]") != 0)
          *section = STYLES;
        else
          {
            log_msg(warn, _("Unknown ssa section '%s' at line '%u'."), line, line_num);

            *section = UNKNOWN;
          }
      }
    else
      result = false;

    return result;
  }

void
ssa_event_append(ssa_event **head, ssa_event ***tail,
                 ssa_event * const e, bool sort)
  {
    ssa_event *t = NULL;

    if (*head == NULL)
      *head = e;
    else if (**tail != NULL && (!sort || e->start > (**tail)->start))
      (**tail)->next = e, *tail = &(**tail)->next;
    else /* *tail == NULL || (tail != NULL && e->start <= (*tail)->start) */
      for (t = *head; t != NULL && t->next != NULL; t = t->next)
        if (t->next->start > e->start)
          {
            e->next = t->next, t->next = e;
            break;
          }

  }
