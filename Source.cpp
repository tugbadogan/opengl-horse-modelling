
#include "Angel.h"
#include <assert.h>

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

const int NumVertices = 36;	//(6 faces)(2 triangles/face)(3 vertices/triangle)
int count = 0;
int running_count = 0;

point4 points[NumVertices];
vec3   normals[NumVertices];

point4 vertices[8] = {
	point4(-0.5, -0.5, 0.5, 1.0),
	point4(-0.5, 0.5, 0.5, 1.0),
	point4(0.5, 0.5, 0.5, 1.0),
	point4(0.5, -0.5, 0.5, 1.0),
	point4(-0.5, -0.5, -0.5, 1.0),
	point4(-0.5, 0.5, -0.5, 1.0),
	point4(0.5, 0.5, -0.5, 1.0),
	point4(0.5, -0.5, -0.5, 1.0)
};

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}


//----------------------------------------------------------------------------

class MatrixStack {
	int    _index;
	int    _size;
	mat4*  _matrices;

public:
	MatrixStack(int numMatrices = 32) :_index(0), _size(numMatrices)
	{
		_matrices = new mat4[numMatrices];
	}

	~MatrixStack()
	{
		delete[]_matrices;
	}

	void push(const mat4& m) {
		assert(_index + 1 < _size);
		_matrices[_index++] = m;
	}

	mat4& pop(void) {
		assert(_index - 1 >= 0);
		_index--;
		return _matrices[_index];
	}
};

MatrixStack  mvstack;
mat4         model_view;
GLuint       ModelView, Projection;

int Index = 0;

//----------------------------------------------------------------------------

#define TORSO_HEIGHT 3.0
#define TORSO_WIDTH 10.0
#define TORSO_DEPTH 3.0

#define NECK_HEIGHT 5.0
#define NECK_WIDTH 2.0
#define NECK_DEPTH 2.0

#define UPPER_ARM_HEIGHT 3.0
#define LOWER_ARM_HEIGHT 3.0
#define UPPER_ARM_WIDTH  1.0
#define LOWER_ARM_WIDTH  1.0

#define UPPER_LEG_HEIGHT 3.0
#define LOWER_LEG_HEIGHT 3.0
#define UPPER_LEG_WIDTH  1.0
#define LOWER_LEG_WIDTH  1.0

#define HEAD_HEIGHT 3.0
#define HEAD_WIDTH 1.5
#define HEAD_DEPTH 2.0

// Set up menu item indices, which we can alos use with the joint angles
enum {
	Torso = 0,
	Head = 1,
	LeftUpperArm = 2,
	LeftLowerArm = 3,
	RightUpperArm = 4,
	RightLowerArm = 5,
	LeftUpperLeg = 6,
	LeftLowerLeg = 7,
	RightUpperLeg = 8,
	RightLowerLeg = 9,
	Neck = 10,
	NumNodes,
	Quit
};

// Joint angles with initial values
GLfloat
theta[NumNodes] = {
	0.0,	// Torso
	-80.0,	// Head
	180.0,	// LeftUpperArm
	0.0,	// LeftLowerArm
	180.0,	// RightUpperArm
	0.0,	// RightLowerArm
	170.0,	// LeftUpperLeg
	10.0,	// LeftLowerLeg
	170.0,	// RightUpperLeg
	10.0,	// RightLowerLeg
	-45.0	// Neck
};

int NEED_STEP = 10;
int status;
enum {
	Stop = 0,
	Walking = 1,
	Running = 2
};
int total = 0;
GLfloat
target[NumNodes] = {
	0.0,	// Torso
	0.0,	// Head
	0.0,	// LeftUpperArm
	0.0,	// LeftLowerArm
	0.0,	// RightUpperArm
	0.0,	// RightLowerArm
	0.0,	// LeftUpperLeg
	0.0,	// LeftLowerLeg
	0.0,	// RightUpperLeg
	0.0,	// RightLowerLeg
	0.0	// Neck
};

GLint angle = Torso;

//----------------------------------------------------------------------------

struct Node {
	mat4  transform;
	void(*render)(void);
	Node* sibling;
	Node* child;

	Node() :
		render(NULL), sibling(NULL), child(NULL) {}

	Node(mat4& m, void(*render)(void), Node* sibling, Node* child) :
		transform(m), render(render), sibling(sibling), child(child) {}
};

Node  nodes[NumNodes];

