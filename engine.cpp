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
#include <map> // Para caché de texturas
#include <cmath> // Para sqrt en normalización de spotlight direction

// --- Headers para Carga de Imágenes (DevIL) ---
#ifdef _WIN32
#include "include/IL/il.h"
#else
#include <IL/il.h>
#endif
// ----------------------------------------------

#define _USE_MATH_DEFINES // Habilitar constantes matemáticas (como M_PI)
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace tinyxml2;

// Debug variable
bool DEBUG = true;

// --- Estructuras ---
struct Camera {
    float posX = 5, posY = 5, posZ = 5;
    float lookAtX = 0, lookAtY = 0, lookAtZ = 0;
    float upX = 0, upY = 1, upZ = 0;
    float fov = 60, near = 1, far = 1000;
};

struct Window {
    int width = 800, height = 600;
};

struct Material {
    float diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float emissive[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float shininess = 0.0f;
};

struct Model {
    vector<float> vertices;
    vector<float> normals;
    vector<float> texCoords;
    vector<unsigned int> faces;

    GLuint vertexBuffer = 0;
    GLuint normalBuffer = 0;
    GLuint texCoordBuffer = 0;
    GLuint indexBuffer = 0;
    bool buffersInitialized = false;

    string textureFile;
    GLuint textureID = 0;
    Material material;
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
    vector<vector<float>> catmullRomCurvePoints;
};

struct Light {
    enum Type { POINT, DIRECTIONAL, SPOTLIGHT }; // Ensure SPOTLIGHT is here
    Type type = POINT;
    float position[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float direction[3] = { 0.0f, 0.0f, -1.0f };
    float cutoff = 180.0f;
    float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct Group {
    Transform transform;
    vector<Model> models;
    vector<Group> children;
};

// --- Variables Globales ---
Window window;
Camera camera;
Group rootGroup;
float cameraTheta = M_PI / 4.0f, cameraPhi = M_PI / 4.0f, cameraRadius = 15.0f;

vector<Light> lights;
map<string, GLuint> textureCache;

// --- Prototipos ---
bool initializeImageLibrary();
bool loadImage(const string& path, GLuint& textureID);
bool readXMLConfig(const string& filename);
void parseCamera(XMLElement* cameraElem);
Group parseGroup(XMLElement* groupElem);
void parseTransform(XMLElement* transformElem, Transform& transform);
bool loadModel(const string& filename, Model& model);
void initModelVBOs(Model& model);
void setupCamera();
void setupLights();
void drawAxes();
void renderGroup(const Group& group);
void applyTransform(const Transform& transform);
void renderModelWithVBOs(const Model& model);
void renderScene();
void keyboardFunc(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void idleFunc();
void reshapeFunc(int w, int h);
void cleanup();
void cleanupGroupVBOs(Group& group);
vector<float> getCatmullRomPoint(float t, const vector<vector<float>>& p);
vector<float> getCatmullRomDerivative(float t, const vector<vector<float>>& p);
void buildRotMatrix(float* m, float* x, float* y, float* z);
void cross(float* a, float* b, float* res);
void normalize(float* a);


// --- Implementación ---

bool initializeImageLibrary() {
    // *** MODIFICADO AQUÍ ***
#ifdef IL_VERSION // Comprobar si el macro genérico de DevIL está definido
    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
    if (DEBUG) cerr << "DevIL inicializado." << endl;
    return true;
#else
    if (DEBUG) cerr << "Advertencia: DevIL no parece estar disponible/incluido (IL_VERSION no definido). La carga de texturas fallará." << endl;
    return false;
#endif
}

bool loadImage(const string& path, GLuint& textureID) {
    // *** MODIFICADO AQUÍ ***
#ifdef IL_VERSION // Comprobar si el macro genérico de DevIL está definido
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage((ILstring)path.c_str())) {
        ILenum error = ilGetError();
        cerr << "Error cargando textura '" << path << "': " << error << endl;
        ilDeleteImages(1, &imageID);
        textureID = 0;
        return false;
    }

    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
        cerr << "Error convirtiendo textura '" << path << "' a RGBA." << endl;
        ilDeleteImages(1, &imageID);
        textureID = 0;
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
        0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilDeleteImages(1, &imageID);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (DEBUG) cerr << "Textura cargada y generada: '" << path << "' (ID: " << textureID << ")" << endl;
    return true;

#else
    textureID = 0;
    return false;
#endif
}

// Lectura de Configuración XML
bool readXMLConfig(const string& filename) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Error cargando archivo XML: " << filename << endl;
        return false;
    }

