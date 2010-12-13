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

#define PROG_NAME "test_parse_ssa"

uint32_t line_num = 0;
extern struct options opts;

int main(int argc, char *argv[])
  {
    ssa_file file;

    memset(&file, 0, sizeof(ssa_file));
    init_ssa_file(&file);

    if (argc < 1)
      exit(EXIT_FAILURE);

    if ((opts.infile = fopen(argv[1], "r")) == (FILE *) 0)
      {
        printf("Can't open input file.\n");
        exit(EXIT_FAILURE);
      }

    if (parse_ssa_file(opts.infile, &file) == false)
      exit(EXIT_FAILURE);

    write_ssa_file(stdout, &file, true);

    putc('\n', stdout);

    exit(EXIT_SUCCESS);
  }