//----------------------------------------------------------------------------

void
quad(int a, int b, int c, int d)
{
	// Initialize temporary vectors along the quad's edge to
	// Compute its face normal 
	vec4 u = vertices[b] - vertices[a];
	vec4 v = vertices[c] - vertices[b];

	vec3 normal = normalize(cross(u, v));
	normals[Index] = normal; points[Index] = vertices[a]; Index++;
	normals[Index] = normal; points[Index] = vertices[b]; Index++;
	normals[Index] = normal; points[Index] = vertices[c]; Index++;
	normals[Index] = normal; points[Index] = vertices[a]; Index++;
	normals[Index] = normal; points[Index] = vertices[c]; Index++;
	normals[Index] = normal; points[Index] = vertices[d]; Index++;
}

void
colorcube(void)
{
	quad(1, 0, 3, 2);
	quad(2, 3, 7, 6);
	quad(3, 0, 4, 7);
	quad(6, 5, 1, 2);
	quad(4, 5, 6, 7);
	quad(5, 4, 0, 1);
}

//----------------------------------------------------------------------------

void
traverse(Node* node)
{
	if (node == NULL) { return; }

	mvstack.push(model_view);

	model_view *= node->transform;
	node->render();

	if (node->child) { traverse(node->child); }

	model_view = mvstack.pop();

	if (node->sibling) { traverse(node->sibling); }
}

//----------------------------------------------------------------------------

void
torso()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * TORSO_HEIGHT, 0.0) * Scale(TORSO_WIDTH, TORSO_HEIGHT, TORSO_DEPTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
neck()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * NECK_HEIGHT, 0.0) * Scale(NECK_WIDTH, NECK_HEIGHT, NECK_DEPTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
head()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * HEAD_HEIGHT, 0.0) * Scale(HEAD_WIDTH, HEAD_HEIGHT, HEAD_DEPTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
left_upper_arm()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0) * Scale(UPPER_ARM_WIDTH, UPPER_ARM_HEIGHT, UPPER_ARM_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
left_lower_arm()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0) * Scale(LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT, LOWER_ARM_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
right_upper_arm()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0) * Scale(UPPER_ARM_WIDTH, UPPER_ARM_HEIGHT, UPPER_ARM_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
right_lower_arm()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0) * Scale(LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT, LOWER_ARM_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
left_upper_leg()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * UPPER_LEG_HEIGHT, 0.0) * Scale(UPPER_LEG_WIDTH, UPPER_LEG_HEIGHT, UPPER_LEG_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
left_lower_leg()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * LOWER_LEG_HEIGHT, 0.0) * Scale(LOWER_LEG_WIDTH, LOWER_LEG_HEIGHT, LOWER_LEG_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
right_upper_leg()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * UPPER_LEG_HEIGHT, 0.0) * Scale(UPPER_LEG_WIDTH, UPPER_LEG_HEIGHT, UPPER_LEG_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

void
right_lower_leg()
{
	mvstack.push(model_view);

	mat4 instance = (Translate(0.0, 0.5 * LOWER_LEG_HEIGHT, 0.0) * Scale(LOWER_LEG_WIDTH, LOWER_LEG_HEIGHT, LOWER_LEG_WIDTH));

	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	model_view = mvstack.pop();
}

//----------------------------------------------------------------------------

void
display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	traverse(&nodes[Torso]);
	glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		theta[angle] += 5.0;
		if (theta[angle] > 360.0) { theta[angle] -= 360.0; }
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		theta[angle] -= 5.0;
		if (theta[angle] < 0.0) { theta[angle] += 360.0; }
	}

	mvstack.push(model_view);

	switch (angle) {
	case Torso:
		nodes[Torso].transform =
			RotateY(theta[Torso]);
		break;

	case Head:
		nodes[Head].transform =
			Translate(0.0, TORSO_HEIGHT + 0.5*HEAD_HEIGHT, 0.0) * RotateZ(theta[Head]) * Translate(0.0, -0.5*HEAD_HEIGHT, 0.0);
		break;

	case LeftUpperArm:
		nodes[LeftUpperArm].transform =
			Translate(-(TORSO_WIDTH + UPPER_ARM_WIDTH), 0.9*TORSO_HEIGHT, 0.0) * RotateX(theta[LeftUpperArm]);
		break;

	case RightUpperArm:
		nodes[RightUpperArm].transform =
			Translate(TORSO_WIDTH + UPPER_ARM_WIDTH, 0.9*TORSO_HEIGHT, 0.0) * RotateX(theta[RightUpperArm]);
		break;

	case RightUpperLeg:
		nodes[RightUpperLeg].transform =
			Translate(TORSO_WIDTH + UPPER_LEG_WIDTH, 0.1*UPPER_LEG_HEIGHT, 0.0) * RotateX(theta[RightUpperLeg]);
		break;

	case LeftUpperLeg:
		nodes[LeftUpperLeg].transform =
			Translate(-(TORSO_WIDTH + UPPER_LEG_WIDTH), 0.1*UPPER_LEG_HEIGHT, 0.0) * RotateX(theta[LeftUpperLeg]);
		break;

	case LeftLowerArm:
		nodes[LeftLowerArm].transform =
			Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateX(theta[LeftLowerArm]);
		break;

	case LeftLowerLeg:
		nodes[LeftLowerLeg].transform =
			Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateX(theta[LeftLowerLeg]);
		break;

	case RightLowerLeg:
		nodes[RightLowerLeg].transform =
			Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateX(theta[RightLowerLeg]);
		break;

	case RightLowerArm:
		nodes[RightLowerArm].transform =
			Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateX(theta[RightLowerArm]);
		break;
	}

	model_view = mvstack.pop();
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void rotate(int which, float angle) {
	theta[which] = angle;
	if (theta[which] > 360.0)
		theta[which] -= 360.0;
	if (theta[which] < 0)
		theta[which] += 360.0;
	switch (which) {
	case Torso:
		nodes[Torso].transform =
			RotateY(theta[Torso]);
		break;

	case Head:
		nodes[Head].transform =
			Translate(0.0, NECK_HEIGHT, 0.0) * RotateZ(theta[Head]);
		break;
	case Neck:
		nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
		break;
	case LeftUpperArm:
		nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);
		break;

	case RightUpperArm:
		nodes[RightUpperArm].transform =
			Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_ARM_HEIGHT, 1.5 - UPPER_ARM_WIDTH / 2) * RotateZ(theta[RightUpperArm]);
		break;

	case RightUpperLeg:
		nodes[RightUpperLeg].transform =
			Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);
		break;

	case LeftUpperLeg:
		nodes[LeftUpperLeg].transform =
			Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);
		break;

	case LeftLowerArm:
		nodes[LeftLowerArm].transform =
			Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);
		break;

	case LeftLowerLeg:
		nodes[LeftLowerLeg].transform =
			Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);
		break;

	case RightLowerLeg:
		nodes[RightLowerLeg].transform =
			Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);
		break;

	case RightLowerArm:
		nodes[RightLowerArm].transform =
			Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);
		break;
	}
}

