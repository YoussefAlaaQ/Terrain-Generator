#version 330

#define MAX_UINT 4294967295.0
#define PI 3.14159265359

in vec2 TexCoord;
in float height;
in vec3 FragPos;
in vec3 geoNormal;
in mat3 TBN;
in vec4 fragPosLightSpace;
in vec3 viewPos_out;

in vec3 tangentLightDir;
in vec3 tangentViewPos;
in vec3 tangentFragPos;


out vec4 FragColor;

uniform sampler2D shadowMap;
uniform sampler2D grassAlbedo;
uniform sampler2D grassNormal;
uniform sampler2D grassHeight;
uniform sampler2D grassRoughness;
uniform sampler2D grassMetallic;
uniform sampler2D grassAO;

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

vec2 parallaxMapping(vec2 oldUV, vec3 viewDir){
	float height_scale = 0.01;
	
	const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;

    float currentLayerDepth = 0.0;
    
    vec2 P = viewDir.xy / viewDir.z * height_scale; 
    vec2 deltaTexCoords = P / numLayers;

    vec2  currentTexCoords     = oldUV;
    float currentDepthMapValue = texture(grassHeight, currentTexCoords).r;
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(grassHeight, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(grassHeight, prevTexCoords).r - currentLayerDepth + layerDepth;
    
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords;
	// float height =  texture(grassHeight, oldUV).r;
	// vec2 p = viewDir.xy / viewDir.z * (height * height_scale);
	// return oldUV - p; 
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main(){
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos); // for parallax mapping
    vec3 worldViewDir = normalize(viewPos_out - FragPos); // for the light calculation
    const vec3 lightDir = normalize(vec3(0.8, 0.4, 0.2));
    vec3 lightColor = vec3(4.0);
    
    float textureTileScale = 0.03; 
    vec2 uv = FragPos.xz * textureTileScale;
    // vec2 uv = TexCoord;
    uv = parallaxMapping(uv, viewDir);
    vec3 albedo = vec3(0.0);
    albedo = texture(grassAlbedo, uv).rgb;
    float roughness = texture(grassRoughness, uv).r;
    float metallic = texture(grassMetallic, uv).r;
    float ao = texture(grassAO, uv).r;
    //NORMAL MAPPING
    vec3 Normal = texture(grassNormal, uv).rgb;
    Normal = Normal * 2.0 - 1.0;
    Normal = normalize(TBN * Normal);

    //PBR
    vec3 h = normalize(worldViewDir + lightDir);
    float cosTheta = max(dot(Normal, lightDir), 0.0);
    vec3 radiance = lightColor;
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F  = fresnelSchlick(max(dot(h, worldViewDir), 0.0), F0); 
    float NDF = DistributionGGX(Normal, h, roughness);
    float G = GeometrySmith(Normal, worldViewDir, lightDir, roughness);
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(Normal, worldViewDir), 0.0) * max(dot(Normal, lightDir), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator; 

    vec3 KS = F;
    vec3 KD = vec3(1.0) - KS;
    KD *= 1.0 - metallic;

    float NdotL = max(dot(Normal, lightDir), 0.0);
    vec3 Lo = (KD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.1) * albedo * ao;
    float shadow = calculateShadow(fragPosLightSpace, geoNormal, lightDir);
    vec3 color = ambient + Lo * (1.0 - shadow);

    // tone mapping and gamma correction 
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
    // FragColor = vec4(1.0);
}
