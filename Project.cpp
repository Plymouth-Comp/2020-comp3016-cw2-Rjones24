#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GLFW/glfw3.h"
#include "LoadShaders.h"
#include <glm/glm.hpp> //includes GLM
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <glm/ext/matrix_transform.hpp> // GLM: translate, rotate
#include <glm/ext/matrix_clip_space.hpp> // GLM: perspective and ortho 
#include <glm/gtc/type_ptr.hpp> // GLM: access to the value_ptr

#include "model.h"
#include "Shader.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include<vector>
#include "Project.h"

GLuint  Maze[5][5];
GLuint  Rotate_Maze[5][5];

GLuint texture1;
enum VAO_IDs { Triangles, Indices, Colours, Tex, NumVAOs = 1 };
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
enum Attrib_IDs { vPosition = 0, cPosition = 1, tPosition = 2 };

GLuint  VAOs[NumVAOs];
GLuint  Buffers[NumBuffers];

const GLuint  NumVertices = 36;
GLuint program;

//camera
glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

float lastX = 400, lastY = 300;
GLuint current_I = 0, current_J = 0, current_piece = 0;
bool firstMouse = true, firstLoad = true;
float yaw = -90.0f, pitch = 0.0f;
bool create = false, play = false, menu = true, finish = false, undo = false;
GLuint collisions = 1;
int clicked = 0;

vector<std::string> faces
{
	"media/textures/skybox/right.jpg",
	"media/textures/skybox/left.jpg",
	"media/textures/skybox/top.jpg",
	"media/textures/skybox/bottom.jpg",
	"media/textures/skybox/front.jpg",
	"media/textures/skybox/back.jpg",
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

GLuint loadSkyBox(vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height,
			&nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
				width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap failed to load at path: " << faces[i]
				<< std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
		GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
		GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
		GL_CLAMP_TO_EDGE);
	return textureID;
}
glm::mat4 view;
glm::mat4 projection;

struct Point
{
	Point() {}
	Point(double x, double y, double z)
		: x(x)
		, y(y)
		, z(z)
	{}
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
};

struct Wall
{
	Wall() : min(), max() {}
	Wall(const Point& min, const Point& max)
		: min(min)
		, max(max)
	{}

	Point min;
	Point max;
};

struct Player
{
	Player() : center(), radius() {}
	Player(const Point& center, double radius)
		: center(center)
		, radius(radius)
	{}

	Point center;
	double radius = 0;
};

struct Wall* walls;
struct Wall wallWin;
struct Wall wallPre[14];
struct Player s1;

