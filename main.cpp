#include <glad/glad.h>
#include "glad.c"

#include <SDL3/SDL.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_opengl.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_opengl3.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui/imgui_impl_sdl3.cpp"
#include "imgui/imgui_impl_opengl3.cpp"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

#include <stdio.h>
#include <stdint.h>

#define internal 	   static
#define local_persist   static
#define global_variable static

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;


#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720


#include "3dMath.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int*)0 = 0;}
#else 
#define Assert(Expression)
#endif

// #define CHUNK_SIZE 513
#define CHUNK_SIZE 241

const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;
# define MAX_CHUNKS MAP_WIDTH * MAP_HEIGHT
const float chunkHalfWidth = (CHUNK_SIZE - 1) / 2.0;

global_variable bool32 Running = true;

enum ProgramModes{
	GAME_MODE,
	FREE_CAM_MODE,
	EDITOR_MODE
};

struct Camera{
	v3 pos	  ;
	v3 direction;
	v3 up       ;	
	v3 front    ;	
	float yaw   ;
	float pitch ;
	
};

struct PBRMaterial{
	uint32 albedo;
	uint32 roughness;
	uint32 ao;
	uint32 normal;
	uint32 metallic;
};

struct Mesh{
	uint32 vbo;
	uint32 vao;
	uint32 ebo;
	
	uint32 index_count;

    PBRMaterial mat;
};

struct input{
	bool32 left;
	bool32 right;
	bool32 forward;
	bool32 backward;
	
	bool32 sprint;
	bool32 up;
	bool32 down;
};

struct Terrain_chunk{
	Mesh *mesh;
	v3 pos;
	int LOD;
	bool32 exists;
};

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

char* loadFile(char* path);

#include "shader.cpp"

char* 
loadFile(char* path){
	size_t fileSize = 0;
    void* loadedFile = SDL_LoadFile(path, &fileSize);
    if (loadedFile == NULL) {
        printf("Failed to load level file '%s': %s\n", path, SDL_GetError());
        return NULL;
    }

    // Allocate +1 byte for '\0'
    char* text = (char*)malloc(fileSize + 1);
    memcpy(text, loadedFile, fileSize);
    text[fileSize] = '\0'; // null-terminate

    SDL_free(loadedFile);
    return text;
}

void
playerInputHandle(input* gameInput, const bool* curruntKeyState){
	gameInput->left = curruntKeyState[SDL_SCANCODE_LEFT]  || curruntKeyState[SDL_SCANCODE_A];
    gameInput->right = curruntKeyState[SDL_SCANCODE_RIGHT] || curruntKeyState[SDL_SCANCODE_D];

	gameInput->forward = curruntKeyState[SDL_SCANCODE_UP] || curruntKeyState[SDL_SCANCODE_W];
	gameInput->backward = curruntKeyState[SDL_SCANCODE_DOWN] || curruntKeyState[SDL_SCANCODE_S];

	gameInput->sprint = curruntKeyState[SDL_SCANCODE_LSHIFT];
	gameInput->up = curruntKeyState[SDL_SCANCODE_SPACE];
	gameInput->down = curruntKeyState[SDL_SCANCODE_LCTRL];
	
}

void 
playerMouseInput(float dx, float dy, float* pitch, float* yaw) {
    const float sensitivity = 0.1f;
    dx *= sensitivity;
    dy *= sensitivity;

    *yaw   += dx;
    *pitch -= dy;

    if (*pitch > 89.0f)  *pitch = 89.0f;
    if (*pitch < -89.0f) *pitch = -89.0f;
}

bool 
init(){
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != true){
		printf("failed to initialize sdl");
		return false;
	}
		
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	return true;
}

uint32 loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        printf("Texture failed to load at path: %s \n",path);
        stbi_image_free(data);
    }

    return textureID;
}

// NOTE: this is not complete
Mesh
create_quad(){
	Mesh result;
	
	float quadVertices[] = {  
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
	
		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};
	
	
    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glBindVertexArray(result.vao);
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	return result;

}

