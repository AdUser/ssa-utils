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

#include "srt.h"
#include "ssa.h"

#define PROG_NAME "srt2ssa"

/* import some usefull stuff */
extern struct options opts;
extern ssa_style ssa_style_template;
extern ssa_event ssa_event_template;

void usage(int exit_code)
  {
    usage_convert(PROG_NAME);
    fputc('\n', stderr);

    usage_common_opts();
    fputc('\n', stderr);

    usage_convert_input();
    /* specific options */
    fprintf(stderr, _("\
  -e                Strict mode. Discard all 'srt' format extensions.\n"));
    fputc('\n', stderr);

    usage_convert_output();
    fputc('\n', stderr);

    exit(exit_code);
 }

/* ssa tags differs from srt not only in format, but in scope too,     *
 * for example, if srt tag acts as borders for scope of some property, *
 * ssa - set this property untill next tag with the same name          */
bool
srt_tags_to_ssa(char *string, ssa_file *file)
  {
    char *p;
    char *value;
    char tags_buf[MAXLINE + 1] = "";
    char common_buf[MAXLINE + 1] = "";
    STACK_ELEM stack[STACK_MAX];
    STACK_ELEM *top = stack;
    STACK_ELEM chr;
    int16_t len = 0;
    uint8_t i = 0;
    uint8_t font_params = 0;
    struct tag ttag;
    ssa_style *style = NULL;
    enum { none, text, tag } last = none;

    if (!string || !file) return false;

    stack_init(stack);
    for (p = string; (len = parse_html_tag(p, &ttag)) != 0; )
      {
        if (len > 0) /* it's a tag! */
          {
            last = tag;
            if (strlen(ttag.data) == 1)
              {
                chr = toupper(ttag.data[0]);
                switch (chr)
                  {
                    case SRT_T_STRIKEOUT :
                    case SRT_T_UNDERLINE :
                      if (file->type == ssa_v4)
                        {
                          log_msg(warn, MSG_W_TAGNOTALL, ttag.data);
                          break;
                        }
                    /* break; */
                    case SRT_T_BOLD      :
                    case SRT_T_ITALIC    :
                      append_char(tags_buf, '\\', MAXLINE);
                      append_char(tags_buf, tolower(chr), MAXLINE);
                      append_char(tags_buf, (ttag.type == opening) ? '1' : '0',
                                    MAXLINE);
                      break;
                    default  :
                      log_msg(warn, MSG_W_UNRECTAG, ttag.data, string);
                      break;
                } /* switch (chr) */
              }
            else if (strcmp(ttag.data, "font") == 0 && ttag.type == opening)
              {
                if ((value = get_tag_param_by_name(&ttag, "size")) != NULL)
                  {
                    append_string(tags_buf, value, "\\fs", MAXLINE, 0);
                    font_params &= SRT_T_FONT_SIZE;
                  }

                if ((value = get_tag_param_by_name(&ttag, "face")) != NULL)
                  {
                    append_string(tags_buf, value, "\\fn", MAXLINE, 0);
                    font_params &= SRT_T_FONT_FACE;
                  }
                else
                if ((value = get_tag_param_by_name(&ttag, "name")) != NULL)
                  {
                    log_msg(warn, MSG_W_TAGNOTFACE);
                    append_string(tags_buf, value, "\\fn", MAXLINE, 0);
                    font_params &= SRT_T_FONT_FACE;
                  }

                if ((value = get_tag_param_by_name(&ttag, "color")) != NULL)
                  {
                    append_char(tags_buf, '\\', MAXLINE);
                    if (file->type == ssa_v4p)
                      append_char(tags_buf, '1', MAXLINE);
                    i = strlen(tags_buf);
                    snprintf((tags_buf + i), MAXLINE - i, "c&H%X&",
                              parse_color(value)); /* TODO: check pointer */
                    font_params &= SRT_T_FONT_COLOR;
                  }
                chr = SRT_T_FONT;
              }
            else if (strcmp(ttag.data, "font") == 0 && ttag.type == closing)
              {
                /* first, find right style for current event *
                 * if not found, default will be used */
                style = &ssa_style_template;

                if (font_params & SRT_T_FONT_FACE)
                  append_string(tags_buf, style->fontname, "\\fn", MAXLINE, 0);

                if (font_params & SRT_T_FONT_COLOR)
                  {
                    append_char(tags_buf, '\\', MAXLINE);
                    if (file->type == ssa_v4p)
                      append_char(tags_buf, '1', MAXLINE);
                    i = strlen(tags_buf);
                    snprintf((tags_buf + i), (MAXLINE - i),
                                "c&H%X&", style->pr_color);
                  }

                if (font_params & SRT_T_FONT_SIZE)
                  {
                    append_string(tags_buf, "\\fs", "", MAXLINE, 0);
                    i = strlen(tags_buf);
                    snprintf((tags_buf + i), (MAXLINE - i),
                               "%.1f", style->fontsize);
                  }

                font_params = 0x0;
                chr = SRT_T_FONT;
              }
            else
              {
                /* as we don't know, how to handle this tag, *
                 * skip stack operations below               */
                log_msg(warn, MSG_W_UNRECTAG, ttag.data, string);
                ttag.type = none; /* skip stack operations */
                len =  -len; /* to handle as text in block below */
              }

            /* stack operations */
            switch (ttag.type)
              {
                case opening :
                  if (*top == chr)
                    log_msg(warn, MSG_W_TAGTWICE, ttag.data, string);
                  stack_push(stack, top, chr);
                  break;
                case closing :
                  if (*top == chr) stack_pop(stack, top);
                  else log_msg(warn, MSG_W_TAGUNCL, ttag.data, string);
                  /* note: stack remains unchanged in second case! */
                  break;
                case standalone :
                  log_msg(info, MSG_W_TAGXMLSRT);
                  /* break; */
                case none :
                default :
                  /* do nothing */
                  break;
              }
          }

        if (len < 0)
          {
            if (last == tag && strlen(tags_buf) != 0)
              {
                append_string(common_buf, tags_buf, "{", MAXLINE, 0);
                append_char(common_buf, '}', MAXLINE);
                memset(tags_buf, 0, MAXLINE);
              }
            append_string(common_buf, p, "", MAXLINE, -len);
            last = text;
          }

        p += (len > 0) ? len : -len ;
      } /* main 'for' cycle ends */

    /* TODO: check stack for unclosed / deranged tags  */
    /* copy temp buffer to right place ^_^ */
    strncpy(string, common_buf, MAXLINE);

    return true;
  }

