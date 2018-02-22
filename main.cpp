#include <windows.h>
#include <iostream>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include <GL\glew.h>
#include <gl\freeglut.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <GL\glaux.h>

using namespace glm;
using namespace std;

GLuint Program;

glm::mat4 NormalMatrix;
glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

AUX_RGBImageRec *image;
unsigned int image_tex;

float zoom = 5.0;
float zoom_prev = 5.0;

struct DirectLight{
	vec4 direction;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct ProjectLight {
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 attenuation;
	vec3 spotDirection;
	float spotExponent;
	float spotCos;
};

struct Material{
	GLuint texture;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct Camera {
	vec3 position;
	vec3 rotation;
	mat4 projection;
} camera;

struct vertex
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

//! Проверка ошибок OpenGL, если есть то вывод в консоль тип ошибки
void checkOpenGLerror()
{
	setlocale(LC_ALL, "Russian");
	GLenum errCode;
	if ((errCode = glGetError()) != GL_NO_ERROR)
		std::cout << "OpenGl error! - " << gluErrorString(errCode);
}

struct Mesh
{
	GLuint vao;
	GLuint vertexbuffer;
	GLuint texturebuffer;
	GLuint normalbuffer;
	GLuint texture;
	int vertices_count;
};

Mesh african_head;
Mesh suzanne;
Mesh cube;

bool loadOBJ(const char * path, std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals) {
	std::cout << "Loading OBJ file..." << std::endl;

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if (file == NULL) {
		std::cout << "Impossible to open the file." << std::endl;
		getchar();
		return false;
	}

	while (true) {
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], 
				&vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else {
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}
	}

