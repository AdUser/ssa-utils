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
#include "microsub.h"
#include "ssa.h"

#define PROG_NAME "microsub2ssa"

void usage(int exit_code)
  {
    usage_convert(PROG_NAME);
    fputc('\n', stderr);

    usage_common_opts();
    fputc('\n', stderr);

    usage_convert_input();
    fputc('\n', stderr);

    usage_convert_output();
    fputc('\n', stderr);

    exit(exit_code);
 }

/* import some usefull stuff */
extern struct options opts;
extern ssa_style ssa_style_template;
extern ssa_event ssa_event_template;

/* options */
unsigned long int line_num = 0;
unsigned long parsed = 0;

int main(int argc, char *argv[])
  {
    microsub_file   source;
    ssa_file        target;
    microsub_event *src;
    ssa_event     **dst;
    char opt;
    char buf[MAXLINE] = "";

    if (argc < 2) usage(EXIT_SUCCESS);

    /* init, stage 1 */
    memset(&source, 0, sizeof(microsub_file));
    init_ssa_file(&target);

    /* parsing options */
    while ((opt = getopt(argc, argv, "qvhi:o:" "ST" "f:x:y:Fw:")) != -1)
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
            case 'h' :
              usage(EXIT_SUCCESS);
              break;
            case 'F' :
              opts.o_fsize_tune = true;
              break;
            default :
              usage(EXIT_FAILURE);
              break;
        }
      }

    /* checks */
    common_checks(&opts);

    if (target.type == ssa_unknown)
      log_msg(error, MSG_O_OREQUIRED, "-f");

    if (opts.o_fsize_tune && !target.res.width && !target.res.height)
      log_msg(error, _("'-F' option requires '-x' and/or '-y'."));

    if (!parse_microsub_file(opts.infile, &source))
      log_msg(error, MSG_U_UNKNOWN);

    if (opts.i_test)
      {
        log_msg(warn, MSG_W_TESTDONE);
        exit(EXIT_SUCCESS);
      }

    /* init, stage 2 */
    src = source.events;
    dst = &target.events;
    CALLOC(target.styles, 1, sizeof(ssa_style));

    memcpy(target.styles, &ssa_style_template, sizeof(ssa_style));

    while (src != (microsub_event *) 0)
      {
        CALLOC(*dst, 1, sizeof(ssa_event));

        memcpy(*dst, &ssa_event_template, sizeof(ssa_event));

        /* copy data */
        (*dst)->type  = DIALOGUE;
        (*dst)->start = src->start;
        (*dst)->end   = src->end;

        strncpy(buf, src->text, MAXLINE);

        /* text wrapping */
        if      (opts.o_wrap == keep)
          text_replace(buf, "|", "\\n", MAXLINE, 0);
        else if (opts.o_wrap == merge)
          text_replace(buf, "|", " ",   MAXLINE, 0);

        _strndup(&((*dst)->text), buf, MAXLINE);

        dst = &((*dst)->next);
        source.events = src->next;
        free(src->text);
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
