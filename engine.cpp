#include <cstdlib>
#include <GL/glut.h>
#include "GivenFiles/toolkits/tinyxml2.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>

using namespace std;
using namespace tinyxml2;

// Constants
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Structures
struct Camera {
    float posX, posY, posZ;
    float lookAtX, lookAtY, lookAtZ;
    float upX, upY, upZ;
    float fov, near, far;
};

struct Window {
    int width, height;
};

struct Model {
    vector<float> vertices;
    vector<int> faces;
};

struct Transform {
    float translate[3] = { 0, 0, 0 };
    float rotate[4] = { 0, 0, 1, 0 }; // angle, x, y, z
    float scale[3] = { 1, 1, 1 };
};

struct Group {
    Transform transform;
    vector<Model> models;
    vector<Group> children;
};

// Global variables
Window window;
Camera camera;
Group rootGroup;
float cameraTheta = 0, cameraPhi = 0, cameraRadius = 10;

vector<vector<float>> modelColors = {
    {1.0f, 0.0f, 0.0f}, // Red
    {0.0f, 1.0f, 0.0f}, // Green
    {0.0f, 0.0f, 1.0f}, // Blue
    {1.0f, 1.0f, 0.0f}, // Yellow
    {1.0f, 0.0f, 1.0f}, // Magenta
    {0.0f, 1.0f, 1.0f}  // Cyan
};

// Function prototypes
bool readXMLConfig(const string& filename, Window& window, Camera& camera, Group& rootGroup);
Group parseGroup(XMLElement* groupElem);
bool loadModel(const string& filename, Model& model);
void setupCamera();
void drawAxes();
void renderModel(const Model& model, const vector<float>& color);
void applyTransform(const Transform& transform);
void renderGroup(const Group& group);
void renderScene();
void keyboardFunc(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);

// XML Parsing
bool readXMLConfig(const string& filename, Window& window, Camera& camera, Group& rootGroup) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Error loading XML file: " << filename << endl;
        return false;
    }

    XMLElement* world = doc.FirstChildElement("world");
    if (!world) {
        cerr << "No 'world' element found" << endl;
        return false;
    }

    // Window settings
    XMLElement* windowElem = world->FirstChildElement("window");
    if (windowElem) {
        windowElem->QueryIntAttribute("width", &window.width);
        windowElem->QueryIntAttribute("height", &window.height);
    }

    // Camera settings
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

    // Initialize camera orbit controls
    cameraRadius = sqrt(camera.posX * camera.posX + camera.posY * camera.posY + camera.posZ * camera.posZ);
    cameraTheta = atan2(camera.posZ, camera.posX);
    cameraPhi = asin(camera.posY / cameraRadius);

    // Parse scene graph
    XMLElement* groupElem = world->FirstChildElement("group");
    if (groupElem) {
        rootGroup = parseGroup(groupElem);
    }

    return true;
}

Group parseGroup(XMLElement* groupElem) {
    Group group;

    // Parse transforms
    XMLElement* transformElem = groupElem->FirstChildElement("transform");
    if (transformElem) {
        XMLElement* translate = transformElem->FirstChildElement("translate");
        if (translate) {
            translate->QueryFloatAttribute("x", &group.transform.translate[0]);
            translate->QueryFloatAttribute("y", &group.transform.translate[1]);
            translate->QueryFloatAttribute("z", &group.transform.translate[2]);
        }

        XMLElement* rotate = transformElem->FirstChildElement("rotate");
        if (rotate) {
            rotate->QueryFloatAttribute("angle", &group.transform.rotate[0]);
            rotate->QueryFloatAttribute("x", &group.transform.rotate[1]);
            rotate->QueryFloatAttribute("y", &group.transform.rotate[2]);
            rotate->QueryFloatAttribute("z", &group.transform.rotate[3]);
        }

        XMLElement* scale = transformElem->FirstChildElement("scale");
        if (scale) {
            scale->QueryFloatAttribute("x", &group.transform.scale[0]);
            scale->QueryFloatAttribute("y", &group.transform.scale[1]);
            scale->QueryFloatAttribute("z", &group.transform.scale[2]);
        }
    }

    // Parse models
    XMLElement* modelsElem = groupElem->FirstChildElement("models");
    if (modelsElem) {
        for (XMLElement* modelElem = modelsElem->FirstChildElement("model"); modelElem; modelElem = modelElem->NextSiblingElement("model")) {
            const char* file = modelElem->Attribute("file");
            if (file) {
                Model model;
                if (loadModel(file, model)) {
                    group.models.push_back(model);
                }
            }
        }
    }

    // Parse child groups
    for (XMLElement* child = groupElem->FirstChildElement("group"); child; child = child->NextSiblingElement("group")) {
        group.children.push_back(parseGroup(child));
    }

    return group;
}