    XMLElement* world = doc.FirstChildElement("world");
    if (!world) {
        cerr << "Elemento 'world' no encontrado en el XML." << endl;
        return false;
    }

    // Configuración de Ventana
    XMLElement* windowElem = world->FirstChildElement("window");
    if (windowElem) {
        windowElem->QueryIntAttribute("width", &window.width);
        windowElem->QueryIntAttribute("height", &window.height);
    }

    // Configuración de Cámara
    XMLElement* cameraElem = world->FirstChildElement("camera");
    if (cameraElem) {
        parseCamera(cameraElem);
    }
    else {
        cerr << "Advertencia: No se encontró configuración de cámara en XML. Usando defaults." << endl;
    }

    // Configuración de Luces (Fase 4)
    XMLElement* lightsElem = world->FirstChildElement("lights");
    if (lightsElem) {
        for (XMLElement* lightElem = lightsElem->FirstChildElement("light"); lightElem; lightElem = lightElem->NextSiblingElement("light")) {
            Light light; // Usa defaults iniciales
            const char* typeStr = lightElem->Attribute("type");
            if (typeStr) {
                bool typeRecognized = false; // Flag para saber si añadimos la luz
                if (strcmp(typeStr, "point") == 0) {
                    light.type = Light::POINT;
                    lightElem->QueryFloatAttribute("posx", &light.position[0]);
                    lightElem->QueryFloatAttribute("posy", &light.position[1]);
                    lightElem->QueryFloatAttribute("posz", &light.position[2]);
                    light.position[3] = 1.0f;
                    typeRecognized = true;
                }
                else if (strcmp(typeStr, "directional") == 0) {
                    light.type = Light::DIRECTIONAL;
                    lightElem->QueryFloatAttribute("dirx", &light.direction[0]);
                    lightElem->QueryFloatAttribute("diry", &light.direction[1]);
                    lightElem->QueryFloatAttribute("dirz", &light.direction[2]);
                    light.position[0] = -light.direction[0];
                    light.position[1] = -light.direction[1];
                    light.position[2] = -light.direction[2];
                    light.position[3] = 0.0f;
                    typeRecognized = true;
                }
                else if (strcmp(typeStr, "spot") == 0) {
                    light.type = Light::SPOTLIGHT;
                    lightElem->QueryFloatAttribute("posx", &light.position[0]);
                    lightElem->QueryFloatAttribute("posy", &light.position[1]);
                    lightElem->QueryFloatAttribute("posz", &light.position[2]);
                    light.position[3] = 1.0f;
                    lightElem->QueryFloatAttribute("dirx", &light.direction[0]);
                    lightElem->QueryFloatAttribute("diry", &light.direction[1]);
                    lightElem->QueryFloatAttribute("dirz", &light.direction[2]);
                    lightElem->QueryFloatAttribute("cutoff", &light.cutoff);
                    typeRecognized = true;
                }
                else {
                    cerr << "Advertencia: Tipo de luz desconocido '" << typeStr << "'. Ignorando." << endl;
                }

                if (typeRecognized) {
                    lights.push_back(light); // Solo añadir si el tipo se reconoció
                }
            }
            else {
                cerr << "Advertencia: Luz sin atributo 'type'. Ignorando." << endl;
            }
        }
        if (DEBUG) cerr << "Cargadas " << lights.size() << " luces desde XML." << endl;
    }
    else {
        if (DEBUG) cerr << "Advertencia: No se encontró sección <lights> en XML." << endl;
    }

