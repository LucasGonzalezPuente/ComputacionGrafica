#include <cstdlib> // Incluir antes de glut.h
#include <GL/glut.h>
#include "GivenFiles/toolkits/tinyxml2.h" // Incluir TinyXML2
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#define _USE_MATH_DEFINES // Habilitar constantes matemáticas (como M_PI)
#include <cmath>

using namespace std;
using namespace tinyxml2;

// Estructura para almacenar la configuración de la cámara
struct Camera {
    float posX, posY, posZ;
    float lookAtX, lookAtY, lookAtZ;
    float upX, upY, upZ;
    float fov, near, far;
};

// Estructura para almacenar la configuración de la ventana
struct Window {
    int width, height;
};

// Estructura para almacenar un modelo 3D
struct Model {
    vector<float> vertices; // Lista de vértices (x, y, z)
    vector<int> faces;      // Lista de caras (índices de vértices)
};

// Variables globales
Window window;
Camera camera;
vector<Model> models;
vector<vector<float>> modelColors = {
    {1.0f, 0.0f, 0.0f}, // Rojo
    {0.0f, 1.0f, 0.0f}, // Verde
    {0.0f, 0.0f, 1.0f}, // Azul
    {1.0f, 1.0f, 0.0f}, // Amarillo
    {1.0f, 0.0f, 1.0f}, // Magenta
    {0.0f, 1.0f, 1.0f}  // Cian
};

// Variables para la cámara
float theta, phi, radius; // Distancia constante desde el origen

// Funcion para leer la configuración desde un archivo XML
bool readXMLConfig(const string& filename, Window& window, Camera& camera, vector<string>& modelFiles) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Error al cargar el archivo XML: " << filename << endl;
        return false;
    }

    XMLElement* world = doc.FirstChildElement("world");
    if (!world) {
        cerr << "Elemento 'world' no encontrado en el XML." << endl;
        return false;
    }

    // Leer configuración de la ventana
    XMLElement* windowElem = world->FirstChildElement("window");
    if (windowElem) {
        windowElem->QueryIntAttribute("width", &window.width);
        windowElem->QueryIntAttribute("height", &window.height);
    }

    // Leer configuración de la cámara
    XMLElement* cameraElem = world->FirstChildElement("camera");
    if (cameraElem) {
        XMLElement* position = cameraElem->FirstChildElement("position");
        if (position) {
            position->QueryFloatAttribute("x", &camera.posX);
            position->QueryFloatAttribute("y", &camera.posY);
            position->QueryFloatAttribute("z", &camera.posZ);
        }

        XMLElement* lookAt = cameraElem->FirstChildElement("lookAt");
        if (lookAt) {
            lookAt->QueryFloatAttribute("x", &camera.lookAtX);
            lookAt->QueryFloatAttribute("y", &camera.lookAtY);
            lookAt->QueryFloatAttribute("z", &camera.lookAtZ);
        }

        XMLElement* up = cameraElem->FirstChildElement("up");
        if (up) {
            up->QueryFloatAttribute("x", &camera.upX);
            up->QueryFloatAttribute("y", &camera.upY);
            up->QueryFloatAttribute("z", &camera.upZ);
        }

        XMLElement* projection = cameraElem->FirstChildElement("projection");
        if (projection) {
            projection->QueryFloatAttribute("fov", &camera.fov);
            projection->QueryFloatAttribute("near", &camera.near);
            projection->QueryFloatAttribute("far", &camera.far);
        }
    }

	// Inicializar las variables de la cámara
    radius = sqrt(camera.posX * camera.posX + camera.posY * camera.posY + camera.posZ * camera.posZ);
    theta = atan2(camera.posZ, camera.posX);
    phi = asin(camera.posY / radius);

    // Leer los modelos
    XMLElement* group = world->FirstChildElement("group");
    if (group) {
        XMLElement* modelsElem = group->FirstChildElement("models");
        if (modelsElem) {
            for (XMLElement* modelElem = modelsElem->FirstChildElement("model"); modelElem; modelElem = modelElem->NextSiblingElement("model")) {
                const char* file = modelElem->Attribute("file");
                if (file) {
                    modelFiles.push_back(file);
                }
            }
        }
    }

    return true;
}

