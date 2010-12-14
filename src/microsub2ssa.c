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

    /* init, stage 1 */
    memset(&source, 0, sizeof(microsub_file));
    init_ssa_file(&target);

    /* parsing options */
    while ((opt = getopt(argc, argv, "hi:o:qvstsw:f:x:y:")) != -1)
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
                log_msg(error, _("E: Input file '%s' isn't readable."), optarg);
              break;
            case 'o' :
              if ((opts.outfile = fopen(optarg, "w")) == NULL)
                {
                  log_msg(warn, _("W: File '%s' isn't writable, stdout will be used.\n"), optarg);
                  opts.outfile = stdout;
                }
              break;
            case 's' :
              log_msg(info, _("I: Events in output file will be sorted by timing.\n"));
              opts.i_sort = true;
              break;
            case 't' :
              log_msg(info, _("I: Only test will be performed.\n"));
              opts.i_test = true;
              break;
            case 'x' :
              target.res_x = atoi(optarg);
              break;
            case 'y' :
              target.res_y = atoi(optarg);
              break;
            case 'w' :
              opts.o_wrap = keep;
              /* TODO: enable, when i finish this feature
              if      (strcmp(optarg, "keep")  == 0) wrap_mode = keep;
              else if (strcmp(optarg, "merge") == 0) wrap_mode = merge;
              else if (strcmp(optarg, "split") == 0) wrap_mode = split;
              */
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
    if (opts.i_test == true && opts.msglevel < warn)
      opts.msglevel = warn;

    if (opts.infile == NULL)
      log_msg(error, _("E: Input file not specified."));

    if (target.type == unknown)
      log_msg(error, _("E: '-f' option is mandatory."));

    if (!parse_microsub_file(opts.infile, &source))
      log_msg(error, _("E: Something went wrong, see errors above."));

    if (opts.i_test)
      {
        log_msg(warn, _("W: Test of input file completed. See warnings above, if any.\n"));
        exit(EXIT_SUCCESS);
      }

    /* init, stage 2 */
    src = source.events;
    dst = &target.events;
    if ((target.styles = calloc(1, sizeof(ssa_style))) == NULL)
      log_msg(error, _("E: Can't allocate memory."));

    memcpy(target.styles, &ssa_style_template, sizeof(ssa_style));

    while (src != (microsub_event *) 0)
      {
        if ((*dst = (ssa_event *) calloc(1, sizeof(ssa_event))) == NULL)
          log_msg(error, _("E: Can't allocate memory."));

        memcpy(*dst, &ssa_event_template, sizeof(ssa_event));

        /* copy data */
        (*dst)->start = src->start;
        (*dst)->end   = src->end;
        strncpy((*dst)->text, src->text, MAXLINE);
        /* text wrapping here */

        dst = &((*dst)->next);
        source.events = src->next;
        free(src); /* decreases memory consumption */
        src = source.events;
      }

    write_ssa_file(opts.outfile, &target, true);

    /* prepare to exit */
    if (opts.infile  != NULL)   fclose(opts.infile);
    if (opts.outfile != NULL &&
        opts.outfile != stdout) fclose(opts.outfile);

    return 0;
  }
