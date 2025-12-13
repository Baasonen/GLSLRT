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

    push_vertex(&temp_positions, 0.0f, 0.0f, 0.0f);

    while (fgets(line_buffer, sizeof(line_buffer), file))
    {
        // Get line type
        if (line_buffer[0] == 'v' && line_buffer[1] == ' ')
        {
            float x, y, z;
            if (sscanf(line_buffer, "v %f %f %f", &x, &y, &z) == 3) {push_vertex(&temp_positions, x, y, z);}
        }
         else if (line_buffer[0] == 'f' && line_buffer[1] == ' ')
        {
            unsigned int v[3];

            if (sscanf(line_buffer, "f %u %u %u", &v[0], &v[1], &v[2]) == 3)
            {
                push_index(&temp_indices, v[0]);
                push_index(&temp_indices, v[1]);
                push_index(&temp_indices, v[2]);
            }
            else if (sscanf(line_buffer, "f %u/%*u/%*u %u/%*u/%*u %u/%*u/%*u", &v[0], &v[1], &v[2]) == 3)
            {
                push_index(&temp_indices, v[0]);
                push_index(&temp_indices, v[1]);
                push_index(&temp_indices, v[2]);
            }
            else{fprintf(stderr, "Unsupported face cormat");}
        }
    }
    fclose(file);

    final_mesh.num_indices = temp_indices.size;
    final_mesh.num_vertices = (temp_positions.size - 1) * 3;

    // Allocate memory for the final arrays
    final_mesh.vertices = (float*)malloc(final_mesh.num_vertices * sizeof(float));
    final_mesh.indices = (unsigned int*)malloc(final_mesh.num_indices * sizeof(unsigned int));

    // Copy and skip dummy vertex
    for (size_t i = 0; i < temp_indices.size; i++)
    {
        final_mesh.indices[i] = temp_indices.data[i] - 1;
    }

    for (size_t i = 1; i < temp_positions.size; i++)
    {
        size_t j = (i - 1) * 3;
        final_mesh.vertices[j + 0] = temp_positions.data[i].x;
        final_mesh.vertices[j + 1] = temp_positions.data[i].y;
        final_mesh.vertices[j + 2] = temp_positions.data[i].z;
    }

    // Free temp lists
    free(temp_positions.data);
    free(temp_indices.data);

    return final_mesh;
}

void free_mesh_data(MeshData* mesh)
{
    if (mesh->vertices)
    {
        free(mesh->vertices);
        mesh->vertices = NULL;
    }
    if (mesh->indices)
    {
        free(mesh->indices);
        mesh->indices = NULL;
    }
    mesh->num_vertices = 0;
    mesh->num_indices = 0;
}