#include <stdio.h>
#include <stdlib.h>

void explode_bomb(void);

void read_six_numbers(const char *str, int seq[6])
{
  if(sscanf(str, "%d %d %d %d %d %d",
        seq, seq + 1, seq + 2, seq + 3, seq + 4, seq + 5) < 6)
    explode_bomb();
}

struct Node {
  int val;
  int idx;
  struct Node *next;
} nodes[6] = {
  { 0x0000014c, 1, &nodes[1] },
  { 0x000000a8, 2, &nodes[2] },
  { 0x0000039c, 3, &nodes[3] },
  { 0x000002b3, 4, &nodes[4] },
  { 0x000001dd, 5, &nodes[5] },
  { 0x000001bb, 6, NULL },
};

void phase_6(const char *str)  // str in %rdi
{
  // phase_6:
  //   4010f4:	41 56                	push   %r14
  //   4010f6:	41 55                	push   %r13
  //   4010f8:	41 54                	push   %r12
  //   4010fa:	55                   	push   %rbp
  //   4010fb:	53                   	push   %rbx
  //   4010fc:	48 83 ec 50          	sub    $0x50,%rsp

  //   401100:	49 89 e5             	mov    %rsp,%r13
  //   401103:	48 89 e6             	mov    %rsp,%rsi
  // long rsi = rsp;
  //   401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>
  struct Node *pnodes[6];
  int seq[6];
  long rsp = (long)&seq[0];
  long r13 = rsp;
  read_six_numbers(str, seq);

  //   40110b:	49 89 e6             	mov    %rsp,%r14
  //   40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d
  // long r14 = rsp;
  int r12d = 0;

  for(;;)  // 要求seq中的6个数为1至6的一个排列
  {
    //   401114:	4c 89 ed             	mov    %r13,%rbp
    long rbp = r13;

    //   401117:	41 8b 45 00          	mov    0x0(%r13),%eax
    //   40111b:	83 e8 01             	sub    $0x1,%eax
    //   40111e:	83 f8 05             	cmp    $0x5,%eax
    //   401121:	76 05                	jbe    401128 <phase_6+0x34>
    //   401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
    if(*(unsigned int *)r13 - 1U > 5U)  // 要求*(unsigned int *)r13在1至6之间
      explode_bomb();

    //   401128:	41 83 c4 01          	add    $0x1,%r12d
    ++r12d;

    //   40112c:	41 83 fc 06          	cmp    $0x6,%r12d
    //   401130:	74 21                	je     401153 <phase_6+0x5f>
    if(r12d == 6)  // 其后代码r12d遍历1至5
      break;

    //   401132:	44 89 e3             	mov    %r12d,%ebx
    for(int ebx = r12d; ebx <= 5; ++ebx)  // 要求seq[r12d]至seq[5]均不与seq[r12d - 1]相等
    {
      //   401135:	48 63 c3             	movslq %ebx,%rax
      //   401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
      //   40113b:	39 45 00             	cmp    %eax,0x0(%rbp)
      //   40113e:	75 05                	jne    401145 <phase_6+0x51>
      //   401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
      if(seq[ebx] == *(int *)rbp)
        explode_bomb();

      //   401145:	83 c3 01             	add    $0x1,%ebx
      //   401148:	83 fb 05             	cmp    $0x5,%ebx
      //   40114b:	7e e8                	jle    401135 <phase_6+0x41>
    }

    //   40114d:	49 83 c5 04          	add    $0x4,%r13
    //   401151:	eb c1                	jmp    401114 <phase_6+0x20>
    r13 += 4;
  }

  //   401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
  //   401158:	4c 89 f0             	mov    %r14,%rax
  //   40115b:	b9 07 00 00 00       	mov    $0x7,%ecx
  long _rsi = (long)&seq[6];
  for(long rax = (long)&seq[0]; rax != _rsi; rax += 4)  // 用7减数组seq每个元素
  {
    //   401160:	89 ca                	mov    %ecx,%edx
    //   401162:	2b 10                	sub    (%rax),%edx
    //   401164:	89 10                	mov    %edx,(%rax)
    *(int *)rax = 7 - *(int *)rax;

    //   401166:	48 83 c0 04          	add    $0x4,%rax
    //   40116a:	48 39 f0             	cmp    %rsi,%rax
    //   40116d:	75 f1                	jne    401160 <phase_6+0x6c>
  }

  //   40116f:	be 00 00 00 00       	mov    $0x0,%esi
  //   401174:	eb 21                	jmp    401197 <phase_6+0xa3>
  for(long rsi = 0; rsi != 24; rsi += 4)  // 将pnodes[i]指向第seq[i]个node
  {
    //   401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx
    //   40119a:	83 f9 01             	cmp    $0x1,%ecx
    //   40119d:	7e e4                	jle    401183 <phase_6+0x8f>
    // if(seq[rsi / 4] <= 1)  // seq[rsi / 4] == 1
    // {
    //   //   401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx
    //   rdx = (long)&nodes[0];
    // }

    //   40119f:	b8 01 00 00 00       	mov    $0x1,%eax
    //   4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx
    //   4011a9:	eb cb                	jmp    401176 <phase_6+0x82>
    struct Node *rdx = &nodes[0];
    for(int eax = 1; eax < seq[rsi / 4]; ++eax)
    {
      //   401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
      //   40117a:	83 c0 01             	add    $0x1,%eax
      //   40117d:	39 c8                	cmp    %ecx,%eax
      //   40117f:	75 f5                	jne    401176 <phase_6+0x82>
      //   401181:	eb 05                	jmp    401188 <phase_6+0x94>
      rdx = rdx->next;
    }

    //   401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)
    //   40118d:	48 83 c6 04          	add    $0x4,%rsi
    //   401191:	48 83 fe 18          	cmp    $0x18,%rsi
    //   401195:	74 14                	je     4011ab <phase_6+0xb7>
    pnodes[rsi / 4] = rdx;
  }

  //   4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx
  //   4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax
  //   4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi
  //   4011ba:	48 89 d9             	mov    %rbx,%rcx
  struct Node *rbx = pnodes[0];  // 链表头
  struct Node **rax = &pnodes[1];  // 指向重排后的第二个链表指针
  struct Node **rsi = &pnodes[6];  // 链表指针数组的尾后位置
  struct Node *rcx = pnodes[0];  // 链表头
  struct Node *rdx;
  for(;;)  // 顺序更新链表节点的下一个节点分别为1至5和NULL
  {
    //   4011bd:	48 8b 10             	mov    (%rax),%rdx
    //   4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)
    //   4011c4:	48 83 c0 08          	add    $0x8,%rax
    rcx->next = rdx = *rax;  // 更新链表节点*rcx的下一个节点
    ++rax;  // 让rax指向下一个链表指针

    //   4011c8:	48 39 f0             	cmp    %rsi,%rax
    //   4011cb:	74 05                	je     4011d2 <phase_6+0xde>
    //   4011cd:	48 89 d1             	mov    %rdx,%rcx
    if(rax == rsi)  // 指针数组遍历完成
      break;
    rcx = rdx;  // 让rcx指向下一个节点
    //   4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9>
  }  // 退出循环时rax指向尾后，rcx指向倒数第二个节点，rdx指向最后一个节点
  //   4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx)
  rdx->next = NULL;

  //   4011d9:	00 
  //   4011da:	bd 05 00 00 00       	mov    $0x5,%ebp
  for(int ebp = 5; ebp; --ebp)  // 检查重排后链表的不升序
  {
    //   4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax
    //   4011e3:	8b 00                	mov    (%rax),%eax
    //   4011e5:	39 03                	cmp    %eax,(%rbx)
    //   4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>
    //   4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
    if(rbx->val < rbx->next->val)
      explode_bomb();
    //   4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx
    rbx = rbx->next;

    //   4011f2:	83 ed 01             	sub    $0x1,%ebp
    //   4011f5:	75 e8                	jne    4011df <phase_6+0xeb>
  }

  //   4011f7:	48 83 c4 50          	add    $0x50,%rsp
  //   4011fb:	5b                   	pop    %rbx
  //   4011fc:	5d                   	pop    %rbp
  //   4011fd:	41 5c                	pop    %r12
  //   4011ff:	41 5d                	pop    %r13
  //   401201:	41 5e                	pop    %r14
  //   401203:	c3                   	retq   
}

#ifndef SOLVE_PHASE_6

void explode_bomb(void)
{
  puts("\nBOOM!!!");
  puts("The bomb has blown up.");
  exit(8);
}

int main(void)
{
  char buf[32];
  if(!fgets(buf, sizeof buf, stdin))
    return 1;
  phase_6(buf);
  return 0;
}

#else

#ifndef __cplusplus
#error need a c++ compiler
#endif

#include <stdexcept>

struct WrongAnswer : std::runtime_error {
  WrongAnswer() : std::runtime_error("Wrong Answer") { }
} const WA;

void explode_bomb()
{
  throw WA;
}

#define LOOP(i) for(int i = 1; i <= 6; ++i)

int main()
{
  char buf[256];
  LOOP(a)
  LOOP(b)
  LOOP(c)
  LOOP(d)
  LOOP(e)
  LOOP(f)
  {
    sprintf(buf, "%d %d %d %d %d %d", a, b, c, d, e, f);
    try {
      phase_6(buf);
    }
    catch(const WrongAnswer &) {
      for(int i = 1; i != 6; ++i)
        nodes[i - 1].next = &nodes[i];
      nodes[5].next = NULL;
      continue;
    }
    puts(buf);
  }
}

#endif  /* SOLVE_PHASE_6 */
