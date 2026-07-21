#version 330 core


#define MAX_UINT 4294967295.0

in vec2 TexCoord;
in vec3 instancePos;

in vec3 Normal;
in vec4 grassLightSpacePos;

uniform sampler2D bladeTexture;
uniform sampler2D shadowMap;

out vec4 FragColor;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

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
    // shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

void main(){
    vec4 texColor = texture(bladeTexture, TexCoord);
    if(texColor.a < 0.7) discard;
    
    vec3 grassColor1 = vec3(0.145, 0.361, 0.078);
    vec3 grassColor2 = vec3(0.294, 0.49, 0.055);
    vec3 grassColor3 = vec3(0.667, 0.733, 0.059);
    vec3 grassColor4 = texColor.rgb;
    
    vec3 lightColor = vec3(1.0f, 0.94f, 0.82f);
    vec3 lightDir = normalize(vec3(0.8f, 0.4f, 0.2f));
    
    float r = hash(instancePos.xz);

    vec3 grassTint = vec3(1.0);
    
    if (r < 0.2)
        grassTint = grassColor1;
    else if (r < 0.8)
        grassTint = grassColor2;
    else
        grassTint = grassColor3;
    
    float NdotL = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = lightColor * NdotL;
    vec3 ambient = vec3(0.8);
    float shadow = calculateShadow(grassLightSpacePos, Normal, lightDir);
    // vec3 lighting = ambient + diffuse;
    vec3 lighting = ambient + (1.0 - shadow) * diffuse;
    vec3 color = mix(texColor.rgb, grassTint, 0.1) * lighting;
    FragColor = vec4(color, 1.0);
    // FragColor = vec4(grassTint,1.0);
    // FragColor = texColor;
    FragColor.rgb = FragColor.rgb / (FragColor.rgb + vec3(1.0));
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.0));
}