Mesh
create_grid_mesh_data(int lod){
	Mesh result = {};
	
	uint32 meshSimplificationIncrument = 0;
	
	if(lod > 6){
		printf("ERR: lod can't exceed 6 \n");
		return result;
	}
	else{
        meshSimplificationIncrument = 1 << lod;
	}
	
	int verticesPerLine = (CHUNK_SIZE - 1) / meshSimplificationIncrument + 1;
	
	// const int CHUNK_SIZE = 100;

	int VERTEX_COUNT = (verticesPerLine) * (verticesPerLine);
	result.index_count  = (verticesPerLine - 1) * (verticesPerLine - 1) * 6;
	int STRIDE = 5;  // x, y, z, u, v
	
	float* vertices = (float*)SDL_malloc(VERTEX_COUNT * STRIDE * sizeof(float));
	uint32* indices = (uint32*)SDL_malloc(result.index_count * sizeof(uint32));
	
	// float vertices[VERTEX_COUNT * STRIDE];
	// unsigned int indices[INDEX_COUNT];
	
	float spacing = 0.5f;
	int idx = 0;
	
	float lastXPos = 0;
	float lastZPos = 0;
	
	//vertex layout: x, y, z, u, v, interpolate
	for(int z = 0; z < verticesPerLine; z++) {
		for(int x = 0; x < verticesPerLine; x++) {
			int realX = x * meshSimplificationIncrument;
			int realZ = z * meshSimplificationIncrument;
	
			float xPos = (realX - (CHUNK_SIZE - 1) / 2.0f) * spacing;
			float zPos = (realZ - (CHUNK_SIZE - 1) / 2.0f) * spacing;
	
			vertices[idx++] = xPos;
			vertices[idx++] = 0.0f;
			vertices[idx++] = zPos;
			vertices[idx++] = (float)realX / (CHUNK_SIZE - 1);
			vertices[idx++] = (float)realZ / (CHUNK_SIZE - 1);
		
		}
	}
	
	bool north = 0;
	bool south = 0;
	bool east = 0;
	bool west = 0;
	
	int i = 0;
	for(int z = 0; z < verticesPerLine-1; z++) {
		for(int x = 0; x < verticesPerLine-1; x++) {
			int topLeft     = z * verticesPerLine + x;
			int topRight    = topLeft + 1;
			int bottomLeft  = (z + 1) * verticesPerLine + x;
			int bottomRight = bottomLeft + 1;
            
            // THIS IS THE UGLIEST CODE I HAVE WRITTEN IN MY LIFE 
            if(lod == 0){
                north = (z == 0);
            	south = (z == verticesPerLine - 2);
            	east  = (x == verticesPerLine - 2);
            	west  = (x == 0);
            	
            	if(north){
            		if(x == 0){
            			indices[i++] = topLeft;
                        indices[i++] = bottomRight;
                        indices[i++] = topRight + 1;
                        
                        indices[i++] = topRight + 1;
                        indices[i++] = bottomRight;
                        indices[i++] = bottomRight + 1;
                        
            			indices[i++] = topLeft;
            			indices[i++] = bottomLeft + 1 * verticesPerLine;
                        indices[i++] = bottomRight;
                        
                        indices[i++] = bottomRight;
            			indices[i++] = bottomLeft + 1 * verticesPerLine;
            			indices[i++] = bottomRight + 1 * verticesPerLine;
            		}
            		else if(x == verticesPerLine - 3){
            			indices[i++] = topLeft;
                        indices[i++] = bottomLeft;
                        indices[i++] = bottomRight;
                        
                        indices[i++] = topLeft;
                        indices[i++] = bottomRight;
                        indices[i++] = topRight + 1;
                        
                        indices[i++] = topRight + 1;
                        indices[i++] = bottomRight;
                        indices[i++] = bottomRight + 1 + 1 * verticesPerLine;
                        
                        indices[i++] = bottomRight;
                        indices[i++] = bottomRight + 1 * verticesPerLine;
                        indices[i++] = bottomRight + 1 + 1 * verticesPerLine;
            		}
            		else if(x % 2 == 0){
                        indices[i++] = topLeft;
                        indices[i++] = bottomLeft;
                        indices[i++] = bottomRight;
                        
                        indices[i++] = topLeft;
                        indices[i++] = bottomRight;
                        indices[i++] = topRight + 1;
                        
                        indices[i++] = topRight + 1;
                        indices[i++] = bottomRight;
                        indices[i++] = bottomRight + 1;
                        
            		}
            	}
            	else if(south){ // SOUTH
            		if(x == 0){
            			
            		}
            		else if(x == verticesPerLine - 3){
            			indices[i++] = bottomLeft;
                        indices[i++] = topRight;
                        indices[i++] = topLeft;
                        
                        indices[i++] = topRight;
                        indices[i++] = bottomLeft;
                        indices[i++] = bottomRight + 1;
                        
                        indices[i++] = topRight;
                        indices[i++] = bottomRight + 1;
                        indices[i++] = topRight + 1 - 1 * verticesPerLine;
                        
                        indices[i++] = topRight - 1 * verticesPerLine;
                        indices[i++] = topRight;
                        indices[i++] = topRight + 1 - 1 * verticesPerLine;
            		}
            		else if(x % 2 == 0){
                        indices[i++] = bottomLeft;
                        indices[i++] = topRight;
                        indices[i++] = topLeft;
                        
                        indices[i++] = bottomLeft;
                        indices[i++] = bottomRight + 1;
                        indices[i++] = topRight;
                        
                        indices[i++] = bottomRight + 1;
                        indices[i++] = topRight + 1;
                        indices[i++] = topRight;
                        
            		}
            	}
            	else if (east){ // EAST
            		if(z == verticesPerLine - 3){
            			
            		}
            		else if(z % 2 == 0){
                    	indices[i++] = topLeft;
                        indices[i++] = bottomLeft;
                        indices[i++] = topRight;
            
                        indices[i++] = bottomLeft;
                        indices[i++] = bottomRight + 1 * verticesPerLine;
                    	indices[i++] = topRight;
                    	
                        indices[i++] = bottomLeft;
                        indices[i++] = bottomLeft + 1 * verticesPerLine;
                        indices[i++] = bottomRight + 1 * verticesPerLine;
                    }
            	}
            	else if(west){ // WEST
                    if(z == 0){
            
                    }
                    else if(z == verticesPerLine - 3){
                    	indices[i++] = topLeft;
                    	indices[i++] = bottomRight;
                        indices[i++] = topRight;
                        
                        indices[i++] = topLeft;
                        indices[i++] = bottomLeft + 1 * verticesPerLine;
                    	indices[i++] = bottomRight;
                    	
                        indices[i++] = bottomRight;
                        indices[i++] = bottomLeft + 1 * verticesPerLine;
                        indices[i++] = bottomRight + 1 + 1 * verticesPerLine;
                        
                        indices[i++] = bottomRight;
                        indices[i++] = bottomRight + 1  + 1 * verticesPerLine;
                        indices[i++] = bottomRight + 1;
                        
                    }
                    else if(z % 2 == 0){
                        indices[i++] = topLeft;
                    	indices[i++] = bottomRight;
                        indices[i++] = topRight;
            
                        indices[i++] = topLeft;
                        indices[i++] = bottomLeft + 1 * verticesPerLine;
                    	indices[i++] = bottomRight;
                    	
                    	indices[i++] = bottomRight;
                        indices[i++] = bottomLeft + 1 * verticesPerLine;
                        indices[i++] = bottomRight + 1 * verticesPerLine;
                    }
            	}
            	else{
                    indices[i++] = topLeft;
                    indices[i++] = bottomLeft;
                    indices[i++] = topRight;
            
                    indices[i++] = topRight;
                    indices[i++] = bottomLeft;
                    indices[i++] = bottomRight;
            	
            	}
            }
            else{
                indices[i++] = topLeft;
                indices[i++] = bottomLeft;
                indices[i++] = topRight;
        
                indices[i++] = topRight;
                indices[i++] = bottomLeft;
                indices[i++] = bottomRight;
            	
            }
		}
	}
	
	//configure the mesh
	glGenVertexArrays(1, &result.vao);
	glGenBuffers(1, &result.vbo);
	glGenBuffers(1, &result.ebo);
	
	glBindVertexArray(result.vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
	glBufferData(GL_ARRAY_BUFFER, VERTEX_COUNT * STRIDE * sizeof(float), vertices, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, result.index_count * sizeof(uint32), indices, GL_STATIC_DRAW);
	
	// position (location = 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	// tex coords (location = 1)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glBindVertexArray(0);
	
	SDL_free(vertices);
	SDL_free(indices);
	
	return result;
}

bool32 
chunkExist(Terrain_chunk *t, int currentAvailableChunk, float x, float z){
	//TODO: THINK OF A BETTER WAY (MAYBE A HASH ??!)
	for(int i = 0; i < currentAvailableChunk; ++i){
		if(t[i].exists && 
		   t[i].pos.x == x && 
		   t[i].pos.z == z)
		{
			return true;
		}
	}
	return false;
}


int getLOD(int dx, int dz) {
    int dist = abs(dx) > abs(dz) ? abs(dx) : abs(dz); // Chebyshev distance
    if (dist <= 0) return 0;
    if (dist <= 2) return 1;
    if (dist <= 4) return 2;
    return 3;
}

void 
generateChunks(Terrain_chunk *terrainChunks, Mesh *terrainMesh, v3 cameraChunkPos, float viewDistance, int *currentAvailableChunk){
	// aggresivly reset the chunks every frame
	*currentAvailableChunk = 0;
	for(int i = 0; i < MAX_CHUNKS; i++){
		terrainChunks[i].exists = false;
	}
	// GENERATE THE CHUNKS
	for(int z = -viewDistance; z <= viewDistance; ++z){
		for(int x = -viewDistance; x <= viewDistance; ++x){
			int lod = getLOD(x, z);
			int cx = cameraChunkPos.x + x;
			int cz = cameraChunkPos.z + z;
			cx *= chunkHalfWidth;
			cz *= chunkHalfWidth;
			
			assert(*currentAvailableChunk < MAX_CHUNKS);
			terrainChunks[*currentAvailableChunk].mesh = &terrainMesh[lod];
			terrainChunks[*currentAvailableChunk].LOD = lod;
			terrainChunks[*currentAvailableChunk].pos = make_vec3(cx, 0.2f, cz);
			terrainChunks[*currentAvailableChunk].exists = true;
			++*currentAvailableChunk;
		}
	}
}

void
mesh_draw_indexed(Mesh *mesh, shader shader, v3 pos, mat4 vp){
	mat4 model = mat4_identity();
	model *= mat4_translate(pos);
	
	mat4 mvp = mat4_multiply(model, vp);
	use_Shader(shader);
	
	setShaderValue_mat4(shader, "mvp", mvp);
	
	glBindVertexArray(mesh->vao);
	
	// glBindTexture(GL_TEXTURE_2D, testTexture);
	
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void
mesh_draw_index_textured(Mesh *mesh, shader shader, v3 pos, mat4 vp, mat4 lightSpaceMatrix){
	mat4 model = mat4_identity();
	model *= mat4_translate(pos);
	
	mat4 mvp = mat4_multiply(model, vp);
	use_Shader(shader);
	
	setShaderValue_mat4(shader, "model", model);
	setShaderValue_mat4(shader, "lightSpaceMatrix", lightSpaceMatrix);
	setShaderValue_mat4(shader, "vp", vp);
	
	glBindVertexArray(mesh->vao);
	
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void
draw_infinite_grid(shader s, mat4 vp, v3 camPos, uint32 vao){
	use_Shader(s);
	setShaderValue_mat4(s, "gVP", vp);
	setShaderValue_v3(s, "cam_world_pos", camPos);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned char* 
extractFace(unsigned char* data, int x, int y,
                           int width, int faceSize, int channels)
{
    unsigned char* face = new unsigned char[faceSize * faceSize * channels];

    for (int row = 0; row < faceSize; row++) {
        memcpy(
            face + row * faceSize * channels,
            data + (( (y + row) * width + x ) * channels),
            faceSize * channels
        );
    }

    return face;
}

int
main(int argc, char** argv){
	if(!init()){
		//log
		printf("failed to initialize");
		return -1;
	}
	
	SDL_Window* mainWindow = SDL_CreateWindow("TERRAIN GENERATOR", 
                                SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_OPENGL
                                );
	// SDL_HideCursor();
	SDL_SetWindowRelativeMouseMode(mainWindow, true);
	
	if(mainWindow != NULL){
		SDL_GLContext ctx = SDL_GL_CreateContext(mainWindow);
		if(ctx == NULL){
			printf("Couldn't create opengl context Error: %s\n", SDL_GetError());
			return -1;
		}
		if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
		{
			printf("Failed to initialize GLAD\n");
			return -1;
		}
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);   
		
		printf("OpenGL version: %s \n", glGetString(GL_VERSION));
		printf("GLSL version: %s \n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		
		//INTIALIZE IMGUI
		IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplSDL3_InitForOpenGL(mainWindow, ctx);
        ImGui_ImplOpenGL3_Init();
		
		//SEARCH: this is not the time for this but see if there is a group allocation (arena for example)
		//to handle gpu allocations 
		//load data and shaders
		uint32 infinteGrid_VBO; // this is here just because opengl demands it
		uint32 infinteGrid_VAO;
		glGenVertexArrays(1, &infinteGrid_VAO);
		glBindVertexArray(infinteGrid_VAO);
		// glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vert_data), &plane_vert_data, GL_STATIC_DRAW);  
		glBindVertexArray(0);
		
		const int MAX_LOD = 4;
		Mesh terrainMesh[MAX_LOD] = {};
		for(int i = 0; i < MAX_LOD; ++i){
			terrainMesh[i] = create_grid_mesh_data(i);
		}
		
		// TODO: THINK OF A BETTER WAY 
		Terrain_chunk terrainChunks[MAX_CHUNKS] = {0};
		int currentAvailableChunk = 0;
		
		//TODO: see if better to make the view distance in terms of chunks 
		// float maxViewDistance = 500.0f;
		float viewDistance = 5.0f;
		v3 lastCameraChunkPos = make_vec3(99999.0f, 0.0f, 99999.0f);

		shader infinte_grid_shader = make_shader("../shaders/infinte_grid.vert", "../shaders/infinte_grid.frag");
		shader terrain_shader = make_shader("../shaders/terrain.vert", "../shaders/terrain.frag");
		shader skyBox_shader = make_shader("../shaders/skybox.vert", "../shaders/skybox.frag");
		shader simpleShadow = make_shader("../shaders/simpleShadow.vert", "../shaders/simpleShadow.frag");
		
		unsigned int skyboxVAO, skyboxVBO;
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		
		uint32 cubemapTexture = 0;
		int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(false);
        unsigned char *cubeData = stbi_load("../data/skybox.png", &width, &height, &nrComponents, 0);
        if (cubeData)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
    
            //TODO: use uv mapping instead of copyint parts of the texture
            int faceSize = width / 4;
            unsigned char* negX = extractFace(cubeData, 0 * faceSize, 1 * faceSize, width, faceSize, nrComponents); // -X
            unsigned char* posZ = extractFace(cubeData, 1 * faceSize, 1 * faceSize, width, faceSize, nrComponents); // +Z
            unsigned char* posX = extractFace(cubeData, 2 * faceSize, 1 * faceSize, width, faceSize, nrComponents); // +X
            unsigned char* negZ = extractFace(cubeData, 3 * faceSize, 1 * faceSize, width, faceSize, nrComponents); // -Z
            unsigned char* posY = extractFace(cubeData, 1 * faceSize, 0 * faceSize, width, faceSize, nrComponents); // +Y
            unsigned char* negY = extractFace(cubeData, 1 * faceSize, 2 * faceSize, width, faceSize, nrComponents); // -Y
            
            glGenTextures(1, &cubemapTexture);
            // glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, posX); // +X
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, negX); // -X
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, posY); // +Y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, negY); // -Y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, posZ); // +Z
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, negZ); // -Z
            
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
            
            delete[] negX;
            delete[] posZ;
            delete[] posX;
            delete[] negZ;
            delete[] posY;
            delete[] negY;
            stbi_image_free(cubeData);
        }
        else
        {
            printf("failed to load cubeMap \n");
            stbi_image_free(cubeData);
        }

        // CREATE DEPTH BUFFER (SHADOW MAP)
        uint32 depthMap;
        uint32 depthMapFBO;
        int shadowWidth  = 8192;
        int shadowHeight = 8192;
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  

        glGenFramebuffers(1, &depthMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		input gameInput = {};
		
		uint32 frameStartTime = 0;
		uint32 frameEndTime = 0;
		float dt = 0.0f;
		
		SDL_Event e = {};
		
		Camera camera = {};
		camera.pos = make_vec3(0.0f, 1020.0f, 3.0f);
		camera.up = make_vec3(0.0f, 1.0f, 0.0f);
		camera.yaw = -90.0f;
		camera.pitch = 0.0f;
		float cameraSpeed = 2.5f;

		ProgramModes mode = FREE_CAM_MODE;
        int octaves = 10;
        float lacunarety = 2.0f;
        float persistance = 0.5f;
        float scale = 1000.0f;
        float heightMultipier = 600.0f;
		
		// v3 lightDir = v3_normalize(v3{1.0f, 1.0f, 0.5f});
		v3 lightDir = v3_normalize(v3{0.8f, 0.4f, 0.2f});
		mat4 projection = mat4_identity();
		projection *= mat4_perspective(radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
        mat4 shadowFitProjection = mat4_perspective_finite(radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 1000.0);
        
        mat4 lightOrtho = mat4_identity();
        // lightOrtho *= mat4_orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
        v4 frustumCorners[8] = {};
        
		bool32 wireMode = false;
		
		setShaderValue_i1(skyBox_shader, "skybox", 0);
		setShaderValue_mat4(skyBox_shader, "projection", projection);
        bool32 once = false;
		frameStartTime = SDL_GetTicks();
		while(Running){
			//handleInput
			while(SDL_PollEvent(&e) != 0){
                ImGui_ImplSDL3_ProcessEvent(&e);
				if(e.type == SDL_EVENT_QUIT){
					Running = false;
				}
				if(e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat){
					if(e.key.key == SDLK_ESCAPE){
						Running = false;
					}
					
					if(e.key.key == SDLK_R){
						wireMode = true;
					}
					if(e.key.key == SDLK_T){
						wireMode = false;
					}
					if(e.key.key == SDLK_1){
						SDL_SetWindowRelativeMouseMode(mainWindow, true);
						mode = FREE_CAM_MODE;
					}
					if(e.key.key == SDLK_2){
                        SDL_SetWindowRelativeMouseMode(mainWindow, false);
						mode = EDITOR_MODE;
					}
					
				}
				if(e.type == SDL_EVENT_MOUSE_MOTION){
					float dx, dy;
					SDL_GetRelativeMouseState(&dx, &dy);
					// if(mode == FREE_CAM_MODE && !io.WantCaptureMouse)
					if(mode == FREE_CAM_MODE)
                        playerMouseInput(dx, dy, &camera.pitch, &camera.yaw);
				}
			}
			const bool* curruntKeyState = SDL_GetKeyboardState(NULL);
			playerInputHandle(&gameInput, curruntKeyState);

			
			if(mode == FREE_CAM_MODE){
                
                if(gameInput.sprint){
                    cameraSpeed = 100.0f;
                }
                else{
                    cameraSpeed = 20.0f;
                }
                
                if(gameInput.forward){
                    camera.pos += camera.front * cameraSpeed * dt;
                }
                else if(gameInput.backward){
                    camera.pos -= camera.front * cameraSpeed * dt;
                }
                
                if(gameInput.left){
                    camera.pos -= v3_normalize(v3_cross(camera.front, camera.up)) * cameraSpeed * dt;
                }
                else if(gameInput.right){
                    camera.pos += v3_normalize(v3_cross(camera.front, camera.up)) * cameraSpeed * dt;
                }
                
                if(gameInput.up){
                    camera.pos.y += cameraSpeed * dt;
                }
                else if(gameInput.down){
                    camera.pos.y -= cameraSpeed * dt;
                }
			}
			else if(mode == EDITOR_MODE){
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL3_NewFrame();
                ImGui::NewFrame();
                // ImGui::ShowDemoWindow();
                //TEST CODE
                bool showTerrainWindow = true;
                bool showStatsWindow = true;
                
                if (ImGui::BeginMainMenuBar())
                {
                    if (ImGui::BeginMenu("Window"))
                    {
                        ImGui::MenuItem("Terrain", NULL, &showTerrainWindow);
                        ImGui::MenuItem("Statistics", NULL, &showStatsWindow);
                
                        ImGui::EndMenu();
                    }
                
                    ImGui::EndMainMenuBar();
                }

                if (showTerrainWindow)
                {
                    ImGui::Begin("Terrain");
                
                    // ImGui::SliderFloat("View Distance",
                                    // &viewDistance,
                                    // 1.0f,
                                    // 20.0f);
                    
                    ImGui::SliderInt("octaves", &octaves, 1, 20);
                    ImGui::SliderFloat("lacunarety", &lacunarety, 1.0f, 10.0f);
                    ImGui::SliderFloat("persistance", &persistance, 0.5f, 10.0f);
                    ImGui::SliderFloat("scale", &scale, 10.0f, 5000.0f);
                    ImGui::SliderFloat("heightMultipier", &heightMultipier, 10.0f, 1000.0f);
                    
                
                    ImGui::End();
                }
                
                if (showStatsWindow)
                {
                    ImGui::Begin("Stats");
                
                    ImGui::Text("FPS: %.1f", 1.0f / dt);
                    ImGui::Text("Chunks: %d", currentAvailableChunk);
                
                    ImGui::End();
                }
                
			}
			
			//update
			camera.direction.x = cosf(radians(camera.yaw)) * cosf(radians(camera.pitch));
			camera.direction.y = sinf(radians(camera.pitch));
			camera.direction.z = sinf(radians(camera.yaw)) * cosf(radians(camera.pitch));
			camera.front = v3_normalize(camera.direction);
			v3 cameraChunkPos = make_vec3(
                floorf((camera.pos.x + chunkHalfWidth * 0.5f) / chunkHalfWidth),
                0.0f,
                floorf((camera.pos.z + chunkHalfWidth * 0.5f) / chunkHalfWidth)
            );
			
			if(cameraChunkPos.x != lastCameraChunkPos.x ||
			   cameraChunkPos.z != lastCameraChunkPos.z)
			{
				generateChunks(terrainChunks, terrainMesh, cameraChunkPos, viewDistance, &currentAvailableChunk);
				lastCameraChunkPos = cameraChunkPos;
			}
			
			mat4 view = mat4_lookAt(camera.pos, camera.pos + camera.front, camera.up);
			mat4 vp = view * projection;
			mat4 shadowFitVP = view * shadowFitProjection;
            
            get_frustum_corners_world_space(vp, frustumCorners);
			v3 center = {0};
			for(int i = 0; i < 8; ++i){
				center += make_vec3(frustumCorners[i]);
			}
			center /= 8.0f;
			
			mat4 lightView = mat4_lookAt(center + lightDir, center, make_vec3(0.0f, 1.0f, 0.0f));
			
			float minX =  FLT_MAX;
			float maxX = -FLT_MAX;
			float minY =  FLT_MAX;
			float maxY = -FLT_MAX;
			float minZ =  FLT_MAX;
			float maxZ = -FLT_MAX;

			for(int i = 0; i < 8; ++i){
				v4 trf = frustumCorners[i] * lightView;
				minX = min(minX, trf.x);
				maxX = max(maxX, trf.x);
				minY = min(minY, trf.y);
				maxY = max(maxY, trf.y);
				minZ = min(minZ, trf.z);
				maxZ = max(maxZ, trf.z);
			}
			
			
			// printf("Light Proj min/max: %.1f %.1f %.1f %.1f\n", minX, maxX, minY, maxY);
			// printf("Light Dir: %.2f %.2f %.2f\n", lightDir.x, lightDir.y, lightDir.z);
			// printf("camera.x: %f, camera.y: %f, camera.z: %f\n", camera.pos.x, camera.pos.y, camera.pos.z);
			
			mat4 lightProj = mat4_orthographic(
                minX - 800.0f, maxX + 800.0f, 
                minY - 800.0f, maxY + 800.0f, 
                minZ - 800.0f, maxZ + 800.0f
            );
			mat4 lightSpaceMatrix = lightView * lightProj;
			
			//RENDER SHADOW MAP
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glViewport(0, 0, shadowWidth, shadowHeight);
			glClear(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			use_Shader(simpleShadow);
			for(int i = 0; i < currentAvailableChunk; ++i){
				mat4 model = mat4_identity();
                model *= mat4_translate(terrainChunks[i].pos);
                
                setShaderValue_mat4(simpleShadow, "model", model);
                setShaderValue_mat4(simpleShadow, "lightSpaceMatrix", lightSpaceMatrix);

                setShaderValue_i1(simpleShadow, "Uoctaves", octaves);
                setShaderValue_f1(simpleShadow, "Ulacunarety", lacunarety);
                setShaderValue_f1(simpleShadow, "Upersistance", persistance);
                setShaderValue_f1(simpleShadow, "Uscale", scale);
                setShaderValue_f1(simpleShadow, "UheightMultipier", heightMultipier);
                
                glBindVertexArray(terrainChunks[i].mesh->vao);
                
                glDrawElements(GL_TRIANGLES, terrainChunks[i].mesh->index_count, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
			}
			//RENDER SCENE
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			// player chunk position
			
			
			//DRAW skybox
			glDepthMask(GL_FALSE);
            use_Shader(skyBox_shader);
            mat4 skyView = mat3_to_mat4(mat4_to_mat3(view));
            setShaderValue_mat4(skyBox_shader, "view", skyView);
            // ... set view and projection matrix
            glBindVertexArray(skyboxVAO);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthMask(GL_TRUE);
			
			
			//RENDER CHUNKS
            if(wireMode)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            
            setShaderValue_v3(terrain_shader, "viewPos", camera.pos);
            for(int i = 0; i < currentAvailableChunk; ++i){
                setShaderValue_i1(terrain_shader, "Uoctaves", octaves);
                setShaderValue_f1(terrain_shader, "Ulacunarety", lacunarety);
                setShaderValue_f1(terrain_shader, "Upersistance", persistance);
                setShaderValue_f1(terrain_shader, "Uscale", scale);
                setShaderValue_f1(terrain_shader, "UheightMultipier", heightMultipier);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, depthMap);
                setShaderValue_i1(terrain_shader, "shadowMap", 0);
                mesh_draw_index_textured(terrainChunks[i].mesh, terrain_shader, terrainChunks[i].pos, vp, lightSpaceMatrix);
            }

			//UI RENDERERING 
			if(mode == EDITOR_MODE){
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}

			SDL_GL_SwapWindow(mainWindow);
    
            #ifdef GAME_SLOW
			// printf("dt: %fms\n", dt * 1000.0);
			#endif
			
			frameEndTime = SDL_GetTicks();
			dt = (frameEndTime - frameStartTime) / 1000.0f;
			frameStartTime = frameEndTime;
			SDL_Delay(1);
		}
		
		shaderDelete(&infinte_grid_shader);
		shaderDelete(&terrain_shader);

		ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
	}

}