    // Parsear la jerarquía de grupos
    XMLElement* groupElem = world->FirstChildElement("group");
    if (groupElem) {
        rootGroup = parseGroup(groupElem);
    }
    else {
        cerr << "Advertencia: No se encontró elemento <group> principal en el XML." << endl;
    }

    if (DEBUG) cerr << "Configuración XML cargada exitosamente." << endl;
    return true;
}

// Parsear Cámara
void parseCamera(XMLElement* cameraElem) {
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

    cameraRadius = sqrt(camera.posX * camera.posX + camera.posY * camera.posY + camera.posZ * camera.posZ);
    if (cameraRadius > numeric_limits<float>::epsilon()) {
        cameraTheta = atan2(camera.posZ, camera.posX);
        cameraPhi = acos(camera.posY / cameraRadius);
    }
    else {
        cameraRadius = 15.0f;
        cameraTheta = M_PI / 4.0f;
        cameraPhi = M_PI / 4.0f;
    }
    if (DEBUG) cerr << "Cámara inicializada: R=" << cameraRadius << ", Theta=" << cameraTheta << ", Phi=" << cameraPhi << endl;
}

// Parsear Transformaciones
void parseTransform(XMLElement* transformElem, Transform& transform) {
    if (!transformElem) return;

    XMLElement* translate = transformElem->FirstChildElement("translate");
    if (translate) {
        if (translate->Attribute("time")) {
            transform.isTimedTranslation = true;
            translate->QueryFloatAttribute("time", &transform.translationTime);
            transform.alignWithPath = translate->BoolAttribute("align", false);
            transform.translationPoints.clear();
            for (XMLElement* point = translate->FirstChildElement("point"); point; point = point->NextSiblingElement("point")) {
                vector<float> p(3);
                point->QueryFloatAttribute("x", &p[0]);
                point->QueryFloatAttribute("y", &p[1]);
                point->QueryFloatAttribute("z", &p[2]);
                transform.translationPoints.push_back(p);
            }
            if (transform.translationPoints.size() < 4) {
                cerr << "Advertencia: Traslación Catmull-Rom necesita al menos 4 puntos." << endl;
                transform.isTimedTranslation = false;
            }
        }
        else {
            transform.isTimedTranslation = false;
            translate->QueryFloatAttribute("x", &transform.translate[0]);
            translate->QueryFloatAttribute("y", &transform.translate[1]);
            translate->QueryFloatAttribute("z", &transform.translate[2]);
        }
    }

    XMLElement* rotate = transformElem->FirstChildElement("rotate");
    if (rotate) {
        if (rotate->Attribute("time")) {
            transform.isTimedRotation = true;
            rotate->QueryFloatAttribute("time", &transform.rotationTime);
        }
        else {
            transform.isTimedRotation = false;
            rotate->QueryFloatAttribute("angle", &transform.rotate[0]);
        }
        rotate->QueryFloatAttribute("x", &transform.rotate[1]);
        rotate->QueryFloatAttribute("y", &transform.rotate[2]);
        rotate->QueryFloatAttribute("z", &transform.rotate[3]);
    }

    XMLElement* scale = transformElem->FirstChildElement("scale");
    if (scale) {
        scale->QueryFloatAttribute("x", &transform.scale[0]);
        scale->QueryFloatAttribute("y", &transform.scale[1]);
        scale->QueryFloatAttribute("z", &transform.scale[2]);
    }
}

// Parsear Grupo (recursivo)
Group parseGroup(XMLElement* groupElem) {
    Group group;
    if (!groupElem) return group;

    parseTransform(groupElem->FirstChildElement("transform"), group.transform);

    XMLElement* modelsElem = groupElem->FirstChildElement("models");
    if (modelsElem) {
        for (XMLElement* modelElem = modelsElem->FirstChildElement("model"); modelElem; modelElem = modelElem->NextSiblingElement("model")) {
            const char* file = modelElem->Attribute("file");
            if (file) {
                Model model;
                if (loadModel(file, model)) {
                    XMLElement* textureElem = modelElem->FirstChildElement("texture");
                    if (textureElem && textureElem->Attribute("file")) {
                        model.textureFile = textureElem->Attribute("file");
                        if (textureCache.count(model.textureFile)) {
                            model.textureID = textureCache[model.textureFile];
                            if (DEBUG) cerr << "Textura '" << model.textureFile << "' encontrada en caché (ID: " << model.textureID << ")" << endl;
                        }
                        else {
                            if (loadImage(model.textureFile, model.textureID)) {
                                textureCache[model.textureFile] = model.textureID;
                            }
                        }
                    }

                    XMLElement* colorElem = modelElem->FirstChildElement("color");
                    if (colorElem) {
                        auto parseColorComponent = [&](const char* elemName, float* target) {
                            XMLElement* compElem = colorElem->FirstChildElement(elemName);
                            if (compElem) {
                                int R = 0, G = 0, B = 0;
                                compElem->QueryIntAttribute("R", &R);
                                compElem->QueryIntAttribute("G", &G);
                                compElem->QueryIntAttribute("B", &B);
                                target[0] = R / 255.0f;
                                target[1] = G / 255.0f;
                                target[2] = B / 255.0f;
                                target[3] = 1.0f;
                            }
                            };
                        parseColorComponent("diffuse", model.material.diffuse);
                        parseColorComponent("ambient", model.material.ambient);
                        parseColorComponent("specular", model.material.specular);
                        parseColorComponent("emissive", model.material.emissive);

                        XMLElement* shininessElem = colorElem->FirstChildElement("shininess");
                        if (shininessElem) {
                            shininessElem->QueryFloatAttribute("value", &model.material.shininess);
                        }
                    }
                    group.models.push_back(model);
                }
                else {
                    cerr << "Error al cargar modelo: " << file << endl;
                }
            }
            else {
                cerr << "Advertencia: Elemento <model> sin atributo 'file'. Ignorando." << endl;
            }
        }
    }

    for (XMLElement* childGroupElem = groupElem->FirstChildElement("group"); childGroupElem; childGroupElem = childGroupElem->NextSiblingElement("group")) {
        group.children.push_back(parseGroup(childGroupElem));
    }

    return group;
}

// Carga de Modelo desde archivo .3d
bool loadModel(const string& filename, Model& model) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error abriendo archivo de modelo: " << filename << endl;
        return false;
    }

    int numVertices = 0;
    file >> numVertices;
    if (numVertices <= 0) { cerr << "Error: Número de vértices inválido (" << numVertices << ") en " << filename << endl; file.close(); return false; }
    model.vertices.resize(numVertices * 3);
    for (int i = 0; i < numVertices * 3; ++i) {
        if (!(file >> model.vertices[i])) { cerr << "Error leyendo vértice " << i / 3 << " en " << filename << endl; file.close(); return false; }
    }

    int numNormals = 0;
    file >> numNormals;
    if (numNormals != numVertices) { cerr << "Advertencia: Número de normales (" << numNormals << ") no coincide con vértices (" << numVertices << ") en " << filename << endl; }
    model.normals.resize(numNormals * 3);
    for (int i = 0; i < numNormals * 3; ++i) {
        if (!(file >> model.normals[i])) { cerr << "Error leyendo normal " << i / 3 << " en " << filename << endl; file.close(); return false; }
    }

    int numTexCoords = 0;
    file >> numTexCoords;
    if (numTexCoords != numVertices) { cerr << "Advertencia: Número de coords. textura (" << numTexCoords << ") no coincide con vértices (" << numVertices << ") en " << filename << endl; }
    model.texCoords.resize(numTexCoords * 2);
    for (int i = 0; i < numTexCoords * 2; ++i) {
        if (!(file >> model.texCoords[i])) { cerr << "Error leyendo coord. textura " << i / 2 << " en " << filename << endl; file.close(); return false; }
    }

    int numFaces = 0;
    file >> numFaces;
    if (numFaces <= 0) { cerr << "Error: Número de caras inválido (" << numFaces << ") en " << filename << endl; file.close(); return false; }
    model.faces.resize(numFaces * 3);
    for (int i = 0; i < numFaces * 3; ++i) {
        if (!(file >> model.faces[i])) { cerr << "Error leyendo índice de cara " << i / 3 << ", componente " << i % 3 << " en " << filename << endl; file.close(); return false; }
    }

    file.close();
    initModelVBOs(model);
    if (DEBUG) cerr << "Modelo cargado: '" << filename << "' (V: " << numVertices << ", N: " << numNormals << ", T: " << numTexCoords << ", F: " << numFaces << ")" << endl;
    return true;
}

