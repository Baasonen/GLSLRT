#version 460 core

const vec2 positions[3] = vec2[]
(
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);

out vec2 v_uv;

void main()
{
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);

    v_uv = 0.5 * positions[gl_VertexID] + 0.5;
}