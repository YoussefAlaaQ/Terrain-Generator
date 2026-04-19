#version 330 core

uniform mat4 gVP;
uniform vec3 cam_world_pos;

out vec3 gridTile_worldPos;
out float gridSize;
out vec3 cameraWorldPos;

const vec3 pos[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0), //bottom left
	vec3( 1.0, 0.0, -1.0), //bottom right 
	vec3( 1.0, 0.0,  1.0), //top right
	vec3(-1.0, 0.0,  1.0)  //top left
);

const int indeces[6] = int[6](
	1, 0, 2,
	3, 2, 0
);

void main(){
	float _gridSize = 100.0;
	int index = indeces[gl_VertexID];
	
	vec3 vPos3 = pos[index] * _gridSize;
	
	vPos3.x += cam_world_pos.x;
	vPos3.z += cam_world_pos.z;
	
	vec4 vPos = vec4(vPos3, 1.0);
	
	gl_Position = vPos * gVP;
	
	gridTile_worldPos = vPos3;
	gridSize = _gridSize;
	cameraWorldPos = cam_world_pos;
}