void
Creator(GLuint  Maze[5][5],vector<Model> Modles, Shader ourShader)
{
	collisions = 4;
	Position = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	view = glm::lookAt(Position, Position + cameraFront, cameraUp);
	projection = glm::perspective(45.0f, 4.0f / 3, 0.1f, 20.0f);

	for (int i = 0; i <= 4; i++) {
		for (int j = 0; j <= 4; j++) {
			glm::mat4 model = glm::mat4(1.0f);
			double posx = 2.1 * double(j), posy = 2.1 * double(i);
			model = glm::scale(model, glm::vec3(0.35f, 0.35f, 1.0f));
			model = glm::translate(model, glm::vec3(-4.2f + posx, -4.2f + posy, -4.0f));
			model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
			if (Rotate_Maze[i][j] == 0) {
				model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0 , 0.0));
			}
			else if ((Rotate_Maze[i][j] == 1) || (Rotate_Maze[i][j] == 3)) {
				model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, -2.0 + Rotate_Maze[i][j], 0.0));
			}

			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));

			// Adding all matrices up to create combined matrix
			glm::mat4 mvp = projection * view * model;

			//adding the Uniform to the shader
			int mvpLoc = glGetUniformLocation(ourShader.Program, "mvp");
			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

			switch ((Maze[i][j])) {
			case 0:
				collisions += 8;
				break;
			case 1:
				collisions += 4;
				break;
			case 2:
				collisions += 2;
				break;
			case 3:
				collisions += 3;
				break;
			case 4:
				collisions += 5;
				break;
			case 5:
				collisions += 3;
				break;
			case 6:
				collisions += 1;
				break;
			}
			Modles[(Maze[i][j]) + 10].Draw(ourShader);
		}
	}
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	if (menu) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			if (xpos >= 290 && xpos <= 500 && ypos >= 300 && ypos <= 400)
			{
				menu = false;
				create = true;
			}
		}
	} else if (create) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
				if (xpos >= 660 && xpos <= 790 && ypos >= 530 && ypos <= 585)
				{
					walls = new Wall[collisions];
					create = false;
					firstLoad = true;
					play = true;
				}
				else if (xpos >= 10 && xpos <= 150 && ypos >= 10 && ypos <= 60)
				{
					create = false;
					menu = true;
				}
				else if (xpos >= 660 && xpos <= 790 && ypos >= 455 && ypos <= 510)
				{
					for (int i = 0; i <= 4; i++) {
						for (int j = 0; j <= 4; j++) {
							Rotate_Maze[i][j] = 0;
							Maze[i][j] = 0;
						}
					}
					Maze[0][0] = 5;
					Rotate_Maze[4][4] = 2;
					Maze[4][4] = 6;
				}
				for (int i = 0; i <= 4; i++) {
					for (int j = 0; j <= 4; j++) {
						if (xpos >= 155 + (100 * j) && xpos <= 250 + (100 * j) && ypos >= 455 - (100 * i) && ypos <= 550 - (100 * i))
						{
							clicked += 1;
							if (!((current_J == j) && (current_I == i))) {
								clicked = 1;
							}
							current_J = j;
							current_I = i;
							if(clicked >=2 ){
								Rotate_Maze[current_I][current_J] += 1;
								clicked = 1;
								if (Rotate_Maze[current_I][current_J] >=4) {
									Rotate_Maze[current_I][current_J] = 0;
								}
							}							
						}
					}
				}
				if (xpos >= 30 && xpos <= 130 && ypos >= 70 && ypos <= 170)
				{
					Maze[current_I][current_J] = 1;
				}
				if (xpos >= 30 && xpos <= 130 && ypos >= 195 && ypos <= 295)
				{
					Maze[current_I][current_J] = 2;
				}
				if (xpos >= 30 && xpos <= 130 && ypos >= 325 && ypos <= 425)
				{
					Maze[current_I][current_J] = 0;
				}
				if (xpos >= 30 && xpos <= 130 && ypos >= 455 && ypos <= 555)
				{
					Maze[current_I][current_J] = 3;
				}
				if (xpos >= 675 && xpos <= 780 && ypos >= 100 && ypos <= 200)
				{
					Maze[current_I][current_J] = 4;
				}
				if (xpos >= 675 && xpos <= 780 && ypos >= 225 && ypos <= 325)
				{
					bool hasStart = false;
					for (int i = 0;i <= 4; i++) {
						for (int j = 0; j <= 4; j++) {
							if (Maze[i][j]==5) {
								hasStart = true;
							}
						}
					}
					if (!hasStart) {
						Maze[current_I][current_J] = 5;
					}
				}
				if (xpos >= 675 && xpos <= 780 && ypos >= 350 && ypos <= 450)
				{
					bool hasFinish = false;
					for (int i = 0; i <= 4; i++) {
						for (int j = 0; j <= 4; j++) {
							if (Maze[i][j] == 6) {
								hasFinish = true;
							}
						}
					}
					if (!hasFinish) {
						Maze[current_I][current_J] = 6;
					}
				}
		}
	}
	else if (finish) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			if (xpos >= 255 && xpos <= 550 && ypos >= 400 && ypos <= 500)
			{
				finish = false;
				menu = true;
			}
		}
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}
		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;
		float sensitivity = 0.1f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;
		yaw += xoffset;
		pitch += yoffset;
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(direction);
}

double SquaredDistPointWall(const Point& p, const Wall& wall)
{
	auto check = [&](
		const double pn,
		const double bmin,
		const double bmax) -> double
	{
		double out = 0;
		double v = pn;

		if (v < bmin)
		{
			double val = (bmin - v);
			out += val * val;
		}

		if (v > bmax)
		{
			double val = (v - bmax);
			out += val * val;
		}

		return out;
	};

	// Squared distance
	double sq = 0.0;

	sq += check(p.x, wall.min.x, wall.max.x);
	sq += check(p.y, wall.min.y, wall.max.y);
	sq += check(p.z, wall.min.z, wall.max.z);

	return sq;
}

bool TestPlayer_Wall(const Player& player, const Wall& wall)
{
	double squaredDistance = SquaredDistPointWall(player.center, wall);

	return squaredDistance <= (player.radius * player.radius);
}

