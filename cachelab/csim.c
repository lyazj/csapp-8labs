#include "cachelab.h"

#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* 0b00000000000000000000000000000000
 *   ttttttttttttttttssssssssbbbbbbbb */

char *argv0;
char *trace_file;
int t;
int s;
int S;
int b;
int E;
int verbose;
uint64_t tag_mask;
uint64_t index_mask;
uint64_t offset_mask;
int miss;
int hit;
int eviction;

struct cache_line {
  /* valid: 1 */
  uint64_t tag;
  /* block: <optimized> */
  struct cache_line *prev;
  struct cache_line *next;
};

struct cache_set {
  struct cache_line *head;
  struct cache_line *tail;
  int size;
} *cache;

/* utility functions */
void print_help(FILE *stream);
void init_masks(void);
void init_cache(void);
void free_cache(void);
void print_cache(FILE *stream);

/* parse the trace and simulate the cached memory access */
void simulate_trace(void);
int simulate_trace_line(FILE *trace);

/* simulate R/W operation */
void cache_readwrite(uint64_t addr);
void cache_set_readwrite(struct cache_set *set, uint64_t tag);

/* on success, pop the line; on failure, return NULL */
struct cache_line *cache_set_search(struct cache_set *set, uint64_t tag);

/* cache by adding tag to the victim queue tail */
void cache_set_cache(struct cache_set *set, struct cache_line *line);

/* check if the victim queue full */
int cache_set_full(const struct cache_set *set);

/* victim queue list element access */
void do_cache_set_insert(struct cache_line *prev, struct cache_line *line);
struct cache_line *do_cache_set_erase(struct cache_line *prev);

