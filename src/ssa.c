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
    /** numeric fields */
    { 0, 0 }, /* PlayResX & PlayResY */
    0,        /* PlayDepth           */
    100.0,    /* Timer               */
    0,        /* Synch Point         */
    0,        /* WrapStyle           */

    ssa_unknown, /* Script Type         */

    /** text parameters */
    (struct slist *) 0,

    0x0, /* flags */

    /** arrays */
    { 0 }, /* style field order */
    { 0 }, /* event field order */

    /** structures */
    (ssa_style *) 0, /* styles list */
    (ssa_event *) 0, /* events list */
    (ssa_media *) 0, /* fonts list  */
    (ssa_media *) 0  /* images list */
  };

ssa_style ssa_style_template =
  {
    (ssa_style *) 0, /* pointer to next style */
    NULL,      /* style name   */
    NULL,      /* font name    */
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
    0          /* 0 - ansi, 204 - russian */
  };

ssa_event ssa_event_template =
{
    (ssa_event *) 0,
    DIALOGUE,  /* event type */
    0,         /* layer    */
    0.0,       /* start    */
    0.0,       /* end      */
    NULL,      /* style    */
    NULL,      /* name     */
    0,         /* margin_r */
    0,         /* margin_l */
    0,         /* margin_l */
    NULL,      /* effect   */
    NULL       /* text     */
};

/* top-level functions */
bool
init_ssa_file(ssa_file * const file)
  {
    if (!file) return false;

    memcpy(file, &ssa_file_template, sizeof(ssa_file));

    if (opts.i_sort)
      file->flags |= SSA_E_SORTED;

    return true;
  }

bool
parse_ssa_file(FILE *infile, ssa_file *file)
  {
    uint16_t len = 0;
    bool get_styles = true; /* skip or not styles? */
    bool get_fonts  = true; /* ... embedded fonts? */
    bool get_graph  = true; /* ... embedded graphics? */
    char line[MAXLINE];
    ssa_event *e = NULL;
    ssa_event **elist_tail  = &file->events;
    ssa_media *f = NULL; /* fonts list handler */
    ssa_media *g = NULL; /* graphics list handler */

    ssa_section section = NONE;

    while (fgets(line, MAXLINE, infile) != NULL)
      {
        line_num++;

        /* unicode handle */
        if (line_num == 1)
          opts.i_chs_type = unicode_check(line, uc_t_ssa);

        trim_newline(line);
        _log(log_rawread, "%s", line);
        trim_spaces(line, LINE_START | LINE_END);
        len = strlen(line);

        if (len == 0)
          continue;

        if (len != 80 && !(section == FONTS || section == GRAPHICS))
          if (ssa_section_switch(&section, line) == true)
            continue;

        switch (section)
          {
            case HEADER :
              if (line[0] == ';')
                continue;
              _log(log_debug, MSG_W_CURRSECTION, line_num, _("header"));
              get_ssa_param(line, file);
              break;
            case STYLES :
              _log(log_debug, MSG_W_CURRSECTION, line_num, _("styles"));
              if      (*line == 'F' || *line == 'f')
                get_styles = set_style_fields_order(line,
                    file->type, file->style_fields_order);
              else if (get_styles && toupper(line[0]) == 'S')
                get_ssa_style(line, &file->styles, file->style_fields_order);
              break;
            case EVENTS :
              _log(log_debug, MSG_W_CURRSECTION, line_num, _("events"));
              if      (*line == 'F' || *line == 'f')
                set_event_fields_order(line,
                    file->type, file->event_fields_order);
              else
                {
                  CALLOC(e, 1, sizeof(ssa_event));
                  switch (toupper(*line))
                    {
                      case 'D' : e->type = DIALOGUE; break;
                      case 'M' : e->type = MOVIE;    break;
                      case 'P' : e->type = PICTURE;  break;
                      case 'S' : e->type = SOUND;    break;
                      case 'C' :
                        if (strncmp(line, "Comment", 7) == 0)
                          e->type = COMMENT;
                        if (strncmp(line, "Command", 7) == 0)
                          e->type = COMMAND;
                        break;
                      default  :
                        _log(log_warn, _("Unknown event type at line '%lu': %s"), line_num, line);
                        continue; /* main loop */
                        break;
                    }
                  if (get_ssa_event(line, e, file->event_fields_order) != false)
                    ssa_event_append(&file->events, &elist_tail, e, opts.i_sort);
                  else free(e);
                }
              break;
            case FONTS :
              _log(log_debug, MSG_W_CURRSECTION, line_num, _("fonts"));
              if (get_fonts == false)
                continue;
              get_ssa_media(&file->fonts, &f, line);
              break;
            case GRAPHICS :
              _log(log_debug, MSG_W_CURRSECTION, line_num, _("graphics"));
              if (get_graph == false)
                continue;
              get_ssa_media(&file->images, &g, line);
              break;
            case UNKNOWN :
              _log(log_debug, MSG_W_CURRSECTION, line_num, _("unknown"));
              break;
            case NONE :
            default :
              _log(log_warn, _("Skipping line %i: not in any ssa section."), line_num);
              break;
          }
        }

      /* some checks and fixes */
      if (file->type == ssa_unknown)
        _log(log_error, _("Missing 'Script Type' line in input file."));

      /* this is needed, if last line was 80 chars also */
      if (f != NULL) fflush(f->data);
      if (g != NULL) fflush(g->data);

      if (file->timer == 0)
        {
          _log(log_warn, _("Undefined or zero 'Timer' value. Default value assumed."));
          file->timer = 100;
        }

      if ((get_styles && file->styles == (ssa_style *) 0) || !get_styles)
        {
          _log(log_warn, _("No styles was defined. Default style assumed."));
          CALLOC(file->styles, 1, sizeof(ssa_style));
          memcpy(file->styles, &ssa_style_template, sizeof(ssa_style));
          STRNDUP(file->styles->name, "Default", MAXLINE);
          STRNDUP(file->styles->fontname, SSA_DEFAULT_FONT, MAXLINE);
        }

      return true;
  }


