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

/* import some usefull stuff */
extern struct options opts;
extern ssa_style ssa_style_template;
extern ssa_event ssa_event_template;

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

    /* init, stage 1 */
    memset(&source, 0, sizeof(srt_file));
    init_ssa_file(&target);
    fesetround(1); /* no nearest integer */

    /* parsing options */
    while ((opt = getopt(argc, argv, "qvhI:O:" "e" "ST" "f:x:y:Fw:")) != -1)
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
            case 'I' :
              if ((opts.infile = fopen(optarg, "r")) == NULL)
                log_msg(error, MSG_F_ORDFAIL, optarg);
              break;
            case 'O' :
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

        /* copy data */
        (*dst)->start = src->start;
        (*dst)->end   = src->end;
        strncpy((*dst)->text, src->text, MAXLINE);
        /* text wrapping here */
        if      (opts.o_wrap == keep)
          text_replace((*dst)->text, "\n", "\\n", MAXLINE, 0);
        else if (opts.o_wrap == merge)
          text_replace((*dst)->text, "\n", " ",   MAXLINE, 0);


        dst = &((*dst)->next);
        source.events = src->next;
        free(src); /* decreases memory consumption */
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
