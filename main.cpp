// note: the height map should be generated only once but for various reasons i can't do this
// TODO:-
// - add grass (instanced also how i will do this ?????!!!!!)
// - fix lod if i have time 


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
// #define CHUNK_SIZE 241
#define CHUNK_SIZE 257

const int MAP_WIDTH  = 50;
const int MAP_HEIGHT = 50;
# define MAX_CHUNKS MAP_WIDTH * MAP_HEIGHT
const float chunkHalfWidth = (CHUNK_SIZE - 1) / 2.0;
const int MAX_LOD = 6;
const int MAX_CHUNK_PER_LEVEL = 1650;
const int MAX_GRASS_INSTANCES = 10000000;

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
	uint32 normal;
	uint32 height;
	uint32 roughness;
	uint32 metallic;
	uint32 ao;
};

struct Mesh{
	uint32 vbo;
	uint32 vao;
	uint32 ebo;

	uint32 instanceVBO;
	
	uint32 index_count;

    PBRMaterial material[2];
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

uint32 Hash(uint32 x)
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

float Random01(int x, int z)
{
    uint32 h = Hash((uint32)x * 73856093u ^ (uint32)z * 19349663u);
    return (float)h / 4294967295.0f;
}

float Random01(uint32 seed)
{
    seed ^= seed >> 16;
    seed *= 0x7feb352d;
    seed ^= seed >> 15;
    seed *= 0x846ca68b;
    seed ^= seed >> 16;

    return (float)seed / (float)UINT32_MAX;
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

uint32 loadTextureSRGB(char const * path, GLint edgeDealing)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        GLenum internalFormat;
        if (nrComponents == 1)
		{
			format = GL_RED;
			internalFormat = GL_R8;
		}
		else if (nrComponents == 3)
		{
			format = GL_RGB;
			internalFormat = GL_SRGB8;
		}
		else if (nrComponents == 4)
		{
			format = GL_RGBA;
			internalFormat = GL_SRGB8_ALPHA8;
		}

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
		
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, edgeDealing);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, edgeDealing);
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

uint32 
loadTextureRGB(char const * path, GLint edgeDealing)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
		{
			format = GL_RED;
		}
		else if (nrComponents == 3)
		{
			format = GL_RGB;
		}
		else if (nrComponents == 4)
		{
			format = GL_RGBA;
		}

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, edgeDealing);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, edgeDealing);
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

Mesh
create_grass_blade(){
    Mesh result = {};

    const int numQuads = 3;
    const float angleStep = PI / numQuads; // 60 degrees apart (half circle is enough since culling is off)

    const int vertexCount = numQuads * 4;
    const int indexCount  = numQuads * 6;
    const int stride = 5; // x, y, z, u, v

    float vertices[vertexCount * stride] = {0};
    uint32 indices[indexCount] = {0};

    // base quad: centered on X, base at Y=0, tip at Y=1
    float baseQuad[4][5] = {
        {-0.5f, 0.0f, 0.0f, 0.0f, 0.0f}, // bottom-left
        { 0.5f, 0.0f, 0.0f, 1.0f, 0.0f}, // bottom-right
        { 0.5f, 1.0f, 0.0f, 1.0f, 1.0f}, // top-right
        {-0.5f, 1.0f, 0.0f, 0.0f, 1.0f}, // top-left
    };

    int vIdx = 0, iIdx = 0;
    for(int q = 0; q < numQuads; ++q){
        float angle = q * angleStep;
        float c = cosf(angle);
        float s = sinf(angle);

        uint32 base = (uint32)(vIdx / stride);

        for(int v = 0; v < 4; ++v){
            float x = baseQuad[v][0];
            float y = baseQuad[v][1];
            float z = baseQuad[v][2];

            float rx = x*c - z*s;
            float rz = x*s + z*c;

            vertices[vIdx++] = rx;
            vertices[vIdx++] = y;
            vertices[vIdx++] = rz;
            vertices[vIdx++] = baseQuad[v][3];
            vertices[vIdx++] = baseQuad[v][4];
        }

        indices[iIdx++] = base + 0;
        indices[iIdx++] = base + 1;
        indices[iIdx++] = base + 2;
        indices[iIdx++] = base + 0;
        indices[iIdx++] = base + 2;
        indices[iIdx++] = base + 3;
    }

    result.index_count = indexCount;

    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);

    glBindVertexArray(result.vao);

    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * stride * sizeof(float), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32), indices, GL_STATIC_DRAW);

    // position (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texcoord (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &result.instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, result.instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, MAX_GRASS_INSTANCES * sizeof(v3), NULL, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(v3), (void*)0);
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    return result;
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
	// float spacing = 0.5f;
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

	//FOR INSTANCING 
	glBindVertexArray(result.vao);

    glGenBuffers(1, &result.instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, result.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_CHUNK_PER_LEVEL * sizeof(v3), NULL, GL_DYNAMIC_DRAW); // pre-allocate worst case

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(v3), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // advance once per instance, not per vertex

    glBindVertexArray(0);
	
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
    if (dist <= 6) return 3;
    if (dist <= 8) return 4;
    return 5;
}

