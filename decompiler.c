#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define BOOTLOADER_START  0x00000
#define BOOTLOADER_END    0x10000 /* exclusive: do not omit file byte 0xFFFF */
#define PROGRAM_1_START   0x10000
#define PROGRAM_1_END     0x3FFFF
#define UNUSED_START      0x40000
#define UNUSED_END        0x47FFF
#define CALIBRATION_START 0x48000
#define CALIBRATION_END   0x4FFFF
#define PROGRAM_2_START   0x50000
#define PROGRAM_2_END     0x7FFFF

struct op {
  char mnemo[16];
  uint8_t size;
};

struct op opcodes[] = {
  {.mnemo="add", .size=2},
  {.mnemo="addb", .size=2},
  {.mnemo="add", .size=4},
  {.mnemo="addb", .size=4},
  {.mnemo="add", .size=4},
  {.mnemo="addb", .size=4},
  {.mnemo="and", .size=4},
  {.mnemo="addb", .size=4},
  {.mnemo="add1)", .size=2},
  {.mnemo="addb1)", .size=2},
  {.mnemo="bfldl", .size=4},
  {.mnemo="mul", .size=2},
  {.mnemo="rol", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="addc", .size=2},
  {.mnemo="addcb", .size=2},
  {.mnemo="addc", .size=4},
  {.mnemo="addcb", .size=4},
  {.mnemo="addc", .size=4},
  {.mnemo="addcb", .size=4},
  {.mnemo="addc", .size=4},
  {.mnemo="addcb", .size=4},
  {.mnemo="addc1)", .size=2},
  {.mnemo="addcb1)", .size=2},
  {.mnemo="bfldh", .size=4},
  {.mnemo="mulu", .size=2},
  {.mnemo="rol", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},

  {.mnemo="sub", .size=2}, //0x2
  {.mnemo="subb", .size=2},
  {.mnemo="sub", .size=4},
  {.mnemo="subb", .size=4},
  {.mnemo="sub", .size=4},
  {.mnemo="subb", .size=4},
  {.mnemo="sub", .size=4},
  {.mnemo="subb", .size=4},
  {.mnemo="sub1)", .size=2},
  {.mnemo="subb1)", .size=2},
  {.mnemo="bcmp", .size=4},
  {.mnemo="prior", .size=2},
  {.mnemo="ror", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="subc", .size=2}, //0x3
  {.mnemo="subcb", .size=2},
  {.mnemo="subc", .size=4},
  {.mnemo="subcb", .size=4},
  {.mnemo="subc", .size=4},
  {.mnemo="subcb", .size=4},
  {.mnemo="subc", .size=4},
  {.mnemo="subcb", .size=4},
  {.mnemo="subc1)", .size=2},
  {.mnemo="cubcb1)", .size=2},
  {.mnemo="bmovn", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="ror", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},


  {.mnemo="cmp", .size=2}, //0x4
  {.mnemo="cmpb", .size=2},
  {.mnemo="cmp", .size=4},
  {.mnemo="cmpb", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="-", .size=0},
  {.mnemo="cmp", .size=4},
  {.mnemo="cmpb", .size=4},
  {.mnemo="cmp1)", .size=2},
  {.mnemo="cmpb1)", .size=2},
  {.mnemo="bmov", .size=4},
  {.mnemo="div", .size=2},
  {.mnemo="shl", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="xor", .size=2}, //0x5
  {.mnemo="xorb", .size=2},
  {.mnemo="xor", .size=4},
  {.mnemo="xorb", .size=4},
  {.mnemo="xor", .size=4},
  {.mnemo="xorb", .size=4},
  {.mnemo="xor", .size=4},
  {.mnemo="xorb", .size=4},
  {.mnemo="xor1)", .size=2},
  {.mnemo="xorb1)", .size=2},
  {.mnemo="bor", .size=4},
  {.mnemo="divu", .size=2},
  {.mnemo="shl", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},

  {.mnemo="and", .size=2}, //0x6
  {.mnemo="andb", .size=2},
  {.mnemo="and", .size=4},
  {.mnemo="andb", .size=4},
  {.mnemo="and", .size=4},
  {.mnemo="andb", .size=4},
  {.mnemo="and", .size=4},
  {.mnemo="andb", .size=4},
  {.mnemo="and1)", .size=2},
  {.mnemo="andb1)", .size=2},
  {.mnemo="band", .size=4},
  {.mnemo="divl", .size=2},
  {.mnemo="shr", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="or", .size=2}, //0x7
  {.mnemo="orb", .size=2},
  {.mnemo="or", .size=4},
  {.mnemo="orb", .size=4},
  {.mnemo="or", .size=4},
  {.mnemo="orb", .size=4},
  {.mnemo="or", .size=4},
  {.mnemo="orb", .size=4},
  {.mnemo="or1)", .size=2},
  {.mnemo="orb1)", .size=2},
  {.mnemo="bxor", .size=4},
  {.mnemo="divlu", .size=2},
  {.mnemo="shr", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},


