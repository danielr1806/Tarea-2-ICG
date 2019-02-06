//Daniel Rodriguez	CI: 24883818
#include <Windows.h>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <iostream>
#include <GL\AntTweakBar.h>
#include <vector>
#include <stack>
#define _USE_MATH_DEFINES // for C
#include <math.h>

using namespace std;

float g_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
float s_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
float angulo = 0.0;
bool grado = false, elimine = true;
typedef enum { PINTAR = 1, SELECT, CONTROL_POINTS, COLOR, DIVIDE} Option;
Option g_CurrentOption = PINTAR;
typedef enum { EDIT = 1, ELIMINAR } Points;
Points p_CurrentOption = EDIT;

int  x_i, y_i, x_f, y_f, clic, aux2 = 0, seleccionada = -1, cont = 0;
char key = '0', key2 = '0';
bool aux_z, aux_y = false, mover = false;

struct Puntos {
	float x;
	float y;
};

vector <Puntos> ControlPoints;

class Curva {
private:
	float color[4];
	int n_puntos;
	vector <Puntos> PuntosControl;
	float BoundingBox[4];

public:
	Curva() {
		color[0] = color[1] = color[2] = 0.0;
		n_puntos = 0;
	}

	~Curva() {
	}

	void setPuntos(vector <Puntos> puntos) {
		PuntosControl = puntos;
		n_puntos = puntos.size();
	}

	vector <Puntos> getPuntos() {
		return PuntosControl;
	}

	Puntos getVertex(int x) {
		return PuntosControl[x];
	}

	void setVertex(int i, float x, float y) {
		PuntosControl[i].x = x;
		PuntosControl[i].y = y;
	}

	void deleteVertex(int i) {
		PuntosControl.erase(PuntosControl.begin() + i);
		setPuntos(PuntosControl);
	}

	void setBoundingBox(vector <float> puntos) {
		for (int i = 0; i < 4; i++)BoundingBox[i] = puntos[i];
	}

	float* getBoundingBox() {
		return BoundingBox;
	}

	int getN() {
		return n_puntos;
	}

	float* getColor() {
		return color;
	}

	void setColor(float r, float g, float b) {
		color[0] = r;
		color[1] = g;
		color[2] = b;
	}
};

vector <Curva> curvas;
stack <vector<Curva> > undo;
stack <vector<Curva> > redo;
vector <Puntos> C;
double paso = 0.001;

void TW_CALL Rotate(void *clientData) {
	if (g_CurrentOption == 2) key = (char)13;
}

void TW_CALL Grado(void *clientData){
	if (g_CurrentOption == 3) key = '+';
}

void TW_CALL Aumentar(void *clientData) {
	if (g_CurrentOption == 2) key = '+';
}

void TW_CALL Reducir(void *clientData) {
	if (g_CurrentOption == 2) key = '-';
}

vector <float> BoundingBox(vector <Puntos> PuntosControl, int NumeroVertices) {
	float x_max, x_min, y_max, y_min;
	if (!PuntosControl.empty()) {
		x_max = x_min = PuntosControl[0].x;
		y_max = y_min = PuntosControl[0].y;
		for (double t = 0.0; t <= 1.0; t += paso) {
			vector <Puntos> C = PuntosControl;
			for (int r = 0; r < (NumeroVertices - 1); r++) {
				for (int i = 0; i < ((NumeroVertices - 1) - r); i++) {
					C[i].x = float((1.0 - t) * C[i].x) + float(t  *C[i + 1].x);
					C[i].y = float((1.0 - t)  *C[i].y) + float(t * C[i + 1].y);
				}
			}
			//Aplico la idea de Casteljau guardando los valores maximos de X y Y de la curva, los cuales seran los puntos del BoundingBox
			if (C[0].x < x_min)x_min = C[0].x;
			if (C[0].y < y_min)y_min = C[0].y;
			if (C[0].x > x_max)x_max = C[0].x;
			if (C[0].y > y_max)y_max = C[0].y;
		}
		vector <float> puntos;
		puntos.push_back(x_max);
		puntos.push_back(y_max);
		puntos.push_back(x_min);
		puntos.push_back(y_min);
		return puntos;
	}
}

