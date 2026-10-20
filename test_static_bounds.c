/* Exercise the actual static implementation, not a duplicate guard function.
 * Opcode sizes below are the legacy table's contract, not ISA validation. */
#define main decompile46_cli_main
#include "decompiler.c"
#undef main

static unsigned checks;

static int check(const char *label, const uint8_t *buffer, size_t length,
                 size_t start, size_t end, int expected) {
  ++checks;
  const int actual = disassemble_section(buffer, length, start, end);
  if (actual != expected) {
    fprintf(stderr, "FAIL: %s: expected %d, got %d\n", label, expected, actual);
    return 1;
  }
  return 0;
}

int main(void) {
  const uint8_t data[] = {0xcc, 0, 0x02, 0, 0, 0, 0x3b};
  int failures = 0;
  failures += check("null", NULL, 0, 0, 0, 2);
  failures += check("reversed", data, sizeof(data), 4, 2, 2);
  failures += check("end outside image", data, sizeof(data), 0, 8, 2);
  failures += check("start outside image", data, sizeof(data), 8, 8, 2);
  /* The size_t API has no signed bounds; converted negatives must HOLD. */
  failures += check("negative start converted", data, sizeof(data),
                    (size_t)-1, 2, 2);
  failures += check("negative end converted", data, sizeof(data),
                    0, (size_t)-1, 2);
  failures += check("both negative converted", data, sizeof(data),
                    (size_t)-1, (size_t)-1, 2);
  failures += check("empty image", data, 0, 0, 0, 0);
  failures += check("empty at end", data, sizeof(data), 7, 7, 0);
  failures += check("nonzero start exact four", data, sizeof(data), 2, 6, 0);
  failures += check("section cuts two", data, sizeof(data), 0, 1, 2);
  failures += check("section cuts four", data, sizeof(data), 2, 5, 2);
  failures += check("unsupported final byte", data, sizeof(data), 6, 7, 2);

  /* Put each short image immediately before a PROT_NONE page. A read beyond
   * its declared length faults, even without a sanitizer. /dev/zero keeps the
   * mapping avoids MAP_ANONYMOUS extensions on supported Unix systems. */
  const long page_size = sysconf(_SC_PAGESIZE);
  if (page_size < 4 || (size_t)page_size > SIZE_MAX / 2) {
    fprintf(stderr, "FAIL: invalid page size\n");
    return 1;
  }
  int fd = open("/dev/zero", O_RDWR);
  if (fd < 0) { perror("FAIL: open /dev/zero"); return 1; }
  const size_t page = (size_t)page_size;
  uint8_t *mapping = mmap(NULL, page * 2, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE, fd, 0);
  close(fd);
  if (mapping == MAP_FAILED) { perror("FAIL: mmap guard"); return 1; }
  if (mprotect(mapping + page, page, PROT_NONE) != 0) {
    perror("FAIL: mprotect guard");
    munmap(mapping, page * 2);
    return 1;
  }

  failures += check("inaccessible empty image", mapping + page, 0, 0, 0, 0);
  failures += check("inaccessible invalid bounds", mapping + page, 0, 0, 1, 2);
  failures += check("inaccessible reversed bounds", mapping + page, 0, 1, 0, 2);
  for (unsigned opcode = 0; opcode < 256; ++opcode) {
    for (size_t length = 1; length <= 4; ++length) {
      uint8_t *buffer = mapping + page - length;
      memset(buffer, 0, length);
      buffer[0] = (uint8_t)opcode;
      const unsigned size = opcodes[opcode].size;
      /* For a 2-byte entry and length 4, the second pair is 00 00 (size 2).
       * Odd remaining lengths and unsupported entries must HOLD. */
      const int expected = ((size == 2 && length % 2 == 0) ||
                            (size == 4 && length == 4)) ? 0 : 2;
      char label[64];
      snprintf(label, sizeof(label), "opcode %02x, length %zu", opcode, length);
      failures += check(label, buffer, length, 0, length, expected);
    }
  }
  if (munmap(mapping, page * 2) != 0) { perror("FAIL: munmap"); return 1; }
  if (failures) { return 1; }
  printf("PASS: %u direct section checks\n", checks);
  return 0;
}
