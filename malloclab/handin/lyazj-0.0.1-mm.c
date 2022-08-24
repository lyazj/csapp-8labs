/*
 * mm.c - lyazj's malloc package.
 */
#define NDEBUG  /* get rid of each assert */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
#include "config.h"

static void app_error(char *msg);

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) < (y) ? (y) : (x))

team_t team = {
    /* Team name */
    "lyazj",
    /* First member's full name */
    "lyazj",
    /* First member's email address */
    "lyazj@github.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    "",
};

/* use word as a unit of size */
#if __SIZEOF_POINTER__ == 4    /* -m32 */
#define WORD_POW (2)
#elif __SIZEOF_POINTER__ == 8  /* -m64 */
#define WORD_POW (3)
#endif
#define S2W(s) ((size_t)(s) >> WORD_POW)
#define W2S(w) ((size_t)(w) << WORD_POW)
#define WORD_SIZE (W2S(1))

/* double word alignment */
#undef ALIGNMENT
#define ALIGNMENT (WORD_SIZE << 1)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

static_assert(!(ALIGNMENT & (WORD_SIZE - 1)));
static_assert(sizeof(size_t) == WORD_SIZE);
static_assert(sizeof(void *) == WORD_SIZE);

/* the f/a bit of the header or footer */
enum block_stat {
  FREE      = 0,
  ALLOCATED = 1,
};

/* the header or footer metadata */
struct block_meta {
  enum block_stat cur_stat : 1;  /* status of the current block */
  enum block_stat pre_stat : 1;  /* status of the previous adjacent block */
  size_t          cur_wrds : ((sizeof(size_t) << 3) - 2);  /* overall words */
};
static_assert(WORD_POW >= 2);
static_assert(sizeof(struct block_meta) == WORD_SIZE);

/* item of the segregated explicit free lists */
struct segr_list_item {
  struct segr_list_item *pred;  /* predecessor block header address */
  struct segr_list_item *succ;  /* successor block header address */
};

/* item of the index into the segregated explicit free lists */
struct segr_list {
  size_t                wrds;   /* max(meta.cur_wrds) + 1 */
  struct segr_list_item *head;  /* block header address */
};

static_assert(sizeof(struct segr_list_item) == WORD_SIZE << 1);
static_assert(sizeof(struct segr_list) == WORD_SIZE << 1);

/*
 * IMPORTANT: require offset(succ) == offset(head)
 */

/* zero payloads are not accepted */
#define PAYLOAD_MIN (1)

/*
 * minimun size of a block
 * FREE:      2 * sizeof(size_t) + sizeof(struct segr_list_item)
 * ALLOCATED: 1 * sizeof(size_t) + PAYLOAD_MIN
 */
#define BLOCK_MIN (ALIGN(2 * sizeof(size_t) + sizeof(struct segr_list_item)))
#define BLOCK_MIN_POW (WORD_POW + 2)
static_assert(BLOCK_MIN == 1 << BLOCK_MIN_POW);

/*
 * the number of the segregated explicit free lists
 */
#define SEGR_MIN_POW (BLOCK_MIN_POW + 1 - WORD_POW)
#define SEGR_MIN (1 << SEGR_MIN_POW)
#define SEGR_MAX_POW (25 - WORD_POW)
#define SEGR_MAX (1 << SEGR_MAX_POW)
#define SEGR_NUM (SEGR_MAX_POW - SEGR_MIN_POW + 1)
static_assert(MAX_HEAP < W2S(SEGR_MAX));

/* minimum extension of the heap */
#define CHUNK_SIZE (1 << 12)  /* 4 KB as page size on Linux */

/* address of the segregated list array */
static struct segr_list *segr;

static struct block_meta *segr_list_hdr(struct segr_list_item *item);
static struct block_meta *segr_list_ftr(struct segr_list_item *item);
static struct segr_list_item *segr_list_prev(struct segr_list_item *item);
static struct segr_list_item *segr_list_next(struct segr_list_item *item);