// Inicialización de VBOs
void initModelVBOs(Model& model) {
    if (model.buffersInitialized) return;
    if (model.vertices.empty() || model.faces.empty()) {
        cerr << "Error VBO: Modelo sin vértices o caras." << endl;
        return;
    }

    glGenBuffers(1, &model.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(float), model.vertices.data(), GL_STATIC_DRAW);

    if (!model.normals.empty()) {
        glGenBuffers(1, &model.normalBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, model.normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(float), model.normals.data(), GL_STATIC_DRAW);
    }
    else {
        model.normalBuffer = 0;
    }

    if (!model.texCoords.empty()) {
        glGenBuffers(1, &model.texCoordBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, model.texCoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, model.texCoords.size() * sizeof(float), model.texCoords.data(), GL_STATIC_DRAW);
    }
    else {
        model.texCoordBuffer = 0;
    }

    glGenBuffers(1, &model.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.faces.size() * sizeof(unsigned int), model.faces.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    model.buffersInitialized = true;
    if (DEBUG) cerr << "VBOs inicializados para modelo (V:" << model.vertexBuffer << ", N:" << model.normalBuffer << ", T:" << model.texCoordBuffer << ", I:" << model.indexBuffer << ")" << endl;
}


// Configuración de Cámara
void setupCamera() {
    float minPhi = 0.01f;
    float maxPhi = M_PI - 0.01f;
    if (cameraPhi < minPhi) cameraPhi = minPhi;
    if (cameraPhi > maxPhi) cameraPhi = maxPhi;

    camera.posX = cameraRadius * sin(cameraPhi) * cos(cameraTheta);
    camera.posY = cameraRadius * cos(cameraPhi);
    camera.posZ = cameraRadius * sin(cameraPhi) * sin(cameraTheta);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.fov, (float)window.width / window.height, camera.near, camera.far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        camera.posX + camera.lookAtX, camera.posY + camera.lookAtY, camera.posZ + camera.lookAtZ,
        camera.lookAtX, camera.lookAtY, camera.lookAtZ,
        camera.upX, camera.upY, camera.upZ
    );
}

