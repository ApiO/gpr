#include <stdio.h>
#include "gpr_containers.h"

typedef struct {
  I32 var1;
  I32 var2;
} entry_t;

void print_entries(gpr_idlut_t *table)
{
  I32 i;
  entry_t *entries = (entry_t*)gpr_idlut_items(table);

  printf("ID\tvar1\tvar2\n");
  for(i = 0; i < table->num_items; i++)
  {
    printf("%d\t%d\t%d\n", table->item_ids[i], entries[i].var1, entries[i].var2);
  }
}

int main()
{
  gpr_idlut_t table;
  entry_t     new_entry;
  U32         id1, id2, id3;

  gpr_idlut_init(&table, 8, sizeof(entry_t));

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

  return 0;
}