static void segr_list_add(struct segr_list_item *item);
static void segr_list_remove(struct segr_list_item *item);

static struct segr_list_item *place(struct segr_list_item *item, size_t wrds);
static struct segr_list_item *extend(size_t wrds);
static struct segr_list_item *coalesce(struct segr_list_item *item);

static void init_segrs(void);
static void init_blocks(void);
static struct segr_list *first_fit_list(size_t wrds);
static struct segr_list_item *first_fit(size_t wrds);

void segr_list_print(void);

/* initialize the malloc package */
int mm_init(void)
{
  size_t list_size, block_size;

  list_size = SEGR_NUM * sizeof(struct segr_list);
  block_size = 2 * sizeof(struct block_meta);

  segr = mem_sbrk(list_size + block_size);
  if(!segr)  /* allocation failed */
    return -1;

  init_segrs();
  init_blocks();

  assert(first_fit_list(0) == segr);
  assert(first_fit_list(7) == segr);
  assert(first_fit_list(8) == segr + 1);
  assert(first_fit_list(15) == segr + 1);
  assert(first_fit_list(16) == segr + 2);
  assert(first_fit_list(SEGR_MAX - 1) == segr + SEGR_NUM - 1);

  return 0;
}

void *mm_malloc(size_t size)
{
  size_t wrds;
  struct segr_list_item *item;

  if(!size)
    return NULL;
  wrds = S2W(ALIGN(size + sizeof(struct block_meta)));
  item = first_fit(wrds);
  if(item)
  {
    segr_list_remove(item);
    return place(item, wrds);
  }
  return extend(wrds);
}

void mm_free(void *ptr)
{
  struct segr_list_item *item = (struct segr_list_item *)ptr;
  struct block_meta *header = segr_list_hdr(item);

  assert(W2S(header->cur_wrds) >= BLOCK_MIN);
  segr_list_add(coalesce(item));
}

void *mm_realloc(void *ptr, size_t size)
{
  struct segr_list_item *item = (struct segr_list_item *)ptr;
  struct block_meta *header = segr_list_hdr(item);
  size_t wrds = S2W(ALIGN(size + sizeof(struct block_meta)));
  void *new_ptr;

  if(!ptr)
    return mm_malloc(size);
  if(!size)
  {
    mm_free(ptr);
    return NULL;
  }

  assert(header->cur_stat == ALLOCATED);

  if(wrds <= header->cur_wrds)
  {
    // size_t new_wrds;
    // struct segr_list_item *new_item;
    // struct block_meta *new_header;

    // new_wrds = header->cur_wrds - wrds;
    // if(S2W(new_wrds) >= BLOCK_MIN)
    // {
    //   header->cur_wrds = wrds;
    //   new_item = segr_list_next(item);
    //   new_header = segr_list_hdr(new_item);
    //   new_header->cur_stat = ALLOCATED;
    //   new_header->pre_stat = ALLOCATED;
    //   new_header->cur_wrds = new_wrds;
    //   segr_list_add(coalesce(new_item));
    // }
    return ptr;
  }
  else
  {
    struct segr_list_item *next_item = segr_list_next(item);
    struct block_meta *next_header = segr_list_hdr(next_item);

    if(next_header->cur_wrds == 1)
    {
      struct segr_list_item *new_item = extend(wrds - header->cur_wrds);
      
      if(!new_item)
        return NULL;
      assert(new_item == next_item);
      header->cur_wrds += next_header->cur_wrds;
      return ptr;
    }

    if(next_header->cur_stat == FREE)
      if(next_header->cur_wrds >= wrds - header->cur_wrds)
      {
        segr_list_remove(next_item);
        header->cur_wrds += next_header->cur_wrds;
        next_item = segr_list_next(next_item);
        next_header = segr_list_hdr(next_item);
        next_header->pre_stat = ALLOCATED;
        if(next_header->cur_stat == FREE)
          *segr_list_ftr(next_item) = *next_header;
        return ptr;
      }
  }

  new_ptr = mm_malloc(size);
  if(!new_ptr)
    return NULL;
  memcpy(new_ptr, ptr, MIN(size, W2S(header->cur_wrds)));
  mm_free(ptr);
  return new_ptr;
}

