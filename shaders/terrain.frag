#version 330

#define MAX_UINT 4294967295.0

in vec2 TexCoord;
in vec3 worldPos;
in float height;
out vec4 FragColor;

void main(){
    float shineness = height;
    // shineness = pow(h, 0.25);

    vec3 color = vec3(shineness);
    if (height < 0.3) {
        color *= vec3(0.2, 0.4, 0.76); // Deep Water
    } 
    else if (height < 0.4) {
        color *= vec3(0.2, 0.4, 0.77); // shallow water
    } 
    else if (height < 0.45) {
        color *= vec3(0.843, 0.827, 0.517); // Sand
    } 
    else if (height < 0.55) {
        color *= vec3(0.333, 0.588, 0.08); // Grass
    } 
    else if (height < 0.6) {
        color *= vec3(0.24, 0.42, 0.07); // Grass2
    } 
    else if (height < 0.7) {
        color *= vec3(0.356, 0.27, 0.247); // Rock
    } 
    else if (height < 0.9) {
        color *= vec3(0.298, 0.235, 0.215); // Rock2
    } 
    else {
        color *= vec3(0.9, 0.9, 0.9); // Snow
    }
    
    FragColor = vec4(color, 1.0);
}
