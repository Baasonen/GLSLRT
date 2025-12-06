#ifndef STRUCT_H
#define STRUCT_H

typedef struct 
{
    float px, py, pz;
    float radius;
    float r, g, b;
    float padding;
} Sphere;

typedef struct 
{
    float px, py, pz;
    float yaw, pitch;
    float focal_length;
    float padding[2];
} Camera;

#endif