/* report an arbitrary application error and exit */
void app_error(char *msg) 
{
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(EXIT_FAILURE);
}

/* initialize the list index */
static void init_segrs(void)
{
  size_t wrds = SEGR_MIN;

  for(int i = 0; i < SEGR_NUM; ++i)
  {
    segr[i].wrds = wrds;
    segr[i].head = NULL;
    wrds <<= 1;
  }
  assert(wrds >> 1 == SEGR_MAX);
}

/* initialize the prologue and epilogue blocks */
static void init_blocks(void)
{
  struct block_meta *meta = (struct block_meta *)&segr[SEGR_NUM];

  /* prologue header */
  meta[0].cur_stat = ALLOCATED;  /* always */
  meta[0].pre_stat = ALLOCATED;  /* never used */
  meta[0].cur_wrds = 1;          /* characteristic */

  /* epilogue header */
  meta[1].cur_stat = ALLOCATED;  /* always */
  meta[1].pre_stat = ALLOCATED;
  meta[1].cur_wrds = 1;          /* characteristic */
}

/* first possible segregated list for aligned block size */
static struct segr_list *first_fit_list(size_t wrds)
{
  int l = -1, u = SEGR_NUM - 1;

  static_assert(SEGR_NUM > 0, "expect SEGR_NUM > 0");
  if(wrds >= SEGR_MAX)
    app_error("first_fit_list: block words too large");

  while(l + 1 != u)
  {
    int m = (l + u) >> 1;
    // if(segr[m].wrds > wrds)
    if((size_t)1 << (m + SEGR_MIN_POW) > wrds)
      u = m;
    else
      l = m;
  }
  return &segr[u];
}

/* first fit free block for aligned block words */
static struct segr_list_item *first_fit(size_t wrds)
{
  struct segr_list *list, *list_end;

  list = first_fit_list(wrds);
  list_end = &segr[SEGR_NUM];
  while(list != list_end)
  {
    struct segr_list_item *item = list->head;

    while(item)
    {
      struct block_meta *header = segr_list_hdr(item);
      assert(header->cur_stat == FREE);
      if(header->cur_wrds >= wrds)
        return item;
      item = item->succ;
    }
    ++list;
  }

  return NULL;
}

static struct block_meta *segr_list_hdr(struct segr_list_item *item)
{
  return (struct block_meta *)item - 1;
}

static struct block_meta *segr_list_ftr(struct segr_list_item *item)
{
  struct block_meta *header = segr_list_hdr(item);

  return &header[header->cur_wrds - 1];
}

static struct segr_list_item *segr_list_prev(struct segr_list_item *item)
{
  struct block_meta *header = segr_list_hdr(item), *prev_footer;

  if(header->pre_stat != FREE)
    return NULL;
  prev_footer = &header[-1];
  assert(prev_footer->cur_stat == FREE);
  return (struct segr_list_item *)(
      (struct block_meta *)item - prev_footer->cur_wrds);
}

static struct segr_list_item *segr_list_next(struct segr_list_item *item)
{
  return (struct segr_list_item *)(
      (struct block_meta *)item + segr_list_hdr(item)->cur_wrds);
}

static void segr_list_add(struct segr_list_item *item)
{
  struct block_meta *header = segr_list_hdr(item);
  struct segr_list *list = first_fit_list(header->cur_wrds);
  struct segr_list_item *head = list->head;

  item->pred = (struct segr_list_item *)list;
  item->succ = head;
  list->head = item;
  if(head)
    head->pred = item;
}

static void segr_list_remove(struct segr_list_item *item)
{
  struct segr_list_item *pred = item->pred;
  struct segr_list_item *succ = item->succ;

  assert(pred);
  pred->succ = succ;
  if(succ)
    succ->pred = pred;
  item->pred = item->succ = NULL;  /* optimizable */
}