// Model Loading
bool loadModel(const string& filename, Model& model) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening model file: " << filename << endl;
        return false;
    }

    int numVertices;
    file >> numVertices;

    for (int i = 0; i < numVertices; i++) {
        float x, y, z;
        file >> x >> y >> z;
        model.vertices.push_back(x);
        model.vertices.push_back(y);
        model.vertices.push_back(z);
    }

    int numFaces;
    file >> numFaces;

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

// Rendering
void setupCamera() {
    // Update camera position based on spherical coordinates
    camera.posX = cameraRadius * cos(cameraPhi) * cos(cameraTheta);
    camera.posY = cameraRadius * sin(cameraPhi);
    camera.posZ = cameraRadius * cos(cameraPhi) * sin(cameraTheta);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.fov, (float)window.width / window.height, camera.near, camera.far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        camera.posX, camera.posY, camera.posZ,
        camera.lookAtX, camera.lookAtY, camera.lookAtZ,
        camera.upX, camera.upY, camera.upZ
    );
}

void drawAxes() {
    glDisable(GL_LIGHTING);
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
    glEnable(GL_LIGHTING);
}

void renderModel(const Model& model, const vector<float>& color) {
    glColor3f(color[0], color[1], color[2]);
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

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void applyTransform(const Transform& transform) {
    glTranslatef(transform.translate[0], transform.translate[1], transform.translate[2]);
    glRotatef(transform.rotate[0], transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    glScalef(transform.scale[0], transform.scale[1], transform.scale[2]);
}

void renderGroup(const Group& group) {
    glPushMatrix();
    applyTransform(group.transform);

    // Render models in this group
    for (size_t i = 0; i < group.models.size(); ++i) {
        renderModel(group.models[i], modelColors[i % modelColors.size()]);
    }

    // Render children
    for (const auto& child : group.children) {
        renderGroup(child);
    }

    glPopMatrix();
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCamera();
    drawAxes();
    renderGroup(rootGroup);
    glutSwapBuffers();
}

// Input Handling
void keyboardFunc(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': cameraTheta -= 0.1; break;
    case 'd': cameraTheta += 0.1; break;
    case 'w': cameraPhi += 0.1; break;
    case 's': cameraPhi -= 0.1; break;
    case 'q': cameraRadius += 0.5; break;
    case 'e': cameraRadius -= 0.5; break;
    case 27: exit(0); break; // ESC key
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP: cameraPhi += 0.1; break;
    case GLUT_KEY_DOWN: cameraPhi -= 0.1; break;
    case GLUT_KEY_LEFT: cameraTheta -= 0.1; break;
    case GLUT_KEY_RIGHT: cameraTheta += 0.1; break;
    }
    glutPostRedisplay();
}

// Main Function
int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <config.xml>" << endl;
        return 1;
    }

    // Load configuration
    if (!readXMLConfig(argv[1], window, camera, rootGroup)) {
        cerr << "Failed to load configuration!" << endl;
        return 1;
    }

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window.width, window.height);
    glutCreateWindow("3D Engine - Phase 2");

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // Register callbacks
    glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeys);

    // Start main loop
    glutMainLoop();

    return 0;
}