	for (unsigned int i = 0; i<vertexIndices.size(); i++) {

		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}
	fclose(file);
	return true;
}

Mesh initMesh(const char *path)
{
	Mesh mesh;

	vector<vec3> vertcoords;
	vector<vec2> textcoords;
	vector<vec3> normcoords;

	loadOBJ(path, vertcoords, textcoords, normcoords);

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glEnableVertexAttribArray(0);
	glGenBuffers(1, &mesh.vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertcoords.size() * sizeof(vec3), &vertcoords[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, NULL);

	glEnableVertexAttribArray(1);
	glGenBuffers(1, &mesh.normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, normcoords.size() * sizeof(vec3), &normcoords[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, NULL);

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &mesh.texturebuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.texturebuffer);
	glBufferData(GL_ARRAY_BUFFER, textcoords.size() * sizeof(vec2), &textcoords[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 0, NULL);

	mesh.vertices_count = vertcoords.size();
	return mesh;
}

void drawMesh(struct Mesh mesh) {

	glBindVertexArray(mesh.vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertices_count);
}

void initMeshes()
{
	african_head = initMesh("african_head.obj");
	suzanne = initMesh("suzanne.obj");
	cube = initMesh("cube.obj");
	checkOpenGLerror();
}

void initGL() {
	glClearColor(0.2, 0.2, 0.2, 0);
	glEnable(GL_DEPTH_TEST);
}

//! Функция печати лога шейдера
void shaderLog(unsigned int shader)
{
	int   infologLen = 0;
	int   charsWritten = 0;
	char *infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);

	if (infologLen > 1)
	{
		infoLog = new char[infologLen];
		if (infoLog == NULL)
		{
			std::cout << "ERROR: Could not allocate InfoLog buffer\n";
			exit(1);
		}
		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
		std::cout << "InfoLog: " << infoLog << "\n\n\n";
		delete[] infoLog;
	}
}

GLuint compileSource(const GLchar* source, GLuint shader_type)
{
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

#ifdef _DEBUG
	shaderLog(shader);
#endif
	return shader;
}

GLuint loadSourcefile(const string& source_file_name, GLuint shader_type)
{
	ifstream  file(source_file_name.c_str());
	if (!file)
	{
		std::cout << source_file_name << " file not found\n";
		return  GL_FALSE;
	}

	std::istreambuf_iterator<char>  begin(file), end;
	string  sourceStr(begin, end);
	file.close();
	return  compileSource(sourceStr.c_str(), shader_type);
}

//! Инициализация шейдеров
void initShader()
{
	//! Переменные для хранения идентификаторов шейдеров
	GLuint vShader = loadSourcefile("vertshader(dirl).txt", GL_VERTEX_SHADER);
	GLuint fShader = loadSourcefile("fragshader(dirl).txt", GL_FRAGMENT_SHADER);

	//! Создаем программу и прикрепляем шейдеры к ней
	Program = glCreateProgram();
	glAttachShader(Program, vShader);
	glAttachShader(Program, fShader);

	//! Линкуем шейдерную программу
	glLinkProgram(Program);

	//! Проверяем статус сборки
	int link_ok;
	glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
	if (!link_ok)
	{
		std::cout << "error attach shaders \n";
		return;
	}

	checkOpenGLerror();
}

//! Освобождение шейдеров
void freeShader()
{
	//! Передавая ноль, мы отключаем шейдрную программу
	glUseProgram(0);
	//! Удаляем шейдерную программу
	glDeleteProgram(Program);
}

void resizeWindow(int width, int height)
{
	glViewport(0, 0, width, height);
	height = height > 0 ? height : 1;
	const GLfloat aspectRatio = (GLfloat)width / (GLfloat)height;

	camera.position = vec3(0.0f, 0.0f, zoom);
	camera.rotation = vec3(0.0f, 0.0f, 0.0f);
	camera.projection = perspective(radians(45.0f), aspectRatio, 1.0f, 200.0f);

	ProjectionMatrix = perspective(radians(45.0f), aspectRatio, 1.0f, 200.0f);
	ProjectionMatrix = translate(ProjectionMatrix, -camera.position);
	ViewMatrix = rotate(ProjectionMatrix, camera.rotation.x, vec3(1.0f, 0.0f, 0.0f));
	ViewMatrix = rotate(ViewMatrix, camera.rotation.y, vec3(0.0f, 1.0f, 0.0f));
	ViewMatrix = rotate(ViewMatrix, camera.rotation.z, vec3(0.0f, 0.0f, 1.0f));
}

//инициализация направленного света
void initDirLight() {
	DirectLight directional;
	directional.direction = vec4(4.0, 1.0, 2.0, 0.0);
	directional.ambient = vec4(0.3, 0.2, 0.2, 1.0);
	directional.diffuse = vec4(0.7, 0.5, 0.5, 1.0);
	directional.specular = vec4(0.9, 0.9, 0.9, 1.0);

	glUniform4fv(glGetUniformLocation(Program, "light.direction"), 1, &directional.direction[0]);
	glUniform4fv(glGetUniformLocation(Program, "light.ambient"), 1, &directional.ambient[0]);
	glUniform4fv(glGetUniformLocation(Program, "light.diffuse"), 1, &directional.diffuse[0]);
	glUniform4fv(glGetUniformLocation(Program, "light.specular"), 1, &directional.specular[0]);

	checkOpenGLerror();
}

//инициализация прожектора
void initProjLight() {
	float Pi = 3.14f;
	ProjectLight projector;
	projector.ambient = vec4(0.3, 0.2, 0.2, 1.0);
	projector.diffuse = vec4(0.7, 0.5, 0.5, 1.0);
	projector.specular = vec4(0.9, 0.9, 0.9, 1.0);
	projector.attenuation = vec3(1, 0.1, 0);
	projector.position = vec4(0, 0, 3.5, 1.0);
	projector.spotDirection = vec3(0.0, 0.0, -1.0);
	float angle = 27.0f;
	projector.spotExponent = 20.0f;
	projector.spotCos = cos((angle*Pi) / 180);

	glUniform4fv(glGetUniformLocation(Program, "light.position"), 1, &projector.position[0]);
	glUniform4fv(glGetUniformLocation(Program, "light.ambient"), 1, &projector.ambient[0]);
	glUniform4fv(glGetUniformLocation(Program, "light.diffuse"), 1, &projector.diffuse[0]);
	glUniform4fv(glGetUniformLocation(Program, "light.specular"), 1, &projector.specular[0]);
	glUniform3fv(glGetUniformLocation(Program, "light.attenuation"), 1, &projector.attenuation[0]);
	glUniform3fv(glGetUniformLocation(Program, "light.spotDirection"), 1, &projector.spotDirection[0]);
	glUniform1f(glGetUniformLocation(Program, "light.spotCos"), projector.spotCos);
	glUniform1f(glGetUniformLocation(Program, "light.spotExponent"), projector.spotExponent);

	checkOpenGLerror();
}

//инициализация материала
void initMaterial(Material &material) {
	glUniform4fv(glGetUniformLocation(Program, "material.ambient"), 1, &material.ambient[0]);
	glUniform4fv(glGetUniformLocation(Program, "material.diffuse"), 1, &material.diffuse[0]);
	glUniform4fv(glGetUniformLocation(Program, "material.specular"), 1, &material.specular[0]);
	glUniform1f(glGetUniformLocation(Program, "material.shininess"), material.shininess);
	checkOpenGLerror();
}

//инициализация матрицы модели
void initCamera(mat4 &model) {
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.viewProjection"), 1, GL_FALSE, &ViewMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, &NormalMatrix[0][0]);
	//передаём позицию наблюдателя--камеры
	glUniform3fv(glGetUniformLocation(Program, "transform.viewPosition"), 1, &camera.position[0]);
	checkOpenGLerror();
}

GLuint loadBMP_custom(const char * imagepath) {

	printf("Reading image %s\n", imagepath);

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(imagepath, "rb");
	if (!file) {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
		getchar();
		return 0;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    fclose(file); return 0; }
	if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    fclose(file); return 0; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

										 // Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	// Everything is in memory now, the file can be closed.
	fclose(file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete[] data;

	// Poor filtering, or ...
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	// ... nice trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// ... which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	// Return the ID of the texture we just created
	return textureID;
}

//! Отрисовка
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(Program);

	initDirLight();
	//initProjLight();

	Material material;
	material.ambient = vec4(0.105882f, 0.058824f, 0.113725f, 1.0f);
	material.diffuse = vec4(0.427451f, 0.470588f, 0.541176f, 1.0f);
	material.specular = vec4(0.333333f, 0.333333f, 0.521569f, 1.0f);
	material.shininess = 9.84615f;
	initMaterial(material);

	mat4 model = mat4(1.0);
	model = translate(model, vec3(-1.3, 0, 0));
	NormalMatrix = transpose((inverse(model)));
	initCamera(model);
	//glUniformMatrix4fv(glGetUniformLocation(Program, "MVP"), 1, GL_FALSE, &NormalMatrix[0][0]);
	
	drawMesh(african_head);

	
	Material material2;
	material2.ambient = vec4(0.1745f, 0.01175f, 0.01175f, 0.55f);
	material2.diffuse = vec4(0.61424f, 0.04136f, 0.04136f, 0.55f);
	material2.specular = vec4(0.727811f, 0.626959f, 0.626959f, 0.55f);
	material2.shininess = 76.8f;
	initMaterial(material2);

	AUX_RGBImageRec *image = auxDIBImageLoad("uvmap.bmp");
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &material2.texture);
	glBindTexture(GL_TEXTURE_2D, material2.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image->sizeX, image->sizeY, 0, GL_BGR, GL_UNSIGNED_BYTE, image->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glUniform1i(glGetUniformLocation(Program, "material.texture"), 0);

	mat4 model2 = mat4(1.0);
	model2 = translate(model2, vec3(0.9, 0, 0));
	model2 = scale(model2, vec3(0.8, 0.8, 0.8));
	NormalMatrix = transpose((inverse(model2)));
	initCamera(model2);
	//glUniformMatrix4fv(glGetUniformLocation(Program, "MVP"), 1, GL_FALSE, &NormalMatrix[0][0]);

	drawMesh(suzanne);

	Material material3;
	material3.ambient = vec4(0.0f, 0.1f, 0.06f, 1.0f);
	material3.diffuse = vec4(0.0f, 0.50980392f, 0.50980392f, 1.0f);
	material3.specular = vec4(0.50196078f, 0.50196078f, 0.50196078f, 1.0f);
	material3.shininess = 32.0f;
	initMaterial(material3);

	mat4 model3 = mat4(1.0);
	model3 = translate(model3, vec3(-0.15, -0.5, 0.7));
	model3 = rotate(model3, 45.0f, vec3(0.8f, -1.0f, .0f));
	model3 = scale(model3, vec3(0.3, 0.3, 0.3));
	//перевод нормалей объекта из локальной системы координат объекта в мировую
	NormalMatrix = transpose((inverse(model3)));
	initCamera(model3);
	//glUniformMatrix4fv(glGetUniformLocation(Program, "MVP"), 1, GL_FALSE, &NormalMatrix[0][0]);

	drawMesh(cube);

	checkOpenGLerror();
	glutSwapBuffers();
}

void specialKeys(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		camera.rotation.x += 0.1f;
		break;
	case GLUT_KEY_DOWN:
		camera.rotation.x -= 0.1f;
		break;
	case GLUT_KEY_RIGHT:
		camera.rotation.y -= 0.1f;
		break;
	case GLUT_KEY_LEFT:
		camera.rotation.y += 0.1f;
		break;

	}
	ViewMatrix = rotate(ProjectionMatrix, camera.rotation.x, vec3(1.0f, 0.0f, 0.0f));
	ViewMatrix = rotate(ViewMatrix, camera.rotation.y, vec3(0.0f, 1.0f, 0.0f));
	ViewMatrix = rotate(ViewMatrix, camera.rotation.z, vec3(0.0f, 0.0f, 1.0f));
	glutPostRedisplay();


	glutPostRedisplay();
}

void mouseWheel(int wheel, int direction, int x, int y)
{
	(direction > 0) ? zoom -= 0.1 : zoom += 0.1;

	camera.position = vec3(0.0f, 0.0f, zoom);

	ProjectionMatrix = translate(ProjectionMatrix, vec3(0.0f, 0.0f, zoom_prev - zoom));
	ViewMatrix = rotate(ProjectionMatrix, camera.rotation.x, vec3(1.0f, 0.0f, 0.0f));
	ViewMatrix = rotate(ViewMatrix, camera.rotation.y, vec3(0.0f, 1.0f, 0.0f));
	ViewMatrix = rotate(ViewMatrix, camera.rotation.z, vec3(0.0f, 0.0f, 1.0f));
	
	zoom_prev = zoom;

	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutCreateWindow("Simple shaders");

	//! Обязательно перед инициализации шейдеров
	GLenum glew_status = glewInit();
	if (GLEW_OK != glew_status)
	{
		//! GLEW не проинициализировалась
		std::cout << "Error: " << glewGetErrorString(glew_status) << "\n";
		return 1;
	}

	//! Проверяем доступность OpenGL 2.0
	if (!GLEW_VERSION_2_0)
	{
		//! OpenGl 2.0 оказалась не доступна
		std::cout << "No support for OpenGL 2.0 found\n";
		return 1;
	}

	initGL();

	initMeshes();
	initShader();

	glutReshapeFunc(resizeWindow);
	glutDisplayFunc(render);

	glutSpecialFunc(specialKeys);
	glutMouseWheelFunc(mouseWheel);

	glutMainLoop();
}