/** low-level parse functions */
/** this functions should return:
 *  true - if all went fine
 * false - if line stays unhandled */

bool
get_ssa_param(char * const line, ssa_file * const h)
  {
    /* line: "Param: Value" */
    char *v = NULL;
    int p_len = 0;

    if ((v = strchr(line, ':')) == NULL)
      {
        _log(log_warn, _("Can't get parameter value at line '%u'."), line_num);
        return false;
      }

    p_len = v - line;

    for (v += 1; *v != '\0' && isspace(*v); v++);

    if (strlen(v) == 0)
      {
        _log(log_info, MSG_W_SKIPEPARAM, line, line_num);
        return true;
      }

    if      (strncmp(line, "PlayResX", p_len) == 0)
      h->res.width  = atoi(v);
    else if (strncmp(line, "PlayResY", p_len) == 0)
      h->res.height = atoi(v);
    else if (strncmp(line, "PlayDepth", p_len) == 0)
      h->depth = atoi(v);
    else if (strncmp(line, "Synch Point", p_len) == 0)
      h->sync = atof(v);
    else if (strncmp(line, "Timer", p_len) == 0)
      h->timer = atof(v);
    else if (strncmp(line, "WrapStyle", p_len) == 0)
      {
        if (h->type == ssa_v4p) h->wrap = atoi(v);
        else _log(log_warn, MSG_W_NOTALLOWED, "Parameter", line);
      }
    else if (strncmp(line, "ScriptType", p_len) == 0)
      {
        if      (strncmp(v, "v4.00+", 6) == 0) h->type = ssa_v4p;
        else if (strncmp(v, "v4.00",  5) == 0) h->type = ssa_v4;
        else if (strncmp(v, "v3.00",  5) == 0) h->type = ssa_v3;
        else h->type = ssa_unknown;
      }
    else if (strncmp(line, "Title", p_len) == 0 || \
             strncmp(line, "Collisions", p_len) == 0 || \
             strncmp(line, "Original Script", p_len) == 0 || \
             strncmp(line, "Original Timing", p_len) == 0 || \
             strncmp(line, "Original Editing", p_len) == 0 || \
             strncmp(line, "Script Updated By", p_len) == 0 || \
             strncmp(line, "Original Translation", p_len) == 0)
      {
        slist_add(&(h->txt_params), line);
      }
    else if (opts.i_strict == true)
      _log(log_warn, MSG_W_SKIPSTRICT, line, line_num);
    else if (opts.i_strict == false)
      {
        _log(log_warn, MSG_W_UNCOMMON, _("parameter"), line_num, line);
        slist_add(&(h->txt_params), line);
      }

    return true;
  }