void
menu(int option)
{
	if (option == Quit)
	{
		exit(EXIT_SUCCESS);
	}

	angle = option;
}

//----------------------------------------------------------------------------

void
reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	GLfloat left = -10.0, right = 10.0;
	GLfloat bottom = -10.0, top = 10.0;
	GLfloat zNear = -10.0, zFar = 10.0;

	GLfloat aspect = GLfloat(width) / height;

	if (aspect > 1.0)
	{
		left *= aspect;
		right *= aspect;
	}
	else
	{
		bottom /= aspect;
		top /= aspect;
	}

	mat4 projection = Ortho(left, right, bottom, top, zNear, zFar);
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

	model_view = mat4(1.0);   // An Identity matrix
}

//----------------------------------------------------------------------------

void
initNodes(void)
{
	mat4  m;

	m = RotateY(theta[Torso]);
	nodes[Torso] = Node(m, torso, NULL, &nodes[Neck]);

	m = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
	nodes[Neck] = Node(m, neck, &nodes[LeftUpperArm], &nodes[Head]);

	m = Translate(0.0, NECK_HEIGHT, 0.0) * RotateZ(theta[Head]);
	nodes[Head] = Node(m, head, NULL, NULL);

	m = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);
	nodes[LeftUpperArm] = Node(m, left_upper_arm, &nodes[RightUpperArm], &nodes[LeftLowerArm]);

	m = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_ARM_HEIGHT, 1.5 - UPPER_ARM_WIDTH / 2) * RotateZ(theta[RightUpperArm]);
	nodes[RightUpperArm] = Node(m, right_upper_arm, &nodes[LeftUpperLeg], &nodes[RightLowerArm]);

	m = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);
	nodes[LeftUpperLeg] = Node(m, left_upper_leg, &nodes[RightUpperLeg], &nodes[LeftLowerLeg]);

	m = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);
	nodes[RightUpperLeg] = Node(m, right_upper_leg, NULL, &nodes[RightLowerLeg]);

	m = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);
	nodes[LeftLowerArm] = Node(m, left_lower_arm, NULL, NULL);

	m = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);
	nodes[RightLowerArm] = Node(m, right_lower_arm, NULL, NULL);

	m = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);
	nodes[LeftLowerLeg] = Node(m, left_lower_leg, NULL, NULL);

	m = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);
	nodes[RightLowerLeg] = Node(m, right_lower_leg, NULL, NULL);
}

