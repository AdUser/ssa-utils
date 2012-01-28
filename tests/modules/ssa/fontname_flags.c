#include "../../tests.h"
#include "../../../src/ssa.h"

uint32_t line_num = 0;

int main(int argc, char **argv)
  {
    struct ssa_uue_data h;
    char *p = NULL;

    SIGCATCH_INIT

    memset(&h, '\0', sizeof(ssa_uue_data));
    h.type = TYPE_FONT;
    h.filename = "test.bin";
    assert((p = get_fontname_with_flags(&h)) != NULL);
    assert(strcmp(p, "test_0.bin") == 0);
    FREE(p);

    h.filename = "test_0.bin";
    assert((p = get_fontname_with_flags(&h)) != NULL);
    assert(strcmp(p, "test_0_0.bin") == 0);
    FREE(p);

    h.filename = "test.bin";
    h.fontenc = 100;
    assert((p = get_fontname_with_flags(&h)) != NULL);
    assert(strcmp(p, "test_100.bin") == 0);
    FREE(p);

    h.fontflags |= FONT_BOLD;
    assert((p = get_fontname_with_flags(&h)) != NULL);
    assert(strcmp(p, "test_B100.bin") == 0);
    FREE(p);

    h.fontflags |= FONT_ITALIC;
    assert((p = get_fontname_with_flags(&h)) != NULL);
    assert(strcmp(p, "test_BI100.bin") == 0);
    FREE(p);

    h.type |= TYPE_IMAGE;
    assert((p = get_fontname_with_flags(&h)) != NULL);
    assert(strcmp(p, "test.bin") == 0);
    FREE(p);

    return EXIT_SUCCESS;
  }
