struct shader{
	uint32 id;
	uint32 vShader;
	uint32 fShader;
	uint32 gShader;
};

shader
make_shader(char* vShaderPath, char* fShaderPath){
	char* vShaderSource = loadFile(vShaderPath);
	char* fShaderSource = loadFile(fShaderPath);
	
	shader result = {};
		
	result.vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(result.vShader, 1, &vShaderSource, NULL);
	glCompileShader(result.vShader);
	int  success;
	char infoLog[512];
	glGetShaderiv(result.vShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(result.vShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s\n",infoLog);
	}
	
	result.fShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(result.fShader, 1, &fShaderSource, NULL);
	glCompileShader(result.fShader);
	glGetShaderiv(result.fShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(result.fShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s\n",infoLog);
	}
	
	result.id = glCreateProgram();
	glAttachShader(result.id, result.vShader);
	glAttachShader(result.id, result.fShader);
	glLinkProgram(result.id);
	glGetProgramiv(result.id, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(result.id, 512, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n %s\n",infoLog);
	}
	
	SDL_free(vShaderSource);
	SDL_free(fShaderSource);
	return result;
}



shader
make_shader(char* vShaderPath, char* fShaderPath, char* gShaderPath){
	char* vShaderSource = loadFile(vShaderPath);
	char* fShaderSource = loadFile(fShaderPath);
	char* gShaderSource = loadFile(gShaderPath);
	shader result = {};
		
	result.vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(result.vShader, 1, &vShaderSource, NULL);
	glCompileShader(result.vShader);
	int  success;
	char infoLog[512];
	glGetShaderiv(result.vShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(result.vShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s\n",infoLog);
	}
	
	result.fShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(result.fShader, 1, &fShaderSource, NULL);
	glCompileShader(result.fShader);
	glGetShaderiv(result.fShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(result.fShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s\n",infoLog);
	}

	result.gShader = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(result.gShader, 1, &gShaderSource, NULL);
	glCompileShader(result.gShader); 
	glGetShaderiv(result.gShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(result.fShader, 512, NULL, infoLog);
		printf("ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n %s\n",infoLog);
	}
	
	result.id = glCreateProgram();
	glAttachShader(result.id, result.vShader);
	glAttachShader(result.id, result.fShader);
	glAttachShader(result.id, result.gShader);
	glLinkProgram(result.id);
	glGetProgramiv(result.id, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(result.id, 512, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n %s\n",infoLog);
	}
	
	SDL_free(vShaderSource);
	SDL_free(fShaderSource);
	return result;
}

void
shaderDelete(shader* shader){
	glDeleteShader(shader->vShader);
	glDeleteShader(shader->fShader);
}

void
use_Shader(shader shader){
	glUseProgram(shader.id);
}

void 
setShaderValue_v3(shader shader, char* uniformName, v3 values){
	use_Shader(shader);
	uint32 location = glGetUniformLocation(shader.id, uniformName);
	glUniform3f(location, values.x, values.y, values.z);
}

void 
setShaderValue_v3(shader shader, char* uniformName, float x, float y, float z){
	use_Shader(shader);
	uint32 location = glGetUniformLocation(shader.id, uniformName);
	glUniform3f(location, x, y, z);
}

void 
setShaderValue_f1(shader shader, char* uniformName, float value){
	use_Shader(shader);
	uint32 location = glGetUniformLocation(shader.id, uniformName);
	glUniform1f(location, value);
}

void 
setShaderValue_i1(shader shader, char* uniformName, int value){
	use_Shader(shader);
	uint32 location = glGetUniformLocation(shader.id, uniformName);
	glUniform1i(location, value);
}

void 
setShaderValue_mat4(shader shader, char* uniformName, mat4 values){
	use_Shader(shader);
	uint32 location = glGetUniformLocation(shader.id, uniformName);
	glUniformMatrix4fv(location, 1, GL_TRUE, (float*)&values);
}

