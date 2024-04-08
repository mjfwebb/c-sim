#include <stdio.h>
#include <stdlib.h>

typedef struct ListItem ListItem;
struct ListItem
{
  ListItem *next;
  int value;
};

void add_item(ListItem **list_item, int value)
{
  ListItem *new_list_item = (ListItem *)malloc(sizeof(ListItem));
  new_list_item->value = value;
  new_list_item->next = NULL;

  if (*list_item == NULL)
  {
    *list_item = new_list_item;
    return;
  }

  ListItem *curr = *list_item;
  while (curr->next)
  {
    curr = curr->next;
  }

  curr->next = new_list_item;

  return;
}

int main(void)
{
  ListItem *list = NULL;
  add_item(&list, 1);
  add_item(&list, 5);
  add_item(&list, 10);

  ListItem *curr = list;
  while (curr != NULL)
  {
    printf("%p with value %d\n", (void *)curr, curr->value);
    curr = curr->next;
  }
}
