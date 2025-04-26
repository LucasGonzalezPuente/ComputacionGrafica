#ifdef _WIN32
#include <GL/glew.h>
#endif
#include <cstdlib> // Incluir antes de glut.h
#include <GL/glut.h>
#include "GivenFiles/toolkits/tinyxml2.h" // Incluir TinyXML2
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <functional>

#define _USE_MATH_DEFINES // Habilitar constantes matemáticas (como M_PI)
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace tinyxml2;

// Debug variable
bool DEBUG = true;

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

    // New for Phase 3 VBO IDs
    GLuint vertexBuffer;
    GLuint indexBuffer;
    bool buffersInitialized = false;
};

struct Transform {
    float translate[3] = { 0, 0, 0 };
    float rotate[4] = { 0, 0, 1, 0 }; // angle, x, y, z
    float scale[3] = { 1, 1, 1 };

    bool isTimedRotation = false;
    float rotationTime = 0;
    bool isTimedTranslation = false;
    float translationTime = 0;
    bool alignWithPath = false;
    vector<vector<float>> translationPoints;
    vector<vector<float>> bezierControlPoints; 
    vector<vector<float>> bezierCurve; 

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
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f}
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

// NEW PHASE 3 functions
// Add these right after the struct declarations
vector<float> getCatmullRomPoint(float t, const vector<vector<float>>& p);
vector<float> getCatmullRomDerivative(float t, const vector<vector<float>>& p);
void initModelVBOs(Model& model);
void renderModelWithVBOs(const Model& model, const vector<float>& color);
vector<vector<float>> generateBezierCurve(const vector<vector<float>>& controlPoints, int segments = 100);
void drawBezierCurve(const vector<vector<float>>& curvePoints);

// XML Parsing
bool readXMLConfig(const string& filename, Window& window, Camera& camera, Group& rootGroup) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        if (DEBUG) cerr << "Error loading XML file: " << filename << endl;
        return false;
    }

    XMLElement* world = doc.FirstChildElement("world");
    if (!world) {
        if (DEBUG) cerr << "No 'world' element found" << endl;
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
    if (DEBUG) cerr << "Camera position: (" << camera.posX << ", " << camera.posY << ", " << camera.posZ << ")" << endl;
	if (DEBUG) cerr << "Window size: " << window.width << "x" << window.height << endl;

    // Initialize camera orbit controls
    cameraRadius = sqrt(camera.posX * camera.posX + camera.posY * camera.posY + camera.posZ * camera.posZ);
    cameraTheta = atan2(camera.posZ, camera.posX);
    cameraPhi = asin(camera.posY / cameraRadius);

	if (DEBUG) cerr << "Camera radius: " << cameraRadius << endl;

    // Parse scene graph
    XMLElement* groupElem = world->FirstChildElement("group");
    if (groupElem) {
        rootGroup = parseGroup(groupElem);
    }

	if (DEBUG) cerr << "Parsed group with " << rootGroup.models.size() << " models." << endl;

    return true;
}

