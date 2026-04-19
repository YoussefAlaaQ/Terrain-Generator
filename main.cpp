#include <glad/glad.h>
#include "glad.c"

#include <SDL3/SDL.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_opengl.h>

#include <stdio.h>
#include <stdint.h>

#include "3dMath.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int*)0 = 0;}
#else 
#define Assert(Expression)
#endif

#define internal static
#define local_persist static
#define global_variable static

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 900

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

global_variable bool32 Running = true;


struct Camera{
	v3 pos	  ;
	v3 direction;
	v3 up       ;	
	v3 front    ;	
	float yaw   ;
	float pitch ;
	
};

struct Mesh{
	uint32 vbo;
	uint32 vao;
	uint32 ebo;
	
	uint32 index_count;

	uint32 texture;

	//NOTE: should i add other informations like the mesh pos and rotation

	// v3 position;
	// NOTE: could be handeled later
	// v3 rotation; 
	// v3 scale;
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
	bool32 exists;
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

#define CHUNK_SIZE 241.0

Mesh
create_grid_mesh_data(int lod){
	Mesh result = {};
	
	float meshSimplificationIncrument = 0;
	
	if(lod > 6){
		printf("ERR: lod can't exceed 6 \n");
		return result;
	}
	else if(lod == 0){
		meshSimplificationIncrument = 1;
	}
	else{
		meshSimplificationIncrument = lod * 2;
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
	
	for(int z = 0; z < verticesPerLine; z++) {
		for(int x = 0; x < verticesPerLine; x++) {
			int realX = x * meshSimplificationIncrument;
			int realZ = z * meshSimplificationIncrument;
	
			float xPos = (realX - CHUNK_SIZE / 2.0f) * spacing;
			float zPos = (realZ - CHUNK_SIZE / 2.0f) * spacing;
	
			vertices[idx++] = xPos;
			vertices[idx++] = 0.0f;
			vertices[idx++] = zPos;
			vertices[idx++] = (float)realX / CHUNK_SIZE;
			vertices[idx++] = (float)realZ / CHUNK_SIZE;
		
			lastXPos = xPos;
			lastZPos = zPos;
		}
	}
	
	printf("lastXPos: %f, lastZPos: %f \n", lastXPos, lastZPos);
	
	int i = 0;
	
	for(int z = 0; z < verticesPerLine-1; z++) {
		for(int x = 0; x < verticesPerLine-1; x++) {
			int topLeft     = z * verticesPerLine + x;
			int topRight    = topLeft + 1;
			int bottomLeft  = (z + 1) * verticesPerLine + x;
			int bottomRight = bottomLeft + 1;
	
			indices[i++] = topLeft;
			indices[i++] = bottomLeft;
			indices[i++] = topRight;
	
			indices[i++] = topRight;
			indices[i++] = bottomLeft;
			indices[i++] = bottomRight;
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

void
mesh_draw_indexed(Mesh *mesh, shader shader, v3 pos, mat4 vp){
	mat4 model = mat4_identity();
	model *= mat4_translate(pos);
	
	mat4 mvp = mat4_multiply(model, vp);
	use_Shader(shader);
	
	// setShaderValue_mat4(shader, "model", model);
	// setShaderValue_mat4(shader, "view", view);
	// setShaderValue_mat4(shader, "projection", projection);
	
	setShaderValue_mat4(shader, "mvp", mvp);
	
	glBindVertexArray(mesh->vao);
	
	// glBindTexture(GL_TEXTURE_2D, testTexture);
	
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void
mesh_draw_index_textured(Mesh *mesh, shader shader, v3 pos, mat4 vp){
	mat4 model = mat4_identity();
	model *= mat4_translate(pos);
	
	mat4 mvp = mat4_multiply(model, vp);
	use_Shader(shader);
	
	setShaderValue_mat4(shader, "model", model);
	// setShaderValue_mat4(shader, "view", view);
	// setShaderValue_mat4(shader, "projection", projection);
	
	setShaderValue_mat4(shader, "mvp", mvp);
	
	glBindVertexArray(mesh->vao);
	
	glBindTexture(GL_TEXTURE_2D, mesh->texture);
	
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

int
main(int argc, char** argv){
	if(!init()){
		//log
		printf("failed to initialize");
		return -1;
	}
	
	SDL_Window* mainWindow = SDL_CreateWindow("OpenGL Revision", 
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
		//SEARCH: this is not the time for this but see if there is a group allocation (arena for example)
		//to handle gpu allocations 
		//load data and shaders
		uint32 infinteGrid_VBO; // this is here just because opengl demands it
		uint32 infinteGrid_VAO;
		glGenVertexArrays(1, &infinteGrid_VAO);
		glBindVertexArray(infinteGrid_VAO);
		// glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vert_data), &plane_vert_data, GL_STATIC_DRAW);  
		glBindVertexArray(0);
		
		uint32 perlinNoiseTex = 0;
		// Mesh testMesh = create_quad();
		
		Mesh terrainMesh = create_grid_mesh_data(0);
		terrainMesh.texture = perlinNoiseTex;
		const int MAP_WIDTH = 20;
		const int MAP_HEIGHT = 20;
# define MAX_CHUNKS MAP_WIDTH * MAP_HEIGHT
		// TODO: THINK OF A BETTER WAY 
		Terrain_chunk terrainChunks[MAX_CHUNKS] = {0};
		int currentAvailableChunk = 0;
		
		float chunckHalfWidth = (CHUNK_SIZE - 1) / 2.0;
		
		//TODO: see if better to make the view distance in terms of chunks 
		// float maxViewDistance = 500.0f;
		float viewDistance = 3.0f;
		
		shader infinte_grid = make_shader("../shaders/infinte_grid.vert", "../shaders/infinte_grid.frag");
		shader terrain_shader = make_shader("../shaders/terrain.vert", "../shaders/terrain.frag");
		
		shader perlin_shader = make_shader("../shaders/perlin.vert", "../shaders/perlin.frag");
		
		input gameInput = {};
		
		uint32 frameStartTime = 0;
		uint32 frameEndTime = 0;
		float dt = 0.0f;
		
		SDL_Event e;
		
		Camera camera = {};
		camera.pos = make_vec3(0.0f, 20.0f, 3.0f);
		camera.up = make_vec3(0.0f, 1.0f, 0.0f);
		camera.yaw = -90.0f;
		camera.pitch = 0.0f;
		float cameraSpeed = 2.5f;
		
		mat4 projection = mat4_identity();
		projection *= mat4_perspective(radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
		
		frameStartTime = SDL_GetTicks();
		while(Running){
			//handleInput
			while(SDL_PollEvent(&e) != 0){
				//note: optional to handle the view port if the window is resized 
				if(e.type == SDL_EVENT_QUIT){
					Running = false;
				}
				if(e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat){
					if(e.key.key == SDLK_ESCAPE){
						Running = false;
					}
					
				}
				if(e.type == SDL_EVENT_MOUSE_MOTION){
					float dx, dy;
					SDL_GetRelativeMouseState(&dx, &dy);
					playerMouseInput(dx, dy, &camera.pitch, &camera.yaw);
				}
			}
			const bool* curruntKeyState = SDL_GetKeyboardState(NULL);
			playerInputHandle(&gameInput, curruntKeyState);
			
			if(gameInput.sprint){
				cameraSpeed = 20.0f;
			}
			else{
				cameraSpeed = 5.0f;
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
			
			//update
			camera.direction.x = cosf(radians(camera.yaw)) * cosf(radians(camera.pitch));
			camera.direction.y = sinf(radians(camera.pitch));
			camera.direction.z = sinf(radians(camera.yaw)) * cosf(radians(camera.pitch));
			camera.front = v3_normalize(camera.direction);
			
			//render
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			mat4 view = mat4_lookAt(camera.pos, camera.pos + camera.front, camera.up);
			
			mat4 vp = mat4_multiply(view, projection);
			
			
			
			use_Shader(infinte_grid);
			setShaderValue_mat4(infinte_grid, "gVP", vp);
			setShaderValue_v3(infinte_grid, "cam_world_pos", camera.pos);
			glBindVertexArray(infinteGrid_VAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			
			// player chunk position
			v2 cameraChunkPos = make_vec2(camera.pos.x / chunckHalfWidth, camera.pos.z / chunckHalfWidth); 
			
			// aggresivly reset the chunks every frame
			for(int i = 0; i < MAX_CHUNKS; i++){
				currentAvailableChunk = 0;
				terrainChunks[i].exists = false;
			}
			// GENERATE THE CHUNKS
			for(int z = -viewDistance; z <= viewDistance; ++z){
				for(int x = -viewDistance; x <= viewDistance; ++x){
					int cx = cameraChunkPos.x + x;
					int cz = cameraChunkPos.y + z;
					cx *= chunckHalfWidth;
					cz *= chunckHalfWidth;
					
					assert(currentAvailableChunk < MAX_CHUNKS);
					// if(!chunkExist(terrainChunks, currentAvailableChunk, cx, cz)){
					terrainChunks[currentAvailableChunk].mesh = &terrainMesh;
					terrainChunks[currentAvailableChunk].pos = make_vec3(cx, 0.2f, cz);
					terrainChunks[currentAvailableChunk].exists = true;
					++currentAvailableChunk;
					// }
				}
			}
			
			//RENDER CHUNKS
			for(int i = 0; i < currentAvailableChunk; ++i){
				mesh_draw_index_textured(terrainChunks[i].mesh, terrain_shader, terrainChunks[i].pos, vp);
			}
			
			SDL_GL_SwapWindow(mainWindow);
			
			frameEndTime = SDL_GetTicks();
			dt = (frameEndTime - frameStartTime) / 1000.0f;
			frameStartTime = frameEndTime;
		}
	}

}