//----------------------------------------------------------------------------

void
init(void)
{
	colorcube();

	// Initialize tree
	initNodes();

	theta[Torso] = 50.0;
	nodes[Torso].transform = RotateY(theta[Torso]);

	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(normals), NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals);

	// Load shaders and use the resulting shader program
	GLuint program = InitShader("vshader_a9.glsl", "fshader_a9.glsl");
	glUseProgram(program);

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));

	// Initialize shader lighting parameters
	point4 light_position(0.0, 0.0, -1.0, 0.0);
	color4 light_ambient(0.2, 0.2, 0.2, 1.0);
	color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
	color4 light_specular(1.0, 1.0, 1.0, 1.0);

	color4 material_ambient(1.0, 0.0, 1.0, 1.0);
	color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
	color4 material_specular(1.0, 0.8, 0.0, 1.0);
	float  material_shininess = 100.0;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, specular_product);

	glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, light_position);

	glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);

	ModelView = glGetUniformLocation(program, "ModelView");
	Projection = glGetUniformLocation(program, "Projection");

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClearColor(0.8, 1.0, 0.6, 1.0);
}

//----------------------------------------------------------------------------
void
step1()
{
	// First Step
	target[LeftUpperArm] = -150; // -150
	target[LeftLowerArm] = -90; // -90
	target[Neck] = -55;
}

void
step2()
{
	// Second Step
	target[RightUpperLeg] = 125; // 125
	target[RightLowerLeg] = 60; // 60
	target[LeftLowerArm] = -50; // 40
	target[Neck] = -45;
}

void
step3()
{
	// Third Step
	target[RightUpperLeg] = 170; // 170
	target[RightLowerLeg] = 70; // 70
	target[LeftLowerArm] = -20; // -20
	target[Neck] = -55;
}

void
step4()
{
	// Forth Step
	target[RightLowerLeg] = 60; // 60
	target[RightLowerArm] = -50; // -50
	target[Neck] = -45;
}

void
step5()
{
	// Fifth Step
	target[RightLowerLeg] = 40; // 40
	target[RightUpperArm] = -150; // 210
	target[RightLowerArm] = -80; // -80
	target[Neck] = -55;
}

void
step6()
{
	// Sixth Step
	target[LeftUpperLeg] = 150; // 150
	target[LeftLowerLeg] = -15; // -15
	target[RightLowerArm] = 50; // 50
	target[Neck] = -45;
}

void
step7()
{
	// Seventh Step
	target[LeftLowerLeg] = 30; // 30
	target[RightLowerArm] = 30; // 30
	target[Neck] = -55;
}

void
step8()
{
	// Eight Step
	target[LeftUpperLeg] = -150; // -150
	target[LeftLowerLeg] = -15; // -15
	target[LeftUpperArm] = 140; // 140
	target[LeftLowerArm] = 20; // 20
	target[Neck] = -55;
}