void DibujarBoundingBox(float* puntos) { //Recibo los puntos  y trazo las lineas
	float x_max = puntos[0], x_min = puntos[2], y_max = puntos[1], y_min = puntos[3];
	glBegin(GL_LINES);
	glColor3f(0.0, 0.0, 1.0);
	glVertex2f(x_max, y_max);
	glVertex2f(x_max, y_min);
	glVertex2f(x_min, y_max);
	glVertex2f(x_min, y_min);
	glVertex2f(x_max, y_max);
	glVertex2f(x_min, y_max);
	glVertex2f(x_max, y_min);
	glVertex2f(x_min, y_min);
	glEnd();
}

void ElevarGrado(int i, int NumeroVertices) {
	Puntos bn, bjA, bj, b;
	vector <Puntos> PuntosNuevo;
	bn = curvas[i].getVertex(NumeroVertices - 1);
	PuntosNuevo.push_back(curvas[i].getVertex(0));
	float n = float(NumeroVertices);
	for (int j = 1; j < NumeroVertices; j++) {
		bjA = curvas[i].getVertex(j - 1);
		bj = curvas[i].getVertex(j);
		float j_aux = float(j);
		float num = j_aux / n;
		b.x = (num*bjA.x) + ((1.0 - num)*bj.x);
		b.y = (num*bjA.y) + ((1.0 - num)*bj.y);
		PuntosNuevo.push_back(b);
	}
	PuntosNuevo.push_back(bn);
	curvas[i].setPuntos(PuntosNuevo);
}

void Subdividir(Curva curva, int NumeroVertices, float *c, int index) {
	vector <Puntos> PuntosControl = curva.getPuntos();
	float aux_x, aux_y;
	bool aux = false;
	aux_x = (float)x_i;
	aux_y = (float)y_i;
	if (!PuntosControl.empty()) {
		for (double t = 0.0; t <= 1.0; t += paso) {
			vector <Puntos> C = PuntosControl; //Puntos de control de la segunda mitad
			vector <Puntos> D = PuntosControl; //Puntos de control de la primera mitad
			D[0] = curva.getVertex(0);
			for (int r = 0; r < (NumeroVertices - 1); r++) {
				for (int i = 0; i < ((NumeroVertices - 1) - r); i++) {
					//Se guardan los puntos de control de la segunda mitad
					C[i].x = float((1.0 - t) * C[i].x) + float(t  *C[i + 1].x);
					C[i].y = float((1.0 - t)  *C[i].y) + float(t * C[i + 1].y);
				}
				//Se guardan los puntos de control de la primera mitad
				D[r+1].x = C[0].x;
				D[r+1].y = C[0].y;
			}
			if ((C[0].x == aux_x && C[0].y == aux_y) && !aux) { //Caso en el que el clic haya sido exactamente en una parte de la curva
				aux = true;
			}else {
				//Se valida que el clic se encuentre en un area cercana a un punto de la curva, se le asigna el valor de la curva al clic
				if ((C[0].x >= aux_x - 1 && C[0].x <= aux_x + 1 && C[0].y >= aux_y - 1 && C[0].y <= aux_y + 1) && !aux) {
					aux_x = C[0].x;
					aux_y = C[0].y;
					aux = true;
				}
			}
			if (C[0].x == aux_x && C[0].y == aux_y) {
				//Se guardan las nuevas curvas
				float R = c[0];
				float G = c[1];
				float B = c[2];
				Curva a;
				a.setPuntos(D);
				a.setBoundingBox(BoundingBox(a.getPuntos(), a.getN()));
				a.setColor(R, G, B);
				curvas.push_back(a);
				Curva b;
				b.setPuntos(C);
				b.setBoundingBox(BoundingBox(b.getPuntos(), b.getN()));
				b.setColor(R, G, B);
				curvas.push_back(b);
				curvas.erase(curvas.begin() + index);
				break;	
			}
		}
	}
}