bool
set_style_fields_order(char *format, ssa_version v, int8_t *fieldlist)
  {
    bool result = true;
    int8_t *fields_order;
    char compare[MAXLINE];
    char buf[MAXLINE + 1] = "";

    strncpy(buf, format, MAXLINE);

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
        case ssa_unknown:
        default :
          return false;
          break;
      }

    /* prepare provided 'Format:' string */
    string_skip_chars(buf, " ");
    string_lowercase(buf, 0);

    /* prepare standart 'Format:' string */
    string_skip_chars(compare, " ");
    string_lowercase(compare, 0);

    if (strcmp(compare,  buf) == 0)
      memcpy(fieldlist, fields_order, sizeof(uint8_t) * MAX_FIELDS);
    else if ((result = detect_style_fields_order(buf, fieldlist)) == true)
      _log(log_warn, MSG_W_WRONGFORDER, _("styles"));
    else
      {
        _log(log_warn, MSG_W_CANTDETECT, _("styles"));
        memcpy(fieldlist, fields_order, sizeof(uint8_t) * MAX_FIELDS);
      }

    return result;
  }

/* 'format' passed to this function must be lowercase string, *
 * contains only alphanumeric chars, commas and no spaces    */
bool
detect_style_fields_order(char *format, int8_t *fieldlist)
  {
    char *p, *token;
    bool result;
    int i;
    int8_t *field = fieldlist;

    if ((p = strchr(format, ':')) == 0)
      {
        _log(log_error, _("Malformed 'Format:' line."));
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
             if (!strcmp(token, "name"))            *field = STYLE_NAME;
        else if (!strcmp(token, "fontname"))        *field = STYLE_FONTNAME;
        else if (!strcmp(token, "fontsize"))        *field = STYLE_FONTSIZE;
        else if (!strcmp(token, "primarycolour"))   *field = STYLE_PCOLOR;
        else if (!strcmp(token, "secondarycolour")) *field = STYLE_SCOLOR;
        else if (!strcmp(token, "backcolour"))      *field = STYLE_BCOLOR;
        else if (!strcmp(token, "tertiarycolour"))  *field = STYLE_TCOLOR;
        else if (!strcmp(token, "outlinecolour"))   *field = STYLE_TCOLOR;
        else if (!strcmp(token, "bold"))            *field = STYLE_BOLD;
        else if (!strcmp(token, "italic"))          *field = STYLE_ITALIC;
        else if (!strcmp(token, "underline"))       *field = STYLE_UNDER;
        else if (!strcmp(token, "strikeout"))       *field = STYLE_STRIKE;
        else if (!strcmp(token, "scalex"))          *field = STYLE_SCALEX;
        else if (!strcmp(token, "scaley"))          *field = STYLE_SCALEY;
        else if (!strcmp(token, "spacing"))         *field = STYLE_SPACING;
        else if (!strcmp(token, "angle"))           *field = STYLE_ANGLE;
        else if (!strcmp(token, "borderstyle"))     *field = STYLE_BORDER;
        else if (!strcmp(token, "outline"))         *field = STYLE_OUTLINE;
        else if (!strcmp(token, "shadow"))          *field = STYLE_SHADOW;
        else if (!strcmp(token, "alignment"))       *field = STYLE_ALIGN;
        else if (!strcmp(token, "marginl"))         *field = STYLE_MARGINL;
        else if (!strcmp(token, "marginr"))         *field = STYLE_MARGINR;
        else if (!strcmp(token, "marginv"))         *field = STYLE_MARGINV;
        else if (!strcmp(token, "encoding"))        *field = STYLE_ENC;
        else if (!strcmp(token, "alphalevel"))      *field = STYLE_ALPHA;
        else
          _log(log_warn, MSG_W_UNRECFIELD, token), result = false;

        field++;
      }
    while ((token = strtok(NULL, ",")) != 0 && *field != 0);

    if (*field == 0)
      _log(log_error, MSG_W_TOOMANYFIELDS, _("styles"));
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
    char token[MAXLINE] = "";
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

    CALLOC(*ptr_alloc, 1, sizeof(ssa_style));
    memcpy(*ptr_alloc, &ssa_style_template, sizeof(ssa_style));

    ptr = *ptr_alloc;

    p = p + 1; /* "Style:| " */

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
                trim_spaces(token, LINE_START | LINE_END);
                STRNDUP(ptr->name, token, MAXLINE);
              break;
            case STYLE_FONTNAME :
                trim_spaces(token, LINE_START | LINE_END);
                STRNDUP(ptr->fontname, token, MAXLINE);
              break;
            case STYLE_FONTSIZE : ptr->fontsize = atof(token);   break;
            case STYLE_BOLD     : ptr->bold = atoi(token);       break;
            case STYLE_ITALIC   : ptr->italic = atoi(token);     break;
            case STYLE_UNDER    : ptr->underlined = atoi(token); break;
            case STYLE_STRIKE   : ptr->strikeout = atoi(token);  break;
            case STYLE_SCALEX   : ptr->scale_x = atoi(token);    break;
            case STYLE_SCALEY   : ptr->scale_y = atoi(token);    break;
            case STYLE_SPACING  : ptr->spacing = atoi(token);    break;
            case STYLE_ANGLE    : ptr->angle = atof(token);      break;
            case STYLE_OUTLINE  : ptr->outline = atoi(token);    break;
            case STYLE_SHADOW   : ptr->shadow = atoi(token);     break;
            case STYLE_ALIGN    : ptr->alignment = atoi(token);  break;
            case STYLE_MARGINL  : ptr->margin_l = atoi(token);   break;
            case STYLE_MARGINR  : ptr->margin_r = atoi(token);   break;
            case STYLE_MARGINV  : ptr->margin_v = atoi(token);   break;
            case STYLE_ENC      : ptr->codepage = atoi(token);   break;
            case STYLE_BORDER   : ptr->brd_style = atoi(token);  break;
            case STYLE_ALPHA    : ptr->a_level  = atoi(token);   break;
            case STYLE_PCOLOR   : ptr->pr_color = ssa_color(token); break;
            case STYLE_SCOLOR   : ptr->se_color = ssa_color(token); break;
            case STYLE_TCOLOR   : ptr->tr_color = ssa_color(token); break;
            case STYLE_BCOLOR   : ptr->bg_color = ssa_color(token); break;
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
    char compare[MAXLINE];

    string_skip_chars(format, " ");
    string_lowercase(format, 0);

    switch (v)
      {
        case ssa_v4p :
          strcpy(compare, SSA_EVENT_V4P_FORMAT);
          break;
        case ssa_v4 :
          strcpy(compare, SSA_EVENT_V4_FORMAT);
          break;
        case ssa_v3:
        case ssa_unknown:
        default :
          return false;
          break;
      }

    string_skip_chars(compare, " ");
    string_lowercase(compare, 0);

    if (strcmp(compare,  format) == 0)
      memcpy(fieldlist, event_fields_normal_order, sizeof(uint8_t) * MAX_FIELDS);
    else if ((result = detect_event_fields_order(format, fieldlist)) == true)
      _log(log_warn, MSG_W_WRONGFORDER, _("events"));
    else
      {
        _log(log_warn, MSG_W_CANTDETECT, _("events"));
        memcpy(fieldlist, event_fields_normal_order, sizeof(uint8_t) * MAX_FIELDS);
      }

    return result;
  }

