#define GLEW_STATIC
#include<stdio.h>
#include<stdlib.h>
#include<math.h> 
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include"LoadShaders.h"
#include"utils/model.h"
#include"camera/Camera.h"
#include"camera/TrackballControls.h"
#include"utils/filesystem.h"
#define MAX_FRAMEBUFFER_WIDTH 2048
#define MAX_FRAMEBUFFER_HEIGHT 2048
//screen resolution
const int WIDTH = 1600;
const int HEIGHT = 1200;


// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera* camera;
TrackballControls* trackballControls;


// Member variables
float aspect;

// Program to construct the linked list (renders the transparent objects)
GLuint  list_build_program;

// Head pointer image and PBO for clearing it
GLuint  head_pointer_texture;
GLuint  head_pointer_clear_buffer;
// Atomic counter buffer
GLuint  atomic_counter_buffer;
// Linked list buffer
GLuint  linked_list_buffer;
GLuint  linked_list_texture;

// Program to render the scene
GLuint render_scene_prog;
struct
{
	GLint light;
	GLint viewPos;
	GLint shininess;
	GLint model_matrix;
	GLint view_matrix;
	GLint projection_matrix;
	GLint side;
} render_scene_uniforms;

struct
{
	GLint resolution;
	GLint model_matrix;
	GLint view_matrix;
	GLint projection_matrix;
} render_plane_uniforms;

// Program to resolve 
GLuint resolve_program;

// Program to render clipping plane 
GLuint render_plane;


// Full Screen Quad
GLuint  quad_vbo;
GLuint  quad_vao;

GLint current_width;
GLint current_height;

Model geoModel(FileSystem::getPath("resources/model/geomodel/l4.obj"));


void InitPrograms() {
	// Create the program for rendering the scene from the viewer's position
	ShaderInfo scene_shaders[] =
	{
		{ GL_VERTEX_SHADER, "shaders/build_lists.vs.glsl" },
		{ GL_FRAGMENT_SHADER, "shaders/build_lists.fs.glsl" },
		{ GL_NONE }
	};

	if (render_scene_prog != -1)
		glDeleteProgram(render_scene_prog);

	render_scene_prog = LoadShaders(scene_shaders);

	render_scene_uniforms.side = glGetUniformLocation(render_scene_prog, "side");
	render_scene_uniforms.light = glGetUniformLocation(render_scene_prog, "light");
	render_scene_uniforms.viewPos = glGetUniformLocation(render_scene_prog, "viewPos");
	render_scene_uniforms.shininess = glGetUniformLocation(render_scene_prog, "shininess");
	render_scene_uniforms.model_matrix = glGetUniformLocation(render_scene_prog, "model");
	render_scene_uniforms.view_matrix = glGetUniformLocation(render_scene_prog, "view");
	render_scene_uniforms.projection_matrix = glGetUniformLocation(render_scene_prog, "projection");
	//render_scene_uniforms.aspect = glGetUniformLocation(render_scene_prog, "aspect");
	//render_scene_uniforms.time = glGetUniformLocation(render_scene_prog, "time");

	ShaderInfo resolve_shaders[] =
	{
		{ GL_VERTEX_SHADER, "shaders/resolve_lists.vs.glsl" },
		{ GL_FRAGMENT_SHADER, "shaders/resolve_lists.fs.glsl" },
		{ GL_NONE }
	};

	resolve_program = LoadShaders(resolve_shaders);
	ShaderInfo plane_shaders[] = {
		{ GL_VERTEX_SHADER, "shaders/plane.vert" },
		{ GL_FRAGMENT_SHADER, "shaders/plane.frag" },
		{ GL_NONE }
	};

	render_plane = LoadShaders(plane_shaders);
	render_plane_uniforms.model_matrix = glGetUniformLocation(render_plane, "model");
	render_plane_uniforms.view_matrix = glGetUniformLocation(render_plane, "view");
	render_plane_uniforms.projection_matrix = glGetUniformLocation(render_plane, "projection");
	render_plane_uniforms.resolution = glGetUniformLocation(render_plane, "resolution");
}

