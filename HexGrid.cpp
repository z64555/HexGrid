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
	* @param[in] maj_orig   Origin offset of the major axis
	* @param[in] min_orig   Origin offset of the minor axis
	* @param[in] grid_size  Maximum value for both axes
	* @param[in] n          Number of divisions along the major axis
	* @param[in] centered   If true, the grid will be centered around the origin. Else, the grid will have one corner
	*                       in the origin.
	* @details The major axis is the one where adjacent hexagons share an edge, and the minor axis is the one where adjacent hexagons share only a vertex.
	*/
	void generate(float maj_orig, float min_orig, float grid_size, int n, bool centered);

	/**
	* @brief Rounds the incoming coordinates to the nearest hexagon center
	* @param[in/out] maj Major coordinate
	* @param[in/out] min Minor coordinate
	* @param[in] dir Direction to round. False is towards grid origin, True is away from grid origin
	*/
	void round(float &maj, float &min);

	// The grid's origin
	struct
	{
		float major;
		float minor;
	} origin;

	// Hexagon center-to-center distances
	struct
	{
		float major;    // If the hexagon is regular, this unit's value = 2 * apothem
		float minor;    // If the hexagon is regular, this unit's value = (3 / 2) * radius
	} offset;

	vector<float> major_axis;
	vector<float> minor_axis;
};

class HexMesh
{
public:
	void generate();

	/**
	 * Tesselates the mesh centered at (x, y) with radius r
	 *
	 * @param[in] x X coordinate of the target hexagon
	 * @param[in] y Y coordinate of the target hexagon
	 * @param[in] r Radius, in number of hexagons, around the target hexagon to tesselate. A value of 0 means only the target hexagon will be tesselated
	 */
	void tesselate(float x, float y, int r);

	HexGrid Grid;
	vector<vec3d> v_arr;	// Vertex array
	vector<face> f_arr;		// Face array
	size_t x_size;	// Number of gridlines along the X axis
	size_t y_size;	// Number of gridlines along the Y axis
	bool x_major;	// True if the x axis is major, false if the y axis is the major line
};

// Rendering globals
GLuint program;				// For shaders
GLint attribute_coord2d;	// 
GLint uniform_mvp;			// Transformation matrix
const int screen_width = 640;
const int screen_height = 480;

HexMesh Mesh;


void HexGrid::generate(float maj_orig, float min_orig, float grid_size, int n, bool centered) {
	const float sin60 = 0.8660254;			// sin(60deg) = 0.8660254
	float maj_off = (grid_size / (n * 2));		// Offsets along the major axis
	float r = (grid_size / (n * 2)) / sin60;	// Radius of the hexagon
	float min_off = r / 2;						// Offsets along the minor axis
	int m;

	origin.major = maj_orig;
	origin.minor = min_orig;

	offset.major = grid_size / n;
	offset.minor = (3 / 2) * r;

	// From here on, min_origin and maj_origin are re-defined as the lower-left corner of the grid
	if (centered) {
		maj_orig -= grid_size / 2;
		min_orig -= (r * (2 + 3 * (int)(n / 2))) / 2;
	}

	// Major axis
	m = ((n * 2) + 1);
	for (int i = 0; i < m; ++i) {
		// Number of major graduations per hexagon (Lines where vertices are)) = 3;
		// Each additional hexagon adds 2 graduations, so the total is (n * 2) + 1
		major_axis.push_back(maj_orig + (maj_off * i));
	}

	// Minor axis
	// First calculate number of hexagons to plot
	if (centered) {
		// odd values of n give a nice super-hexagon, while
		// even values of n give a parallelogram
		m = n | 1;	// +1 to even numbers to make them odd.
	} else {
		// Try to fit within square
		if (n == 1) {
			// Special case. Hexagon will spill outside the square along the minor edge
			m = 1;
		} else {
			// A regular hexagon's diameter is (1 / sin60) * h;
			// If its height is 1.0f, then its width is 1.155. Likewise, if we have a maximum of n cells on the major axis, then we have (1.155 * n) cells on the minor axis that will fit in a square
			m = int(float(n) / sin60);
		}
	}

	// Then, calculate number of graduations
	m = (m * 3) + 2;
	for (int i = 0; i < m; ++i) {
		// Number of minor graduataions per hexagon = 4 + 1
		// Each additional hexagon adds 2 + 1 graduations, so the total is (m * 3) + 2
		// Doing a few tricks here. A regular hexagon can be composed of 6 equilateral triangles.
		// If an eqilateral triangle has its base along an axis, such as the x axis, the apex is located exactly in between the two foot vertices. (x, y) = (b/2, h);
		// Thus, a hexagon can be formed from (m * 3) + 1 graduations, plus one "phantom" graduation in the dead center of each hexagon. (m * 4) + 1;
		if ((i + 1) % 3 == 0) {
			// Skip every third multiple, the "phantom" graduation
			continue;
		}

		minor_axis.push_back(min_orig + (min_off * i));
	}
}