void Rotar(int i) {
	float *puntos = curvas[i].getBoundingBox();
	//Centro de la BoundingBox
	float Xr = (puntos[0] + puntos[2])/2;
	float Yr = (puntos[1] + puntos[3])/2;
	for (int j = 0; j < curvas[i].getN(); j++) { //Se reubican los puntos de control
		float x = curvas[i].getVertex(j).x;
		float y = curvas[i].getVertex(j).y;
		float x_f = Xr + (x - Xr)*cos(angulo * M_PI / 180.0) - (y - Yr)*sin(angulo * M_PI / 180.0);
		float y_f = Yr + (x - Xr)*sin(angulo * M_PI / 180.0) + (y - Yr)*cos(angulo * M_PI / 180.0);
		curvas[i].setVertex(j, x_f, y_f);
	}
}

void Escalar(int i, float Sx, float Sy) {
	float *puntos = curvas[i].getBoundingBox();
	//Centro de la BoundingBox
	float Xr = (puntos[0] + puntos[2]) / 2;
	float Yr = (puntos[1] + puntos[3]) / 2;
	for (int j = 0; j < curvas[i].getN(); j++) { //Se reubican los puntos de control
		float x = curvas[i].getVertex(j).x;
		float y = curvas[i].getVertex(j).y;
		float x_f = x*Sx + Xr*(1 - Sx);
		float y_f = y*Sy + Yr*(1 - Sy);
		curvas[i].setVertex(j, x_f, y_f);
	}
}

void Casteljau(vector <Puntos> PuntosControl, int NumeroVertices, float *color) {
	float VertXY1[2];
	glBegin(GL_LINE_STRIP);
	if (!PuntosControl.empty()) {
		for (double t = 0.0; t <= 1.0; t += paso) {
			vector <Puntos> C = PuntosControl;
			for (int r = 0; r < (NumeroVertices - 1); r++) {
				for (int i = 0; i < ((NumeroVertices - 1) - r); i++) {
					C[i].x = float((1.0 - t) * C[i].x) + float(t  *C[i + 1].x);
					C[i].y = float((1.0 - t)  *C[i].y) + float(t * C[i + 1].y);
				}
			}
			VertXY1[0] = C[0].x;
			VertXY1[1] = C[0].y;
			glColor3f(color[0], color[1], color[2]);
			glVertex2fv(VertXY1);
		}
	}
	glEnd();
}

void Control_Points(vector <Puntos> PuntosControl) {
	if (!PuntosControl.empty()) {
		glBegin(GL_LINES);
		glColor3f(0.0, 1.0, 0.0);
		for (int i = 0; i < PuntosControl.size() - 1; i++) { //Pinta las lineas que los conectan a los puntos de control
			glVertex2f(PuntosControl[i].x, PuntosControl[i].y);
			glVertex2f(PuntosControl[i + 1].x, PuntosControl[i + 1].y);
		}
		glEnd();
	}
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glColor3f(1.0, 0.0, 0.0);

	for (int i = 0; i < PuntosControl.size(); i++) { //Pinta los puntos de control
		glVertex2f(PuntosControl[i].x, PuntosControl[i].y);
	}
	glEnd();
}

