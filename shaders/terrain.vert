#version 330

#define MAX_UINT 4294967295.0
#define PI 3.14159265359

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 mvp;

out vec2 TexCoord;
out vec3 worldPos;
out float height;

uint pcg_hash(uint value)
{
    uint state = value * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

vec2 gradiant_v2(uint x, uint y){
	vec2 result;
    uint seed = x + y * 1619u;
    
    uint hashX = pcg_hash(seed);
    uint hashY = pcg_hash(seed + 314159265u);
    
    // Convert both to [0.0, 1.0]
    float randX = float(hashX) / MAX_UINT;
    float randY = float(hashY) / MAX_UINT;
    
    result = vec2(2.0 * randX - 1.0, 2.0 * randY - 1.0);
    
    // result = normalize(result);
    
    return result;
}

float perlin_noise(vec2 p){
    //THIS IS A HACK FOR NOW 
    ivec2 i = ivec2(floor(p));
    
    vec2 f = fract(p);
    
    vec2 s = f * f * (3.0 - 2.0 * f);
    
    uint minX = uint(i.x);
    uint minY = uint(i.y);
    uint maxX = minX + 1u;
    uint maxY = minY + 1u;
    
    vec2 topLeft_grad     = gradiant_v2(minX, minY);
    vec2 topRight_grad    = gradiant_v2(maxX, minY);
    vec2 bottomLeft_grad  = gradiant_v2(minX, maxY);
    vec2 bottomRight_grad = gradiant_v2(maxX, maxY);
    
    vec2 topLeft_toPoint     = f - vec2(0.0, 0.0);
    vec2 topRight_toPoint    = f - vec2(1.0, 0.0);
    vec2 bottomLeft_toPoint  = f - vec2(0.0, 1.0);
    vec2 bottomRight_toPoint = f - vec2(1.0, 1.0);
    
    float tl = dot(topLeft_grad, topLeft_toPoint);
    float tr = dot(topRight_grad, topRight_toPoint);
    float bl = dot(bottomLeft_grad, bottomLeft_toPoint);
    float br = dot(bottomRight_grad, bottomRight_toPoint);
    
    float top = mix(tl, tr, s.x);
    float bottom = mix(bl, br, s.x);
    
    float result = mix(top, bottom, s.y);
    
    return result;
}

float fbm(vec2 p, uint seed, float scale, int octaves, float persistance, float lacunarity){
	float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;
	
	if(scale == 0.0){
		scale = 0.0001;
	}
	
	for(int i = 0; i < octaves; ++i){
		uint octaveSeed = pcg_hash(seed + uint(i));
        vec2 octaveOffset = vec2(pcg_hash(octaveSeed), pcg_hash(octaveSeed + 100u)) / MAX_UINT * 100.0;
        
		vec2 sampledP = p / scale * frequency + octaveOffset;
		float n = perlin_noise(sampledP);
        value     += n * amplitude;
        maxValue  += amplitude;
        amplitude *= persistance;
        frequency *= lacunarity;
	}
	
	if(value > maxValue){
		value = maxValue;
	}
	
	return value;
}

void main(){
	
	vec4 wp = vec4(aPos, 1.0) * model;
	worldPos = wp.xyz;
	
	float heightMultiplier = 50.0;
    
    float h = fbm(worldPos.xz , 3u, 100.0, 5, 0.5, 2.2);
	h = h * 0.5 + 0.5;
	height = h;
    
	//easing function
	h = h * h * h * h * h;
    
	h *= heightMultiplier;
    
	
	gl_Position = vec4(aPos.x, h, aPos.z, 1.0) * mvp;
	TexCoord = aTexCoord;
}