// Configuración de Luces
void setupLights() {
    glEnable(GL_LIGHTING);

    float global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    int maxLights = 8;
    for (int i = 0; i < lights.size() && i < maxLights; ++i) {
        GLenum lightID = GL_LIGHT0 + i;
        glEnable(lightID);

        glLightfv(lightID, GL_POSITION, lights[i].position);
        glLightfv(lightID, GL_AMBIENT, lights[i].ambient);
        glLightfv(lightID, GL_DIFFUSE, lights[i].diffuse);
        glLightfv(lightID, GL_SPECULAR, lights[i].specular);

        if (lights[i].type == Light::SPOTLIGHT) {
            if (DEBUG) {
                cerr << "Setting Spotlight " << i << ":" << endl;
                cerr << "  Position: " << lights[i].position[0] << "," << lights[i].position[1] << "," << lights[i].position[2] << "," << lights[i].position[3] << endl;
                cerr << "  Direction: " << lights[i].direction[0] << "," << lights[i].direction[1] << "," << lights[i].direction[2] << endl;
                cerr << "  Cutoff: " << lights[i].cutoff << endl;
            }

            // Normalize direction (good practice)
            float dir[3] = { lights[i].direction[0], lights[i].direction[1], lights[i].direction[2] };
            float len = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
            if (len > 1e-6f) {
                dir[0] /= len;
                dir[1] /= len;
                dir[2] /= len;
            }
            if (DEBUG) cerr << "  Normalized Direction: " << dir[0] << "," << dir[1] << "," << dir[2] << endl;

            glLightfv(lightID, GL_SPOT_DIRECTION, dir);
            glLightf(lightID, GL_SPOT_CUTOFF, lights[i].cutoff);
            glLightf(lightID, GL_SPOT_EXPONENT, 0.0f);
        }
        else {
            glLightf(lightID, GL_SPOT_CUTOFF, 180.0f);
        }
    }

    for (int i = lights.size(); i < maxLights; ++i) {
        glDisable(GL_LIGHT0 + i);
    }
}

