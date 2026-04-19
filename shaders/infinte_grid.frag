#version 330 core
 
 in vec3 gridTile_worldPos;
 in vec3 cameraWorldPos;
 in float gridSize;

layout(location = 0) out vec4 FragColor;

float satf(float x)
{
    float f = clamp(x, 0.0, 1.0);
    return f;
}

vec2 satv(vec2 x)
{
    vec2 v = clamp(x, vec2(0.0), vec2(1.0));
    return v;
}

float max2(vec2 v)
{
    float f = max(v.x, v.y);
    return f;
}

float log10(float x){
	float result = log(x) / log(10);
	return result;
}

void main()
{
	float gridMinPixelsBetweenCells = 2.0;
	float gridCellSize = 0.025;
	// gridCellSize = 1.0;
	vec4 gridColorThin = vec4(0.3, 0.3, 0.3, 1.0);
	vec4 gridColorThick = vec4(0.5, 0.5, 0.5, 1.0);
	
	vec2 dvx = vec2(dFdx(gridTile_worldPos.x), dFdy(gridTile_worldPos.x));
	vec2 dvy = vec2(dFdx(gridTile_worldPos.z), dFdy(gridTile_worldPos.z));
	float lx = length(dvx);
	float ly = length(dvy);
	
	vec2 dudv = vec2(lx, ly);
	
	float l = length(dudv);
	
	float LOD = max(0.0, log10(l * gridMinPixelsBetweenCells / gridCellSize) + 1.0);
	
	float gridCellSizeLod0 = gridCellSize * pow(10.0, floor(LOD));
	float gridCellSizeLod1 = gridCellSizeLod0 * 10.0;
	float gridCellSizeLod2 = gridCellSizeLod1 * 10.0;
	
	dudv *= 4.0;
	
	vec2 mod_div_dudv = mod(gridTile_worldPos.xz, gridCellSizeLod0) / (dudv);
	mod_div_dudv = clamp(mod_div_dudv, 0.0, 1.0);
	mod_div_dudv = mod_div_dudv * 2.0 - 1.0;
	mod_div_dudv = abs(mod_div_dudv); // making shure that the far grid doesn't appear connected because of the small size
	mod_div_dudv = 1.0 - mod_div_dudv;
	
	float lod0a = max(mod_div_dudv.x, mod_div_dudv.y);
	
	
	mod_div_dudv = mod(gridTile_worldPos.xz, gridCellSizeLod1) / (dudv);
	mod_div_dudv = clamp(mod_div_dudv, 0.0, 1.0);
	mod_div_dudv = mod_div_dudv * 2.0 - 1.0;
	mod_div_dudv = abs(mod_div_dudv); // making shure that the far grid doesn't appear connected because of the small size
	mod_div_dudv = 1.0 - mod_div_dudv;
	
	float lod1a = max(mod_div_dudv.x, mod_div_dudv.y);
	
	
	mod_div_dudv = mod(gridTile_worldPos.xz, gridCellSizeLod2) / (dudv);
	mod_div_dudv = clamp(mod_div_dudv, 0.0, 1.0);
	mod_div_dudv = mod_div_dudv * 2.0 - 1.0;
	mod_div_dudv = abs(mod_div_dudv); // making shure that the far grid doesn't appear connected because of the small size
	mod_div_dudv = 1.0 - mod_div_dudv;
	
	float lod2a = max(mod_div_dudv.x, mod_div_dudv.y);
	
	float lod_fade = fract(LOD);
	
	vec4 color;
	
	if(lod2a > 0.0){
		color = gridColorThick;
		color.a *= lod2a;
	}
	else{
		if(lod1a > 0.0){
			color = mix(gridColorThick, gridColorThin, lod_fade);
			color.a *= lod1a;
		}
		else{
			color = gridColorThin;
			color.a *= (lod0a * (1.0 - lod_fade));
		}
	}
	
	float opacityFalloff = (1.0 - satf(length(gridTile_worldPos.xz - cameraWorldPos.xz) / gridSize));
	
	color.a *= opacityFalloff;
	
    FragColor = color;
}