void render() {

	glDisable(GL_POINT_SMOOTH);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (g_CurrentOption == 1) { //Dibujar Curva
		Casteljau(ControlPoints, ControlPoints.size(), g_color);
		Control_Points(ControlPoints);
	}

	if ((g_CurrentOption == 1 && key == (char)13) || g_CurrentOption != 1) { //Guardar curva dibujada
		if (!ControlPoints.empty()) {
			Curva nueva;
			nueva.setPuntos(ControlPoints);
			nueva.setBoundingBox(BoundingBox(nueva.getPuntos(), nueva.getN()));
			nueva.setColor(g_color[0], g_color[1], g_color[2]);
			key = '0';
			while (!ControlPoints.empty()) ControlPoints.pop_back();
			curvas.push_back(nueva);
		}
	}

	for (int i = 0; i < curvas.size(); i++) {
		if (g_CurrentOption != 1 && seleccionada == i) {

			if (g_CurrentOption == 4) curvas[i].setColor(g_color[0], g_color[1], g_color[2]); //Cambio color de la curva seleccionada

			if (key == '+' && g_CurrentOption == 3) { //Elevar Grado
				ElevarGrado(seleccionada, curvas[i].getN());
				key = '0';
			}

			if (key == (char)13 && g_CurrentOption == 2) {	//Rotar
				Rotar(seleccionada);
				curvas[i].setBoundingBox(BoundingBox(curvas[i].getPuntos(), curvas[i].getN()));
				key = '0';
			}

			if (g_CurrentOption == 2) {	//Escalar
				if (key == '+') Escalar(seleccionada, 1.1, 1.1);	//Aumentar tamaño
				if (key == '-') Escalar(seleccionada, 0.9, 0.9);	//Reducir tamaño
				curvas[i].setBoundingBox(BoundingBox(curvas[i].getPuntos(), curvas[i].getN()));
				key = '0';
			}

			if (clic == 1) {
				if (g_CurrentOption == 2) { //Se muestra el BoundingBox de una curva seleccionada
					for (int j = 0; j < curvas[i].getN(); j++) { //Transaladar curva
						curvas[i].setVertex(j, curvas[i].getVertex(j).x + (float)(x_f - x_i), curvas[i].getVertex(j).y + (float)(y_f - y_i));
					}
					float *aux_box = curvas[i].getBoundingBox();
					vector <float> box;
					box.push_back(aux_box[0] + (float)(x_f - x_i));
					box.push_back(aux_box[1] + (float)(y_f - y_i));
					box.push_back(aux_box[2] + (float)(x_f - x_i));
					box.push_back(aux_box[3] + (float)(y_f - y_i));
					curvas[i].setBoundingBox(box);
					x_i = x_f;
					y_i = y_f;
				}

				if (g_CurrentOption == 3) { //Se muestran los Puntos de Control de una curva seleccionada
					if (p_CurrentOption == 1) { //Editar Puntos
						for (int j = 0; j < curvas[i].getN(); j++) {
							if ((float)x_i - 2 <= curvas[i].getVertex(j).x && curvas[i].getVertex(j).x <= (float)x_i + 2 && (float)y_i - 2 <= curvas[i].getVertex(j).y && curvas[i].getVertex(j).y <= (float)y_i + 2) {
								curvas[i].setVertex(j, curvas[i].getVertex(j).x + (float)(x_f - x_i), curvas[i].getVertex(j).y + (float)(y_f - y_i));
							}
						}
						curvas[i].setBoundingBox(BoundingBox(curvas[i].getPuntos(), curvas[i].getN()));
						x_i = x_f;
						y_i = y_f;
					}

					if (p_CurrentOption == 2) { //Eliminar Puntos
						int j = 0;
						while (!elimine && j < curvas[i].getN()) {
							if ((float)x_i - 2 <= curvas[i].getVertex(j).x && curvas[i].getVertex(j).x <= (float)x_i + 2 && (float)y_i - 2 <= curvas[i].getVertex(j).y && curvas[i].getVertex(j).y <= (float)y_i + 2) {
								curvas[i].deleteVertex(j);
								if (curvas[i].getN()>0) curvas[i].setBoundingBox(BoundingBox(curvas[i].getPuntos(), curvas[i].getN()));
								else {
									curvas.erase(curvas.begin() + i);
									elimine = true;
									seleccionada = -1;
								}
							}
							j++;
						}
					}
				}

				if (g_CurrentOption == 5) {	//Subdividir
					Subdividir(curvas[seleccionada], curvas[seleccionada].getN(), curvas[seleccionada].getColor(), seleccionada);
					key = '0';
					seleccionada = -1;
				}
			}
		}
		if(!elimine) Casteljau(curvas[i].getPuntos(), curvas[i].getN(), curvas[i].getColor()); //Pintar Curvas

		if (g_CurrentOption == 2 || g_CurrentOption == 4 || g_CurrentOption == 5) {
			if (seleccionada == i) { //Pintar BoundingBox
				DibujarBoundingBox(curvas[i].getBoundingBox());
			}
		}
		
		if (g_CurrentOption == 3 && !elimine) {
			if (seleccionada == i) { //Pintar Puntos de Control
				Control_Points(curvas[i].getPuntos());
			}
		}

		elimine = false;
	}

	TwDraw();
	glutSwapBuffers();
	glutPostRedisplay();
}