Group parseGroup(XMLElement* groupElem) {
    Group group;

    if (!groupElem) {
        if (DEBUG) cerr << "Error: groupElem is null" << endl;
        return group;
    }

    if (DEBUG) cerr << "Parsing group element" << endl;

    // Parse transforms
    XMLElement* transformElem = groupElem->FirstChildElement("transform");
    if (transformElem) {
        // Handle translation (static or time-based)
        XMLElement* translate = transformElem->FirstChildElement("translate");
        if (translate) {
            const char* timeAttr = translate->Attribute("time");
            if (timeAttr) {
                group.transform.isTimedTranslation = true;
                group.transform.translationTime = stof(timeAttr);
                const char* alignAttr = translate->Attribute("align");
                if (alignAttr && (strcmp(alignAttr, "true") == 0 || strcmp(alignAttr, "1") == 0)) {
                    group.transform.alignWithPath = true;
                }

                // Read Catmull-Rom points
                for (XMLElement* point = translate->FirstChildElement("point"); point; point = point->NextSiblingElement("point")) {
                    vector<float> p(3);
                    point->QueryFloatAttribute("x", &p[0]);
                    point->QueryFloatAttribute("y", &p[1]);
                    point->QueryFloatAttribute("z", &p[2]);
                    group.transform.translationPoints.push_back(p);
                }

                // Generate Bézier curve if we have enough points
                if (group.transform.translationPoints.size() >= 4) {
                    group.transform.bezierCurve = generateBezierCurve(group.transform.translationPoints);
                }
            }
            else {
                translate->QueryFloatAttribute("x", &group.transform.translate[0]);
                translate->QueryFloatAttribute("y", &group.transform.translate[1]);
                translate->QueryFloatAttribute("z", &group.transform.translate[2]);
            }
        }

        // Handle rotation (static or time-based)
        XMLElement* rotate = transformElem->FirstChildElement("rotate");
        if (rotate) {
            const char* timeAttr = rotate->Attribute("time");
            if (timeAttr) {
                group.transform.isTimedRotation = true;
                group.transform.rotationTime = stof(timeAttr);
            }
            else {
                rotate->QueryFloatAttribute("angle", &group.transform.rotate[0]);
            }
            rotate->QueryFloatAttribute("x", &group.transform.rotate[1]);
            rotate->QueryFloatAttribute("y", &group.transform.rotate[2]);
            rotate->QueryFloatAttribute("z", &group.transform.rotate[3]);
        }

        // Scale remains the same
        XMLElement* scale = transformElem->FirstChildElement("scale");
        if (scale) {
            scale->QueryFloatAttribute("x", &group.transform.scale[0]);
            scale->QueryFloatAttribute("y", &group.transform.scale[1]);
            scale->QueryFloatAttribute("z", &group.transform.scale[2]);
        }

        // Handle Bézier curve points if they exist
        XMLElement* bezier = transformElem->FirstChildElement("bezier");
        if (bezier) {
            for (XMLElement* point = bezier->FirstChildElement("point"); point; point = point->NextSiblingElement("point")) {
                vector<float> p(3);
                point->QueryFloatAttribute("x", &p[0]);
                point->QueryFloatAttribute("y", &p[1]);
                point->QueryFloatAttribute("z", &p[2]);
                group.transform.bezierControlPoints.push_back(p);
            }
            
            // Generate Bézier curve from control points
            if (group.transform.bezierControlPoints.size() >= 4) {
                group.transform.bezierCurve = generateBezierCurve(group.transform.bezierControlPoints);
            }
        }
    }

    if (DEBUG) cerr << "parsed tranforms" << endl;

    // Parse models
    XMLElement* modelsElem = groupElem->FirstChildElement("models");
    if (modelsElem) {
        if (DEBUG) cerr << "Parsing models element" << endl;

        for (XMLElement* modelElem = modelsElem->FirstChildElement("model"); modelElem; modelElem = modelElem->NextSiblingElement("model")) {
            const char* file = modelElem->Attribute("file");

            if (DEBUG) cerr << "Parsing model file: " << (file ? file : "null") << endl;
            
            if (file) {
                Model model;
                if (loadModel(file, model)) {
                    group.models.push_back(model);
                    if (DEBUG) cerr << "Successfully loaded model: " << file << endl;
                }
                else {
                    if (DEBUG) cerr << "Failed to load model: " << file << endl;
                }
            }
            else {
                if (DEBUG) cerr << "Model file attribute is missing" << endl;
            }
        }
    }

    if (DEBUG) cerr << "Parsed " << group.models.size() << " models." << endl;

    // Parse child groups
    for (XMLElement* child = groupElem->FirstChildElement("group"); child; child = child->NextSiblingElement("group")) {
        group.children.push_back(parseGroup(child));
    }

    if (DEBUG) cerr << "Parsed group with " << group.models.size() << " models and " << group.children.size() << " children." << endl;

    return group;
}