// Dibujar Ejes
void drawAxes() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0); glVertex3f(-100.0, 0.0, 0.0); glVertex3f(100.0, 0.0, 0.0);
    glColor3f(0.0, 1.0, 0.0); glVertex3f(0.0, -100.0, 0.0); glVertex3f(0.0, 100.0, 0.0);
    glColor3f(0.0, 0.0, 1.0); glVertex3f(0.0, 0.0, -100.0); glVertex3f(0.0, 0.0, 100.0);
    glEnd();
    glLineWidth(1.0f);
}

// Aplicar Transformaciones
void applyTransform(const Transform& transform) {
    if (transform.isTimedTranslation && !transform.translationPoints.empty()) {
        float timeElapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float totalTime = transform.translationTime > 0 ? transform.translationTime : 1.0f;
        float t_global = fmod(timeElapsed / totalTime, 1.0f);
        int numSegments = transform.translationPoints.size() - 3;
        if (numSegments < 1) return;

        float t_segment = t_global * numSegments;
        int currentSegment = floor(t_segment);
        float t_local = t_segment - currentSegment;

        vector<vector<float>> controlPoints(4);
        for (int i = 0; i < 4; ++i) controlPoints[i] = transform.translationPoints[currentSegment + i];

        vector<float> pos = getCatmullRomPoint(t_local, controlPoints);
        glTranslatef(pos[0], pos[1], pos[2]);

        if (transform.alignWithPath) {
            vector<float> deriv = getCatmullRomDerivative(t_local, controlPoints);
            normalize(deriv.data());
            float up[3] = { 0, 1, 0 };
            float left[3];
            cross(deriv.data(), up, left);
            normalize(left);
            float newUp[3];
            cross(left, deriv.data(), newUp);
            normalize(newUp);
            float rotMatrix[16];
            buildRotMatrix(rotMatrix, deriv.data(), newUp, left);
            glMultMatrixf(rotMatrix);
        }
    }
    else {
        glTranslatef(transform.translate[0], transform.translate[1], transform.translate[2]);
    }

    if (transform.isTimedRotation) {
        float timeElapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float totalTime = transform.rotationTime > 0 ? transform.rotationTime : 1.0f;
        float currentAngle = fmod((timeElapsed / totalTime) * 360.0f, 360.0f);
        glRotatef(currentAngle, transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    }
    else {
        glRotatef(transform.rotate[0], transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    }

    glScalef(transform.scale[0], transform.scale[1], transform.scale[2]);
}

// Renderizar Grupo
void renderGroup(const Group& group) {
    glPushMatrix();
    applyTransform(group.transform);
    for (const auto& model : group.models) {
        renderModelWithVBOs(model);
    }
    for (const auto& child : group.children) {
        renderGroup(child);
    }
    glPopMatrix();
}

// Renderizar Modelo con VBOs
void renderModelWithVBOs(const Model& model) {
    if (!model.buffersInitialized || model.vertexBuffer == 0 || model.indexBuffer == 0) return;

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, model.material.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, model.material.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, model.material.specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, model.material.emissive);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, model.material.shininess);

    bool hasTexture = (model.textureID != 0 && model.texCoordBuffer != 0);
    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, model.textureID);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, model.texCoordBuffer);
        glTexCoordPointer(2, GL_FLOAT, 0, 0);
    }
    else {
        glDisable(GL_TEXTURE_2D); // Make sure texturing is off if no texture
    }

    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    if (model.normalBuffer != 0) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, model.normalBuffer);
        glNormalPointer(GL_FLOAT, 0, 0);
    }
    else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glDrawElements(GL_TRIANGLES, model.faces.size(), GL_UNSIGNED_INT, 0);

    // --- CLEANUP ---
    glDisableClientState(GL_VERTEX_ARRAY); // Disable vertex array
    if (model.normalBuffer != 0) glDisableClientState(GL_NORMAL_ARRAY); // Disable normal array if it was enabled
    if (hasTexture) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY); // Disable tex coord array if it was enabled
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
        glDisable(GL_TEXTURE_2D); // Explicitly disable texture unit again if needed
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO
}


