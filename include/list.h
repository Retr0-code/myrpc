#pragma once

typedef struct linked_list_t
{
    struct linked_list_t *next;
} linked_list_t;

typedef void (ll_destructor_t)(void*);

#define ll_destructor_default &free

void linked_list_delete(linked_list_t *ll, ll_destructor_t *deleter);

linked_list_t *linked_list_last(linked_list_t *ll);