// Función para cargar un modelo desde un archivo .3d
bool loadModel(const string& filename, Model& model) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        return false;
    }

    int numVertices;
    file >> numVertices;
    cout << "Cargando " << numVertices << " vertices desde " << filename << endl;

    for (int i = 0; i < numVertices; i++) {
        float x, y, z;
        file >> x >> y >> z;
        model.vertices.push_back(x);
        model.vertices.push_back(y);
        model.vertices.push_back(z);
    }

    int numFaces;
    file >> numFaces;
    cout << "Cargando " << numFaces << " caras desde " << filename << endl;

    for (int i = 0; i < numFaces; i++) {
        int v1, v2, v3;
        file >> v1 >> v2 >> v3;
        model.faces.push_back(v1);
        model.faces.push_back(v2);
        model.faces.push_back(v3);
    }

    file.close();
    return true;
}

// Función para configurar la cámara en OpenGL
void setupCamera() {
    camera.posX = radius * cos(phi) * cos(theta);
    camera.posY = radius * sin(phi);
    camera.posZ = radius * cos(phi) * sin(theta);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.fov, (float)window.width / window.height, camera.near, camera.far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera.posX, camera.posY, camera.posZ,
        camera.lookAtX, camera.lookAtY, camera.lookAtZ,
        camera.upX, camera.upY, camera.upZ);
}


//Función para dibujar los ejes
void drawAxes()
{
    glBegin(GL_LINES);
    // X-axis (Red)
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(-15.0, 0.0, 0.0);
    glVertex3f(15.0, 0.0, 0.0);

    // Y-axis (Green)
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, -15.0, 0.0);
    glVertex3f(0.0, 15.0, 0.0);

    // Z-axis (Blue)
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, -15.0);
    glVertex3f(0.0, 0.0, 15.0);
    glEnd();
}

// Función para renderizar un modelo
void renderModel(const Model& model, const vector<float>& color) {
    cout << "Renderizando modelo con " << model.faces.size() / 3 << " caras." << endl;

    // Establecer el color del modelo
    glColor3f(color[0], color[1], color[2]);

    // Dibujar solo las líneas del modelo
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < model.faces.size(); i += 3) {
        int v1 = model.faces[i] * 3;
        int v2 = model.faces[i + 1] * 3;
        int v3 = model.faces[i + 2] * 3;

        glVertex3f(model.vertices[v1], model.vertices[v1 + 1], model.vertices[v1 + 2]);
        glVertex3f(model.vertices[v2], model.vertices[v2 + 1], model.vertices[v2 + 2]);
        glVertex3f(model.vertices[v3], model.vertices[v3 + 1], model.vertices[v3 + 2]);
    }
    glEnd();

    // Volver al modo de relleno
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Función de renderizado principal
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configurar la cámara
    setupCamera();

    drawAxes(); // Dibujar ejes

    // Habilitar iluminación básica
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // Renderizar todos los modelos
    for (size_t i = 0; i < models.size(); ++i) {
        renderModel(models[i], modelColors[i % modelColors.size()]);
    }

    glutSwapBuffers();
}

// Función de teclado
void keyboardFunc(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': theta -= 0.1; break;
    case 'd': theta += 0.1; break;
    case 'w': phi += 0.1; break;
    case 's': phi -= 0.1; break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Uso: engine <config_file.xml>" << endl;
        return 1;
    }

    // Declarar modelFiles como un vector<string>
    vector<string> modelFiles;

    // Leer el archivo XML
    if (!readXMLConfig(argv[1], window, camera, modelFiles)) {
        return 1;
    }

    // Cargar los modelos
    for (const auto& modelFile : modelFiles) {
        Model model;
        if (loadModel(modelFile, model)) {
            models.push_back(model);
        }
        else {
            cerr << "Error al cargar el modelo: " << modelFile << endl;
        }
    }

    // Inicializar GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window.width, window.height);
    glutCreateWindow("Engine");

    // Habilitar el test de profundidad
    glEnable(GL_DEPTH_TEST);

    // Establecer el color de fondo
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Registrar la función de renderizado
    glutDisplayFunc(renderScene);

    // Registrar la función de teclado
    glutKeyboardFunc(keyboardFunc);

    // Iniciar el bucle principal de GLUT
    glutMainLoop();

    return 0;
}


