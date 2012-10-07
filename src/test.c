#include <stdio.h>
#include "gpr_containers.h"
#include <time.h>
#include <string.h>

typedef struct {
  I32 var1;
  I32 var2;
} entry;

void print_entries(gpr_idlut_t *table)
{
  I32 i;
  entry *entries = (entry*)gpr_idlut_items(table);

  printf("ID\tvar1\tvar2\n");
  for(i = 0; i < table->num_items; i++)
  {
    printf("%d\t%d\t%d\n", table->item_ids[i], entries[i].var1, entries[i].var2);
  }
}

GPR_IDLUT_DEF(entry)

void print_entries_st(gpr_entry_idlut_t *table)
{
  I32 i;
  gpr_entry_idlut_item *entries = gpr_entry_idlut_items(table);

  printf("ID\tvar1\tvar2\n");
  for(i = 0; i < table->num_items; i++)
  {
    printf("%d\t%d\t%d\n", entries[i].id, entries[i].value.var1, entries[i].value.var2);
  }
}

#define ITER 1024*1024*2

int main()
{
  { // generic test
    gpr_idlut_t table;
    entry     new_entry;
    U32         id1, id2, id3;

    gpr_idlut_init(&table, 8, sizeof(entry));

    new_entry.var1 = 1;
    new_entry.var2 = 2;
    id1 = gpr_idlut_add(&table, &new_entry);

    new_entry.var1 = 3;
    new_entry.var2 = 4;
    id2 = gpr_idlut_add(&table, &new_entry);

    new_entry.var1 = 5;
    new_entry.var2 = 6;
    id3 = gpr_idlut_add(&table, &new_entry);

    printf("initial setup\n");
    print_entries(&table);

    printf("remove %d\n", id2);
    gpr_idlut_remove(&table, id2);
    print_entries(&table);

    printf("add a new entry\n");
    new_entry.var1 = 7;
    new_entry.var2 = 8;
    gpr_idlut_add(&table, &new_entry);
    print_entries(&table);

    printf("remove %d\n", id1);
    gpr_idlut_remove(&table, id1);
    print_entries(&table);

    printf("perf test (lowest is better)\n");
    {
      I32 i; U32 id;
      entry *lp_entry;
      clock_t start, end;

      start = clock();
      for(i = 0; i < ITER; i++)
      {
        id = gpr_idlut_add(&table, &new_entry);
        gpr_idlut_has(&table, id);
        gpr_idlut_lookup(&table, id);
        gpr_idlut_remove(&table, id);
      }
      end = clock();
      printf("done in: %d ms.\n", (end-start)/ (CLOCKS_PER_SEC / 1000));
    }
  }

  // strongly typed test
  {
    gpr_entry_idlut_t table;
    entry             new_entry;
    U32               id1, id2, id3;
    printf("--------------------------------------------------\n");
    printf("Strongly typed\n");
    printf("--------------------------------------------------\n");

    gpr_entry_idlut_init(&table, 8);

    new_entry.var1 = 1;
    new_entry.var2 = 2;
    id1 = gpr_entry_idlut_add(&table, &new_entry);

    new_entry.var1 = 3;
    new_entry.var2 = 4;
    id2 = gpr_entry_idlut_add(&table, &new_entry);

    new_entry.var1 = 5;
    new_entry.var2 = 6;
    id3 = gpr_entry_idlut_add(&table, &new_entry);

    printf("initial setup\n");
    print_entries_st(&table);

    printf("remove %d\n", id2);
    gpr_entry_idlut_remove(&table, id2);
    print_entries_st(&table);

    printf("add a new entry\n");
    new_entry.var1 = 7;
    new_entry.var2 = 8;
    gpr_entry_idlut_add(&table, &new_entry);
    print_entries_st(&table);

    printf("remove %d\n", id1);
    gpr_entry_idlut_remove(&table, id1);
    print_entries_st(&table);

    printf("perf test (lowest is better)\n");
    {
      I32 i; U32 id;
      entry *lp_entry;
      clock_t start, end;

      start = clock();
      for(i = 0; i < ITER; i++)
      {
        id = gpr_entry_idlut_add(&table, &new_entry);
        gpr_entry_idlut_has(&table, id);
        gpr_entry_idlut_lookup(&table, id);
        gpr_entry_idlut_remove(&table, id);
      }
      end = clock();
      printf("done in: %d ms.\n", (end-start)/ (CLOCKS_PER_SEC / 1000));
    }
  }

  return 0;
}