// Renderizar Escena
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCamera();
    setupLights();
    drawAxes();

    glEnable(GL_LIGHTING); // Ensure lighting is enabled before drawing groups
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    renderGroup(rootGroup);

    glutSwapBuffers();
}


// --- Callbacks y Controles ---
void keyboardFunc(unsigned char key, int x, int y) {
    float moveStep = 0.5f;
    float angleStep = 0.1f;
    switch (key) {
    case 'a': cameraTheta -= angleStep; break;
    case 'd': cameraTheta += angleStep; break;
    case 'w': cameraPhi -= angleStep; break;
    case 's': cameraPhi += angleStep; break;
    case 'q': cameraRadius -= moveStep; if (cameraRadius < 0.1f) cameraRadius = 0.1f; break;
    case 'e': cameraRadius += moveStep; break;
    case 27: cleanup(); exit(0); break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    float angleStep = 0.1f;
    switch (key) {
    case GLUT_KEY_UP:    cameraPhi -= angleStep; break;
    case GLUT_KEY_DOWN:  cameraPhi += angleStep; break;
    case GLUT_KEY_LEFT:  cameraTheta -= angleStep; break;
    case GLUT_KEY_RIGHT: cameraTheta += angleStep; break;
    }
    glutPostRedisplay();
}

void idleFunc() { glutPostRedisplay(); }

void reshapeFunc(int w, int h) {
    if (h == 0) h = 1;
    window.width = w;
    window.height = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

// --- Limpieza ---
void cleanupGroupVBOs(Group& group) {
    for (auto& model : group.models) {
        if (model.buffersInitialized) {
            if (model.vertexBuffer != 0) glDeleteBuffers(1, &model.vertexBuffer);
            if (model.normalBuffer != 0) glDeleteBuffers(1, &model.normalBuffer);
            if (model.texCoordBuffer != 0) glDeleteBuffers(1, &model.texCoordBuffer);
            if (model.indexBuffer != 0) glDeleteBuffers(1, &model.indexBuffer);
            model.buffersInitialized = false;
            model.vertexBuffer = model.normalBuffer = model.texCoordBuffer = model.indexBuffer = 0;
        }
    }
    for (auto& child : group.children) {
        cleanupGroupVBOs(child);
    }
}

void cleanup() {
    if (DEBUG) std::cerr << "Iniciando limpieza..." << std::endl;
    cleanupGroupVBOs(rootGroup);
    // C++17 structured binding
    // for (auto const& [key, val] : textureCache) {
    // C++11/14 compatible loop
    for (auto const& pair : textureCache) {
        const std::string& key = pair.first;
        GLuint val = pair.second;
        if (val != 0) {
            glDeleteTextures(1, &val);
            if (DEBUG) std::cerr << "Textura borrada: ID " << val << " (Archivo: " << key << ")" << std::endl;
        }
    }
    textureCache.clear();
    if (DEBUG) std::cerr << "Limpieza completada." << std::endl;
}


// --- Funciones de Animación (Fase 3) ---
vector<float> getCatmullRomPoint(float t, const vector<vector<float>>& p) {
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f}, { 1.0f, -2.5f,  2.0f, -0.5f}, {-0.5f,  0.0f,  0.5f,  0.0f}, { 0.0f,  1.0f,  0.0f,  0.0f} };
    vector<float> point(3, 0.0f);
    float T[4] = { t * t * t, t * t, t, 1 };
    for (int i = 0; i < 3; ++i) {
        float C[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        for (int j = 0; j < 4; ++j) { C[j] = m[j][0] * p[0][i] + m[j][1] * p[1][i] + m[j][2] * p[2][i] + m[j][3] * p[3][i]; }
        point[i] = T[0] * C[0] + T[1] * C[1] + T[2] * C[2] + T[3] * C[3];
    }
    return point;
}

vector<float> getCatmullRomDerivative(float t, const vector<vector<float>>& p) {
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f}, { 1.0f, -2.5f,  2.0f, -0.5f}, {-0.5f,  0.0f,  0.5f,  0.0f}, { 0.0f,  1.0f,  0.0f,  0.0f} };
    vector<float> deriv(3, 0.0f);
    float T_deriv[4] = { 3 * t * t, 2 * t, 1, 0 };
    for (int i = 0; i < 3; ++i) {
        float C[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        for (int j = 0; j < 4; ++j) { C[j] = m[j][0] * p[0][i] + m[j][1] * p[1][i] + m[j][2] * p[2][i] + m[j][3] * p[3][i]; }
        deriv[i] = T_deriv[0] * C[0] + T_deriv[1] * C[1] + T_deriv[2] * C[2] + T_deriv[3] * C[3];
    }
    return deriv;
}