void
walking()
{
	// First Step
	theta[LeftUpperArm] = -150;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);
	theta[LeftLowerArm] = -90;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	// Second Step
	theta[RightUpperLeg] = 125;
	nodes[RightUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);
	theta[RightLowerLeg] = 60;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);
	theta[LeftLowerArm] = -50;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	// Third Step
	theta[RightUpperLeg] = 170;
	nodes[RightUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);
	theta[RightLowerLeg] = 70;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);
	theta[LeftLowerArm] = -20;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	// Forth Step
	theta[RightLowerLeg] = 60;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);
	theta[RightLowerArm] = -50;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	// Fifth Step
	theta[RightLowerLeg] = 40;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);
	theta[RightUpperArm] = -150;
	nodes[RightUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_ARM_HEIGHT, 1.5 - UPPER_ARM_WIDTH / 2) * RotateZ(theta[RightUpperArm]);
	theta[RightLowerArm] = -80;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	// Sixth Step
	theta[LeftUpperLeg] = 150;
	nodes[LeftUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);
	theta[LeftLowerLeg] = -15;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);
	theta[RightLowerArm] = 50;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	// Seventh Step
	theta[LeftLowerLeg] = 30;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);
	theta[RightLowerArm] = 30;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);
	
	// Eight Step
	theta[LeftUpperLeg] = -150;
	nodes[LeftUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);
	theta[LeftLowerLeg] = -15;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);
	theta[LeftUpperArm] = 140;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);
	theta[LeftLowerArm] = 20;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);
}

void
running_step1()
{
	theta[RightUpperArm] = -150;
	nodes[RightUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_ARM_HEIGHT, 1.5 - UPPER_ARM_WIDTH / 2) * RotateZ(theta[RightUpperArm]);

	theta[RightLowerArm] = -80;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	theta[LeftUpperArm] = -100;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);

	theta[LeftLowerArm] = -80;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[RightUpperLeg] = -150;
	nodes[RightUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);

	theta[LeftUpperLeg] = -120;
	nodes[LeftUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);

	theta[LeftLowerLeg] = 80;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);

	theta[RightLowerLeg] = 80;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);

	theta[Neck] = -55;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step2()
{
	theta[LeftLowerArm] = -30;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[RightLowerArm] = -40;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	theta[RightLowerLeg] = 40;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);

	theta[LeftUpperLeg] = 140;
	nodes[LeftUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);
	
	theta[Neck] = -45;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step3()
{
	theta[LeftUpperArm] = -160;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);

	theta[LeftLowerArm] = -10;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);
	
	theta[RightLowerArm] = 30;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	theta[LeftLowerLeg] = -10;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);

	theta[Neck] = -55;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step4()
{
	theta[LeftUpperArm] = -180;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);

	theta[LeftLowerArm] = -10;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[RightUpperLeg] = 140;
	nodes[RightUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);

	theta[RightLowerLeg] = 10;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);

	theta[Neck] = -45;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step5()
{
	theta[RightUpperArm] = -170;
	nodes[RightUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_ARM_HEIGHT, 1.5 - UPPER_ARM_WIDTH / 2) * RotateZ(theta[RightUpperArm]);
	
	theta[RightLowerArm] = 10;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	theta[LeftUpperArm] = 140;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);
	
	theta[LeftLowerArm] = -25;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[LeftUpperLeg] = 160;
	nodes[LeftUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);
	
	theta[LeftLowerLeg] = 60;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);

	theta[Neck] = -55;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step6()
{
	theta[RightLowerArm] = -100;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);
	
	theta[RightLowerLeg] = 80;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);

	theta[LeftUpperArm] = 160;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);

	theta[LeftLowerArm] = -15;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[RightUpperLeg] = 160;
	nodes[RightUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);

	theta[Neck] = -55;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step7()
{
	theta[RightUpperArm] = 140;
	nodes[RightUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_ARM_HEIGHT, 1.5 - UPPER_ARM_WIDTH / 2) * RotateZ(theta[RightUpperArm]);

	theta[RightLowerArm] = 10;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	theta[LeftUpperArm] = 180;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);

	theta[LeftLowerArm] = -85;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[RightLowerLeg] = 95;
	nodes[RightLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[RightLowerLeg]);

	theta[LeftLowerLeg] = 85;
	nodes[LeftLowerLeg].transform = Translate(0.0, UPPER_LEG_HEIGHT, 0.0) * RotateZ(theta[LeftLowerLeg]);

	theta[Neck] = -45;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);
}

