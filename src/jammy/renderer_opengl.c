#if defined(JM_LINUX)
#include "renderer.h"

#include <GL/glew.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <jammy/shaders/opengl/color.vs.h>
#include <jammy/shaders/opengl/color.fs.h>
#include <jammy/shaders/opengl/texture.vs.h>
#include <jammy/shaders/opengl/texture.fs.h>
#include <jammy/shaders/opengl/text.vs.h>
#include <jammy/shaders/opengl/text.fs.h>

typedef struct jm_renderer
{
    GLuint dynamicVertexBuffer;
    GLuint dynamicIndexBuffer;

    GLuint shaderPrograms[JM_SHADER_PROGRAM_COUNT];
} jm_renderer;

jm_renderer g_renderer;

void load_shader_program(
    jm_shader_program shaderProgram,
    const char* vertexShaderCode, 
    const char* fragmentShaderCode)
{
	// Create the shaders
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	GLint result = GL_FALSE;
	int infoLogLength;

	// Compile Vertex Shader
	//printf("Compiling shader : %s\n", vertexFilePath);
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexShaderCode , NULL);
	glCompileShader(vertexShader);

	// Check Vertex Shader
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0)
    {
		char* vertexShaderErrorMessage = malloc(infoLogLength + 1);
		glGetShaderInfoLog(vertexShader, infoLogLength, NULL, vertexShaderErrorMessage);
		fprintf(stderr, "%s\n", vertexShaderErrorMessage);
        free(vertexShaderErrorMessage);
        exit(1);
	}

	// Compile Fragment Shader
	//printf("Compiling shader : %s\n", fragmentFilePath);
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentShaderCode , NULL);
	glCompileShader(fragmentShader);

	// Check Fragment Shader
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0)
    {
		char* fragmentShaderErrorMessage = malloc(infoLogLength + 1);
		glGetShaderInfoLog(fragmentShader, infoLogLength, NULL, fragmentShaderErrorMessage);
		fprintf(stderr, "%s\n", fragmentShaderErrorMessage);
        free(fragmentShaderErrorMessage);
        exit(1);
	}

	// Link the program
	//printf("Linking program\n");
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	// Check the program
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0){
		char* programErrorMessage = malloc(infoLogLength + 1);
		glGetProgramInfoLog(program, infoLogLength, NULL, programErrorMessage);
		printf("%s\n", programErrorMessage);
        free(programErrorMessage);
        exit(1);
	}
	
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

    g_renderer.shaderPrograms[shaderProgram] = program;
}

void GLAPIENTRY
MessageCallback( 
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        // these are just annoying
        return;
    }

    printf("OpenGL message: %s\n", message);
    printf("type: ");
    switch (type) 
    {
    case GL_DEBUG_TYPE_ERROR:
        printf("ERROR");
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        printf("DEPRECATED_BEHAVIOR");
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        printf("UNDEFINED_BEHAVIOR");
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        printf("PORTABILITY");
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        printf("PERFORMANCE");
        break;
    case GL_DEBUG_TYPE_OTHER:
        printf("OTHER");
        break;
    }
    printf(", ");
 
    printf("id: %u, ", id);
    printf("severity: ");
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        printf("NOTIFICATION");
        break;
    case GL_DEBUG_SEVERITY_LOW:
        printf("LOW");
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        printf("MEDIUM");
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        printf("HIGH");
        break;
    }
    printf("\n");
}

int jm_renderer_init()
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    load_shader_program(JM_SHADER_PROGRAM_COLOR, jm_embedded_vs_color, jm_embedded_fs_color);
    load_shader_program(JM_SHADER_PROGRAM_TEXTURE, jm_embedded_vs_texture, jm_embedded_fs_texture);
    load_shader_program(JM_SHADER_PROGRAM_TEXT, jm_embedded_vs_text, jm_embedded_fs_text);

    const size_t dynamicBufferSize = 32 * 1024 * 1024;

    glGenBuffers(1, &g_renderer.dynamicVertexBuffer);
    glGenBuffers(1, &g_renderer.dynamicIndexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, g_renderer.dynamicVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, dynamicBufferSize, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_renderer.dynamicIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, dynamicBufferSize, NULL, GL_DYNAMIC_DRAW);

    return 0;
}

void jm_renderer_create_texture_resource(
	const jm_texture_resource_desc* desc,
	jm_texture_resource* resource)
{
    GLuint tex;
    glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)desc->width, (GLsizei)desc->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, desc->data);
    *resource = tex;
}

jm_buffer_resource jm_renderer_get_dynamic_vertex_buffer()
{
    return g_renderer.dynamicVertexBuffer;
}

jm_buffer_resource jm_renderer_get_dynamic_index_buffer()
{
    return g_renderer.dynamicIndexBuffer;
}

void jm_renderer_set_shader_program(
	jm_shader_program shaderProgram)
{
    glUseProgram(g_renderer.shaderPrograms[shaderProgram]);
}

GLuint jm_renderer_get_shader_program(
	jm_shader_program shaderProgram)
{
    return g_renderer.shaderPrograms[shaderProgram];
}

GLuint jm_renderer_get_uniform_location(
	jm_shader_program shaderProgram,
	const GLchar* name)
{
    return glGetUniformLocation(g_renderer.shaderPrograms[shaderProgram], name);
}

#endif