/* 'format' passed to this function must be lowercase string, *
 * contains only alphanumeric chars, commas and no spaces    */
bool
detect_event_fields_order(char *format, int8_t *fieldlist)
  {
    char *p, *token;
    char buf[MAXLINE + 1] = "";
    bool result;
    int i;
    int8_t *field = fieldlist;

    if (!format || !fieldlist) return false;

    strncpy(buf, format, MAXLINE);
    if ((p = strchr(buf, ':')) == 0)
      {
        _log(log_error, _("Malformed 'Format:' line."));
        return false;
      }

    /* this saves from of duplicate code below *
     * possible, i should do it with memset()? */
    *(field + MAX_FIELDS) = 0; /* set list-terminator */
    for (i = 0; i < MAX_FIELDS; i++) *field++ = -1;

    token = strtok(++p, ",");
    do
      {
             if (!strcmp(token, "layer"))   *field = EVENT_LAYER;
        else if (!strcmp(token, "start"))   *field = EVENT_START;
        else if (!strcmp(token, "end"))     *field = EVENT_END;
        else if (!strcmp(token, "style"))   *field = EVENT_STYLE;
        else if (!strcmp(token, "name"))    *field = EVENT_NAME;
        else if (!strcmp(token, "marginl")) *field = EVENT_MARGINL;
        else if (!strcmp(token, "marginr")) *field = EVENT_MARGINR;
        else if (!strcmp(token, "marginv")) *field = EVENT_MARGINV;
        else if (!strcmp(token, "effect"))  *field = EVENT_EFFECT;
        else if (!strcmp(token, "text"))    *field = EVENT_TEXT;
        else _log(log_warn, MSG_W_UNRECFIELD, token), result = false;

        field++;
      }
    while ((token = strtok(NULL, ",")) != 0 && *field != 0);

    if (*field == 0)
      _log(log_error, MSG_W_TOOMANYFIELDS, _("events"));
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
    char buf[MAXLINE];
    int len = -1;
    double *t;

    if (!event || !line || !fieldlist)
      return false;

    if ((p = strchr(line, ':')) == NULL) return false;

    p = p + 1; /* "EventType:|" */

    while (*field != 0)
      {
        len = _strtok(p, delim);
        if (len < 0) return false;

        if (len > 0 && len < MAXLINE)
          {
            strncpy(buf, p, len);
            p += (len + strlen(delim));
            buf[len] = '\0';
          }
        else /* len == 0 || len > MAXLINE */
          buf[0] = '\0', p++;

        switch (*field)
          {
            case EVENT_LAYER :
              event->layer = atoi(buf); /* little hack */
              break;
            case EVENT_START :
            case EVENT_END :
              t = (*field == EVENT_START) ? &event->start : &event->end;
              if (!str2subtime(buf, &st))
                {
                  _log(log_warn, _("Can't get timing at line '%u'."), line_num);
                  return false;
                }
              else
                subtime2double(&st, t);
              break;
            case EVENT_STYLE :
              STRNDUP(event->style, buf, MAXLINE);
              break;
            case EVENT_NAME :
              STRNDUP(event->name, buf, MAXLINE);
              break;
            case EVENT_MARGINL :
              event->margin_l = atoi(buf);
              break;
            case EVENT_MARGINR :
              event->margin_r = atoi(buf);
              break;
            case EVENT_MARGINV :
              event->margin_v = atoi(buf);
              break;
            case EVENT_EFFECT :
              STRNDUP(event->effect, buf, MAXLINE);
              break;
            case EVENT_TEXT :
              STRNDUP(event->text, buf, MAXLINE);
              break;
            default :
              break;
          }
        field++;
        if (*field == EVENT_TEXT) delim = "";
      }

    return true;
  }