void
running_step8()
{
	theta[LeftUpperArm] = 195;
	nodes[LeftUpperArm].transform = Translate(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2, 0.1*UPPER_LEG_HEIGHT, -1.5 + UPPER_LEG_WIDTH / 2) * RotateZ(theta[LeftUpperArm]);

	theta[RightUpperLeg] = 140;
	nodes[RightUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_LEG_WIDTH / 2), 0.1*UPPER_LEG_HEIGHT, 1.5 - UPPER_LEG_WIDTH / 2) * RotateZ(theta[RightUpperLeg]);

	theta[LeftUpperLeg] = 140;
	nodes[LeftUpperLeg].transform = Translate(-(TORSO_WIDTH / 2 - UPPER_ARM_WIDTH / 2), 0.1*UPPER_ARM_HEIGHT, -1.5 + UPPER_ARM_WIDTH / 2) * RotateZ(theta[LeftUpperLeg]);

	theta[LeftLowerArm] = -45;
	nodes[LeftLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[LeftLowerArm]);

	theta[RightLowerArm] = -70;
	nodes[RightLowerArm].transform = Translate(0.0, UPPER_ARM_HEIGHT, 0.0) * RotateZ(theta[RightLowerArm]);

	theta[Neck] = -55;
	nodes[Neck].transform = Translate(TORSO_WIDTH / 2 - NECK_WIDTH / 2, TORSO_HEIGHT, 0.0) * RotateZ(theta[Neck]);

	running_count = 0;
}

void walking_step(int current_step) {
	if (current_step == 1)
	{
		step1();
	}
	else if (current_step == 2)
	{
		step2();
	}
	else if (current_step == 3)
	{
		step3();
	}
	else if (current_step == 4)
	{
		step4();
	}
	else if (current_step == 5)
	{
		step5();
	}
	else if (current_step == 6)
	{
		step6();
	}
	else if (current_step == 7)
	{
		step7();
	}
	else
	{
		step8();
	}
}
void running_step(int running_count) {
	if (running_count == 1)
	{
		running_step1();
	}

	else if (running_count == 2)
	{
		running_step2();
	}
	else if (running_count == 3)
	{
		running_step3();
	}
	else if (running_count == 4)
	{
		running_step4();
	}
	else if (running_count == 5)
	{
		running_step5();
	}
	else if (running_count == 6)
	{
		running_step6();
	}
	else if (running_count == 7)
	{
		running_step7();
	}
	else
	{
		running_step8();
	}
}

int current_step;
int processed_steps = 0;
void idle() {
	if (status != Stop) {
		if (processed_steps == 0 && status == Walking) {
			for (int i = 0; i < NumNodes; i++) {
				target[i] = 0;
			}
			walking_step(current_step);
			for (int i = 0; i < NumNodes; i++)
			{
				if (target[i] != 0)
					rotate(i, target[i]);
			}
		}
		else if (processed_steps == 0 && status == Running)
			running_step(current_step);

		if (processed_steps == NEED_STEP)
		{
			printf("Step %d\n", current_step);
			current_step++;
			if (current_step == 9) {
				current_step = 1;
			}
			processed_steps = 0;
		}
		else {
			processed_steps++;
		}
	}
	glutPostRedisplay();
}

void
keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 033: // Escape Key
		case 'q': case 'Q':
			exit(EXIT_SUCCESS);
			break;
		case 'a':
			NEED_STEP = 20;
			current_step = 1;
			status = Walking;
			processed_steps = 0;
			break;
		case 'b':
			NEED_STEP = 10;
			current_step = 1;
			status = Running;
			processed_steps = 0;
			break;
	}
}

//----------------------------------------------------------------------------

int
main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutInitContextVersion(3, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow("robot");
	
	//26.08.2011 ATES - GLEW kutuphanesiyle ilgili bir sorundan oturu asagidaki satiri eklemezsek init() metodu icerisinde
	//glGenVertexArrays cagirildiginda access violation hatasi veriyor
	glewExperimental = GL_TRUE;

	glewInit();

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutIdleFunc(idle);

	glutCreateMenu(menu);
	glutAddMenuEntry("torso", Torso);
	glutAddMenuEntry("head", Head);
	glutAddMenuEntry("right_upper_arm", RightUpperArm);
	glutAddMenuEntry("right_lower_arm", RightLowerArm);
	glutAddMenuEntry("left_upper_arm", LeftUpperArm);
	glutAddMenuEntry("left_lower_arm", LeftLowerArm);
	glutAddMenuEntry("right_upper_leg", RightUpperLeg);
	glutAddMenuEntry("right_lower_leg", RightLowerLeg);
	glutAddMenuEntry("left_upper_leg", LeftUpperLeg);
	glutAddMenuEntry("left_lower_leg", LeftLowerLeg);
	glutAddMenuEntry("quit", Quit);
	glutAttachMenu(GLUT_MIDDLE_BUTTON);

	glutMainLoop();
	return 0;
}