void HexGrid::round(float &maj, float &min) {
	float major = maj - origin.major;
	float minor = min - origin.minor;
	bool is_odd = false;

	minor /= offset.minor;
	roundf(minor);
	is_odd = int(minor) % 2;
	min = minor * offset.minor;
	min += origin.minor;

	// Scale down
	major /= offset.major;

	// Shift away from center
	if (is_odd) {
		if (major > 0) {
			major += 0.5f;
		} else {
			major -= 0.5f;
		}
	}

	// Snap to nearest integer
	roundf(major);

	// Shift towards center
	if (is_odd) {
		if (major > 0) {
			major -= 0.5f;
		} else {
			major += 0.5f;
		}
	}

	// Scale up
	maj = major * offset.major;
	maj += origin.major;
}


void HexMesh::generate() {
	int n = 9;
	Grid.generate(0.0f, 0.0f, 2.0f, n, true);
	//Grid.generate(-1.0f,-1.0f, 2.0f, n, false);
	vec3d vert = { 0.0f, 0.0f, 0.0f };

	size_t major_size = Grid.major_axis.size();	// Number of vertices along the Y (major) axis
	size_t minor_size = Grid.minor_axis.size();	// Number of vertices along the X (minor) axis

	y_size = major_size / 2;
	x_size = minor_size / 2;

	v_arr.reserve(major_size * minor_size);	// TODO: Find a closer approximation. This one will reserve way more than what's needed

	// Create vertex strips up the major axis
	int j = 0;
	bool zig = ((j % 2) == 1);;
	for (; j < (minor_size - 1); j += 2) {
		for (int i = 0; i < major_size; ++i) {
			if (zig) {
				vert.x = Grid.minor_axis[j];	// Minor axis
				vert.y = Grid.major_axis[i];	// Major axis
			} else {
				vert.x = Grid.minor_axis[j + 1];	// Minor axis
				vert.y = Grid.major_axis[i];		// Major axis
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

	size_t major_size = Mesh.Grid.major_axis.size();
	size_t minor_size = Mesh.Grid.minor_axis.size();
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
		0,                 // No data between verts
		square             // pointer to the C array
		);

	glDrawArrays(GL_LINE_LOOP, 0, 4);

	// Draw reference cross
	GLfloat cross[] = {
		-1.0f, -1.0f,
		 1.0f,  1.0f,
		-1.0f,  1.0f,
		 1.0f, -1.0f
	};

	glVertexAttribPointer(
		attribute_coord2d, // attribute
		2,                 // number of elements per vertex, here (x,y)
		GL_FLOAT,          // the type of each element
		GL_FALSE,          // take our values as-is
		0,                 // No data between verts
		cross              // pointer to the C array
		);

	glDrawArrays(GL_LINES, 0, 4);

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