bool
get_ssa_media(ssa_media **list, ssa_media **h, char const * const line)
  {
    char *p = NULL;

    if (list == NULL || h == NULL || line == NULL)
      return false;

    switch (detect_media_line_type(line))
      {
        case MEDIA_HEADER :
          if ((*h) != NULL) {
              fflush((*h)->data);
              CALLOC((*h)->next, 1, sizeof(ssa_media));
              *h = (*h)->next;
            } else {
              CALLOC((*h), 1, sizeof(ssa_media));
              *list = *h;
            }
          if (strncmp(line, "fontname", 8) == 0)
            (*h)->type = type_font;
          if (strncmp(line, "filename", 8) == 0)
            (*h)->type = type_image;
          if ((p = strchr(line, ':')) != NULL)
          {
            for (p += 1; *p != '\0' && isspace(*p); p++);
            STRNDUP((*h)->filename, p, MAXLINE);
          }
          TMPFILE((*h)->data);
          break;
        case MEDIA_UUE_LINE :
          fputs(line, (*h)->data);
          fputs("\n", (*h)->data);
          break;
        case MEDIA_UUE_TAIL :
          fputs(line, (*h)->data);
          fputs("\n", (*h)->data);
          fflush((*h)->data);
          break;
        default :
          /* do nothing */
          break;
      }

    return true;
  }