float grassSpacingForLOD(int chunkLOD){
    switch(chunkLOD){
        case 0: return 0.8f;
        case 1: return 1.05f;
        case 2: return 4.0f;
        case 3: return 8.0f;
        default: return 0.0f;
    }
}

float mix(float a, float b, float t) {
    return a + (b - a) * t;
}

float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}


float grassSpacingForDistance(float distanceToCamera, float nearDist, float farDist, float minSpacing, float maxSpacing){
    float t = clampf((distanceToCamera - nearDist) / (farDist - nearDist), 0.0f, 1.0f);
    return mix(minSpacing, maxSpacing, t); 
}

void 
generateChunks(Terrain_chunk terrainChunks[MAX_LOD][MAX_CHUNK_PER_LEVEL], Mesh *terrainMesh, v3 cameraChunkPos, float viewDistance, int lodCounts[MAX_LOD]){
	// aggresivly reset the chunks every frame
	// *currentAvailableChunk = 0;
	// for(int i = 0; i < MAX_CHUNKS; i++){
		// terrainChunks[i].exists = false;
	// }
	for(int i = 0; i < MAX_LOD; ++i){
		lodCounts[i] = 0;
	}
	// lodCounts = {0};
	// GENERATE THE CHUNKS
	for(int z = -viewDistance; z <= viewDistance; ++z){
		for(int x = -viewDistance; x <= viewDistance; ++x){
			int lod = getLOD(x, z);
			int lodIDX = 0;
			switch(lod){
				case 0: lodIDX = lodCounts[0]++;
					break;
				case 1: lodIDX = lodCounts[1]++;
					break;
				case 2: lodIDX = lodCounts[2]++;
					break;
				case 3: lodIDX = lodCounts[3]++;
					break;
				case 4: lodIDX = lodCounts[4]++;
					break;
				case 5: lodIDX = lodCounts[5]++;
					break;
			}
			int cx = cameraChunkPos.x + x;
			int cz = cameraChunkPos.z + z;
			cx *= chunkHalfWidth;
			cz *= chunkHalfWidth;
			
			// assert(currentAvailableChunk < MAX_CHUNKS);
			// printf("%i \n",lodIDX);
			assert(lodIDX < MAX_CHUNK_PER_LEVEL);
			terrainChunks[lod][lodIDX].mesh = &terrainMesh[lod];
			terrainChunks[lod][lodIDX].LOD = lod;
			terrainChunks[lod][lodIDX].pos = make_vec3(cx, 0.2f, cz);
			terrainChunks[lod][lodIDX].exists = true;
		}
	}
}

