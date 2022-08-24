#include <stdio.h>
#include <stdlib.h>

#define EXEC_OFFSET  (0x400000)
#define FARM_ELF     "rtarget"
#define FARM_START   (0x199a)  // inclusive
#define FARM_END     (0x1ab2)  // not inclusive
#define FARM_SIZE    (FARM_END - FARM_START)
#define FARM_OFFSET  (FARM_START + EXEC_OFFSET)

const char *regq[] = {
  "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
};

const char *regl[] = {
  "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
};

void mine_instructions(unsigned long addr, int n)
{
  char buf[8192];
  sprintf(buf, "gdb --batch --eval-command=\'x/%di %#lx\' "
      FARM_ELF, n, addr);
  system(buf);
  puts("");
}

void mine_popqs(const unsigned char *buf)
{
  puts("popq:");
  static unsigned char code[] = {
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  };
  for(int i = 0; i < FARM_SIZE; ++i)
    for(int c = 0; c < (int)(sizeof code / sizeof (code[0])); ++c)
      if(buf[i] == code[c])
      {
        unsigned long addr = FARM_OFFSET + i;
        printf("\t%#lx:\t%x      \tpopq\t%%%s\n\n", addr, buf[i], regq[c]);
        fflush(stdout);
        mine_instructions(addr, 5);
        puts("");
      }
}

void mine_movls(const unsigned char *buf)
{
  puts("movl:");
  for(int i = 0; i < FARM_SIZE - 1; ++i)
    if(buf[i] == 0x89)
    {
      int operands = buf[i + 1];
      if(operands >= 0xc0)
      {
        int source = (operands >> 3) & 7;
        int destination = operands & 7;
        unsigned long addr = FARM_OFFSET + i;
        printf("\t%#lx:\t%x %x   \tmovl\t%%%s, %%%s\n\n", addr,
            buf[i], buf[i + 1], regl[source], regl[destination]);
        fflush(stdout);
        mine_instructions(addr, 5);
        puts("");
      }
    }
}

void mine_movqs(const unsigned char *buf)
{
  puts("movq:");
  for(int i = 1; i < FARM_SIZE - 1; ++i)
    if(buf[i - 1] == 0x48 && buf[i] == 0x89)
    {
      int operands = buf[i + 1];
      if(operands >= 0xc0)
      {
        int source = (operands >> 3) & 7;
        int destination = operands & 7;
        unsigned long addr = FARM_OFFSET + i - 1;
        printf("\t%#lx:\t%x %x %x\tmovq\t%%%s, %%%s\n\n",
            addr, buf[i - 1], buf[i], buf[i + 1],
            regq[source], regq[destination]);
        fflush(stdout);
        mine_instructions(addr, 5);
        puts("");
      }
    }
}

int main(void)
{
  unsigned char *buf = malloc(FARM_SIZE);
  FILE *file = fopen(FARM_ELF, "rb");
  fseek(file, FARM_START, SEEK_SET);
  fread(buf, 1, FARM_SIZE, file);

  mine_movqs(buf);
  mine_popqs(buf);
  mine_movls(buf);

  free(buf);
  return 0;
}
