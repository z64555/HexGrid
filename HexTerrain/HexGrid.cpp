///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        HexGrid.cpp
// Purpose:     Procedural Hexagonal Grid generation
// Author:      Allen "z64555" Babb ( z1281110@gmail.com )
// Modified by: 
// Created:     30/11/2016       (DD/MM/YYYY)
// Page width:  120 char
// Tab width:   4 spaces
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>

#include <GL/glew.h>
#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


using namespace std;

typedef union vec3d
{
	struct
	{
		float x;
		float y;
		float z;
	};

	float a1d[3];
};

typedef union vec4d
{
	struct
	{
		float x;
		float y;
		float z;
		float w;
	};

	float a1d[4];
} vec4d;

struct face
{
	vec3d* v[3];
};

class HexGrid
{
public:
	/**
	* @brief Generates a hexagonal grid
	*
	* @param[in] x_orig Origin offset of the X axis
	* @param[in] y_orig Origin offset of the Y axis
	* @param[in] size   Maximum X (and Y) value
	* @param[in] n      Number of divisions along the major axis
	* @param[in] x_maj Major axis. False = Y axis, True = X axis
	* @details The major axis is the one where adjacent hexagons share an edge, and the minor axis is the one where adjacent hexagons share only a vertex.
	*/
	void generate(float x_orig, float y_orig, float grid_size, int n, bool x_maj);

	bool major_axis;		// False = Y axis is major, True = X axis is major
	vector<float> x_axis;
	vector<float> y_axis;
};

class HexMesh
{
public:
	void generate();

	HexGrid Grid;
	vector<vec3d> v_arr;	// Vertex array
	vector<face> f_arr;		// Face array
	size_t x_size;	// Number of gridlines along the X axis
	size_t y_size;	// Number of gridlines along the Y axis
};

// Rendering globals
GLuint program;				// For shaders
GLint attribute_coord2d;	// 
GLint uniform_mvp;			// Transformation matrix
const int screen_width = 640;
const int screen_height = 480;

HexMesh Mesh;


void HexGrid::generate(float x_orig, float y_orig, float grid_size, int n, bool x_maj) {
	const float sin60 = 0.8660254;	// sin(60deg) = 0.8660254

	float maj_off = (grid_size / (n * 2));
	float min_off = maj_off / (sin60 * 2);

	major_axis = x_maj;
	// Seems to be a glitch with trying to use vector<float> *maj_lines in MSVS. Compiler won't see the function arguments after trying to set maj_lines = &x_lines

	// Major
	for (int i = 0; i < (n * 2) + 1; ++i) {
		if (x_maj) {
			x_axis.push_back(x_orig + (maj_off * i));
		} else {
			y_axis.push_back(y_orig + (maj_off * i));
		}
	}

	// Minor
	for (int i = 0; i < int((n * 4) * sin60) + 1; ++i) {
		if ((i + 1) % 3 == 0) {
			// Skip every third multiple
			continue;
		}

		if (!x_maj) {
			x_axis.push_back(x_orig + (min_off * i));
		} else {
			y_axis.push_back(y_orig + (min_off * i));
		}
	}
}


void HexMesh::generate() {
	int n = 9;
	bool major = false;
	Grid.generate(-1.0f, -1.0f, 2.0f, n, major);
	vec3d vert = { 0.0f, 0.0f, 0.0f };

	size_t major_size = Grid.y_axis.size();	// Number of vertices along the Y (major) axis
	size_t minor_size = Grid.x_axis.size();	// Number of vertices along the X (minor) axis

	y_size = major_size / 2;
	x_size = minor_size / 2;

	v_arr.reserve(major_size * minor_size);	// TODO: Find a closer approximation. This one will reserve way more than what's needed

	// Create vertex strips up the major axis
	int j = 0;
	bool zig = ((j % 2) == 1);;
	for (; j < (minor_size - 1); j += 2) {
		for (int i = 0; i < major_size; ++i) {
			if (zig) {
				vert.x = Grid.x_axis[j];	// Minor axis
				vert.y = Grid.y_axis[i];	// Major axis
			} else {
				vert.x = Grid.x_axis[j + 1];	// Minor axis
				vert.y = Grid.y_axis[i];		// Major axis
			}

			v_arr.push_back(vert);
			zig = !zig;
		}
//		zig = !zig;
	}
}

