#version 330 core

#define MAX_UINT 4294967295.0
#define PI 3.14159265359

layout (location = 0) in vec3 aPos;        // blade-local vertex position
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aInstancePos; // world X,Z + placeholder Y from CPU

uniform int Uoctaves;
uniform float Ulacunarety, Upersistance, Uscale, UheightMultipier;

uniform mat4 vp;
uniform mat4 lightSpaceMatrix;

out vec3 Normal;
out vec2 TexCoord;
out vec3 instancePos;
out vec4 grassLightSpacePos;

uint pcg_hash(uint value)
{
    uint state = value * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

vec2 v2_hash(vec2 p){
	uvec2 q = uvec2(ivec2(p));  // ✅ handles negative coords correctly
    uint seed = q.x + q.y * 1619u;
    uint hx = pcg_hash(seed);
    uint hy = pcg_hash(seed + 314159265u);
    float rx = float(hx) / MAX_UINT * 2.0 - 1.0;
    float ry = float(hy) / MAX_UINT * 2.0 - 1.0;
    return normalize(vec2(rx, ry));
}

vec3 perlinNoise(in vec2 p){
	vec2 i = floor(p);
	vec2 f = fract(p);

	// quantic instead of linear
	vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du = 30.0*f*f*(f*(f-2.0)+1.0);
    
    vec2 gb = v2_hash(i + vec2(1.0,0.0));
    vec2 gc = v2_hash(i + vec2(0.0,1.0));
    vec2 ga = v2_hash(i + vec2(0.0,0.0));
    vec2 gd = v2_hash(i + vec2(1.0,1.0));
    
    float va = dot(ga, f - vec2(0.0,0.0));
    float vb = dot(gb, f - vec2(1.0,0.0));
    float vc = dot(gc, f - vec2(0.0,1.0));
    float vd = dot(gd, f - vec2(1.0,1.0));
    
    return vec3( va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd),   // value
                 ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) +  // derivatives
                 du * (u.yx*(va-vb-vc+vd) + vec2(vb,vc) - va));

}

vec3 fbm(in vec2 x){
	int octaves = Uoctaves;
	// float f = 2.0;  // could be 2.0
    // float s = 0.5;  // could be 0.5
	float f = Ulacunarety;  // could be 2.0
    float s = Upersistance;  // could be 0.5
    float a = 0.0;
    float b = 0.5;
    vec2  d = vec2(0.0);
    mat2  m = mat2(1.0,0.0,
                   0.0,1.0);
    const mat2 m2 = mat2(  0.80,  0.60,
                      -0.60,  0.80 );
    const mat2 m2i = mat2( 0.80, -0.60,
                       0.60,  0.80 );
    for( int i=0; i<octaves; i++ )
    {
        vec3 n = perlinNoise(x);
        a += b*n.x;          // accumulate values
        d += b*m*n.yz;      // accumulate derivatives
        b *= s;
        x = f*m2*x;
        m = f*m2i*m;
    }
    return vec3( a, d );
	
}

void main(){
    vec3 e = fbm((aInstancePos.xz) / Uscale + vec2(1.0, -2.0));
    float terrainHeight = UheightMultipier * e.x + UheightMultipier;
    e.yz = UheightMultipier*e.yz;
    
    vec3 worldPos = aPos + vec3(aInstancePos.x, terrainHeight, aInstancePos.z);
    gl_Position = vec4(worldPos, 1.0) * vp;
    grassLightSpacePos = vec4(worldPos, 1.0f) * lightSpaceMatrix;
    // Normal = normalize( vec3(-e.y,1.0,-e.z) );
    Normal = vec3(0.0, 1.0, 0.0);
    TexCoord = aTexCoord;
    instancePos = aInstancePos;
}
