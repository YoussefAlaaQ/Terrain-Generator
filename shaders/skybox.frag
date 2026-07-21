#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    float gamma = 2.2;
    FragColor = texture(skybox, TexCoords);
    FragColor.rgb = FragColor.rgb / (FragColor.rgb + vec3(1.0));
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/gamma));
}
