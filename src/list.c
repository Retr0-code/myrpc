#include <stdio.h>

#include "list.h"

void linked_list_delete(linked_list_t *ll, ll_destructor_t *deleter)
{
    if (ll == NULL)
        return ;

    if (ll->next != NULL)
        linked_list_delete(ll->next, deleter);

    (*deleter)(ll);
}

linked_list_t *linked_list_last(linked_list_t *ll)
{
    for (; ll->next != NULL; ll = ll->next);
    return ll;
}
