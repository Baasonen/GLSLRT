#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct 
{
    float* vertices;
    unsigned int* indices;
    size_t num_vertices;
    size_t num_indices;
} MeshData;

MeshData load_obj(const char* filename);

// Free memory allocated my load_obj
void free_mesh_data(MeshData* mesh);

#endif