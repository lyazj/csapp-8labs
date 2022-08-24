#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void explode_bomb(void);

// 0000000000400fce <func4>:
int func4(int a, int min, int max)  // expected return value: 0
                                    // a >= 0 && a <= 14
                                    // initial: min == 0 && max == 14
{
//   400fce:	48 83 ec 08          	sub    $0x8,%rsp

//   400fd2:	89 d0                	mov    %edx,%eax
//   400fd4:	29 f0                	sub    %esi,%eax
  int eax = max - min;
//   400fd6:	89 c1                	mov    %eax,%ecx
//   400fd8:	c1 e9 1f             	shr    $0x1f,%ecx
  int ecx = (unsigned)eax >> 31;  // 符号位
//   400fdb:	01 c8                	add    %ecx,%eax
//   400fdd:	d1 f8                	sar    %eax
  eax = (eax + ecx) >> 1;  // eax /= 2, 向远离0方向圆整
//   400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx
  ecx = eax + min;  // min, max平均值, 向远离0方向圆整
//   400fe2:	39 f9                	cmp    %edi,%ecx
//   400fe4:	7e 0c                	jle    400ff2 <func4+0x24>
//   400fe6:	8d 51 ff             	lea    -0x1(%rcx),%edx
//   400fe9:	e8 e0 ff ff ff       	callq  400fce <func4>
//   400fee:	01 c0                	add    %eax,%eax
//   400ff0:	eb 15                	jmp    401007 <func4+0x39>
  if(ecx > a)
    return 2 * func4(a, min, ecx - 1);
//   400ff2:	b8 00 00 00 00       	mov    $0x0,%eax
//   400ff7:	39 f9                	cmp    %edi,%ecx
//   400ff9:	7d 0c                	jge    401007 <func4+0x39>
//   400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
//   400ffe:	e8 cb ff ff ff       	callq  400fce <func4>
//   401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax
  else
  {
    if(ecx >= a)  // ecx == a
      return 0;
    return 2 * func4(a, ecx + 1, max) + 1;  // ecx < a
  }

//   401007:	48 83 c4 08          	add    $0x8,%rsp
//   40100b:	c3                   	retq   
}

/**
 * func(a, 0, 14)
 *   mean = 7
 *   a < 7 -> 2 * func(a, 0, 6)
 *   func(a, 0, 6)
 *     mean = 3
 *     a < 3 -> 2 * func(a, 0, 2)
 *     func(a, 0, 2)
 *       mean = 1
 *       a < 1 (a == 0) -> 2 * func(a, 0, 0) -> 0 (OK!)
 *       a == 1 -> 0 (OK!)
 *       a > 1 (a == 2) -> 2 * func(a, 2, 2) + 1 (NO!)
 *     a == 3 -> 0 (OK!)
 *     a > 3 -> 2 * func(a, 4, 6) + 1 (NO!)
 *   a == 7 -> 0 (OK!)
 *   a > 7 -> 2 * func(a, 8, 14) + 1 (NO!)
 *
 * answers:
 *   14 / 2 = 7
 *   (7 - 1) / 2 = 3
 *   (3 - 1) / 2 = 1
 *   (1 - 1) / 2 = 0
 **/

// 000000000040100c <phase_4>:
void phase_4(const char *str)
{
  //   40100c:	48 83 ec 18          	sub    $0x18,%rsp
  int a;  // *(int *)(rsp + 8)
  int b;  // *(int *)(rsp + 12)

  //   401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  //   401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  //   40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi
  //   40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  //   401024:	e8 c7 fb ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  //   401029:	83 f8 02             	cmp    $0x2,%eax
  //   40102c:	75 07                	jne    401035 <phase_4+0x29>
  if(sscanf(str, "%d %d", &a, &b) != 2)
    explode_bomb();

  //   40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)
  //   401033:	76 05                	jbe    40103a <phase_4+0x2e>
  //   401035:	e8 00 04 00 00       	callq  40143a <explode_bomb>
  if((unsigned)a > 14U)
    explode_bomb();

  //   40103a:	ba 0e 00 00 00       	mov    $0xe,%edx
  //   40103f:	be 00 00 00 00       	mov    $0x0,%esi
  //   401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi
  //   401048:	e8 81 ff ff ff       	callq  400fce <func4>
  //   40104d:	85 c0                	test   %eax,%eax
  //   40104f:	75 07                	jne    401058 <phase_4+0x4c>
  if(func4(a, 0, 14))
    explode_bomb();

  //   401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  //   401056:	74 05                	je     40105d <phase_4+0x51>
  //   401058:	e8 dd 03 00 00       	callq  40143a <explode_bomb>
  if(b != 0)
    explode_bomb();

  //   40105d:	48 83 c4 18          	add    $0x18,%rsp
  //   401061:	c3                   	retq   
}

void explode_bomb(void)
{
  puts("\nBOOM!!!");
  puts("The bomb has blown up.");
  exit(8);
}

int main(void)
{
  char buf[32];
  char ans_beg[15][32], (*ans_end)[32] = ans_beg;
  for(int a = 0; a <= 14; ++a)
  {
    int f = func4(a, 0, 14);
    printf("a = %2d, func4(a, 0, 14) = %2d\n", a, f);
    if(!f)
    {
      sprintf(buf, "%d %d", a, 0);
      phase_4(buf);  // if bomb; then exit 8; fi
      strcpy(*ans_end++, buf);
    }
  }
  puts("\nAnswers:");
  for(char (*pans)[32] = ans_beg; pans != ans_end; ++pans)
    puts(*pans);
  return 0;
}