  {.mnemo="cmpi1", .size=2}, //0x8
  {.mnemo="neg", .size=2},
  {.mnemo="cmpi1", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="cmpi1", .size=4},
  {.mnemo="idle", .size=4},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="jb", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="-", .size=0},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="cmpi2", .size=2}, //0x9
  {.mnemo="cpl", .size=2},
  {.mnemo="cmpi2", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="cmpi2", .size=4},
  {.mnemo="pwrdn", .size=4},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="jnb", .size=4},
  {.mnemo="trap", .size=2},
  {.mnemo="jmpi", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},

  {.mnemo="cmpd1", .size=2}, //0xA
  {.mnemo="negb", .size=2},
  {.mnemo="cmpd1", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="movb", .size=4},
  {.mnemo="diswdt", .size=4},
  {.mnemo="cmpd1", .size=4},
  {.mnemo="srvwdt", .size=4},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="jbc", .size=4},
  {.mnemo="calli", .size=2},
  {.mnemo="ashr", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="cmpd2", .size=2}, //0xB
  {.mnemo="cplb", .size=2},
  {.mnemo="cmpd2", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="movb", .size=4},
  {.mnemo="einit", .size=4},
  {.mnemo="cmpd2", .size=4},
  {.mnemo="srst", .size=4},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="jnbs", .size=4},
  {.mnemo="callr", .size=2},
  {.mnemo="ashr", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},


  {.mnemo="movbz", .size=2}, //0xC
  {.mnemo="-", .size=0},
  {.mnemo="movbz", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=4},
  {.mnemo="movbz", .size=4},
  {.mnemo="scxt", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="calla", .size=4},
  {.mnemo="ret", .size=2},
  {.mnemo="nop", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="movbs", .size=2}, //0xD
  {.mnemo="atomic2)|extr2)", .size=2},
  {.mnemo="movbs", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=4},
  {.mnemo="movbs", .size=4},
  {.mnemo="scxt", .size=4},
  {.mnemo="extpR2)extsR2)", .size=4},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="calls", .size=4},
  {.mnemo="rets", .size=2},
  {.mnemo="extpR2)extsR2)", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},

  {.mnemo="mov", .size=2}, //0xE
  {.mnemo="movb", .size=2},
  {.mnemo="pcall", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="movb", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=4},
  {.mnemo="movb", .size=4},
  {.mnemo="mov", .size=2},
  {.mnemo="movb", .size=2},
  {.mnemo="jmpa", .size=4},
  {.mnemo="retp", .size=2},
  {.mnemo="push", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
  {.mnemo="mov", .size=2}, //0xF
  {.mnemo="movb", .size=2},
  {.mnemo="mov", .size=4},
  {.mnemo="movb", .size=4},
  {.mnemo="movb", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="mov", .size=4},
  {.mnemo="movb", .size=4},
  {.mnemo="-", .size=0},
  {.mnemo="-", .size=0},
  {.mnemo="jmps", .size=4},
  {.mnemo="reti", .size=2},
  {.mnemo="pop", .size=2},
  {.mnemo="jmpr", .size=2},
  {.mnemo="bclr", .size=2},
  {.mnemo="bset", .size=2},
};

void quit(const char * description) {
  printf("Aborting %s: %s\n", description, strerror(errno));
  exit(1);
}

_Static_assert(sizeof(opcodes) / sizeof(opcodes[0]) == 256,
               "C166 opcode-byte table must have all 256 entries");

/* This is bounded instruction-size stepping, not validated semantic
 * decompilation, control-flow reconstruction or a target-exact C167 decoder.
 * Unsupported opcodes and instructions straddling the requested section HOLD
 * rather than guessing a length or looking beyond the mapped image. */
static int disassemble_section(const uint8_t *buffer, size_t length,
                               size_t start, size_t end) {
  if (buffer == NULL || start > end || end > length) {
    fprintf(stderr, "HOLD: disassembly bounds outside input image\n");
    return 2;
  }
  for (size_t index = start; index < end;) {
    uint8_t opcode = buffer[index];
    const struct op current = opcodes[opcode];
    if (current.size != 2 && current.size != 4) {
      fprintf(stderr,
              "HOLD: unknown/unsupported instruction at 0x%05zx, opcode 0x%02x\n",
              index, opcode);
      return 2;
    }
    if ((size_t)current.size > end - index ||
        (size_t)current.size > length - index) {
      fprintf(stderr,
              "HOLD: instruction at 0x%05zx crosses section/file boundary\n",
              index);
      return 2;
    }
    printf("[%05zx][", index);
    for (size_t i = 0; i < current.size; ++i) {
      printf("%s%02x", i ? " " : "", buffer[index + i]);
    }
    printf("] %s", current.mnemo);
    for (size_t i = 1; i < current.size; ++i) {
      printf(" %02x", buffer[index + i]);
    }
    putchar('\n');
    index += current.size;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <owned-ms42-image.bin>\n", argv[0]);
    return 2;
  }
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) { perror("HOLD: cannot open input"); return 2; }
  struct stat st;
  if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) ||
      st.st_size < BOOTLOADER_END || st.st_size > 16 * 1024 * 1024) {
    fprintf(stderr, "HOLD: expected regular file with at least first 64KiB and <=16MiB\n");
    close(fd);
    return 2;
  }
  const size_t length = (size_t)st.st_size;
  const uint8_t *buffer = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buffer == MAP_FAILED) {
    perror("HOLD: mmap");
    close(fd);
    return 2;
  }
  printf("Image: %s; bytes=%zu; bounded region=[0x00000,0x10000)\n",
         argv[1], length);
  const int code = disassemble_section(buffer, length,
                                       BOOTLOADER_START, BOOTLOADER_END);
  munmap((void *)buffer, length);
  close(fd);
  return code;
}