void keyboard(unsigned char x, int y, int z) {
	if(!TwEventKeyboardGLUT(x,y,z)) key = x;
}

void  raton_mov(int x, int y) {
	if (!TwEventMouseMotionGLUT(x, y)) {
		x_f = x;
		y_f = y;
		glutPostRedisplay(); //Redisplay 
	}
}


void raton(int button, int status, int x, int y) {
	if (!TwEventMouseButtonGLUT(button, status, x, y)) {
		if (button == GLUT_LEFT_BUTTON && status == GLUT_DOWN) {
			//Cuando se hace click la primera vez
			x_i = x;
			x_f = x;
			y_i = y;
			y_f = y;
			Puntos p;
			p.x = (float)x;
			p.y = (float)y;
			if (g_CurrentOption == 1) {
				ControlPoints.push_back(p);
				seleccionada = -1;
			}
			if (g_CurrentOption != 1) { //Opcion para seleccionar una curva
				
				if (seleccionada >= 0 && g_CurrentOption == 3) { //Opcion para mostrar los puntos de control
					int aux_sel = seleccionada;
					seleccionada = -1;
					int i = 0;
					while (i < curvas[aux_sel].getN() && seleccionada == -1) {
						if ((float)x_i - 2 <= curvas[aux_sel].getVertex(i).x && curvas[aux_sel].getVertex(i).x <= (float)x_i + 2 && (float)y_i - 2 <= curvas[aux_sel].getVertex(i).y && curvas[aux_sel].getVertex(i).y <= (float)y_i + 2){
							seleccionada = aux_sel;
						}
						i++;
					}
				}else { //Opcion para mostrar BoundingBox
					seleccionada = -1;
					for (int i = 0; i < curvas.size(); i++) {
						float* aux = curvas[i].getBoundingBox();
						if (x <= aux[0] && x >= aux[2] && y <= aux[1] && y >= aux[3]) {
							seleccionada = i;
						}
					}
				}
			}
			clic = 1;
		}
		if (button == GLUT_LEFT_BUTTON && status == GLUT_UP) {
			//Cuando suelto el raton
			x_f = x;
			y_f = y;
			clic = 0;
		}
	}
}

void Reshape(int width, int height)
{
	// Nuevos parametros para OpenGL
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, width, height, 0); //Igual al tamano de ventana
	glClearColor(s_color[0], s_color[1], s_color[2], 0);
	glColor3f(0.0, 0.0, 0.0);
	// Establece la nueva ventana de AntTweakBar
	TwWindowSize(width, height);
}

//Inicializadora, impotante!
void init() {
	glViewport(0, 0, 0, 0);
	gluOrtho2D(0, 640, 480, 0); //Igual al tamano de ventana
	glClearColor(s_color[0], s_color[1], s_color[2], 0);
	glColor3f(0.0, 0.0, 0.0);
}