/* write functions */

bool
write_ssa_file(FILE *outfile, ssa_file *f, bool memfree)
  {
    bool result = true;

    if (!outfile || !f)
      return false;

    result &= write_ssa_header(outfile, f, memfree);

    if (f->styles)
      result &= write_ssa_styles(outfile, f->styles, f->type, memfree);

    if (f->events)
      result &= write_ssa_events(outfile, f->events, f->type, memfree);

    if (f->fonts)
      result &= write_ssa_media(outfile, f->fonts, memfree);

    if (f->images)
      result &= write_ssa_media(outfile, f->images, memfree);

    return result;
  }

bool
write_ssa_txt_param(FILE *outfile, char *name,\
                    char *data, bool flag, bool memfree)
  {
    if (!outfile || !name)
      return false;

    /* data CAN be NULL */
    if (flag && data)
      fprintf(outfile, "%s: %s\n", name, data);
    else
      fprintf(outfile, ";%s:\n", name);

    if (memfree && data)
      free(data);

    return true;
  }

bool
write_ssa_header(FILE *outfile, ssa_file * const f, bool memfree)
  {
    struct slist *p = NULL, *l = NULL;

    if (!outfile || !f) return false;

    if (fprintf(outfile, "%s\n", "[Script Info]") < 0)
      _log(log_error, MSG_F_WRFAIL);

    fprintf(outfile, "; Generated by: %s %.2f\n",
              COMMON_PROG_NAME, VERSION);

    /* text fields. validity should be checked during parsing */
    for (p = f->txt_params; p != NULL;)
      {
        fprintf(outfile, "%s\n", p->value);
        l = p, p = p->next;
        if (memfree) free(l->value), free(l);
      }

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

    return true;
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
        case ssa_unknown:
        default :
          return false;
          break;
      }

    if (section_header)
      write = fprintf(outfile, "[V4%s Styles]\n", s);

    if (format_string)
      write = fprintf(outfile, format);

    if (write < 0)
      _log(log_error, MSG_F_WRFAIL);

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
        case ssa_unknown:
        default :
          color_format = "%i,%i,%i,%i,";
          break;
      }

    fprintf(outfile, color_format, \
              style->pr_color, style->se_color, \
              style->tr_color, style->bg_color);

    fprintf(outfile, "%i,%i,", style->bold, style->italic);

    /* ssa v4+ specific parameters ^_^ */
    if (v == ssa_v4p)
      fprintf(outfile, "%i,%i,%i,%i,%i,%.0f,", \
                style->underlined, style->strikeout, \
                style->scale_x, style->scale_y, \
                style->spacing, style->angle);

    fprintf(outfile, "%i,%i,%i,%i,",  \
                style->brd_style, style->outline, \
                style->shadow,    style->alignment);

    fprintf(outfile, "%i,%i,%i,",  \
                style->margin_l, style->margin_r, style->margin_v);

    if (alphalevel)
        fprintf(outfile, "%i,", style->a_level);

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
        case ssa_unknown:
        default :
          return false;
          break;
      }

    if (section_header)
      write = fprintf(outfile, "[Events]\n");

    if (format_string)
      write = fprintf(outfile, "%s\n", format);

    if (write < 0)
      _log(log_error, MSG_F_WRFAIL);

    while (ptr != NULL)
      {
        write_ssa_event(outfile, ptr, v);
        prev = ptr;
        ptr = ptr->next;
        if (memfree) free(prev->text), free(prev);
      }

    fputc('\n', outfile);

    return true;
  }

bool
write_ssa_event(FILE *outfile, ssa_event * const event, ssa_version v)
  {
    struct subtime st;
    char *type = "";

    switch (event->type)
      {
        case COMMAND  :
        case DIALOGUE : type = "Dialogue"; break;
        case COMMENT  : type = "Comment";  break;
        case MOVIE    : type = "Movie";    break;
        case PICTURE  : type = "Picture";  break;
        case SOUND    : type = "Sound";    break;
        default       : type = "";         break;
      }
    fprintf(outfile, "%s: %s%i,", type, (v == ssa_v4) ? "Marked=" : "", event->layer);

    event->start = round(event->start * 100.0) / 100.0;
    double2subtime(event->start, &st);
    fprintf(outfile, "%i:%02i:%02i.%02i,",
        st.hrs, st.min, st.sec, st.msec / 10);

    event->end   = round(event->end   * 100.0) / 100.0;
    double2subtime(event->end, &st);
    fprintf(outfile, "%i:%02i:%02i.%02i,",
        st.hrs, st.min, st.sec, st.msec / 10);

    fprintf(outfile, "%s,%s,%04i,%04i,%04i,%s,%s", \
                    event->style, event->name,\
                    event->margin_l, event->margin_r, event->margin_v, \
                    event->effect, event->text);
    fputc('\n', outfile);

    return true;
  }

