#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_SIZE (16 * 1024)

#define OPEN(file, path, flags) \
    if (((file) = open((path), (flags))) == -1) \
      { \
        perror(NULL); \
        exit(EXIT_FAILURE); \
      }

#define READ(file, buf, read_size, bytes_read) \
  if (((bytes_read) = read((file), (buf), (read_size))) < 0) \
    { \
      perror(NULL); \
      exit(EXIT_FAILURE); \
    }

#define CMP_SIZE(x, y) \
  if ((x) != (y)) \
      { \
        fprintf(stderr, "Size mismatch.\n"); \
        exit(EXIT_FAILURE); \
      }

int main(int argc, char **argv)
  {
    int file1;
    int file2;
    uint8_t buf1[BUF_SIZE];
    uint8_t buf2[BUF_SIZE];
    ssize_t read1;
    ssize_t read2;


    if (argc < 2)
      {
        fprintf(stderr, "Usage: compare <file1> <file2>\n");
        exit(EXIT_FAILURE);
      }

    OPEN(file1, argv[1], O_RDONLY);
    OPEN(file2, argv[2], O_RDONLY);

    CMP_SIZE(lseek(file1, 0, SEEK_END), lseek(file2, 0, SEEK_END));

    lseek(file1, 0, SEEK_SET);
    lseek(file2, 0, SEEK_SET);

    while (1)
      {
        READ(file1, buf1, BUF_SIZE, read1);
        READ(file2, buf2, BUF_SIZE, read2);

        CMP_SIZE(read1, read2);

        if (read1 == 0)
          break;

        if (memcmp(buf1, buf2, read1) != 0)
          {
            fprintf(stderr, "Content mismatch.\n");
            exit(EXIT_FAILURE);
          }
      }

    fprintf(stderr, "Files equal.\n");
    return EXIT_SUCCESS;
  }