void
mesh_draw_indexed(Mesh *mesh, shader shader, v3 pos, mat4 vp){
	mat4 model = mat4_identity();
	model *= mat4_translate(pos);
	
	mat4 mvp = model * vp;
	use_Shader(shader);
	
	setShaderValue_mat4(shader, "mvp", mvp);
	
	glBindVertexArray(mesh->vao);
	
	
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void
mesh_draw_index_textured(Mesh *mesh, shader shader, v3 pos, mat4 vp, mat4 lightSpaceMatrix){
	mat4 model = mat4_identity();
	model *= mat4_translate(pos);
	
	use_Shader(shader);
	
	setShaderValue_mat4(shader, "model", model);
	setShaderValue_mat4(shader, "lightSpaceMatrix", lightSpaceMatrix);
	setShaderValue_mat4(shader, "vp", vp);
	
	glBindVertexArray(mesh->vao);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mesh->material[0].albedo);
	setShaderValue_i1(shader, "grassAlbedo", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, mesh->material[0].normal);
	setShaderValue_i1(shader, "grassNormal", 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, mesh->material[0].height);
	setShaderValue_i1(shader, "grassHeight", 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, mesh->material[0].roughness);
	setShaderValue_i1(shader, "grassRoughness", 4);
	
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, mesh->material[0].metallic);
	setShaderValue_i1(shader, "grassMetallic", 5);
	
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, mesh->material[0].ao);
	setShaderValue_i1(shader, "grassAO", 6);
	
	
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
		// SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
		// glEnable(GL_FRAMEBUFFER_SRGB); 
		
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

		stbi_set_flip_vertically_on_load(true);
		PBRMaterial grassGround = {};
		PBRMaterial dirt  = {};
		grassGround.albedo    = loadTextureSRGB("../data/moss/mixedmoss-albedo2.png", GL_REPEAT);
		grassGround.normal    = loadTextureRGB("../data/moss/mixedmoss-normal2.png", GL_REPEAT);
		grassGround.height    = loadTextureRGB("../data/moss/mixedmoss-height.png", GL_REPEAT);
		grassGround.roughness = loadTextureRGB("../data/moss/mixedmoss-roughness.png", GL_REPEAT);
		grassGround.metallic  = loadTextureRGB("../data/moss/mixedmoss-metalness.png", GL_REPEAT);
		grassGround.ao 	   = loadTextureRGB("../data/moss/mixedmoss-ao2.png", GL_REPEAT);
		
		dirt.albedo = loadTextureSRGB("../data/dirt/dry-dirt1-albedo.png", GL_REPEAT);

		uint32 grassBladeTexture = loadTextureSRGB("../data/grass.png", GL_REPEAT);
		
		Mesh terrainMesh[MAX_LOD] = {};
		for(int i = 0; i < MAX_LOD; ++i){
			terrainMesh[i] = create_grid_mesh_data(i);
			terrainMesh[i].material[0] = grassGround;
			terrainMesh[i].material[1] = dirt;
		}

		Mesh grassBladeMesh = create_grass_blade();
		
		// how wide is a chunk in world units?

		const float spacing = 0.5f;
		int grassCount = 0;
		// v3 grassPositions[MAX_GRASS_INSTANCES];
		v3 *grassPositions = (v3*)SDL_malloc(MAX_GRASS_INSTANCES * sizeof(v3));
		const float chunkWorldSize = (CHUNK_SIZE - 1) * spacing;
		
		const int bladesPerSide = (int)(chunkWorldSize / 1.0f); //  unit spacing
		const int maxBladesPerChunk = bladesPerSide * bladesPerSide;
		
		// v3 grassLocalPositions[maxBladesPerChunk];
		v3 *grassLocalPositions = (v3*)SDL_malloc(maxBladesPerChunk * sizeof(v3)); 
		int idx = 0;
		for(int z = 0; z < bladesPerSide; ++z){
			for(int x = 0; x < bladesPerSide; ++x){
				float lx = x * 1.0f - chunkWorldSize * 0.5f; // center the grid on the chunk
				float lz = z * 1.0f - chunkWorldSize * 0.5f;
				grassLocalPositions[idx++] = make_vec3(lx, 0.0f, lz);
			}
		}
		
		// TODO: THINK OF A BETTER WAY 
		// Terrain_chunk terrainChunks[MAX_CHUNKS] = {0};
		Terrain_chunk terrainChunks[MAX_LOD][MAX_CHUNK_PER_LEVEL] = {0}; 
		int lodCounts[MAX_LOD] = {0};
		//TODO: see if better to make the view distance in terms of chunks 
		// float maxViewDistance = 500.0f;
		int viewDistance = 5;
		int GrassMaxLOD = 0;
		
		
		v3 lastCameraChunkPos = make_vec3(99999.0f, 0.0f, 99999.0f);

		shader infinte_grid_shader = make_shader("../shaders/infinte_grid.vert", "../shaders/infinte_grid.frag");
		shader terrain_shader = make_shader("../shaders/terrain.vert", "../shaders/terrain.frag");
		shader skyBox_shader = make_shader("../shaders/skybox.vert", "../shaders/skybox.frag");
		shader simpleShadow = make_shader("../shaders/simpleShadow.vert", "../shaders/simpleShadow.frag");
		shader grass_shader = make_shader("../shaders/grass.vert", "../shaders/grass.frag");
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
			GLenum internalFormat;
			
			if (nrComponents == 1)
			{
				format = GL_RED;
				internalFormat = GL_R8;
			}
			else if (nrComponents == 3)
			{
				format = GL_RGB;
				internalFormat = GL_SRGB8;
			}
			else if (nrComponents == 4)
			{
				format = GL_RGBA;
				internalFormat = GL_SRGB8_ALPHA8;
			}
    
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
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, posX); // +X
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, negX); // -X
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, posY); // +Y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, negY); // -Y
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, posZ); // +Z
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, negZ); // -Z
            
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
		camera.pos = make_vec3(0.0f, 800.0f, 3.0f);
		camera.up = make_vec3(0.0f, 1.0f, 0.0f);
		camera.yaw = -90.0f;
		camera.pitch = 0.0f;
		float cameraSpeed = 2.5f;

		ProgramModes mode = FREE_CAM_MODE;
        int octaves = 10;
        float lacunarety = 1.85f;
        float persistance = 0.49f;
        float scale = 1000.0f;
        float heightMultipier = 500.0f;
		
		// v3 lightDir = v3_normalize(v3{1.0f, 1.0f, 0.5f});
		v3 lightDir = v3_normalize(make_vec3(0.8f, 0.4f, 0.2f));
		mat4 projection = mat4_identity();
		projection = mat4_perspective(radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 2000.0f);
        mat4 shadowFitProjection = mat4_perspective_finite(radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 3000.0);
        
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
                    cameraSpeed = 15.0f;
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
                
                    ImGui::SliderInt("View Distance",
                                    &viewDistance,
                                    1,
                                    20);
                    
                    ImGui::SliderInt("octaves", &octaves, 1, 20);
                    ImGui::SliderFloat("lacunarety", &lacunarety, 1.0f, 10.0f);
                    ImGui::SliderFloat("persistance", &persistance, 0.5f, 10.0f);
                    ImGui::SliderFloat("scale", &scale, 10.0f, 5000.0f);
                    ImGui::SliderFloat("heightMultipier", &heightMultipier, 10.0f, 1000.0f);
                    ImGui::SliderInt("GrassDistance", &GrassMaxLOD, 0, 4);
                
                    ImGui::End();
                }
                
                if (showStatsWindow)
                {
                    ImGui::Begin("Stats");
                
                    ImGui::Text("FPS: %.1f", 1.0f / dt);
                    // ImGui::Text("Chunks: %d", currentAvailableChunk);
                    ImGui::Text("lod0 chunks: %d", lodCounts[0]);
                    ImGui::Text("lod1 chunks: %d", lodCounts[1]);
                    ImGui::Text("lod2 chunks: %d", lodCounts[2]);
                    ImGui::Text("lod3 chunks: %d", lodCounts[3]);
                    ImGui::Text("lod4 chunks: %d", lodCounts[4]);
                    ImGui::Text("lod5 chunks: %d", lodCounts[5]);
                    ImGui::Text("Grass Count: %d", grassCount);
                
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
			
			v3 lodPositions[MAX_LOD][MAX_CHUNK_PER_LEVEL]; // temp veriable
			if(cameraChunkPos.x != lastCameraChunkPos.x ||
			   cameraChunkPos.z != lastCameraChunkPos.z)
			{
				generateChunks(terrainChunks, terrainMesh, cameraChunkPos, viewDistance, lodCounts);
				lastCameraChunkPos = cameraChunkPos;

				for(int lod = 0; lod < MAX_LOD; ++lod){
					for(int i = 0; i < lodCounts[lod]; ++i){
						lodPositions[lod][i] = terrainChunks[lod][i].pos;
					}
			
					if(lodCounts[lod] > 0){
						glBindBuffer(GL_ARRAY_BUFFER, terrainMesh[lod].instanceVBO);
						glBufferSubData(GL_ARRAY_BUFFER, 0, lodCounts[lod] * sizeof(v3), lodPositions[lod]);
					}
				}
				
				grassCount = 0;
				for(int lod = 0; lod <= GrassMaxLOD; ++lod){
					float bladeSpacing = grassSpacingForLOD(lod) / 2.0f;
					if(bladeSpacing <= 0.0f) continue;
					
					int bladesPerSideThisLOD = (int)(chunkWorldSize / bladeSpacing);
					
					for(int i = 0; i < lodCounts[lod]; ++i){
						v3 chunkPos = terrainChunks[lod][i].pos;
						for(int z = 0; z <= bladesPerSideThisLOD; ++z){
							for(int x = 0; x <= bladesPerSideThisLOD; ++x){
								assert(grassCount < MAX_GRASS_INSTANCES);
								uint32 seed = x +
											  z * 73856093u +
											  i * 19349663u +
											  lod * 83492791u;
								
								float rx = (Random01(seed)     - 0.5f) * bladeSpacing;
								float rz = (Random01(seed + 1) - 0.5f) * bladeSpacing;
								float lx = x * bladeSpacing - chunkWorldSize * 0.5f;
								float lz = z * bladeSpacing - chunkWorldSize * 0.5f;
								grassPositions[grassCount].x = chunkPos.x + lx + rx;
								grassPositions[grassCount].y = 0.0f;
								grassPositions[grassCount].z = chunkPos.z + lz + rz;
								grassCount++;
							}
						}
					}
				}
				// grassCount = 0;
				// for(int lod = 0; lod <= 3; ++lod){
					// for(int i = 0; i < lodCounts[lod]; ++i){
						// v3 chunkPos = terrainChunks[lod][i].pos;
						// float dx = chunkPos.x - camera.pos.x;
						// float dz = chunkPos.z - camera.pos.z;
						// float dist = sqrtf(dx*dx + dz*dz);
						// float bladeSpacing = grassSpacingForDistance(dist, 0.0f, 500.0f, 0.5f, 8.0f);
						
						// int bladesPerSideThisLOD = (int)(chunkWorldSize / bladeSpacing);
				
						// for(int z = 0; z < bladesPerSideThisLOD; ++z){
							// for(int x = 0; x < bladesPerSideThisLOD; ++x){
								// assert(grassCount < MAX_GRASS_INSTANCES);
								// float lx = x * bladeSpacing - chunkWorldSize * 0.5f;
								// float lz = z * bladeSpacing - chunkWorldSize * 0.5f;
								// grassPositions[grassCount].x = chunkPos.x + lx;
								// grassPositions[grassCount].y = 0.0f;
								// grassPositions[grassCount].z = chunkPos.z + lz;
								// grassCount++;
							// }
						// }
					// }
				// }
				
				// glBindBuffer(GL_ARRAY_BUFFER, grassBladeMesh.instanceVBO);
				// glBufferSubData(GL_ARRAY_BUFFER, 0, grassCount * sizeof(v3), grassPositions);
				// grassCount = 0;
				// int grassViewDistance = grassScaler * chunkWorldSize;
				// float bladeSpacing = 0.4;
				// for(int z = -grassViewDistance; z < grassViewDistance; ++z){
					// for(int x = -grassViewDistance; x < grassViewDistance; ++x){
						// Assert(grassCount < MAX_GRASS_INSTANCES);
						// float bladeX = camera.pos.x + bladeSpacing * x;
						// float bladeZ = camera.pos.z + bladeSpacing * z;
						// float randX = Random01(x, z);
						// float randZ = Random01(z, x);
						// randX = randX * 2.0f - 1.0f;
						// randZ = randZ * 2.0f - 1.0f;
						// float maxOffset = bladeSpacing * 0.45f;

						// bladeX += randX * maxOffset;
						// bladeZ += randZ * maxOffset;
						// float dx = bladeX - camera.pos.x;
						// float dz = bladeZ - camera.pos.z;
						// float dist = sqrtf(dx * dx + dz * dz);
						// float t = clampf(dist / grassViewDistance, 0.0f, 1.0f);
						// float density = 1.0f - t;
						// if (Random01(bladeX, bladeZ) > density)
							// continue;
						// grassPositions[grassCount].x = bladeX; 
						// grassPositions[grassCount].y = 0.0f;
						// grassPositions[grassCount].z = bladeZ;
						// grassCount++;
					// }
				// }
				glBindBuffer(GL_ARRAY_BUFFER, grassBladeMesh.instanceVBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, grassCount * sizeof(v3), grassPositions);
			
			}
			
			
			mat4 view = mat4_lookAt(camera.pos, camera.pos + camera.front, camera.up);
			mat4 vp = view * projection;
			mat4 shadowFitVp = view * shadowFitProjection;
            
            get_frustum_corners_world_space(shadowFitVp, frustumCorners);
			// get_frustum_corners_world_space(vp, frustumCorners, camera.pos, camera.front, 2000.0f);
			v3 center = {};
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
			
			float orthoSize = 1500.0f;

			mat4 lightProj = mat4_orthographic(
				-orthoSize, orthoSize,
				-orthoSize, orthoSize,
				-2000.0f, 2000.0f
			);
			// mat4 lightProj = mat4_orthographic(minX, maxX, minY, maxY, minZ, maxZ);
            
			mat4 lightSpaceMatrix = lightView * lightProj;

			//Texel snapping 
			mat4 shadowMatrix = lightView * lightProj;
			v4 shadowOrigin = make_vec4(0.0f, 0.0f, 0.0f, 1.0f) * shadowMatrix;
			shadowOrigin = shadowOrigin * ((float)shadowWidth / 2.0f);
			
			v4 roundedOrigin = make_vec4(roundf(shadowOrigin.x), roundf(shadowOrigin.y), roundf(shadowOrigin.z), shadowOrigin.w);
			v4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * (2.0f / (float)shadowWidth);
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;
			
			mat4 shadowProj = lightProj;
			shadowProj.m[3][0] += roundOffset.x;
			shadowProj.m[3][1] += roundOffset.y;
			lightSpaceMatrix = lightView * shadowProj;

			//RENDER SHADOW MAP
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glViewport(0, 0, shadowWidth, shadowHeight);
			glClear(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			use_Shader(simpleShadow);
			// for(int i = 0; i < currentAvailableChunk; ++i){
			// mat4 model = mat4_identity();
            // model *= mat4_translate(terrainChunks[i].pos);
			
            // setShaderValue_mat4(simpleShadow, "model", model);
            setShaderValue_mat4(simpleShadow, "lightSpaceMatrix", lightSpaceMatrix);

            setShaderValue_i1(simpleShadow, "Uoctaves", octaves);
            setShaderValue_f1(simpleShadow, "Ulacunarety", lacunarety);
            setShaderValue_f1(simpleShadow, "Upersistance", persistance);
            setShaderValue_f1(simpleShadow, "Uscale", scale);
            setShaderValue_f1(simpleShadow, "UheightMultipier", heightMultipier);

            // glBindVertexArray(terrainChunks[i].mesh->vao);
            
            // glDrawElements(GL_TRIANGLES, terrainChunks[i].mesh->index_count, GL_UNSIGNED_INT, 0);
            for(int lod = 0; lod < MAX_LOD; ++lod){
				if(lodCounts[lod] == 0) continue;
				glBindVertexArray(terrainMesh[lod].vao);
				glDrawElementsInstanced(GL_TRIANGLES, terrainMesh[lod].index_count, GL_UNSIGNED_INT, 0, lodCounts[lod]);
			}
            glBindVertexArray(0);
			
			//RENDER SCENE
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
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
            
            
            setShaderValue_i1(terrain_shader, "Uoctaves", octaves);
            setShaderValue_f1(terrain_shader, "Ulacunarety", lacunarety);
            setShaderValue_f1(terrain_shader, "Upersistance", persistance);
            setShaderValue_f1(terrain_shader, "Uscale", scale);
            setShaderValue_f1(terrain_shader, "UheightMultipier", heightMultipier);
			setShaderValue_v3(terrain_shader, "viewPos", camera.pos);
            setShaderValue_v3(terrain_shader, "lightDir", lightDir);
            setShaderValue_mat4(terrain_shader, "vp", vp);
			setShaderValue_mat4(terrain_shader, "lightSpaceMatrix", lightSpaceMatrix);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            setShaderValue_i1(terrain_shader, "shadowMap", 0);
            
            for(int lod = 0; lod < MAX_LOD; ++lod){
                if(lodCounts[lod] == 0) continue;
				
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, terrainMesh[lod].material[0].albedo);
				setShaderValue_i1(terrain_shader, "grassAlbedo", 1);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, terrainMesh[lod].material[0].normal);
				setShaderValue_i1(terrain_shader, "grassNormal", 2);
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, terrainMesh[lod].material[0].height);
				setShaderValue_i1(terrain_shader, "grassHeight", 3);
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, terrainMesh[lod].material[0].roughness);
				setShaderValue_i1(terrain_shader, "grassRoughness", 4);
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, terrainMesh[lod].material[0].metallic);
				setShaderValue_i1(terrain_shader, "grassMetallic", 5);
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, terrainMesh[lod].material[0].ao);
				setShaderValue_i1(terrain_shader, "grassAO", 6);
			
				glBindVertexArray(terrainMesh[lod].vao);
				glDrawElementsInstanced(GL_TRIANGLES, terrainMesh[lod].index_count, GL_UNSIGNED_INT, 0, lodCounts[lod]);
                // mesh_draw_index_textured(terrainChunks[i].mesh, terrain_shader, terrainChunks[i].pos, vp, lightSpaceMatrix);
            }

			//GRASS DRAWING
			glDisable(GL_CULL_FACE);
			use_Shader(grass_shader);
			setShaderValue_mat4(grass_shader, "vp", vp);
			setShaderValue_i1(grass_shader, "Uoctaves", octaves);       // needs same terrain noise uniforms
			setShaderValue_f1(grass_shader, "Ulacunarety", lacunarety);
			setShaderValue_f1(grass_shader, "Upersistance", persistance);
			setShaderValue_f1(grass_shader, "Uscale", scale);
			setShaderValue_f1(grass_shader, "UheightMultipier", heightMultipier);
			// setShaderValue_v3(grass_shader, "lightDir", lightDir);
            
            setShaderValue_mat4(grass_shader, "lightSpaceMatrix", lightSpaceMatrix);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, grassBladeTexture);
			setShaderValue_i1(grass_shader, "bladeTexture", 0);
			
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			setShaderValue_i1(grass_shader, "shadowMap", 1);
			
			glBindVertexArray(grassBladeMesh.vao);
			glDrawElementsInstanced(GL_TRIANGLES, grassBladeMesh.index_count, GL_UNSIGNED_INT, 0, grassCount);
			glBindVertexArray(0);
			
			glEnable(GL_CULL_FACE); // restore for terrain/other opaque geometry

			
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