bool
write_ssa_media(FILE * outfile, ssa_media * const list, bool memfree)
  {
    ssa_media *h = NULL;
    ssa_media *t = NULL;
    char *media_type = NULL;
    size_t read = 0;
    uint8_t buf[MAXLINE];

    if (!outfile || !list)
      return false;

    h = list;
    media_type = (h->type == type_font) ? "Fonts" : "Graphics";
    fprintf(outfile, "[%s]\n", media_type);

    while (h != NULL)
      {
        media_type = (h->type == type_font) ? "fontname" : "filename";
        fprintf(outfile, "%s: %s\n", media_type, h->filename);

        rewind(h->data);
        while ((read = fread(buf, sizeof(uint8_t), MAXLINE, h->data)) > 0)
          if (fwrite(buf, sizeof(uint8_t), read, outfile) == 0 && ferror(outfile) != 0)
            _log(log_error, MSG_F_WRFAIL);

        if (errno)
          _log(log_error, "%s", strerror(errno));
        fflush(outfile);

        t = h;
        h = h->next;
        if (memfree == true)
          {
            free(t->filename);
            fclose(t->data);
            free(t);
          }
      }

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
        case ssa_unknown :
        default :
          return "";
          break;
      }
  }

/* returns true, if changes section and false otherwise */
bool
ssa_section_switch(enum ssa_section *section, char const * const line)
  {
    char buf[MAXLINE];

    if (!section || !line)
      return false;

    if (line[0] != '[')
      return false;

    strncpy(buf, line, MAXLINE);
    string_lowercase(buf, 0);

         if (!strcmp(buf, "[script info]")) *section = HEADER;
    else if (!strcmp(buf, "[events]"))      *section = EVENTS;
    else if (!strcmp(buf, "[fonts]"))       *section = FONTS;
    else if (!strcmp(buf, "[graphics]"))    *section = GRAPHICS;
    else if (!strcmp(buf, "[v4 styles]"))   *section = STYLES;
    else if (!strcmp(buf, "[v4+ styles]"))  *section = STYLES;
    else
      {
        _log(log_warn, _("Unknown ssa section '%s' at line '%u'."), line, line_num);
        *section = UNKNOWN; /* by default */
        return false;
      }

    return true;
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

ssa_style *
find_ssa_style_by_name(ssa_file *f, char *name)
  {
    ssa_style *s = NULL;

    if (!f || !name)
      return NULL;

    /* else */
    for (s = f->styles; s != NULL; s = s->next)
      if (strcmp(name, s->name) == 0)
        return s;

    /* if this not works */
    return NULL;
  }

int8_t
detect_media_line_type(char const * const line)
  {
    ssize_t len = 0;

    len = strlen(line);
    if (len == 80)
      return MEDIA_UUE_LINE;

    if (len == 0)
      return MEDIA_UUE_TAIL;

    if (strncmp(line, "fontname:", 9) == 0)
      return MEDIA_HEADER;

    if (strncmp(line, "filename:", 9) == 0)
      return MEDIA_HEADER;

    /* The keywords above MUST be lowercase, but we should     *
     * remember about not-well-coded applications, who think   *
     * that 'filename' should conforms to style of other lines */
    if (len >= 9 && (strncmp((line + 1), "ontname:", 8) == 0 ||
                     strncmp((line + 1), "ilename:", 8) == 0))
      {
        _log(log_warn, _("Keyword '*name' must be fully lowercase: %u:%s"), \
                      line_num, line);
        return MEDIA_HEADER;
      }

    if (len > 0 && len < 80)
      return MEDIA_UUE_TAIL;

    return MEDIA_UNKNOWN;
  }
