#version 460 core
out vec4 FragColor;
in vec2 v_uv;

struct Sphere {
    vec3 pos;
    float radius;
    vec3 color;
    float padding;
};

layout(std430, binding = 0) buffer SceneData {
    Sphere spheres[];
};

uniform vec2 u_resolution;
uniform int u_frameCount;
uniform sampler2D u_historyTexture;
uniform vec3 u_cameraPos;
uniform float u_cameraYaw;
uniform float u_cameraPitch;
uniform int u_isDisplayPass;

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

int findClosestHit(vec3 ro, vec3 rd, out float minT)
{
    minT = 10000.0;
    int hitIndex = -1;

    for(int i = 0; i < spheres.length(); i++)
    {
        float t = hitSphere(spheres[i], ro, rd);
        if (t > 0.001 && t < minT)
        {
            minT = t;
            hitIndex = i;
        }
    }
    return hitIndex;
}

float hash(vec2 p)
{
    p = fract(p * vec2(5.3121, 6.7845));
    p += dot(p, p.yx + 21.0);
    return fract(p.x * p.y);
}

const float SHININESS = 1000.0;

vec3 shade(vec3 hitPost, vec3 normal, vec3 sphereColor, vec3 rd)
{
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 V = -rd;

    // Lambertian Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * sphereColor;

    // Specular
    vec3 halfwayDir = normalize(lightDir + V);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), SHININESS);
    vec3 specular = vec3(1.0) * spec;

    // Ambient
    vec3 ambient = 0.1 * sphereColor;

    return ambient + diffuse + specular;
}

void main() 
{
    if (u_isDisplayPass == 1)
    {
        FragColor = texture(u_historyTexture, v_uv);
        return;
    }

    vec2 pixel_coord = gl_FragCoord.xy;

    float seed = hash(pixel_coord + float(u_frameCount));
    vec2 jitter = (vec2(hash(pixel_coord.xy), hash(pixel_coord.xy + 1.0)) * 2.0 - 1.0);
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
    vec3 finalColor = vec3(0.0, 0.0, 0.0);
    vec3 throughput = vec3(1.0, 1.0, 1.0);

    const int MAX_BOUNCES = 50;

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        float minT;
        int hitIndex = findClosestHit(ro, rd, minT);

        if (hitIndex != -1)
        {
            vec3 hitPos = ro + rd * minT;
            vec3 normal = normalize(hitPos - spheres[hitIndex].pos);

            // Direct Only
            if (bounce == 0)
            {
                finalColor += throughput * shade(hitPos, normal, spheres[hitIndex].color, rd);
            }

            // Reflect Ray For Next Bounce

            rd = reflect(rd, normal);
            ro = hitPos + normal * 0.001; // Epsilon Push

            throughput *= spheres[hitIndex].color * 0.9;
        }
        else // Hit Sky / OOB
        {
            vec3 skyColor = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), 0.5 * (uv.y + 1.0));
            finalColor += throughput * skyColor;
            break;
        }
    }

    // Gamma Correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    vec3 prevColor = texture(u_historyTexture, pixel_coord / u_resolution).rgb;

    float weight = 1.0 / float(u_frameCount);
    if (u_frameCount == 1) {weight = 1.0;}

    vec3 accumulatedColor = mix(prevColor, finalColor, weight);
    FragColor = vec4(accumulatedColor, 1.0);
}