/* options */
unsigned long int line_num = 0;
unsigned long parsed = 0;

int main(int argc, char *argv[])
  {
    srt_file source;
    ssa_file target;
    srt_event *src;
    ssa_event **dst;
    char opt;
    char buf[MAXLINE] = "";

    if (argc < 2) usage(EXIT_SUCCESS);

    /* init, stage 1 */
    memset(&source, 0, sizeof(srt_file));
    init_ssa_file(&target);
    fesetround(1); /* no nearest integer */

    /* parsing options */
    while ((opt = getopt(argc, argv, "qvhi:o:" "e" "ST" "f:x:y:Fw:")) != -1)
      {
        switch (opt)
          {
            case 'f' :
              if      (strcmp(optarg, "ssa") == 0) target.type = ssa_v4;
              else if (strcmp(optarg, "ass") == 0) target.type = ssa_v4p;
              break;
            case 'q' :
            case 'v' :
              msglevel_change(&opts.msglevel, (opt == 'q') ? '-' : '+');
              break;
            case 'i' :
              if ((opts.infile = fopen(optarg, "r")) == NULL)
                log_msg(error, MSG_F_ORDFAIL, optarg);
              break;
            case 'o' :
              if ((opts.outfile = fopen(optarg, "w")) == NULL)
                {
                  log_msg(warn, MSG_F_OWRFAILSO, optarg);
                  opts.outfile = stdout;
                }
              break;
            case 'e' :
              log_msg(info, _("Strict mode. No mercy for malformed lines or uncommon extensions!"));
              target.flags |= SRT_E_STRICT;
              break;
            case 'S' :
              opts.i_sort = true;
              break;
            case 'T' :
              opts.i_test = true;
              break;
            case 'x' :
              target.res.width  = atoi(optarg);
              break;
            case 'y' :
              target.res.height = atoi(optarg);
              break;
            case 'w' :
              set_wrap(&opts.o_wrap, optarg);
              break;
            case 'F' :
              opts.o_fsize_tune = true;
              break;
            case 'h' :
              usage(EXIT_SUCCESS);
              break;
            default :
              usage(EXIT_FAILURE);
              break;
        }
      }

    /* checks */
    common_checks(&opts);

    if (target.type == unknown)
      log_msg(error, MSG_O_OREQUIRED, "-f");

    if (opts.o_fsize_tune && !target.res.width && !target.res.height)
      log_msg(error, _("'-F' option requires '-x' and/or '-y'."));

    if (!parse_srt_file(opts.infile, &source))
      log_msg(error, MSG_U_UNKNOWN);

    if (opts.i_test)
      {
        log_msg(warn, MSG_W_TESTDONE);
        exit(EXIT_SUCCESS);
      }

    /* init, stage 2 */
    src = source.events;
    dst = &target.events;
    if ((target.styles = calloc(1, sizeof(ssa_style))) == NULL)
      log_msg(error, MSG_M_OOM);

    memcpy(target.styles, &ssa_style_template, sizeof(ssa_style));

    while (src != (srt_event *) 0)
      {
        if ((*dst = (ssa_event *) calloc(1, sizeof(ssa_event))) == NULL)
          log_msg(error, MSG_M_OOM);

        memcpy(*dst, &ssa_event_template, sizeof(ssa_event));

        /* copy simple data */
        (*dst)->type  = DIALOGUE;
        (*dst)->start = src->start;
        (*dst)->end   = src->end;

        strncpy(buf, src->text, MAXLINE);
        free(src->text);

        /* convert tags */
        if (strchr(buf, '<') != NULL)
          srt_tags_to_ssa(buf, &target);

        /* text wrapping */
        if      (opts.o_wrap == keep)
          text_replace(buf, "\n", "\\n", MAXLINE, 0);
        else if (opts.o_wrap == merge)
          text_replace(buf, "\n", " ",   MAXLINE, 0);

        strncpy((*dst)->text, buf, MAXLINE);
        /* line above is dummy plug. Should be replaced with next soon:
        if (((*dst)->text = strndup(buf, MAXLINE)) == NULL)
          log_msg(error, MSG_M_OOM); */

        /* events list operations */
        dst = &((*dst)->next);
        source.events = src->next;
        free(src);
        src = source.events;
      }

    font_size_normalize(&target.res, &target.styles->fontsize);

    write_ssa_file(opts.outfile, &target, true);

    /* prepare to exit */
    if (opts.infile  != NULL)   fclose(opts.infile);
    if (opts.outfile != NULL &&
        opts.outfile != stdout) fclose(opts.outfile);

    return 0;
  }