int main(int argc, char *argv[])
{
  int opt;
  const char *optstr = ":hvs:E:b:t:";

  argv0 = argv[0];

  while((opt = getopt(argc, argv, optstr)) != -1)
    switch(opt)
  {

  case '?':  /* error: unexpected option character */
    fprintf(stderr,
        "%s: unexpected option character: %c\n", argv0, optopt);
    exit(EXIT_FAILURE);

  case ':':  /* error: missing option argument */
    fprintf(stderr,
        "%s: missing argument for option: \'-%c\'\n", argv0, optopt);
    exit(EXIT_FAILURE);

  case 'h':  /* help */
    print_help(stdout);
    exit(EXIT_SUCCESS);

  case 'v':  /* verbose */
    verbose += 1;
    break;

  case 's':  /* the number of set index bits */
    if(s > 0)
    {
      fprintf(stderr,
          "%s: duplicate option: \'-s\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    if(sscanf(optarg, "%d", &s) != 1 || s <= 0 || s > 64)
    {
      fprintf(stderr,
          "%s: invalid argument for option: \'-s\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    break;

  case 'E':  /* the associativity */
    if(E > 0)
    {
      fprintf(stderr,
          "%s: duplicate option: \'-E\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    if(sscanf(optarg, "%d", &E) != 1 || E <= 0)
    {
      fprintf(stderr,
          "%s: invalid argument for option: \'-E\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    break;

  case 'b':  /* the number of block offset bits */
    if(b > 0)
    {
      fprintf(stderr,
          "%s: duplicate option: \'-b\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    if(sscanf(optarg, "%d", &b) != 1 || b <= 0 || b > 64)
    {
      fprintf(stderr,
          "%s: invalid argument for option: \'-b\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    break;

  case 't':  /* the trace file */
    if(trace_file)
    {
      fprintf(stderr,
          "%s: duplicate option: \'-t\'\n", argv0);
      exit(EXIT_FAILURE);
    }
    trace_file = optarg;
    break;

  default:  /* what happened? */
    fprintf(stderr,
        "%s: unknown error\n", argv0);
    exit(EXIT_FAILURE);

  }

  if(optind != argc)  /* nonoption arguments remained */
  {
    fprintf(stderr,
        "%s: unexpected nonoption argument: %s\n", argv0, argv[optind]);
    exit(EXIT_FAILURE);
  }

  /* check compulsory options */
  if(s <= 0)
  {
    fprintf(stderr,
        "%s: missing compulsory option: \'-s\'\n", argv0);
    exit(EXIT_FAILURE);
  }
  S = 1 << s;
  if(E <= 0)
  {
    fprintf(stderr,
        "%s: missing compulsory option: \'-E\'\n", argv0);
    exit(EXIT_FAILURE);
  }
  if(b <= 0)
  {
    fprintf(stderr,
        "%s: missing compulsory option: \'-b\'\n", argv0);
    exit(EXIT_FAILURE);
  }
  t = b + s;
  if(t > 64)
  {
    fprintf(stderr,
        "%s: b + s = %d too large\n", argv0, t);
    exit(EXIT_FAILURE);
  }
  t = 64 - t;
  if(!trace_file)
  {
    fprintf(stderr,
        "%s: missing compulsory option: \'-t\'\n", argv0);
    exit(EXIT_FAILURE);
  }

  init_masks();
  init_cache();
  simulate_trace();
  free_cache();
  printSummary(hit, miss, eviction);

  return 0;
}

void print_help(FILE *stream)
{
  fprintf(stream,
      "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
      "Options:\n"
      "  -h         Print this help message.\n"
      "  -v         Optional verbose flag.\n"
      "  -s <num>   Number of set index bits.\n"
      "  -E <num>   Number of lines per set.\n"
      "  -b <num>   Number of block offset bits.\n"
      "  -t <file>  Trace file.\n"
      "\n"
      "Examples:\n"
      "  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
      "  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n"
  , argv0, argv0, argv0);
}

void init_masks(void)
{
  offset_mask = (uint64_t)-1 >> (64 - b);
  tag_mask = (uint64_t)-1 << (64 - t);
  index_mask = ((uint64_t)-1 ^ offset_mask) ^ tag_mask;
}

void init_cache(void)
{
  cache = malloc(S * sizeof *cache);
  for(int i = 0; i < S; ++i)
  {
    cache[i].head = malloc(sizeof *cache[i].head);
    cache[i].head->prev = NULL;
    cache[i].head->next = NULL;
    cache[i].tail = cache[i].head;
    cache[i].size = 0;
  }
}

static void free_lines(struct cache_line *line)
{
  if(line)
  {
    free_lines(line->next);
    free(line);
  }
}

void free_cache(void)
{
  for(int i = 0; i < S; ++i)
    free_lines(cache[i].head);
  free(cache);
  cache = NULL;
}

int cache_set_full(const struct cache_set *set)
{
  return set->size == E;
}

struct cache_line *do_cache_set_erase(struct cache_line *line)
{
  struct cache_line *prev = line->prev;
  struct cache_line *next = line->next;
  if(prev)
    prev->next = next;
  if(next)
    next->prev = prev;
  line->prev = NULL;
  line->next = NULL;
  return line;
}

void do_cache_set_insert(struct cache_line *prev, struct cache_line *line)
{
  struct cache_line *next = prev->next;
  prev->next = line;
  line->next = next;
  if(next)
    next->prev = line;
  line->prev = prev;
}

void cache_set_cache(struct cache_set *set, struct cache_line *line)
{
  struct cache_line *head = set->head;
  struct cache_line *tail = set->tail;

  if(cache_set_full(set))  /* eviction needed */
  {
    struct cache_line *new_tail = tail->prev;
    struct cache_line *victim = do_cache_set_erase(tail);
    /* write back if needed... */
    free(victim);
    ++eviction;
    if(verbose)
      fprintf(stderr, " eviction");
    tail = new_tail;
  }
  else
    ++set->size;

  set->tail = tail == head ? line : tail;
  do_cache_set_insert(head, line);
}

struct cache_line *cache_set_search(struct cache_set *set, uint64_t tag)
{
  struct cache_line *prev, *line, *tail = set->tail;

  /* loop skipped if cache set list empty */
  for(prev = set->head; prev != tail; prev = line)
  {
    line = prev->next;
    if(line->tag == tag)  /* success */
    {
      if(line == tail)
        set->tail = prev;
      --set->size;
      if(verbose)
        fprintf(stderr, " hit");
      ++hit;
      return do_cache_set_erase(line);
    }
  }

  if(verbose)
    fprintf(stderr, " miss");
  ++miss;
  return NULL;  /* failure */
}

void cache_set_readwrite(struct cache_set *set, uint64_t tag)
{
  struct cache_line *line = cache_set_search(set, tag);
  if(!line)
  {
    line = malloc(sizeof *line);
    line->tag = tag;
    line->prev = NULL;
    line->next = NULL;
    /* fetch data from the next lower level to the block... */
  }
  /* write data to the block if needed... */
  cache_set_cache(set, line);
}

void cache_readwrite(uint64_t addr)
{
  uint64_t tag = addr & tag_mask;
  uint64_t index = (addr & index_mask) >> b;
  cache_set_readwrite(&cache[index], tag);
}

void simulate_trace(void)
{
  int op;
  FILE *trace = fopen(trace_file, "r");
  if(!trace)
  {
    fprintf(stderr,
        "%s: error opening file: %s\n", argv0, trace_file);
    exit(EXIT_FAILURE);
  }

  while((op = fgetc(trace)) != EOF)
    switch(op)
  {
  case '=':  /* message from valgrind */
  case '-':  /* message from valgrind */
  case 'I':  /* instruction fetch */
    while((op = fgetc(trace)) != '\n');
    break;

  case ' ':  /* L/S/M */
    if(!simulate_trace_line(trace))
      break;
    fprintf(stderr,
        "%s: error parsing file: %s\n", argv0, trace_file);
    exit(EXIT_FAILURE);

  default:  /* invalid character */
    fprintf(stderr,
        "%s: invalid beginning character: %c\n", argv0, op);
    fprintf(stderr,
        "%s: error parsing file: %s\n", argv0, trace_file);
    exit(EXIT_FAILURE);
  }

  fclose(trace);
}

int simulate_trace_line(FILE *trace)
{
  char op;
  uint64_t addr;
  int size;

  if(fscanf(trace, "%c %lx,%x", &op, &addr, &size) != 3)
    return -1;
  if(verbose)
    fprintf(stderr,
        "%c %lx,%x (tag: 0x%.8lx, ind: 0x%.4lx, off: 0x%.4lx)", op, addr, size,
        addr & tag_mask, addr & index_mask, addr & offset_mask);
  cache_readwrite(addr);
  if(op == 'M')
    cache_readwrite(addr);
  if(verbose)
    fprintf(stderr, "\n");
  if(verbose >= 2)
    print_cache(stderr);
  while((op = fgetc(trace)) != '\n');

  return 0;
}

void print_cache(FILE *stream)
{
  for(int i = 0; i < S; ++i)
  {
    struct cache_set *set = &cache[i];
    struct cache_line *tail = set->tail;
    struct cache_line *prev, *line;

    fprintf(stream, "  cache-set %d:\n", i);
    for(prev = set->head; prev != tail; prev = line)
    {
      line = prev->next;
      fprintf(stream, "    tag: 0x%.16lx\n", line->tag);
    }
  }
}
