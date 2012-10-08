#include <stdio.h>
#include "gpr_containers.h"
#include <time.h>
#include <string.h>

typedef struct {
  I32 var1;
  I32 var2;
} entry;

GPR_IDLUT_INIT(entry)

void print_entries_st(gpr_entry_idlut_t *table)
{
  I32 i;
  gpr_idlut_item(entry) *items = gpr_idlut_items(entry, table);

  printf("ID\tvar1\tvar2\n");
  for(i = 0; i < table->num_items; i++)
  {
    printf("%d\t%d\t%d\n", items[i].id, items[i].value.var1, items[i].value.var2);
  }
}

int main()
{
  entry new_entry;
  U32   id1, id2, id3;

  gpr_idlut_t(entry) table;
  gpr_idlut_init(entry, &table, 8);

  new_entry.var1 = 1;
  new_entry.var2 = 2;
  id1 = gpr_idlut_add(entry, &table, &new_entry);

  new_entry.var1 = 3;
  new_entry.var2 = 4;
  id2 = gpr_idlut_add(entry, &table, &new_entry);

  new_entry.var1 = 5;
  new_entry.var2 = 6;
  id3 = gpr_idlut_add(entry, &table, &new_entry);

  printf("initial setup\n");
  print_entries_st(&table);

  printf("remove %d\n", id2);
  gpr_idlut_remove(entry, &table, id2);
  print_entries_st(&table);

  printf("add a new entry\n");
  new_entry.var1 = 7;
  new_entry.var2 = 8;
  gpr_idlut_add(entry, &table, &new_entry);
  print_entries_st(&table);

  printf("remove %d\n", id1);
  gpr_idlut_remove(entry, &table, id1);
  print_entries_st(&table);

  return 0;
}