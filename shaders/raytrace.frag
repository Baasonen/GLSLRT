#version 460 core
out vec4 FragColor;
in vec2 v_uv;

struct Sphere {
    vec3 pos;
    float radius;
    int material_index;
    float padding[3];
};

struct Material
{
    vec4 color;
    float roughness;
    float metallic;
    float emission;
    float opacity;
};

layout(std430, binding = 0) buffer SceneData {Sphere spheres[];};
layout(std430, binding = 1) buffer MaterialData {Material materials[];};

layout(std430, binding = 2) buffer VertexData {vec3 vertices[];};
layout(std430, binding = 3) buffer IndexData {uint indices[];};

uniform vec2 u_resolution;
uniform int u_frameCount;
uniform sampler2D u_historyTexture;
uniform vec3 u_cameraPos;
uniform float u_cameraYaw;
uniform float u_cameraPitch;
uniform int u_isDisplayPass;

const float M_PI = 3.1415926;

uint pcg_hash(inout uint state)
{
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float random(inout uint state)
{
    return float(pcg_hash(state) * 2.3283064365386963e-10);
}

vec3 cosHemisphere(vec3 n, inout uint seed)
{
    float r1 = random(seed);
    float r2 = random(seed);

    float phi = 2.0 * M_PI * r1;
    float cosTheta = sqrt(1.0 - r2);
    float sinTheta = sqrt(r2);

    vec3 localRay = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(n.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return tangent * localRay.x + bitangent * localRay.y + n * localRay.z;
}

float hitTriangleIndexed(int triIndex, vec3 ro, vec3 rd)
{
    uint i0 = indices[3 * triIndex + 0];
    uint i1 = indices[3 * triIndex + 1];
    uint i2 = indices[3 * triIndex + 2];

    vec3 v0 = vertices[i0];
    vec3 v1 = vertices[i1];
    vec3 v2 = vertices[i2];

    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rd, edge2);
    float a = dot(edge1, h);

    // Parallel check
    if (a > -0.001 && a < 0.001) {return -1.0;}

    float f = 1.0 / a;
    vec3 s = ro - v0;
    float u = f * dot(s, h);

    if (u <  0.0 || u > 1.0) {return -1.0;}

    vec3 q = cross(s, edge1);
    float v = f * dot(rd, q);

    if (v < 0.0 || u + v > 1.0) {return -1.0;}

    float t = f * dot(edge2, q);

    if (t > 0.001) {return t;}
    return -1.0;
}

// Returns distance t to intersection (or -1.0 if miss)
float hitSphere(Sphere s, vec3 ro, vec3 rd) {
    vec3 oc = ro - s.pos;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - s.radius * s.radius;
    float h = b*b - c; // discriminant (simplified because a=dot(D,D)=1)
    
    if (h < 0.0) return -1.0;
    
    // We want the closest hit (t = -b - sqrt(h))
    return -b - sqrt(h); 
}

// Hit type: 0 miss, 1 sphere, 2 triangle
void findClosestHit(vec3 ro, vec3 rd, out float minT, out int hitIndex, out int hitType)
{
    minT = 10000.0;
    hitIndex = -1;
    hitType = 0;

    // Check for sphere
    for(int i = 0; i < spheres.length(); i++)
    {
        float t = hitSphere(spheres[i], ro, rd);
        if (t > 0.001 && t < minT)
        {
            minT = t;
            hitIndex = i;
            hitType = 1;
        }
    }

    // Check for triangle
    int triCount = indices.length() / 3;
    for(int i = 0; i < triCount; i++)
    {
        float t = hitTriangleIndexed(i, ro, rd);
        if (t > 0.001 && t < minT)
        {
            minT = t;
            hitIndex = i;
            hitType = 2;
        }
    }
}

float hash(vec2 p)
{
    p = fract(p * vec2(5.3121, 6.7845));
    p += dot(p, p.yx + 21.0);
    return fract(p.x * p.y);
}

const float SHININESS = 1000.0;

vec3 shade(vec3 hitPost, vec3 normal, vec3 rd, int matIndex)
{
    Material mat = materials[matIndex];
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 V = -rd;

    // Emission
    vec3 emissive = mat.color.rgb * mat.emission;

    // Lambertian Diffuse
    vec3 diffuseColor = mat.color.rgb * (1.0 - mat.metallic);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * diffuseColor;

    // Specular
    vec3 specularColor = mix(vec3(0.04), mat.color.rgb, mat.metallic);
    vec3 halfwayDir = normalize(lightDir + V);
    float shininess = 1.0 / (mat.roughness * mat.roughness + 0.001);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularColor * spec;

    // Ambient
    vec3 ambient = 0.1 * mat.color.rgb;

    return emissive + ambient + diffuse + specular;
}

void main() 
{
    if (u_isDisplayPass == 1)
    {
        FragColor = texture(u_historyTexture, v_uv);
        return;
    }

    vec2 pixel_coord = gl_FragCoord.xy;

    uint seed = uint(pixel_coord.x) + uint(pixel_coord.y) * uint(u_resolution.x) + uint(u_frameCount) * 7125413u;

    // Camera setup
    vec2 jitter = (vec2(random(seed), random(seed)) - 0.5);
    float aspect = u_resolution.x / u_resolution.y;
    vec2 uv = (pixel_coord + jitter) / u_resolution * 2.0 - 1.0;
    uv.x *= aspect;

    vec3 ro = u_cameraPos;
    float yawRads = radians(u_cameraYaw);
    float pitchRads = radians(u_cameraPitch);

    vec3 forward;
    forward.x = cos(yawRads) * cos(pitchRads);
    forward.y = sin(pitchRads);
    forward.z = sin(yawRads) * cos(pitchRads);

    vec3 worldUp = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(forward, worldUp));
    vec3 up = normalize(cross(right, forward));

    float fov = 45.0;
    float tanFov = tan(radians(fov) * 0.5);
    vec3 rd = normalize(forward + uv.x * tanFov * right + uv.y * tanFov * up);

    // Recursive Raytracing
    vec3 accumulatedLight = vec3(0.0, 0.0, 0.0);
    vec3 throughput = vec3(1.0, 1.0, 1.0);
    vec3 current_ro = u_cameraPos;
    vec3 current_rd = rd;

    const int MAX_BOUNCES = 15;

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        float minT;
        int hitIndex;
        int hitType;
        findClosestHit(current_ro, current_rd, minT, hitIndex, hitType);

        if (hitIndex != -1)
        {
            vec3 hitPos = current_ro + current_rd * minT;
            vec3 normal;
            int matIndex;

            if (hitType == 1)
            {
                Sphere s = spheres[hitIndex];
                normal = normalize(hitPos - s.pos);
                matIndex = s.material_index;   
            }
            else if (hitType == 2)
            {
                uint i0 = indices[3 * hitIndex + 0];
                uint i1 = indices[3 * hitIndex + 1];
                uint i2 = indices[3 * hitIndex + 2];
                
                vec3 v0 = vertices[i0];
                vec3 v1 = vertices[i1];
                vec3 v2 = vertices[i2];

                vec3 edge1 = v1 - v0;
                vec3 edge2 = v2 - v0;

                normal = normalize(cross(edge1, edge2));
                matIndex = 5;

                // Flip normal if hit back face
                if (dot(normal, current_rd) > 0.0) {normal = -normal;}
            }
            Material mat = materials[matIndex];

            // Emissive
            accumulatedLight += mat.color.rgb * mat.emission * throughput;

            // Simple Schlik Fresnel approx.
            float fresnel = 0.04 + (1.0 - 0.04) * pow(1.0 - max(dot(normal, -current_rd), 0.0), 5.0);
            bool isSpecular = random(seed) < mix(fresnel, 1.0, mat.metallic);

            if (isSpecular)
            {
                vec3 reflectDir = reflect(current_rd, normal);

                current_rd = normalize(mix(reflectDir, cosHemisphere(normal, seed), mat.roughness * mat.roughness));
                throughput *= mix(vec3(1.0), mat.color.rgb, mat.metallic);
            }
            else
            {
                current_rd = cosHemisphere(normal, seed);
                throughput *= mat.color.rgb;
            }

            // Offset to prevent self intersection
            current_ro = hitPos + normal * 0.001;

            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (random(seed) > p) break;
            throughput /= p;
        }
        else // Hit Sky / OOB
        {   
            float skyT = 0.5 * (current_rd.y + 1.0);
            //vec3 skyColor = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), 0.5 * (current_rd.y + 1.0));
            vec3 skyColor = vec3(0.0, 0.0, 0.0);
            accumulatedLight += throughput * skyColor;
            break;
        }
    }

    // Gamma Correction
    vec3 prevColor = texture(u_historyTexture, pixel_coord / u_resolution).rgb;
    prevColor = pow(prevColor, vec3(2.2));

    float weight = 1.0 / float(u_frameCount);
    vec3 finalLinear = mix(prevColor, accumulatedLight, weight);

    FragColor = vec4(pow(finalLinear, vec3(1.0 / 2.2)), 1.0);
}