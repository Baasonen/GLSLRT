#ifndef STRUCT_H
#define STRUCT_H

typedef struct 
{
    float cr, cg, cb;
    float padding;
    float roughness;
    float metallic;
    float emission;
    float opacity;
} Material;

typedef struct 
{
    float px, py, pz;
    float radius;
    int material_index;
    float padding_sphere[3];
} Sphere;

typedef struct 
{
    float v0[3]; float pad0;
    float v1[3]; float pad1;
    float v2[3]; float pad2;
    int material_index;
    float padding[3];
} Triangle;

typedef struct 
{
    float px, py, pz;
    float yaw, pitch;
    float focal_length;
    float padding[2];
} Camera;

#endif