bool init_resources() {
	GLint compile_ok = GL_FALSE, link_ok = GL_FALSE;

	// Vertex shader
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char *vs_source =
		//"#version 100\n"  // OpenGL ES 2.0
		"#version 120\n"  // OpenGL 2.1
		"attribute vec2 coord2d;                        "
		"uniform mat4 mvp;                              "
		"void main(void) {                              "
		"  gl_Position = mvp * vec4(coord2d, 0.0, 1.0); "
		"}";
	glShaderSource(vs, 1, &vs_source, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
	if (!compile_ok) {
		cerr << "Error in vertex shader" << endl;
		return false;
	}

	// Fragment shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fs_source =
		//"#version 100\n"  // OpenGL ES 2.0
		"#version 120\n"  // OpenGL 2.1
		"void main(void) {        "
		"  gl_FragColor[0] = 0.1; "
		"  gl_FragColor[1] = 1.0; "
		"  gl_FragColor[2] = 0.1; "
		"}";
	glShaderSource(fs, 1, &fs_source, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
	if (!compile_ok) {
		cerr << "Error in fragment shader" << endl;
		return false;
	}

	// Linking
	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		cerr << "Error in glLinkProgram" << endl;
		return false;
	}

	// Bind handles to GLSL code
	const char* attribute_name = "coord2d";
	attribute_coord2d = glGetAttribLocation(program, attribute_name);
	if (attribute_coord2d == -1) {
		cerr << "Could not bind attribute " << attribute_name << endl;
		return false;
	}

	const char* uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if (uniform_mvp == -1) {
		cerr << "Could not bind uniform " << uniform_name << endl;
		return false;
	}

	return true;
}

void free_resources() {
	glDeleteProgram(program);
}

void logic() {
	glm::mat4 ident(1.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, -2.0));

	glm::vec3 eye(0.0, 0.0, 0.0);
	glm::vec3 center(0.0, 0.0, -2.0);
	glm::vec3 uvec(0.0, 1.0, 0.0);
	glm::mat4 view = glm::lookAt(eye, center, uvec);

	glm::mat4 projection = glm::perspective(45.0f, (1.0f * (float(screen_width) / float(screen_height))), 0.1f, 10.0f); // (1.0f * (screen_width / screen_height))

	glm::mat4 mvp;
	mvp = projection * view * model;

	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
};

void render(SDL_Window* window) {
	// Background as black
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);
	glEnableVertexAttribArray(attribute_coord2d);

	// Draw Vertices and major lines
	glVertexAttribPointer(
		attribute_coord2d, // attribute
		3,                 // number of elements per vertex, here (x,y)
		GL_FLOAT,          // the type of each element
		GL_FALSE,          // take our values as-is
		0,                 // no extra data between each position
		&Mesh.v_arr[0]     // pointer to the C array
	);

	size_t major_size = Mesh.Grid.y_axis.size();
	size_t minor_size = Mesh.Grid.x_axis.size();
	size_t count = minor_size / 2;

	glDrawArrays(GL_POINTS, 0, Mesh.v_arr.size());

	for (int i = 0; i < (count - 1); ++i) {
		glDrawArrays(GL_LINE_STRIP, (major_size * i), major_size);
	}

	if (count % 2) {
		// Odd number of major lines, so skip the first and last segments
		glDrawArrays(GL_LINE_STRIP, (major_size * (count - 1)) + 1, major_size - 2);
	} else {
		// Even number of major lines, draw the last line normally
		glDrawArrays(GL_LINE_STRIP, (major_size * (count - 1)), major_size);
	}

	// Draw minor lines
	for (int j = 0; j < major_size; j += 2) {
		glVertexAttribPointer(
			attribute_coord2d, // attribute
			3,                 // number of elements per vertex, here (x,y)
			GL_FLOAT,          // the type of each element
			GL_FALSE,          // take our values as-is
			sizeof(vec3d) * major_size,  // Stride between verts is is major_size
			&Mesh.v_arr[j]     // pointer to the C array
			);

		glDrawArrays(GL_LINES, 0, count);
	}

	count += count % 2;
	for (int j = 1; j < major_size; j += 2) {
		glVertexAttribPointer(
			attribute_coord2d, // attribute
			3,                 // number of elements per vertex, here (x,y)
			GL_FLOAT,          // the type of each element
			GL_FALSE,          // take our values as-is
			sizeof(vec3d) * major_size,  // Stride between verts is is major_size
			&Mesh.v_arr[j]     // pointer to the C array
			);

		glDrawArrays(GL_LINES, 1, count - 2);
	}

	// Draw reference square
	GLfloat square[] = {
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f
	};

	glVertexAttribPointer(
		attribute_coord2d, // attribute
		2,                 // number of elements per vertex, here (x,y)
		GL_FLOAT,          // the type of each element
		GL_FALSE,          // take our values as-is
		0,                  // No data between verts
		square     // pointer to the C array
		);

	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glDisableVertexAttribArray(attribute_coord2d);

	/* Display the result */
	SDL_GL_SwapWindow(window);
}

void mainLoop(SDL_Window* window) {
	while (true) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				return;
		}
		logic();
		render(window);
	}
}

int main(int argc, char argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("HexGrid",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		screen_width, screen_height,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_GL_CreateContext(window);

	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK) {
		cerr << "Error: glewInit:" << glewGetErrorString(glew_status) << endl;
		return EXIT_FAILURE;
	}

	// Init GL stuff
	if (!init_resources()) {
		return EXIT_FAILURE;
	}

	// Init stuff to test:
	Mesh.generate();

	// Run and cleanup, etc.
	mainLoop(window);

	free_resources();
	return EXIT_SUCCESS;
}

/*
	// Render minor lines
	for (int i = 0; i < Mesh.y_size; ++i) {
		for (int j = 0; j < Mesh.x_size; ++j) {
			Mesh.v_arr[i + (Mesh.x_size * j)];	// Render me!
		}
	}

	// Render major lines
	for (int j = 0; j < Mesh.x_size; ++j) {
		for (int i = 0; i < Mesh.y_size; ++i) {
			Mesh.v_arr[i + (Mesh.y_size * j)];	// Render me!
		}
	}
	*/