void
Build_Maze(GLuint  Maze[5][5], vector<Model> Modles, Shader ourShader)
{
	int colpos = 4;
	view = glm::lookAt(Position, Position + cameraFront, cameraUp);
	projection = glm::perspective(45.0f, 4.0f / 3, 0.1f, 20.0f);


	// for extiria
	walls[0] = { Point(-1, -1, 1),Point(9, 1, 1) };
	walls[1] = { Point(-1, -1, -9),Point(-1, 1, 1) };
	walls[2] = { Point(-1, -1, -9),Point(9, 1, -9) };
	walls[3] = { Point(9, -1, -9),Point(9, 1, 1) };

	s1 = { Point(Position.x, Position.y, Position.z), 0.5 };

	for (int i = 0; i <= 4; i++) {
		for (int j = 0; j <= 4; j++) {
			double posx = 2 * double(j), posz = -2 * double(i);
			if (firstLoad) {
			 colpos = 4;
				if (Maze[i][j] == 5) {
					firstLoad = false;
					Position = glm::vec3(float(posx), 0.0f, float(posz));
				}
			}

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
			model = glm::translate(model, glm::vec3(float(posx), -1.0f, float(posz)));
			if ((Rotate_Maze[i][j] == 0) || (Rotate_Maze[i][j] == 2)) {
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, -1.0 + Rotate_Maze[i][j], 0.0));
			}
			else if ((Rotate_Maze[i][j] == 1) || (Rotate_Maze[i][j] == 3)) {
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, -1.0, 0.0));
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, -2.0 + Rotate_Maze[i][j], 0.0));
			}

			// Adding all matrices up to create combined matrix
			glm::mat4 mvp = projection * view * model;

			//adding the Uniform to the shader
			int mvpLoc = glGetUniformLocation(ourShader.Program, "mvp");
			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

#pragma region Collision_Mapping

			//4 sides
			wallPre[0] = { Point(-1 + posx, -1, 1 + posz),Point(1 + posx, 1, 1 + posz) };
			wallPre[1] = { Point(-1 + posx, -1, -1 + posz),Point(-1 + posx, 1, 1 + posz) };
			wallPre[2] = { Point(-1 + posx, -1, -1 + posz),Point(1 + posx, 1, -1 + posz) };
			wallPre[3] = { Point(1 + posx, -1, -1 + posz),Point(1 + posx, 1, 1 + posz) };

			//4 corners
			wallPre[4] = { Point(-1 + posx, -1, -0.85 + posz),Point(-0.85 + posx, 1, -0.85 + posz) };
			wallPre[5] = { Point(-0.85 + posx, -1, -1 + posz),Point(-0.85 + posx, 1, -0.85 + posz) };

			wallPre[6] = { Point(0.85 + posx, -1, -1 + posz),Point(0.85 + posx, 1, -0.85 + posz) };
			wallPre[7] = { Point(0.85 + posx, -1, -0.85 + posz),Point(1 + posx, 1, -0.85 + posz) };

			wallPre[8] = { Point(0.85 + posx, -1, 0.85 + posz),Point(0.85 + posx, 1, 1 + posz) };
			wallPre[9] = { Point(0.85 + posx, -1, 0.85 + posz),Point(1 + posx, 1, 0.85 + posz) };

			wallPre[10] = { Point(-1 + posx, -1, 0.85 + posz),Point(-0.85 + posx, -1, 0.85 + posz) };
			wallPre[11] = { Point(-0.85 + posx, -1, 0.85 + posz),Point(-0.85 + posx, 1, 1 + posz) };

			//win collisions
			wallPre[12] = { Point(-1 + posx, -1, 0 + posz),Point(1 + posx, 1, 0 + posz) };
			wallPre[13] = { Point(0 + posx, -1, -1 + posz),Point(0 + posx, 1, 1 + posz) };