static struct segr_list_item *place(struct segr_list_item *item, size_t wrds)
{
  struct block_meta *header = segr_list_hdr(item);
  size_t tail_wrds = header->cur_wrds - wrds;

  assert(header->cur_stat == FREE);
  assert(header->cur_wrds >= wrds);

  if(S2W(tail_wrds) < BLOCK_MIN)
  {
    struct segr_list_item *next_item;
    struct block_meta *next_header;

    next_item = segr_list_next(item);
    next_header = segr_list_hdr(next_item);
    next_header->pre_stat = ALLOCATED;
    if(next_header->cur_stat == FREE)
      *segr_list_ftr(next_item) = *next_header;
  }
  else  /* split */
  {
    struct segr_list_item *tail_item;
    struct block_meta *tail_header;

    header->cur_wrds = wrds;
    tail_item = segr_list_next(item);
    tail_header = segr_list_hdr(tail_item);
    tail_header->cur_stat = FREE;
    tail_header->cur_wrds = tail_wrds;
    tail_header->pre_stat = ALLOCATED;
    *segr_list_ftr(tail_item) = *tail_header;
    segr_list_add(tail_item);
  }

  header->cur_stat = ALLOCATED;
  return item;
}

static struct segr_list_item *extend(size_t wrds)
{
  size_t inc_wrds = MAX(wrds, S2W(CHUNK_SIZE));
  struct segr_list_item *item =
    (struct segr_list_item *)mem_sbrk(inc_wrds * WORD_SIZE);
  struct block_meta *header = segr_list_hdr(item);
  struct block_meta *epi_header = &header[inc_wrds];

  if(!item)
    return NULL;
  epi_header->cur_stat = ALLOCATED;
  epi_header->pre_stat = FREE;
  epi_header->cur_wrds = 1;
  header->cur_stat = FREE;
  header->cur_wrds = inc_wrds;
  // item = coalesce(item);
  return place(item, wrds);
}

void segr_list_print(void)
{
  for(int i = 0; i < SEGR_NUM; ++i)
  {
    struct segr_list_item *item = segr[i].head;

    if(!item)
      continue;
    printf("SEGR %zu:\n", segr[i].wrds);
    while(item)
    {
      struct block_meta *header = segr_list_hdr(item);

      if(header->cur_stat == FREE)
      {
        struct block_meta *footer = segr_list_ftr(item);
        assert(*(size_t *)footer == *(size_t *)header);
      }
      printf("    %p: c(f/a): %d  p(f/a): %d  c(wrds): %zu\n",
          item, header->cur_stat, header->pre_stat, header->cur_wrds);

      item = item->succ;
    }
    printf("\n");
  }
}

static struct segr_list_item *coalesce(struct segr_list_item *item)
{
  struct block_meta *header = segr_list_hdr(item);
  struct block_meta *footer = segr_list_ftr(item);
  struct segr_list_item *prev_item = segr_list_prev(item);
  struct segr_list_item *next_item = segr_list_next(item);
  struct block_meta *next_header = segr_list_hdr(next_item);

  assert(header->cur_stat == next_header->pre_stat);

  if(prev_item)
  {
    struct block_meta *prev_header = segr_list_hdr(prev_item);

    assert(prev_header->cur_stat == FREE);
    assert(prev_header->pre_stat == ALLOCATED);
    segr_list_remove(prev_item);
    prev_header->cur_wrds += header->cur_wrds;
    header = prev_header;
    item = prev_item;
  }
  else
     header->cur_stat = FREE;

  if(next_header->cur_stat == FREE)
  {
    segr_list_remove(next_item);
    header->cur_wrds += next_header->cur_wrds;
    footer = segr_list_ftr(next_item);
  }
  else
    next_header->pre_stat = FREE;

  *footer = *header;
  return item;
}