// Model Loading
bool loadModel(const string& filename, Model& model) {

	if (DEBUG) cerr << "Loading model from file: " << filename << endl;

    ifstream file(filename);
    if (!file.is_open()) {
        if (DEBUG) cerr << "Error opening model file: " << filename << endl;
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

	if (DEBUG) cerr << "Loaded " << numVertices << " vertices and " << numFaces << " faces." << endl;

	// Initialize VBOs for the model
	initModelVBOs(model);

	if (DEBUG) cerr << "Model loaded successfully." << endl;

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
    /*
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
    */

    renderModelWithVBOs(model, color);
}

void applyTransform(const Transform& transform) {
    // Handle time-based rotation
    if (transform.isTimedRotation) {
        float elapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float angle = fmod((elapsed / transform.rotationTime) * 360.0f, 360.0f);
        glRotatef(angle, transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    }
    // Static rotation
    else if (transform.rotate[0] != 0) {
        glRotatef(transform.rotate[0], transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    }

    // Handle time-based translation
    if (transform.isTimedTranslation && !transform.translationPoints.empty()) {
        float elapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float t = fmod(elapsed / transform.translationTime, 1.0f);

        int numSegments = transform.translationPoints.size() - 3;
        float segmentT = t * numSegments;
        int segmentIndex = floor(segmentT);
        segmentT = segmentT - segmentIndex;

        vector<vector<float>> p(4);
        for (int i = 0; i < 4; i++) {
            p[i] = transform.translationPoints[segmentIndex + i];
            p[i][1] = -p[i][1];  // Invertir coordenada Y
        }

        vector<float> pos = getCatmullRomPoint(segmentT, p);
        glTranslatef(pos[0], pos[1], pos[2]);

        if (transform.alignWithPath) {
            vector<float> deriv = getCatmullRomDerivative(segmentT, p);
            float len = sqrt(deriv[0] * deriv[0] + deriv[1] * deriv[1] + deriv[2] * deriv[2]);
            if (len > 0.0001f) {
                deriv[0] /= len;
                deriv[1] /= len;
                deriv[2] /= len;

                // Compensación para la rotación inicial de -90° en X
                glRotatef(-90, 1, 0, 0);

                // Cálculo de orientación corregido para Y invertido
                float angle = atan2(deriv[0], deriv[2]) * 180.0f / M_PI;
                glRotatef(angle, 0, 1, 0);

                float pitch = asin(deriv[1]) * 180.0f / M_PI;
                glRotatef(pitch, 1, 0, 0);
            }
        }
    }
    // Static translation
    else {
        glTranslatef(transform.translate[0], transform.translate[1], transform.translate[2]);
    }

    // Scale remains the same
    glScalef(transform.scale[0], transform.scale[1], transform.scale[2]);
}

void renderGroup(const Group& group) {
    /*
    // 1. First draw the rotating curve (using original coordinates, no Y inversion)
    if (!group.transform.bezierCurve.empty()) {
        glPushMatrix();

        // Apply only Y-axis rotation (same as teapot will get)
        float currentRotation = 0.0f;
        if (group.transform.isTimedRotation) {
            float elapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            currentRotation = fmod((elapsed / group.transform.rotationTime) * 360.0f, 360.0f);
        }
        else if (group.transform.rotate[0] != 0) {
            currentRotation = group.transform.rotate[0];
        }
        glRotatef(currentRotation, 0.0f, 1.0f, 0.0f);

        // Draw curve with ORIGINAL coordinates (no Y inversion)
        drawBezierCurve(group.transform.bezierCurve);

        glPopMatrix();
    }
    */
    // 2. Render teapot with full transformations
    glPushMatrix();
    applyTransform(group.transform); // This handles the rotation AND path following

    for (size_t i = 0; i < group.models.size(); ++i) {
        renderModel(group.models[i], modelColors[i % modelColors.size()]);
    }
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

// NEW PHASE 3 functions //////////////////////////////////////////////////
// Helper functions for Catmull-Rom curve calculations
vector<float> getCatmullRomPoint(float t, const vector<vector<float>>& p) {
    // Catmull-Rom matrix
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f},
                      { 1.0f, -2.5f,  2.0f, -0.5f},
                      {-0.5f,  0.0f,  0.5f,  0.0f},
                      { 0.0f,  1.0f,  0.0f,  0.0f} };

    // Compute point at parameter t
    float t3 = t * t * t;
    float t2 = t * t;

    vector<float> point(3, 0);
    for (int i = 0; i < 3; i++) {
        float a = m[0][0] * t3 + m[1][0] * t2 + m[2][0] * t + m[3][0];
        float b = m[0][1] * t3 + m[1][1] * t2 + m[2][1] * t + m[3][1];
        float c = m[0][2] * t3 + m[1][2] * t2 + m[2][2] * t + m[3][2];
        float d = m[0][3] * t3 + m[1][3] * t2 + m[2][3] * t + m[3][3];

        point[i] = a * p[0][i] + b * p[1][i] + c * p[2][i] + d * p[3][i];
    }

    return point;
}

vector<float> getCatmullRomDerivative(float t, const vector<vector<float>>& p) {
    // Catmull-Rom matrix derivative
    float m[4][4] = { {-1.5f,  4.5f, -4.5f,  1.5f},
                      { 2.0f, -5.0f,  4.0f, -1.0f},
                      {-0.5f,  0.0f,  0.5f,  0.0f},
                      { 0.0f,  1.0f,  0.0f,  0.0f} };

    // Compute derivative at parameter t
    float t2 = t * t;

    vector<float> derivative(3, 0);
    for (int i = 0; i < 3; i++) {
        float a = m[0][0] * t2 + m[1][0] * t + m[2][0];
        float b = m[0][1] * t2 + m[1][1] * t + m[2][1];
        float c = m[0][2] * t2 + m[1][2] * t + m[2][2];
        float d = m[0][3] * t2 + m[1][3] * t + m[2][3];

        derivative[i] = a * p[0][i] + b * p[1][i] + c * p[2][i] + d * p[3][i];
    }

    return derivative;
}

vector<vector<float>> generateBezierCurve(const vector<vector<float>>& controlPoints, int segments) {
    vector<vector<float>> curvePoints;
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float u = 1.0f - t;

        // Fórmula de Bézier cúbica
        float x = u * u * u * controlPoints[0][0] + 3 * u * u * t * controlPoints[1][0] +
            3 * u * t * t * controlPoints[2][0] + t * t * t * controlPoints[3][0];
        float y = u * u * u * controlPoints[0][1] + 3 * u * u * t * controlPoints[1][1] +
            3 * u * t * t * controlPoints[2][1] + t * t * t * controlPoints[3][1];
        float z = u * u * u * controlPoints[0][2] + 3 * u * u * t * controlPoints[1][2] +
            3 * u * t * t * controlPoints[2][2] + t * t * t * controlPoints[3][2];

        curvePoints.push_back({ x, y, z });
    }
    return curvePoints;
}

void drawBezierCurve(const vector<vector<float>>& curvePoints) {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.0f, 1.0f); // Magenta color for better visibility
    glBegin(GL_LINE_STRIP);
    for (const auto& point : curvePoints) {
        glVertex3f(point[0], point[1], point[2]);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// Functions to initialize and render with VBOs
void initModelVBOs(Model& model) {
    if (model.buffersInitialized) return;

    if (DEBUG) cerr << "Initializing VBOs..." << endl;

    // Check for valid OpenGL context
    if (!glGenBuffers) {
        if (DEBUG) cerr << "OpenGL buffer functions not loaded!" << endl;
        return;
    }

    // Generate vertex buffer
    glGenBuffers(1, &model.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
        model.vertices.size() * sizeof(float),
        model.vertices.data(),
        GL_STATIC_DRAW);

    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        if (DEBUG) cerr << "OpenGL error in vertex buffer creation: " << err << endl;
    }

    // Generate index buffer
    glGenBuffers(1, &model.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        model.faces.size() * sizeof(int),
        model.faces.data(),
        GL_STATIC_DRAW);

    err = glGetError();
    if (err != GL_NO_ERROR) {
        if (DEBUG) cerr << "OpenGL error in index buffer creation: " << err << endl;
    }

    model.buffersInitialized = true;
    if (DEBUG) cerr << "VBOs initialized successfully" << endl;
}

void renderModelWithVBOs(const Model& model, const vector<float>& color) {
    if (!model.buffersInitialized) {
        // This shouldn't happen since we initialize VBOs during loading
        if (DEBUG) cerr << "Error: Trying to render model with uninitialized VBOs" << endl;
        return;
    }

    glColor3f(color[0], color[1], color[2]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Enable vertex array and specify its format
    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    // Bind index buffer and draw
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glDrawElements(GL_TRIANGLES, model.faces.size(), GL_UNSIGNED_INT, 0);

    // Cleanup
    glDisableClientState(GL_VERTEX_ARRAY);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Main Function
int main(int argc, char** argv) {

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <config.xml>" << endl;
        return 1;
    }

	if (DEBUG) cerr << "Starting program with config: " << argv[1] << endl;

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window.width, window.height);
    glutCreateWindow("3D Engine - Phase 3");

	if (DEBUG) cerr << "GLUT initialized" << endl;

    // Initialize GLEW (needed for VBOs on Windows)
#ifdef WIN32
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        if (DEBUG) cerr << "Error initializing GLEW: " << glewGetErrorString(err) << endl;
        return 1;
    }
#endif

	if (DEBUG) cerr << "GLEW initialized" << endl;

    // Load configuration
    if (!readXMLConfig(argv[1], window, camera, rootGroup)) {
        if (DEBUG) cerr << "Failed to load configuration!" << endl;
        return 1;
    }
    
    // Resize window if needed
    if (window.width != 800 || window.height != 600) {
        glutReshapeWindow(window.width, window.height);
    }
    if (DEBUG) cerr << "Config loaded" << endl;

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // Set up lighting
    GLfloat light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // Register callbacks
    glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeys);

    // Enable idle function for animations
    glutIdleFunc([]() {
        static int lastTime = 0;
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - lastTime > 16) { // ~60 FPS
            glutPostRedisplay();
            lastTime = currentTime;
        }
        });

    // Start main loop
    glutMainLoop();

    // Clean up VBOs
    function<void(Group&)> cleanupModels;
    cleanupModels = [&cleanupModels](Group& group) {
        for (auto& model : group.models) {
            if (model.buffersInitialized) {
                glDeleteBuffers(1, &model.vertexBuffer);
                glDeleteBuffers(1, &model.indexBuffer);
            }
        }
        for (auto& child : group.children) {
            cleanupModels(child);
        }
        };
    cleanupModels(rootGroup);

    return 0;
}