#pragma endregion

			switch ((Maze[i][j])) {
			case 0:
				walls[colpos] = wallPre[4];
				walls[colpos + 1] = wallPre[5];
				walls[colpos + 2] = wallPre[9];
				walls[colpos + 3] = wallPre[9];

				walls[colpos + 4] = wallPre[8];
				walls[colpos + 5] = wallPre[9];
				walls[colpos + 6] = wallPre[10];
				walls[colpos + 7] = wallPre[11];

				colpos += 8;
				break;
			case 1:
				switch ((Rotate_Maze[i][j])) {
				case 0:
					walls[colpos] = wallPre[0];
					walls[colpos + 1] = wallPre[1];
					walls[colpos + 2] = wallPre[6];
					walls[colpos + 3] = wallPre[7];

					colpos += 4;
					break;
				case 1:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[8];
					walls[colpos + 3] = wallPre[9];

					colpos += 4;
					break;
				case 2:
					walls[colpos] = wallPre[2];
					walls[colpos + 1] = wallPre[3];
					walls[colpos + 2] = wallPre[10];
					walls[colpos + 3] = wallPre[11];

					colpos += 4;
					break;
				case 3:
					walls[colpos] = wallPre[0];
					walls[colpos + 1] = wallPre[3];
					walls[colpos + 2] = wallPre[4];
					walls[colpos + 3] = wallPre[5];

					colpos += 4;
					break;
				}
				break;

			case 2:
				switch ((Rotate_Maze[i][j])) {
				case 0:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[3];
					colpos += 2;
					break;
				case 1:
					walls[colpos] = wallPre[2];
					walls[colpos + 1] = wallPre[0];
					colpos += 2;
					break;

				case 2:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[3];
					colpos += 2;
					break;
				case 3:
					walls[colpos] = wallPre[2];
					walls[colpos + 1] = wallPre[0];
					colpos += 2;
					break;
				}
				break;
			case 3:
				switch ((Rotate_Maze[i][j])) {
				case 0:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[0];
					walls[colpos + 2] = wallPre[3];

					colpos += 3;
					break;
				case 1:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[0];
					colpos += 3;
					break;
				case 2:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[3];
					colpos += 3;
					break;
				case 3:
					walls[colpos] = wallPre[0];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[3];
					colpos += 3;
					break;
				}
				break;
			case 4:
				switch ((Rotate_Maze[i][j])) {
				case 0:
					walls[colpos] = wallPre[0];
					walls[colpos + 1] = wallPre[6];
					walls[colpos + 2] = wallPre[7];

					walls[colpos + 3] = wallPre[4];
					walls[colpos + 4] = wallPre[5];
					colpos += 5;
					break;
				case 1:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[6];
					walls[colpos + 2] = wallPre[7];

					walls[colpos + 3] = wallPre[8];
					walls[colpos + 4] = wallPre[9];
					colpos += 5;
					break;

				case 2:
					walls[colpos] = wallPre[2];
					walls[colpos + 1] = wallPre[10];
					walls[colpos + 2] = wallPre[11];

					walls[colpos + 3] = wallPre[8];
					walls[colpos + 4] = wallPre[9];
					colpos += 5;
					break;
				case 3:
					walls[colpos] = wallPre[3];
					walls[colpos + 1] = wallPre[4];
					walls[colpos + 2] = wallPre[5];

					walls[colpos + 3] = wallPre[10];
					walls[colpos + 4] = wallPre[11];
					colpos += 5;
					break;
				}
				break;
			case 5:
				switch ((Rotate_Maze[i][j])) {
				case 0:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[0];
					walls[colpos + 2] = wallPre[3];

					colpos += 3;
					break;
				case 1:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[0];
					colpos += 3;
					break;
				case 2:
					walls[colpos] = wallPre[1];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[3];
					colpos += 3;
					break;
				case 3:
					walls[colpos] = wallPre[0];
					walls[colpos + 1] = wallPre[2];
					walls[colpos + 2] = wallPre[3];
					colpos += 3;
					break;
				}
				break;
			case 6:
				switch ((Rotate_Maze[i][j])) {
				case 0:
					walls[colpos] = wallPre[12];
					wallWin = wallPre[12];
					colpos += 1;
					break;
				case 1:
					walls[colpos] = wallPre[13];
					wallWin = wallPre[13];
					colpos += 1;
					break;
				case 2:
					walls[colpos] = wallPre[12];
					wallWin = wallPre[12];
					colpos += 1;
					break;
				case 3:
					walls[colpos] = wallPre[13];
					wallWin = wallPre[13];
					colpos += 1;
					break;
				}
				break;
			}
			Modles[(Maze[i][j])].Draw(ourShader);
		}
	}
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		
		firstLoad = true;
		play = false;
		create = true;
	}

		float cameraSpeed = 2.5f * deltaTime;
		
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			for (int i = 0; i <= collisions-1; i++) {
				if (bool t1 = TestPlayer_Wall(s1, walls[i])) {
					cameraSpeed = -(2.5f * deltaTime);

				}
				if (bool t1 = TestPlayer_Wall(s1, wallWin)) {
					play = false;
					finish = true;
				}
				else if (bool t1 = TestPlayer_Wall(s1, wallWin)) {
					play = false;
					finish = true;
				}
			}
			Position += cameraSpeed * cameraFront;
			for (int i = 0; i <= 13; i++) {
				
			}
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
			Position -= cameraSpeed * cameraFront;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
			//for (int i = 0; i <= 11; i++) {
				//if (bool t1 = TestPlayer_Wall(s1, walls[i])) {
				//	cameraSpeed = -(2.5f * deltaTime);
				//}
			//}
			Position -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
			//for (int i = 0; i <= 11; i++) {
				//if (bool t1 = TestPlayer_Wall(s1, walls[i])) {
				//	cameraSpeed = -(2.5f * deltaTime);
				//}
			//}
			Position += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		}

	Position.y = 0.0f;
}


