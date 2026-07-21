#version 330

#define MAX_UINT 4294967295.0

in vec2 TexCoord;
in float height;
in vec3 FragPos;
in vec3 normal;
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

void main(){
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos); // for parallax mappin
    vec3 worldViewDir = normalize(viewPos_out - FragPos); // for the light calulations
    
    float textureTileScale = 0.05; 
    // vec2 uv = FragPos.xz * textureTileScale;
    vec2 uv = TexCoord;
    uv = parallaxMapping(uv, viewDir);
    vec3 color = vec3(0.0);
    color = texture(grassAlbedo, uv).rgb;
    float roughness = texture(grassRoughness, uv).r;
    float metallic = texture(grassMetallic, uv).r;
    float ao = texture(grassAO, uv).r;
    //NORMAL MAPPING
    vec3 texNormal = texture(grassNormal, uv).rgb;
    texNormal = texNormal * 2.0 - 1.0;
    texNormal = normalize(TBN * texNormal);

    const vec3 lightDir = normalize(vec3(0.8, 0.4, 0.2));
    vec3 lightColor = vec3(1.0);
    vec3 wi = lightDir;
    float cosTheta = max(dot(texNormal, wi), 0.0);
    vec3 radiance = lightColor * cosTheta;
    
    
    
    // //ambiant
    float ambiantStrength = 0.1;
    vec3 ambiant = ambiantStrength * lightColor;

    //Diffused
    
    float gamma = 2.2;
    float diffuseFactor = max(dot(texNormal, lightDir), 0.0);
    vec3 diffuse = diffuseFactor * lightColor;
    
    vec3 result;
    float shadow = calculateShadow(fragPosLightSpace, normal, lightDir);
    result = (ambiant + (diffuse * (1.0 - shadow))) * color;

    // result = (normal * 0.5) + 0.5;
    result = pow(result, vec3(1.0/gamma));
    FragColor = vec4(result, 1.0);
    // FragColor = vec4(1.0);
}
