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

typedef struct 
{
    unsigned int* data;
    size_t size;
    size_t capacity;
} TempIndexList;

void init_vertex_list(TempVertexList* list)
{
    list->size = 0;
    list->capacity = 10;
    list->data = (TempVertex*)malloc(list->capacity * sizeof(TempVertex));
}

void push_vertex(TempVertexList* list, float x, float y, float z)
{
    if (list->size >= list->capacity)
    {
        list->capacity *= 2;
        list->data = (TempVertex*)realloc(list->data, list->capacity * sizeof(TempVertex));
    }
    list->data[list->size++] = (TempVertex){x, y, z};
}

void init_index_list(TempIndexList* list)
{
    list->size = 0;
    list->capacity = 30; // 10 Tris
    list->data = (unsigned int*)malloc(list->capacity * sizeof(unsigned int));
}

void push_index(TempIndexList* list, unsigned int index)
{
    if (list->size >= list->capacity)
    {
        list->capacity *= 2;
        list->data = (unsigned int*)realloc(list->data, list->capacity * sizeof(unsigned int));
    }
    list->data[list->size++] = index;
}

// Main loader
MeshData load_obj(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Could not open file %s\n", filename);
        return (MeshData){NULL, NULL, 0, 0};
    }

    TempVertexList temp_positions;
    TempIndexList temp_indices;
    init_vertex_list(&temp_positions);
    init_index_list(&temp_indices);

    char line_buffer[128];
    MeshData final_mesh = {NULL, NULL, 0, 0};
}