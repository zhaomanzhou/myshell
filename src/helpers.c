#include "../include/helpers.h"

int comparator(void* v1, void* v2)
{
    node_t* n1 = (node_t*)v1;
    node_t* n2 = (node_t*)v2;
    ProcessEntry_t* p1 = (ProcessEntry_t*)n1->value;
    ProcessEntry_t* p2 = (ProcessEntry_t*)n2->value;
    return p2->seconds-p1->seconds;
}