void cross(float* a, float* b, float* res) {
    res[0] = a[1] * b[2] - a[2] * b[1];
    res[1] = a[2] * b[0] - a[0] * b[2];
    res[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize(float* a) {
    float l = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    if (l > numeric_limits<float>::epsilon()) {
        a[0] = a[0] / l;
        a[1] = a[1] / l;
        a[2] = a[2] / l;
    }
}

void buildRotMatrix(float* m, float* x, float* y, float* z) {
    m[0] = x[0]; m[1] = x[1]; m[2] = x[2]; m[3] = 0;
    m[4] = y[0]; m[5] = y[1]; m[6] = y[2]; m[7] = 0;
    m[8] = z[0]; m[9] = z[1]; m[10] = z[2]; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

// --- Función Principal ---
int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <config.xml>" << endl;
        return 1;
    }
    string configFile = argv[1];

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(window.width, window.height);
    glutCreateWindow("CG Engine - Fase 4");

    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshapeFunc);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idleFunc);
    atexit(cleanup);

#ifdef _WIN32
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        cerr << "Error inicializando GLEW: " << glewGetErrorString(glewErr) << endl; return 1;
    }
#endif

    if (!initializeImageLibrary()) {
        cerr << "Fallo al inicializar la biblioteca de imágenes. Las texturas no funcionarán." << endl;
    }

    if (!readXMLConfig(configFile)) {
        cerr << "Fallo al cargar el archivo de configuración: " << configFile << endl;
        return 1;
    }

    glutReshapeWindow(window.width, window.height);

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE); // Re-enable culling if you need it
    glFrontFace(GL_CCW);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_RESCALE_NORMAL);

    float low_global_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, low_global_ambient);

    // NO NEED TO ENABLE TEXTURE GLOBALLY if handled per model
    // glEnable(GL_TEXTURE_2D);

    if (DEBUG) cerr << "Iniciando bucle principal de GLUT..." << endl;
    glutMainLoop();

    return 0;
}