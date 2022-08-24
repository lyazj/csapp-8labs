#pragma GCC optimize 3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORIGINAL    "bomb"
#define CRACKED     "cracked"
#define EXEC_OFFSET 0x400000
#define TARGET      "explode_bomb"
#define BUF_SIZE    (1 << 20)  // 1 MB

int main(void)
{
  FILE *dmp = popen("objdump -d " ORIGINAL " | grep " TARGET, "r");
  char *buf = malloc(BUF_SIZE);
  fread(buf, 1, BUF_SIZE, dmp);
  pclose(dmp);

  char *ptr = strstr(buf, "\n0000000000");
  if(!ptr)
  {
    fprintf(stderr, "ERROR: "
        "cannot determine address of the target: " TARGET "\n");
    return 1;
  }
  unsigned long addr = strtol(ptr + 11, NULL, 16);
  fprintf(stderr, "INFO: "
      "find target " TARGET " at %#lx\n", addr);

  for(char *nptr = strchr(ptr + 1, '\n'); *nptr; ++nptr)
    *ptr++ = *nptr;
  *ptr = 0;

  unsigned long new_addr;
  dmp = popen(
      "gdb --batch --eval-command=\'disas " TARGET "\' " ORIGINAL
      " | tail -3 | head -1", "r");
  fscanf(dmp, "%lx", &new_addr);
  pclose(dmp);
  fprintf(stderr, "INFO: "
      "redirecting target to %#lx\n", new_addr);

  system("cp " ORIGINAL " " CRACKED);
  FILE *elf = fopen(CRACKED, "r+b");
  unsigned long op_addr;
  ptr = buf;
  while(ptr && sscanf(ptr, " %lx", &op_addr) != EOF)
  {
    ptr = strchr(ptr + 1, '\n');
    long diff = new_addr - (op_addr + 5);
    fseek(elf, op_addr + 1 - EXEC_OFFSET, SEEK_SET);
    fwrite(&diff, 4, 1, elf);
  }
  fclose(elf);

  free(buf);
  return 0;
}
