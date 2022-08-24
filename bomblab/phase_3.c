#include <stdio.h>
#include <stdlib.h>

void explode_bomb(void);

// 0000000000400f43 <phase_3>:
void phase_3(const char *str)
{
  static void *jump_table[8] = {
    &&L_400f7c,
    &&L_400fb9,
    &&L_400f83,
    &&L_400f8a,
    &&L_400f91,
    &&L_400f98,
    &&L_400f9f,
    &&L_400fa6,
  };

  //   400f43:	48 83 ec 18          	sub    $0x18,%rsp

  int a;  // *(int *)(rsp + 8)
  int b;  // *(int *)(rsp + 12)
  //   400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  //   400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  //   400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  //   400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  //   400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  //   400f60:	83 f8 01             	cmp    $0x1,%eax
  //   400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  //   400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
  if(sscanf(str, "%d %d", &a, &b) <= 1)
    explode_bomb();

  //   400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)
  //   400f6f:	77 3c                	ja     400fad <phase_3+0x6a>
  if((unsigned)a > 7)
    goto L_400fad;

  //   400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax
  //   400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)
  int eax;
  goto *jump_table[a];

L_400f7c:  // case 0
  //   400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  //   400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  eax = 0xcf;
  goto L_400fbe;  // break

L_400f83:  // case 2
  //   400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  //   400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  eax = 0x2c3;
  goto L_400fbe;  // break

L_400f8a:  // case 3
  //   400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  //   400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  eax = 0x100;
  goto L_400fbe;  // break

L_400f91:  // case 4
  //   400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  //   400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  eax = 0x185;
  goto L_400fbe;  // break

L_400f98:  // case 5
  //   400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  //   400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  eax = 0xce;
  goto L_400fbe;  // break

L_400f9f:  // case 6
  //   400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  //   400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  eax = 0x2aa;
  goto L_400fbe;  // break

L_400fa6:  // case 7
  //   400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  //   400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  eax = 0x147;
  goto L_400fbe;  // break

L_400fad:  // default
  //   400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  //   400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  //   400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  explode_bomb();
  eax = 0;
  goto L_400fbe;  // break

L_400fb9:  // case 1
  //   400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  eax = 0x137;
  goto L_400fbe;  // break

L_400fbe:
  //   400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  //   400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  //   400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  if(eax != b)
    explode_bomb();

  //   400fc9:	48 83 c4 18          	add    $0x18,%rsp
  //   400fcd:	c3                   	retq   
}

int abps[8][2] = {
  { 0, 0xcf },
  { 1, 0x137 },
  { 2, 0x2c3 },
  { 3, 0x100 },
  { 4, 0x185 },
  { 5, 0xce },
  { 6, 0x2aa },
  { 7, 0x147 },
};  // a-b pairs

void explode_bomb(void)
{
  puts("\nBOOM!!!");
  puts("The bomb has blown up.");
  exit(8);
}

int main(void)
{
  char buf[32];
  for(int i = 0; i < 8; ++i)
  {
    int a = abps[i][0], b = abps[i][1];
    sprintf(buf, "%d %d", a, b);
    phase_3(buf);
    puts(buf);
  }
  return 0;
}
