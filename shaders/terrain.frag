#version 330

#define MAX_UINT 4294967295.0

in vec2 TexCoord;
in float height;
in vec3 FragPos;
in vec3 normal;
in vec4 fragPosLightSpace;

out vec4 FragColor;

uniform vec3 viewPos;

uniform sampler2D shadowMap;

float calculateShadow(vec4 fp_lightSpace, vec3 normal, vec3 lightDir){
    vec3 projCoords = fp_lightSpace.xyz / fp_lightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0)
        return 0.0;

    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.005);
    // bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main(){
    float shineness = height;
    // shineness = pow(height, 1);
    
    // vec3 color = vec3(1.0);
    vec3 rock  = vec3(0.35, 0.30, 0.25);   // dark grey-brown rock
    vec3 dirt  = vec3(0.52, 0.40, 0.28);   // warm brown dirt
    vec3 grass = vec3(0.25, 0.40, 0.18);   // muted green grass
    vec3 snow  = vec3(0.85, 0.87, 0.90);   // slightly blue-tinted snow
    
    // slope: flat = grass/dirt, steep = rock
    float slope = 1.0 - normal.y;  // 0 = flat, 1 = vertical
    
    // blend dirt and grass by height
    vec3 ground = mix(dirt, grass, smoothstep(0.3, 0.6, height));
    
    // steep slopes become rock
    vec3 color = mix(ground, rock, smoothstep(0.3, 0.7, slope));
    
    // high altitude becomes snow
    color = mix(color, snow, smoothstep(0.75, 0.85, height));
    
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    // //ambiant
    float ambiantStrength = 0.15;
    vec3 ambiant = ambiantStrength * lightColor;

    //Diffused
    const vec3 lightDir = normalize(vec3(0.8, 0.4, 0.2));
    // const vec3  kSunDir = vec3(0.624695, 0.468521, 0.624695);
    // vec3 lightDir = normalize(vec3(1.0f, 1.0f, 0.5f));
    
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseFactor * lightColor;

    // color = vec3(0.7, 0.7, 0.7);
    vec3 result;
    float shadow = calculateShadow(fragPosLightSpace, normal, lightDir);
    result = (ambiant + (diffuse) * (1.0 - shadow)) * color;

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // result = vec3(closestDepth);
    // result = vec3(projCoords.xy, 0.0);
    
    // result = (normal * 0.5) + 0.5;
    FragColor = vec4(result, 1.0);
    // FragColor = vec4(1.0);
}