//----------------------------------------------------------------------------
//
// main
//

int
main(int argc, char** argv)
{
	for (int i = 0; i <= 4; i++) {
		for (int j = 0; j <= 4; j++) {
				Maze[i][j] = 0;
				Rotate_Maze[i][j] = 0;
		}
	}
	Maze[0][0] = 5;
	Maze[4][4] = 6;
	Rotate_Maze[4][4] = 2;

	glfwInit();

	GLFWwindow* window = glfwCreateWindow(800, 600, "Maze creator", NULL, NULL);

	glfwMakeContextCurrent(window);
	glewInit();

	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(skyboxVAO);

	GLuint cubemapTexture = loadSkyBox(faces);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	stbi_set_flip_vertically_on_load(true);
	glEnable(GL_DEPTH_TEST);

	Shader ourShader("media/triangles.vert", "media/triangles.frag");
	Shader skyboxShader("media/cube.vert", "media/cube.frag");

	//Maze 
	Model ourModel("resources/Models/Cross/cross.obj");
	Model ourModel_1("resources/Models/Bend/Bend.obj");
	Model ourModel_2("resources/Models/Coridor/Coridor.obj");
	Model ourModel_3("resources/Models/Dead_End/DeadEnd.obj");
	Model ourModel_4("resources/Models/T_Junkstion/T_Junction.obj");
	Model ourModel_5("resources/Models/Start/start.obj");
	Model ourModel_6("resources/Models/Finish/finish.obj");
	//screens
	Model ourModel_7("resources/Models/Finish_Screen/Finish.obj");
	Model ourModel_8("resources/Models/Main_Menu/Menu.obj");
	Model ourModel_9("resources/Models/Create_Screen/creater.obj");
	// maze creater pices
	Model ourModel_10("resources/Models/Cross_piece/Cross_piece.obj");
	Model ourModel_11("resources/Models/Bend_piece/Bend_piece.obj");
	Model ourModel_12("resources/Models/Coridor_piece/Coridor_piece.obj");
	Model ourModel_13("resources/Models/Dead_piece/Deadend_piece.obj");
	Model ourModel_14("resources/Models/T_piece/T_piece.obj");
	Model ourModel_15("resources/Models/Start_piece/Start_piece.obj");
	Model ourModel_16("resources/Models/Finish_piece/Finish_piece.obj");
	
	vector<Model> Modles
	{
		ourModel,
		ourModel_1,
		ourModel_2,
		ourModel_3,
		ourModel_4,
		ourModel_5,
		ourModel_6,
		ourModel_7,
		ourModel_8,
		ourModel_9,
		ourModel_10,
		ourModel_11,
		ourModel_12,
		ourModel_13,
		ourModel_14,
		ourModel_15,
		ourModel_16
	};

	while (!glfwWindowShouldClose(window))
	{	
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		view = glm::lookAt(Position, Position + cameraFront, cameraUp);
		projection = glm::perspective(45.0f, 4.0f / 3, 0.1f, 20.0f);
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.4f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));

		// Adding all matrices up to create combined matrix
		glm::mat4 mvp = projection * view * model;

		//adding the Uniform to the shader
		int mvpLoc = glGetUniformLocation(ourShader.Program, "mvp");
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glUseProgram(ourShader.Program);
		if (menu) {
			Position = glm::vec3(0.0f, 0.0f, 0.0f);
			cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
			cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, NULL);
			Modles[8].Draw(ourShader);
		}
		else if (create) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, NULL);
			Modles[9].Draw(ourShader);
			Creator(Maze, Modles, ourShader);
		}else if (finish) {
			Position = glm::vec3(0.0f, 0.0f, 0.0f);
			cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
			cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, NULL);
			Modles[7].Draw(ourShader);
		}
		else if (play) {
			glfwSetCursorPosCallback(window, mouse_callback);
			// input
			processInput(window);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			// rendering commands here
			glUseProgram(ourShader.Program);

			Build_Maze(Maze, Modles, ourShader);

			glDepthFunc(GL_LEQUAL);
			glUseProgram(skyboxShader.Program);
			view = glm::mat4(glm::mat3(lookAt(Position, Position + cameraFront, cameraUp)));
			glm::mat4 mp = projection * view;
			int mpLoc = glGetUniformLocation(skyboxShader.Program, "mp");
			glUniformMatrix4fv(mpLoc, 1, GL_FALSE, glm::value_ptr(mp));
			// skybox cube
			glBindVertexArray(skyboxVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
			glDepthFunc(GL_LESS);
		}

		// check and call events and swap the buffers
		glfwSwapBuffers(window);	
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}