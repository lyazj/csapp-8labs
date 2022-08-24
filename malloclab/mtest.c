#include "memlib.h"
#include "mm.h"

static size_t mitem[] = {1000, 1000, 1000};
#define MSIZE (sizeof mitem / sizeof mitem[0])
static void *maddr[MSIZE];
static size_t fitem[MSIZE] = {0, 2, 1};

void segr_list_print(void);

int main(void)
{
  mem_init();
  mm_init();

  for(size_t i = 0; i < MSIZE; ++i)
  {
    printf("Invoking: malloc(%zu)\n", mitem[i]);
    maddr[i] = mm_malloc(mitem[i]);
    printf("Returned: %p\n", maddr[i]);
    segr_list_print();
  }

  for(size_t i = 0; i < MSIZE; ++i)
  {
    printf("Invoking: free(%p)\n", maddr[fitem[i]]);
    mm_free(maddr[fitem[i]]);
    segr_list_print();
  }
}
