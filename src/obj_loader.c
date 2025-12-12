#include "obj_loader.h"
#include <string.h>

typedef struct 
{
    float x, y, z;
} TempVertex;

typedef struct 
{
    TempVertex* data;
    size_t size;
    size_t capacity;
} TempVertexList;