void changeViewPort(int w, int h) {
	glViewport(0, 0, w, h);
}


int main(int argc, char* argv[]) {

				// Initialize GLUT
	glutInit(&argc, argv);
	// Set up some memory buffers for our display
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	// Set the window size
	glutInitWindowSize(800, 600);
	// Create the window with the title
	glutCreateWindow("Tarea 2 ICG Daniel Rodriguez");
	glutCreateMenu(NULL);
	// Bind the two functions (above) to respond when necessary
	glutReshapeFunc(Reshape);
	glutDisplayFunc(render);

	// Very important!  This initializes the entry points in the OpenGL driver so we can 
	// call all the functions in the API.
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW error");
		return 1;
	}

	//Inicializacion del AntTweakBar
	TwInit(TW_OPENGL, NULL);
	TwWindowSize(300, 300);
	//Creo una barra
	TwBar *myBar;
	myBar = TwNewBar("Mi Barra");
	TwDefine(" 'Mi Barra' alpha=255 ");

	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	// - Send 'glutGetModifers' function pointer to AntTweakBar;
	// required because the GLUT key event functions do not report key modifiers states.
	TwGLUTModifiersFunc(glutGetModifiers);
	
	//TwAddVarRW(myBar, "Color Fondo", TW_TYPE_COLOR3F, &s_color, "label = 'Color Fondo' ");

	{
		TwEnumVal optionEV[5] = { { PINTAR, "Dibujar Curva" },{ SELECT, "Seleccionar" },{ CONTROL_POINTS, "Puntos de Control" },{ COLOR, "Cambiar Color" },{ DIVIDE, "Subdividir" } };
		TwType optionType = TwDefineEnum("Opcion", optionEV, 5);
		TwAddVarRW(myBar, "Menú", optionType, &g_CurrentOption, " keyIncr='<' keyDecr='>' help='Dibujar Curva: Dibuja una curva.\n\nSeleccionar: Se muestra el BoundingBox de la curva seleccionada. Se debe seleccionar esta opción si además se desea Rotar, Aumentar o Reducir Tamaño.\n\nPuntos de Control: Se selecciona una curva y se muestran los puntos de control. Se debe seleccionar esta opción si además se desea Editar, Eliminar o Elevar Grado\n\nCambiar Color: Se selecciona una curva y se cambia su color.\n\nSubdividir: Se hace clic en una curva y se divide en el punto donde se da el clic.\n\n' ");
	}

	{
		TwEnumVal pointsEV[2] = { { EDIT, "Editar" },{ ELIMINAR, "Eliminar" } };
		TwType pointsType = TwDefineEnum("Puntos de Control", pointsEV, 2);
		TwAddVarRW(myBar, "Puntos de Control", pointsType, &p_CurrentOption, " keyIncr='<' keyDecr='>' help='Editar: Se modifica un punto de control arrastrándolo hacia una nueva posición.\n\nEliminar: Elimina aquel punto de control sobre el cual se haga clic.\n\n' ");
	}

	TwAddVarRW(myBar, "Color Curva", TW_TYPE_COLOR3F, &g_color, "label = 'Color Curva' ");

	TwAddVarRW(myBar, "Ángulo de Rotación", TW_TYPE_FLOAT, &angulo, " min=0 max=360 step=1.0 ");

	TwAddButton(myBar, "Rotar", Rotate, NULL, "");

	TwAddButton(myBar, "Elevar Grado", Grado, NULL, "");

	TwAddButton(myBar, "Aumentar Tamaño", Aumentar, NULL, "");

	TwAddButton(myBar, "Reducir Tamaño", Reducir, NULL, "");

	glutKeyboardFunc(keyboard);
	glutMouseFunc(raton);
	glutMotionFunc(raton_mov);
	glutPassiveMotionFunc(raton_mov);

	glutMainLoop();
	return 0;
}