void Initialize() {
	GLuint* data;
	render_scene_prog = -1;

	InitPrograms();

	// Create head pointer texture
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &head_pointer_texture);
	glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindImageTexture(0, head_pointer_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

	// Create buffer for clearing the head pointer texture
	glGenBuffers(1, &head_pointer_clear_buffer);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, head_pointer_clear_buffer);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * sizeof(GLuint), NULL, GL_STATIC_DRAW);
	data = (GLuint*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	memset(data, 0x00, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * sizeof(GLuint));
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	// Create the atomic counter buffer
	glGenBuffers(1, &atomic_counter_buffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter_buffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_COPY);

	// Create the linked list storage buffer
	glGenBuffers(1, &linked_list_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, linked_list_buffer);
	glBufferData(GL_TEXTURE_BUFFER, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * 4 * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	// Bind it to a texture (for use as a TBO)
	glGenTextures(1, &linked_list_texture);
	glBindTexture(GL_TEXTURE_BUFFER, linked_list_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, linked_list_buffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	glBindImageTexture(1, linked_list_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);

	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);

	static const GLfloat quad_verts[] =
	{
		-1.0f, -1.0f,0.0f,
		 1.0f, -1.0f,0.0f,
		-1.0f,  1.0f,0.0f,
		 1.0f,  1.0f,0.0f,
	};

	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	glClearDepth(1.0f);
}
void DrawPlane(float angle, glm::vec3 axis, glm::vec3 position) {
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(quad_vao);
	glUseProgram(render_plane);
	glm::mat4 model_matrix = glm::mat4(1.0f);
	model_matrix = glm::translate(model_matrix, position);
	
	model_matrix = glm::rotate(model_matrix, glm::radians(angle), axis);
	model_matrix = glm::scale(model_matrix, glm::vec3(80.0, 80.0, 80.0));


	glm::mat4 view_matrix = camera->viewMatrix;
	glm::mat4 projection_matrix = camera->projectionMatrix;

	glUniform2f(render_plane_uniforms.resolution, WIDTH, HEIGHT);
	glUniformMatrix4fv(render_plane_uniforms.model_matrix, 1, GL_FALSE, &model_matrix[0][0]);
	glUniformMatrix4fv(render_plane_uniforms.view_matrix, 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(render_plane_uniforms.projection_matrix, 1, GL_FALSE, &projection_matrix[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void RenderGeoModel() {
	GLuint* data;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);         // 启用剔除
	glCullFace(GL_BACK);            // 剔除背向面

	// Reset atomic counter
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomic_counter_buffer);
	data = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	data[0] = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

	// Clear head-pointer image
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, head_pointer_clear_buffer);
	glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bind head-pointer image for read-write
	glBindImageTexture(0, head_pointer_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	// Bind linked-list buffer for write
	glBindImageTexture(1, linked_list_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);

	glUseProgram(render_scene_prog);
	glm::mat4 model_matrix = glm::mat4(1.0f);
	//model_matrix = glm::scale(model_matrix, glm::vec3(90.0, 90.0, 90.0));
	glm::mat4 view_matrix = camera->viewMatrix;
	glm::mat4 projection_matrix = camera->projectionMatrix;
	glUniform1ui(render_scene_uniforms.side, 1);
	glUniform1f(render_scene_uniforms.shininess, 1.0);
	glUniform3f(render_scene_uniforms.light + 0, 40.0, 90.0, -40.0);
	glUniform3f(render_scene_uniforms.light + 1, 0.2,0.2,0.2);
	glUniform3f(render_scene_uniforms.light + 2, 1.0, 1.0, 1.0);
	glUniform3f(render_scene_uniforms.light + 3, 1.0, 1.0, 1.0);
	glUniform1f(render_scene_uniforms.light + 4, 1.0);
	glUniform1f(render_scene_uniforms.light + 5, 0.00);
	glUniform1f(render_scene_uniforms.light + 6, 0.2);
	glUniform3f(render_scene_uniforms.viewPos, camera->position.x, camera->position.y, camera->position.z);
	glUniformMatrix4fv(render_scene_uniforms.model_matrix, 1, GL_FALSE, &model_matrix[0][0]);
	glUniformMatrix4fv(render_scene_uniforms.view_matrix, 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(render_scene_uniforms.projection_matrix, 1, GL_FALSE, &projection_matrix[0][0]);
	geoModel.Draw(render_scene_prog);
	glEnable(GL_CULL_FACE);         // 启用剔除
	glCullFace(GL_FRONT);
	glUniform1ui(render_scene_uniforms.side, 2);
	geoModel.Draw(render_scene_prog);
}
void Display() {

	/*float t;

	unsigned int current_time = app_time();

	t = (float)(current_time & 0xFFFFF) / (float)0x3FFF;*/

	GLuint* data;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glClearColor(0.39f, 0.58f, 0.92f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	// Reset atomic counter
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomic_counter_buffer);
	data = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	data[0] = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

	// Clear head-pointer image
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, head_pointer_clear_buffer);
	glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bind head-pointer image for read-write
	glBindImageTexture(0, head_pointer_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	// Bind linked-list buffer for write
	glBindImageTexture(1, linked_list_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);

	//object.Render(0, 8 * 8 * 8);


	/*glDisable(GL_BLEND);

	glBindVertexArray(quad_vao);
	glUseProgram(resolve_program);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);*/
}

void Finalize() {
	glUseProgram(0);
	glDeleteProgram(render_scene_prog);
	glDeleteBuffers(1, &quad_vbo);
	glDeleteVertexArrays(1, &quad_vao);
}

void Resize() {

}
#pragma region 窗口回调函数
void OnResize(GLFWwindow* window, int w, int h) {
	//firstpersonControls->OnResize(window, w, h);
	trackballControls->OnResize(window, w, h);
}
void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	//firstpersonControls->glfw_mouse_button_callback(window, button, action, mods);
	trackballControls->glfw_mouse_button_callback(window, button, action, mods);
}
void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	//firstpersonControls->glfw_scroll_callback(window, xoffset, yoffset);
	trackballControls->glfw_scroll_callback(window, xoffset, yoffset);
}
void glfw_keyboard_input_callback(GLFWwindow* window) {
	//firstpersonControls->glfw_keyboard_input_callback(window);
}
void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) {
	//firstpersonControls->glfw_cursor_pos_callback(window, x, y);
	trackballControls->glfw_cursor_pos_callback(window, x, y);
}
void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	//firstpersonControls->glfw_key_callback(window, key, scancode, action, mods);
}
#pragma endregion

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "VolumeRendering", NULL, NULL);
	if (window == NULL)
	{
		printf("window create failed");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	//初始化GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		printf("Init GlEW failed");
		glfwTerminate();
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, OnResize);
	glfwSetScrollCallback(window, glfw_scroll_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetCursorPosCallback(window, glfw_cursor_pos_callback);
	glViewport(0, 0, WIDTH, HEIGHT);
	//InitPrograms();
	Initialize();
	geoModel.loadModel();
	//glEnable(GL_DEPTH_TEST);
	


	camera = new Camera(45, float(WIDTH) / HEIGHT, 0.1, 500);

	camera->position = glm::vec3(45, 90, 45);
	camera->lookAt(glm::vec3(0, 0, 0));
	//firstpersonControls=new FirstPersonControls(camera,window);
	trackballControls = new TrackballControls(camera, window);



	//glEnable(GL_DEPTH_TEST);
	int i = 0;
	float sum = 0;
	stbi_set_flip_vertically_on_load(true);
	//Initialize();
	while (!glfwWindowShouldClose(window))
	{

		// per-frame time logic
	   // --------------------
		auto currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		sum += deltaTime;
		i++;
		if (i % 100 == 0) {
			cout << "帧率为：" << 100 / sum << endl;
			sum = 0;
		}
		
		glClearColor(0.39f, 0.58f, 0.92f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderGeoModel();
		glClearColor(0.39f, 0.58f, 0.92f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		for (int i = -24; i < 32; i += 4)
		{
			DrawPlane(0.0, glm::vec3(1, 0, 0), glm::vec3(0, 0, i));
		}
		for (int i = -32; i < 32; i += 4)
		{
			DrawPlane(90.0, glm::vec3(0, 1, 0), glm::vec3(i, 0, 0));
		}


		trackballControls->update();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	Finalize();
	return 0;
}