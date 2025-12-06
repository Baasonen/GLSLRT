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

float hash(vec2 p)
{
    p = fract(p * vec2(5.3121, 6.7845));
    p += dot(p, p.yx + 21.0);
    return fract(p.x * p.y);
}

void main() 
{
    vec2 pixel_coord = gl_FragCoord.xy;
    float seed = hash(pixel_coord + float(u_frameCount));

    vec2 jitter = (vec2(hash(pixel_coord.xy), hash(pixel_coord.xy + 1.0)) * 2.0 - 1.0) / u_resolution;    

    float aspect = u_resolution.x / u_resolution.y;
    //vec2 uv = (pixel_coord + jitter) / u_resolution * 2.0 - 1;
    vec2 uv = (pixel_coord / u_resolution) * 2.0 - 1.0;
    uv.x *= aspect;
    
    vec3 ro = u_cameraPos;
    vec3 lookAt = vec3(0.0, 0.0, 0.0);
    vec3 forward = normalize(lookAt - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    float fov = 45.0;
    float tanFov = tan(radians(fov) * 0.5);
    vec3 rd = normalize(forward + uv.x * tanFov * right + uv.y * tanFov * up);

    //vec3 rd = normalize(vec3(uv, -1.0));
    
    // Sky Color (Gradient)
    vec3 col = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), 0.5 * (uv.y + 1.0));
    
    float minT = 10000.0;
    int hitIndex = -1;

    for (int i = 0; i < spheres.length(); i++) {
        float t = hitSphere(spheres[i], ro, rd);
        if (t > 0.0 && t < minT) {
            minT = t;
            hitIndex = i;
        }
    }
    
    // If we hit something, perform basic shading
    if (hitIndex != -1) {
        vec3 hitPos = ro + rd * minT;
        vec3 normal = normalize(hitPos - spheres[hitIndex].pos);
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        
        // Lambertian Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * spheres[hitIndex].color;
        
        // Simple Ambient
        vec3 ambient = 0.1 * spheres[hitIndex].color;
        
        col = ambient + diffuse;
    }
    
    // Gamma correction
    col = pow(col, vec3(1.0/2.2));

    vec3 prevColor = texture(u_historyTexture, pixel_coord / u_resolution).rgb;

    float weight = 1.0 / float(u_frameCount);
    if (u_frameCount == 1)
    {
        weight = 1.0;
    }

    //vec3 finalColor = mix(prevColor, col, weight);
    vec3 finalColor = col;
    FragColor = vec4(finalColor, 1.0);
}