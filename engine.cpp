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

// --- Headers para Carga de Imágenes (DevIL) ---
// Necesitas instalar DevIL y enlazar las librerías (DevIL.lib, ILU.lib, ILUT.lib)
// O adaptar para usar otra biblioteca como stb_image.h
#ifdef _WIN32
#include "include/IL/il.h"
// #include <IL/ilu.h> // Podría ser necesario
// #include <IL/ilut.h> // Podría ser necesario para helpers de OpenGL
#else // Linux/macOS (ajustar rutas si es necesario)
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
    float posX = 5, posY = 5, posZ = 5; // Defaults razonables
    float lookAtX = 0, lookAtY = 0, lookAtZ = 0;
    float upX = 0, upY = 1, upZ = 0;
    float fov = 60, near = 1, far = 1000;
};

struct Window {
    int width = 800, height = 600; // Defaults razonables
};

// Propiedades de Material (Fase 4)
struct Material {
    float diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f }; // Gris por defecto
    float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Ambiente bajo por defecto
    float specular[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Sin especular por defecto
    float emissive[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Sin emisión por defecto
    float shininess = 0.0f;                     // Sin brillo por defecto
};

// Modelo 3D (Actualizado Fase 4)
struct Model {
    vector<float> vertices;
    vector<float> normals;      // Normales por vértice
    vector<float> texCoords;    // Coordenadas de textura por vértice
    vector<unsigned int> faces; // Índices de las caras

    GLuint vertexBuffer = 0;
    GLuint normalBuffer = 0;    // VBO para normales
    GLuint texCoordBuffer = 0;  // VBO para coords. textura
    GLuint indexBuffer = 0;
    bool buffersInitialized = false;

    string textureFile;         // Nombre archivo textura (del XML)
    GLuint textureID = 0;       // ID de textura OpenGL (0 si no tiene)
    Material material;          // Propiedades del material (del XML)
};

// Transformaciones Geométricas (Actualizado Fase 3)
struct Transform {
    float translate[3] = { 0, 0, 0 };
    float rotate[4] = { 0, 0, 1, 0 }; // angle, x, y, z
    float scale[3] = { 1, 1, 1 };

    // Para animación (Fase 3)
    bool isTimedRotation = false;
    float rotationTime = 0; // Segundos para 360 grados
    bool isTimedTranslation = false;
    float translationTime = 0; // Segundos para recorrer la curva
    bool alignWithPath = false; // Alinear objeto con curva Catmull-Rom
    vector<vector<float>> translationPoints; // Puntos Catmull-Rom
    vector<vector<float>> catmullRomCurvePoints; // Puntos precalculados curva (si es necesario)
};

// Fuente de Luz (Fase 4)
struct Light {
    enum Type { POINT, DIRECTIONAL, SPOTLIGHT };
    Type type = POINT;
    float position[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // w=1 para punto/foco, w=0 para direccional
    float direction[3] = { 0.0f, 0.0f, -1.0f };     // Usado para direccional/foco
    float cutoff = 180.0f;                          // Ángulo de corte para foco (grados)
    // Podrías añadir colores específicos por luz aquí (ambient, diffuse, specular)
    float ambient[4] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Color ambiente de esta luz
    float diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Color difuso de esta luz
    float specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };// Color especular de esta luz
};

// Nodo Grupo (Actualizado Fase 3)
struct Group {
    Transform transform;
    vector<Model> models;
    vector<Group> children;
};

// --- Variables Globales ---
Window window;
Camera camera;
Group rootGroup;
float cameraTheta = M_PI / 4.0f, cameraPhi = M_PI / 4.0f, cameraRadius = 15.0f; // Inicialización cámara orbital

vector<Light> lights;             // Almacena las luces de la escena (Fase 4)
map<string, GLuint> textureCache; // Cache para texturas cargadas (Fase 4)


// --- Prototipos de Funciones ---
// Inicialización y Carga
bool initializeImageLibrary();
bool loadImage(const string& path, GLuint& textureID);
bool readXMLConfig(const string& filename);
void parseCamera(XMLElement* cameraElem);
Group parseGroup(XMLElement* groupElem);
void parseTransform(XMLElement* transformElem, Transform& transform);
bool loadModel(const string& filename, Model& model);
void initModelVBOs(Model& model);

// Renderizado
void setupCamera();
void setupLights();
void drawAxes();
void renderGroup(const Group& group);
void applyTransform(const Transform& transform);
void renderModelWithVBOs(const Model& model);
void renderScene();

// Callbacks y Controles
void keyboardFunc(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void idleFunc();
void reshapeFunc(int w, int h);

// Limpieza
void cleanup();
void cleanupGroupVBOs(Group& group);

// Animación (Fase 3)
vector<float> getCatmullRomPoint(float t, const vector<vector<float>>& p);
vector<float> getCatmullRomDerivative(float t, const vector<vector<float>>& p);
void buildRotMatrix(float* m, float* x, float* y, float* z);
void cross(float* a, float* b, float* res);
void normalize(float* a);


// --- Implementación ---

// Inicialización de Biblioteca de Imágenes (DevIL)
bool initializeImageLibrary() {
#ifdef IL_VERSION_1_6_7 // Comprobar si DevIL está disponible
    ilInit();
    // iluInit(); // Podría ser necesario
    // ilutRenderer(ILUT_OPENGL); // Podría ser necesario para helpers OpenGL
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT); // Origen estándar para texturas OpenGL
    if (DEBUG) cerr << "DevIL inicializado." << endl;
    return true;
#else
    if (DEBUG) cerr << "Advertencia: DevIL no parece estar disponible/incluido. La carga de texturas fallará." << endl;
    return false;
#endif
}

// Carga de Textura (Usando DevIL - ¡Necesita Implementación Completa!)
bool loadImage(const string& path, GLuint& textureID) {
#ifdef IL_VERSION_1_6_7
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    // Intentar cargar la imagen
    if (!ilLoadImage((ILstring)path.c_str())) {
        ILenum error = ilGetError();
        cerr << "Error cargando textura '" << path << "': " << error << /*ilGetString(error) <<*/ endl; // iluErrorString es mejor si usas ILU
        ilDeleteImages(1, &imageID);
        textureID = 0;
        return false;
    }

    // Convertir a RGBA si es necesario (buena práctica)
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
        cerr << "Error convirtiendo textura '" << path << "' a RGBA." << endl;
        ilDeleteImages(1, &imageID);
        textureID = 0;
        return false;
    }

    // Generar textura OpenGL
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configurar parámetros de textura (importante)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtro lineal para magnificación
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Filtro con mipmaps para minificación

    // Cargar datos a OpenGL y generar mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
        0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D); // Generar mipmaps automáticamente

    // Liberar memoria de DevIL y desvincular
    ilDeleteImages(1, &imageID);
    glBindTexture(GL_TEXTURE_2D, 0); // Desvincular

    if (DEBUG) cerr << "Textura cargada y generada: '" << path << "' (ID: " << textureID << ")" << endl;
    return true;

#else
    // Si DevIL no está, siempre falla
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
        parseCamera(cameraElem); // Llama a función separada
    }
    else {
        cerr << "Advertencia: No se encontró configuración de cámara en XML. Usando defaults." << endl;
    }

    // Configuración de Luces (Fase 4)
    XMLElement* lightsElem = world->FirstChildElement("lights");
    if (lightsElem) {
        for (XMLElement* lightElem = lightsElem->FirstChildElement("light"); lightElem; lightElem = lightElem->NextSiblingElement("light")) {
            Light light;
            const char* typeStr = lightElem->Attribute("type");
            if (typeStr) {
                if (strcmp(typeStr, "point") == 0) {
                    light.type = Light::POINT;
                    lightElem->QueryFloatAttribute("posx", &light.position[0]);
                    lightElem->QueryFloatAttribute("posy", &light.position[1]);
                    lightElem->QueryFloatAttribute("posz", &light.position[2]);
                    light.position[3] = 1.0f;
                }
                else if (strcmp(typeStr, "directional") == 0) {
                    light.type = Light::DIRECTIONAL;
                    lightElem->QueryFloatAttribute("dirx", &light.direction[0]);
                    lightElem->QueryFloatAttribute("diry", &light.direction[1]);
                    lightElem->QueryFloatAttribute("dirz", &light.direction[2]);
                    // En OpenGL, la posición de luz direccional define la dirección DESDE la que viene
                    // Ajustamos según la nota: "light direction is assumed to be the direction towards the light" -> invertimos
                    light.position[0] = -light.direction[0];
                    light.position[1] = -light.direction[1];
                    light.position[2] = -light.direction[2];
                    light.position[3] = 0.0f; // w=0 indica direccional
                }
                else if (strcmp(typeStr, "spotlight") == 0) {
                    light.type = Light::SPOTLIGHT;
                    lightElem->QueryFloatAttribute("posx", &light.position[0]);
                    lightElem->QueryFloatAttribute("posy", &light.position[1]);
                    lightElem->QueryFloatAttribute("posz", &light.position[2]);
                    light.position[3] = 1.0f; // w=1 para foco
                    // La dirección del foco SÍ es hacia dónde apunta
                    lightElem->QueryFloatAttribute("dirx", &light.direction[0]);
                    lightElem->QueryFloatAttribute("diry", &light.direction[1]);
                    lightElem->QueryFloatAttribute("dirz", &light.direction[2]);
                    lightElem->QueryFloatAttribute("cutoff", &light.cutoff);
                }
                else {
                    cerr << "Advertencia: Tipo de luz desconocido '" << typeStr << "'. Ignorando." << endl;
                    continue; // Saltar esta luz
                }
                // Aquí podrías añadir lectura de colores por luz si lo defines en el XML
                lights.push_back(light);
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


    // Parsear la jerarquía de grupos (empezando desde el nodo raíz implícito)
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

    // Inicializar cámara orbital basada en la posición leída
    cameraRadius = sqrt(camera.posX * camera.posX + camera.posY * camera.posY + camera.posZ * camera.posZ);
    // Evitar división por cero si la cámara está en el origen
    if (cameraRadius > numeric_limits<float>::epsilon()) {
        // Calcular theta (ángulo azimutal en plano XZ)
        cameraTheta = atan2(camera.posZ, camera.posX);
        // Calcular phi (ángulo polar desde eje Y+)
        cameraPhi = acos(camera.posY / cameraRadius); // acos da [0, PI]
    }
    else {
        // Si está en el origen, usar valores por defecto o los últimos válidos
        cameraRadius = 15.0f; // Valor por defecto
        cameraTheta = M_PI / 4.0f;
        cameraPhi = M_PI / 4.0f;
    }
    if (DEBUG) cerr << "Cámara inicializada: R=" << cameraRadius << ", Theta=" << cameraTheta << ", Phi=" << cameraPhi << endl;
}

// Parsear Transformaciones (función auxiliar)
void parseTransform(XMLElement* transformElem, Transform& transform) {
    if (!transformElem) return;

    // Traslación (Estática o Dinámica)
    XMLElement* translate = transformElem->FirstChildElement("translate");
    if (translate) {
        if (translate->Attribute("time")) {
            transform.isTimedTranslation = true;
            translate->QueryFloatAttribute("time", &transform.translationTime);
            transform.alignWithPath = translate->BoolAttribute("align", false); // Default a false si no está o es inválido
            transform.translationPoints.clear(); // Limpiar puntos anteriores
            for (XMLElement* point = translate->FirstChildElement("point"); point; point = point->NextSiblingElement("point")) {
                vector<float> p(3);
                point->QueryFloatAttribute("x", &p[0]);
                point->QueryFloatAttribute("y", &p[1]);
                point->QueryFloatAttribute("z", &p[2]);
                transform.translationPoints.push_back(p);
            }
            if (transform.translationPoints.size() < 4) {
                cerr << "Advertencia: Traslación Catmull-Rom necesita al menos 4 puntos." << endl;
                transform.isTimedTranslation = false; // Desactivar si no hay suficientes puntos
            }
        }
        else {
            transform.isTimedTranslation = false;
            translate->QueryFloatAttribute("x", &transform.translate[0]);
            translate->QueryFloatAttribute("y", &transform.translate[1]);
            translate->QueryFloatAttribute("z", &transform.translate[2]);
        }
    }

    // Rotación (Estática o Dinámica)
    XMLElement* rotate = transformElem->FirstChildElement("rotate");
    if (rotate) {
        if (rotate->Attribute("time")) {
            transform.isTimedRotation = true;
            rotate->QueryFloatAttribute("time", &transform.rotationTime);
            // El ángulo se ignora en rotación temporal, pero el eje se usa
        }
        else {
            transform.isTimedRotation = false;
            rotate->QueryFloatAttribute("angle", &transform.rotate[0]);
        }
        // El eje siempre se lee
        rotate->QueryFloatAttribute("x", &transform.rotate[1]);
        rotate->QueryFloatAttribute("y", &transform.rotate[2]);
        rotate->QueryFloatAttribute("z", &transform.rotate[3]);
    }

    // Escalado (Siempre estático)
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
    if (!groupElem) return group; // Devuelve grupo vacío si el elemento es nulo

    // 1. Parsear Transformaciones del Grupo Actual
    parseTransform(groupElem->FirstChildElement("transform"), group.transform);

    // 2. Parsear Modelos del Grupo Actual
    XMLElement* modelsElem = groupElem->FirstChildElement("models");
    if (modelsElem) {
        for (XMLElement* modelElem = modelsElem->FirstChildElement("model"); modelElem; modelElem = modelElem->NextSiblingElement("model")) {
            const char* file = modelElem->Attribute("file");
            if (file) {
                Model model;
                if (loadModel(file, model)) { // Carga vértices, normales, texCoords
                    // --- Leer Textura y Material (Fase 4) ---
                    XMLElement* textureElem = modelElem->FirstChildElement("texture");
                    if (textureElem && textureElem->Attribute("file")) {
                        model.textureFile = textureElem->Attribute("file");
                        // Intentar cargar desde caché o cargar nueva
                        if (textureCache.count(model.textureFile)) {
                            model.textureID = textureCache[model.textureFile];
                            if (DEBUG) cerr << "Textura '" << model.textureFile << "' encontrada en caché (ID: " << model.textureID << ")" << endl;
                        }
                        else {
                            if (loadImage(model.textureFile, model.textureID)) {
                                textureCache[model.textureFile] = model.textureID; // Añadir a caché si carga ok
                            }
                            else {
                                // loadImage ya imprime error, textureID será 0
                            }
                        }
                    }

                    // Leer Color/Material
                    XMLElement* colorElem = modelElem->FirstChildElement("color");
                    if (colorElem) {
                        auto parseColorComponent = [&](const char* elemName, float* target) {
                            XMLElement* compElem = colorElem->FirstChildElement(elemName);
                            if (compElem) {
                                int R = 0, G = 0, B = 0; // Inicializar a 0
                                compElem->QueryIntAttribute("R", &R);
                                compElem->QueryIntAttribute("G", &G);
                                compElem->QueryIntAttribute("B", &B);
                                target[0] = R / 255.0f;
                                target[1] = G / 255.0f;
                                target[2] = B / 255.0f;
                                target[3] = 1.0f; // Alpha siempre 1 por ahora
                            }
                            else {
                                // Si no se especifica, usa los defaults de la struct Material
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
                    // --- Fin Lectura Textura y Material ---

                    group.models.push_back(model); // Añadir modelo al grupo
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

    // 3. Parsear Grupos Hijos (Recursivo)
    for (XMLElement* childGroupElem = groupElem->FirstChildElement("group"); childGroupElem; childGroupElem = childGroupElem->NextSiblingElement("group")) {
        group.children.push_back(parseGroup(childGroupElem));
    }

    return group;
}

// Carga de Modelo desde archivo .3d (Actualizado Fase 4)
bool loadModel(const string& filename, Model& model) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error abriendo archivo de modelo: " << filename << endl;
        return false;
    }

    // Leer Vértices
    int numVertices = 0;
    file >> numVertices;
    if (numVertices <= 0) { cerr << "Error: Número de vértices inválido (" << numVertices << ") en " << filename << endl; file.close(); return false; }
    model.vertices.resize(numVertices * 3);
    for (int i = 0; i < numVertices * 3; ++i) {
        if (!(file >> model.vertices[i])) { cerr << "Error leyendo vértice " << i / 3 << " en " << filename << endl; file.close(); return false; }
    }

    // Leer Normales
    int numNormals = 0;
    file >> numNormals;
    if (numNormals != numVertices) { cerr << "Advertencia: Número de normales (" << numNormals << ") no coincide con vértices (" << numVertices << ") en " << filename << endl; }
    model.normals.resize(numNormals * 3);
    for (int i = 0; i < numNormals * 3; ++i) {
        if (!(file >> model.normals[i])) { cerr << "Error leyendo normal " << i / 3 << " en " << filename << endl; file.close(); return false; }
    }

    // Leer Coordenadas de Textura
    int numTexCoords = 0;
    file >> numTexCoords;
    if (numTexCoords != numVertices) { cerr << "Advertencia: Número de coords. textura (" << numTexCoords << ") no coincide con vértices (" << numVertices << ") en " << filename << endl; }
    model.texCoords.resize(numTexCoords * 2); // 2 componentes (u, v)
    for (int i = 0; i < numTexCoords * 2; ++i) {
        if (!(file >> model.texCoords[i])) { cerr << "Error leyendo coord. textura " << i / 2 << " en " << filename << endl; file.close(); return false; }
    }

    // Leer Caras (Índices)
    int numFaces = 0;
    file >> numFaces;
    if (numFaces <= 0) { cerr << "Error: Número de caras inválido (" << numFaces << ") en " << filename << endl; file.close(); return false; }
    model.faces.resize(numFaces * 3);
    for (int i = 0; i < numFaces * 3; ++i) {
        if (!(file >> model.faces[i])) { cerr << "Error leyendo índice de cara " << i / 3 << ", componente " << i % 3 << " en " << filename << endl; file.close(); return false; }
    }

    file.close();

    // Inicializar VBOs para este modelo
    initModelVBOs(model);

    if (DEBUG) cerr << "Modelo cargado: '" << filename << "' (V: " << numVertices << ", N: " << numNormals << ", T: " << numTexCoords << ", F: " << numFaces << ")" << endl;
    return true;
}

// Inicialización de VBOs (Actualizado Fase 4)
void initModelVBOs(Model& model) {
    if (model.buffersInitialized) return; // Ya inicializados
    if (model.vertices.empty() || model.faces.empty()) {
        cerr << "Error VBO: Modelo sin vértices o caras." << endl;
        return; // No crear VBOs si no hay datos
    }

    // 1. Buffer de Vértices
    glGenBuffers(1, &model.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(float), model.vertices.data(), GL_STATIC_DRAW);

    // 2. Buffer de Normales (si existen)
    if (!model.normals.empty()) {
        glGenBuffers(1, &model.normalBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, model.normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(float), model.normals.data(), GL_STATIC_DRAW);
    }
    else {
        model.normalBuffer = 0; // Marcar como inexistente
    }

    // 3. Buffer de Coordenadas de Textura (si existen)
    if (!model.texCoords.empty()) {
        glGenBuffers(1, &model.texCoordBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, model.texCoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, model.texCoords.size() * sizeof(float), model.texCoords.data(), GL_STATIC_DRAW);
    }
    else {
        model.texCoordBuffer = 0; // Marcar como inexistente
    }

    // 4. Buffer de Índices (Caras)
    glGenBuffers(1, &model.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.faces.size() * sizeof(unsigned int), model.faces.data(), GL_STATIC_DRAW);

    // Desvincular buffers (buena práctica)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    model.buffersInitialized = true;
    if (DEBUG) cerr << "VBOs inicializados para modelo (V:" << model.vertexBuffer << ", N:" << model.normalBuffer << ", T:" << model.texCoordBuffer << ", I:" << model.indexBuffer << ")" << endl;
}


// Configuración de Cámara para Renderizado
void setupCamera() {
    // Actualizar posición cartesiana desde coordenadas esféricas (orbital)
    // Asegurar que phi no llegue exactamente a 0 o PI para evitar problemas con 'up' vector
    float minPhi = 0.01f;
    float maxPhi = M_PI - 0.01f;
    if (cameraPhi < minPhi) cameraPhi = minPhi;
    if (cameraPhi > maxPhi) cameraPhi = maxPhi;

    // Conversión esférica a cartesiana (Y es arriba)
    camera.posX = cameraRadius * sin(cameraPhi) * cos(cameraTheta);
    camera.posY = cameraRadius * cos(cameraPhi);
    camera.posZ = cameraRadius * sin(cameraPhi) * sin(cameraTheta);

    // Configurar Proyección
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.fov, (float)window.width / window.height, camera.near, camera.far);

    // Configurar Vista (Modelo-Vista)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        camera.posX + camera.lookAtX, camera.posY + camera.lookAtY, camera.posZ + camera.lookAtZ, // Posición relativa al punto de mira
        camera.lookAtX, camera.lookAtY, camera.lookAtZ,          // Punto al que se mira
        camera.upX, camera.upY, camera.upZ                       // Vector 'arriba'
    );
}

// Configuración de Luces para Renderizado (Fase 4)
void setupLights() {
    glEnable(GL_LIGHTING); // Habilitar iluminación general

    // Configurar luz ambiente global (según nota)
    float global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Ambiente global bajo por defecto
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient); // Usar este o el de la nota si quieres ambiente blanco

    // Configurar cada luz definida en el XML
    int maxLights = 8; // GL_LIGHT0 a GL_LIGHT7
    for (int i = 0; i < lights.size() && i < maxLights; ++i) {
        GLenum lightID = GL_LIGHT0 + i;
        glEnable(lightID); // Habilitar esta fuente de luz

        // Establecer propiedades comunes (posición/dirección ya incluye 'w')
        glLightfv(lightID, GL_POSITION, lights[i].position);
        // Establecer colores específicos de la luz (podrían leerse del XML)
        glLightfv(lightID, GL_AMBIENT, lights[i].ambient);
        glLightfv(lightID, GL_DIFFUSE, lights[i].diffuse);
        glLightfv(lightID, GL_SPECULAR, lights[i].specular);

        // Propiedades específicas de Foco (Spotlight)
        if (lights[i].type == Light::SPOTLIGHT) {
            glLightfv(lightID, GL_SPOT_DIRECTION, lights[i].direction);
            glLightf(lightID, GL_SPOT_CUTOFF, lights[i].cutoff);
            glLightf(lightID, GL_SPOT_EXPONENT, 0.0f); // Exponente bajo (distribución uniforme dentro del cono)
            // Podrías añadir atenuación aquí: glLightf(lightID, GL_CONSTANT_ATTENUATION, 1.0f); etc.
        }
        else {
            // Asegurarse de que no actúe como foco si no lo es
            glLightf(lightID, GL_SPOT_CUTOFF, 180.0f); // Ángulo completo
        }
    }

    // Deshabilitar luces no usadas
    for (int i = lights.size(); i < maxLights; ++i) {
        glDisable(GL_LIGHT0 + i);
    }
}


// Dibujar Ejes Coordenados
void drawAxes() {
    glDisable(GL_LIGHTING); // Dibujar sin iluminación
    glLineWidth(2.0f); // Líneas un poco más gruesas
    glBegin(GL_LINES);
    // Eje X (Rojo)
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(-100.0, 0.0, 0.0);
    glVertex3f(100.0, 0.0, 0.0);
    // Eje Y (Verde)
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, -100.0, 0.0);
    glVertex3f(0.0, 100.0, 0.0);
    // Eje Z (Azul)
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, -100.0);
    glVertex3f(0.0, 0.0, 100.0);
    glEnd();
    glLineWidth(1.0f); // Restaurar grosor
    // No re-habilitar iluminación aquí, se hace en renderScene
}


// Aplicar Transformaciones (Estáticas y Dinámicas)
void applyTransform(const Transform& transform) {
    // 1. Traslación
    if (transform.isTimedTranslation && !transform.translationPoints.empty()) {
        // Animación Catmull-Rom
        float timeElapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Tiempo en segundos
        float totalTime = transform.translationTime > 0 ? transform.translationTime : 1.0f; // Evitar división por 0
        float t_global = fmod(timeElapsed / totalTime, 1.0f); // Parámetro global [0, 1)

        int numSegments = transform.translationPoints.size() - 3;
        if (numSegments < 1) return; // No se puede calcular

        float t_segment = t_global * numSegments; // Mapear t global al segmento actual
        int currentSegment = floor(t_segment);
        float t_local = t_segment - currentSegment; // Parámetro local [0, 1) dentro del segmento

        // Obtener los 4 puntos de control para el segmento actual
        vector<vector<float>> controlPoints(4);
        for (int i = 0; i < 4; ++i) controlPoints[i] = transform.translationPoints[currentSegment + i];

        // Calcular posición y derivada en la curva
        vector<float> pos = getCatmullRomPoint(t_local, controlPoints);
        glTranslatef(pos[0], pos[1], pos[2]); // Aplicar traslación

        // Alinear con la curva si se especificó
        if (transform.alignWithPath) {
            vector<float> deriv = getCatmullRomDerivative(t_local, controlPoints);
            normalize(deriv.data()); // Normalizar vector tangente (X)

            // Calcular nuevos ejes Y y Z
            float up[3] = { 0, 1, 0 }; // Vector 'up' global inicial
            float left[3]; // Vector 'left' (Z')
            cross(deriv.data(), up, left); // Z' = X' x Y
            normalize(left);

            float newUp[3]; // Nuevo vector 'up' (Y')
            cross(left, deriv.data(), newUp); // Y' = Z' x X'
            normalize(newUp);

            // Construir matriz de rotación y aplicarla
            float rotMatrix[16];
            buildRotMatrix(rotMatrix, deriv.data(), newUp, left);
            glMultMatrixf(rotMatrix);
        }

    }
    else {
        // Traslación estática
        glTranslatef(transform.translate[0], transform.translate[1], transform.translate[2]);
    }

    // 2. Rotación
    if (transform.isTimedRotation) {
        // Rotación animada
        float timeElapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        float totalTime = transform.rotationTime > 0 ? transform.rotationTime : 1.0f;
        float currentAngle = fmod((timeElapsed / totalTime) * 360.0f, 360.0f);
        glRotatef(currentAngle, transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    }
    else {
        // Rotación estática
        glRotatef(transform.rotate[0], transform.rotate[1], transform.rotate[2], transform.rotate[3]);
    }

    // 3. Escalado
    glScalef(transform.scale[0], transform.scale[1], transform.scale[2]);
}


// Renderizar un Grupo (Recursivo)
void renderGroup(const Group& group) {
    glPushMatrix(); // Guardar estado de transformación actual

    // Aplicar transformaciones de este grupo
    applyTransform(group.transform);

    // Renderizar modelos de este grupo
    for (const auto& model : group.models) {
        renderModelWithVBOs(model);
    }

    // Renderizar grupos hijos (recursivo)
    for (const auto& child : group.children) {
        renderGroup(child);
    }

    glPopMatrix(); // Restaurar estado de transformación anterior
}

// Renderizar un Modelo usando VBOs (Actualizado Fase 4)
void renderModelWithVBOs(const Model& model) {
    if (!model.buffersInitialized || model.vertexBuffer == 0 || model.indexBuffer == 0) {
        // No renderizar si los VBOs esenciales no están listos
        return;
    }

    // 1. Aplicar Material del modelo
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, model.material.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, model.material.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, model.material.specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, model.material.emissive);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, model.material.shininess);


    // 2. Aplicar Textura (si existe)
    bool hasTexture = (model.textureID != 0 && model.texCoordBuffer != 0);
    if (hasTexture) {
        glEnable(GL_TEXTURE_2D); // Habilitar texturizado 2D
        glBindTexture(GL_TEXTURE_2D, model.textureID); // Vincular la textura correcta
        glEnableClientState(GL_TEXTURE_COORD_ARRAY); // Habilitar array de coords. textura
        glBindBuffer(GL_ARRAY_BUFFER, model.texCoordBuffer); // Vincular VBO de coords. textura
        glTexCoordPointer(2, GL_FLOAT, 0, 0); // Especificar formato (2 floats por coord.)
    }
    else {
        glDisable(GL_TEXTURE_2D); // Deshabilitar si no hay textura para este modelo
    }

    // 3. Vincular VBOs y habilitar arrays de cliente
    // Vértices
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0); // 3 floats por vértice

    // Normales (si existen)
    if (model.normalBuffer != 0) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, model.normalBuffer);
        glNormalPointer(GL_FLOAT, 0, 0); // 3 floats por normal
    }
    else {
        glDisableClientState(GL_NORMAL_ARRAY); // Deshabilitar si no hay normales
    }

    // 4. Vincular Buffer de Índices y Dibujar
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer);
    glDrawElements(GL_TRIANGLES,         // Primitiva a dibujar
        model.faces.size(),   // Número de índices a usar
        GL_UNSIGNED_INT,      // Tipo de los índices
        0);                   // Offset en el buffer de índices

    // 5. Limpieza: Deshabilitar arrays y desvincular buffers/texturas
    glDisableClientState(GL_VERTEX_ARRAY);
    if (model.normalBuffer != 0) {
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    if (hasTexture) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glBindTexture(GL_TEXTURE_2D, 0); // Desvincular textura
        glDisable(GL_TEXTURE_2D);
    }

    // Desvincular buffers (opcional pero buena práctica)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


// Función Principal de Renderizado
void renderScene() {
    // Limpiar buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configurar cámara
    setupCamera();

    // Configurar luces
    setupLights();

    // Dibujar ejes (sin iluminación)
    drawAxes();

    // Habilitar iluminación para la escena principal
    glEnable(GL_LIGHTING);
    // Establecer modo de polígono (relleno por defecto)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Cambiar a GL_LINE para modo alámbrico

    // Renderizar la jerarquía de grupos
    renderGroup(rootGroup);

    // Intercambiar buffers (mostrar lo dibujado)
    glutSwapBuffers();
}


// --- Callbacks y Controles ---

void keyboardFunc(unsigned char key, int x, int y) {
    float moveStep = 0.5f;
    float angleStep = 0.1f;
    switch (key) {
        // Controles de cámara orbital básicos
    case 'a': cameraTheta -= angleStep; break;
    case 'd': cameraTheta += angleStep; break;
    case 'w': cameraPhi -= angleStep; break; // Mover arriba (disminuir phi)
    case 's': cameraPhi += angleStep; break; // Mover abajo (aumentar phi)
    case 'q': cameraRadius -= moveStep; if (cameraRadius < 0.1f) cameraRadius = 0.1f; break; // Acercar
    case 'e': cameraRadius += moveStep; break; // Alejar

    case 27: // Tecla ESC
        cleanup(); // Limpiar recursos antes de salir
        exit(0);
        break;
    }
    glutPostRedisplay(); // Solicitar redibujado
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

// Función Idle para animación continua
void idleFunc() {
    // Forzar redibujado constante para animaciones basadas en tiempo
    glutPostRedisplay();
}

// Callback de Redimensionamiento de Ventana
void reshapeFunc(int w, int h) {
    if (h == 0) h = 1; // Prevenir división por cero
    window.width = w;
    window.height = h;
    glViewport(0, 0, w, h);
    // La proyección se recalcula en setupCamera() antes de cada frame
    glutPostRedisplay(); // Solicitar redibujado con nuevas dimensiones
}


// --- Limpieza ---

// Limpiar VBOs y Texturas recursivamente
void cleanupGroupVBOs(Group& group) {
    for (auto& model : group.models) {
        if (model.buffersInitialized) {
            // Borrar VBOs
            if (model.vertexBuffer != 0) glDeleteBuffers(1, &model.vertexBuffer);
            if (model.normalBuffer != 0) glDeleteBuffers(1, &model.normalBuffer);
            if (model.texCoordBuffer != 0) glDeleteBuffers(1, &model.texCoordBuffer);
            if (model.indexBuffer != 0) glDeleteBuffers(1, &model.indexBuffer);
            // Marcar como no inicializados
            model.buffersInitialized = false;
            model.vertexBuffer = model.normalBuffer = model.texCoordBuffer = model.indexBuffer = 0;
        }
        // Textura no se borra aquí, se borra del caché global
    }
    for (auto& child : group.children) {
        cleanupGroupVBOs(child);
    }
}

// Limpieza global de recursos
void cleanup() {
    if (DEBUG) std::cerr << "Iniciando limpieza..." << std::endl; // Usar std:: explícitamente puede ayudar

    // Limpiar VBOs de todos los modelos
    cleanupGroupVBOs(rootGroup);

    // Limpiar caché de texturas
    // Bucle estilo C++11/14 (sin structured bindings)
    for (auto const& pair : textureCache) { // Iterar sobre los pares (key, value)
        const std::string& key = pair.first;  // Acceder a la clave (asumiendo que es string)
        GLuint val = pair.second;            // Acceder al valor (asumiendo que es GLuint)

        if (val != 0) {
            glDeleteTextures(1, &val);
            // Usar std:: explícitamente aquí también por si acaso
            if (DEBUG) std::cerr << "Textura borrada: ID " << val << " (Archivo: " << key << ")" << std::endl;
        }
    }
    textureCache.clear();

    // Si inicializaste DevIL u otra biblioteca, podrías cerrarla aquí
    // if (ilIsInitialized()) ilShutdown(); // Ejemplo DevIL

    if (DEBUG) std::cerr << "Limpieza completada." << std::endl;
}


// --- Funciones de Animación (Fase 3) ---

// Catmull-Rom Point Calculation
vector<float> getCatmullRomPoint(float t, const vector<vector<float>>& p) {
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f},
                      { 1.0f, -2.5f,  2.0f, -0.5f},
                      {-0.5f,  0.0f,  0.5f,  0.0f},
                      { 0.0f,  1.0f,  0.0f,  0.0f} };
    vector<float> point(3, 0.0f);
    float T[4] = { t * t * t, t * t, t, 1 };

    for (int i = 0; i < 3; ++i) { // x, y, z
        float C[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Coeficientes A,B,C,D para este componente
        for (int j = 0; j < 4; ++j) { // Calcular A,B,C,D
            C[j] = m[j][0] * p[0][i] + m[j][1] * p[1][i] + m[j][2] * p[2][i] + m[j][3] * p[3][i];
        }
        point[i] = T[0] * C[0] + T[1] * C[1] + T[2] * C[2] + T[3] * C[3];
    }
    return point;
}

// Catmull-Rom Derivative Calculation
vector<float> getCatmullRomDerivative(float t, const vector<vector<float>>& p) {
    // Derivada de T = [3t^2, 2t, 1, 0]
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f},
                      { 1.0f, -2.5f,  2.0f, -0.5f},
                      {-0.5f,  0.0f,  0.5f,  0.0f},
                      { 0.0f,  1.0f,  0.0f,  0.0f} };
    vector<float> deriv(3, 0.0f);
    float T_deriv[4] = { 3 * t * t, 2 * t, 1, 0 };

    for (int i = 0; i < 3; ++i) { // x, y, z
        float C[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Coeficientes A,B,C,D para este componente
        for (int j = 0; j < 4; ++j) { // Calcular A,B,C,D
            C[j] = m[j][0] * p[0][i] + m[j][1] * p[1][i] + m[j][2] * p[2][i] + m[j][3] * p[3][i];
        }
        deriv[i] = T_deriv[0] * C[0] + T_deriv[1] * C[1] + T_deriv[2] * C[2] + T_deriv[3] * C[3];
    }
    return deriv;
}

// Helper: Producto vectorial
void cross(float* a, float* b, float* res) {
    res[0] = a[1] * b[2] - a[2] * b[1];
    res[1] = a[2] * b[0] - a[0] * b[2];
    res[2] = a[0] * b[1] - a[1] * b[0];
}

// Helper: Normalizar vector
void normalize(float* a) {
    float l = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    if (l > numeric_limits<float>::epsilon()) {
        a[0] = a[0] / l;
        a[1] = a[1] / l;
        a[2] = a[2] / l;
    }
}

// Helper: Construir matriz de rotación a partir de ejes ortonormales
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

    // Inicializar GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // RGBA para texturas con alpha
    // Tamaño inicial, se ajustará si el XML lo especifica
    glutInitWindowSize(window.width, window.height);
    glutCreateWindow("CG Engine - Fase 4");

    // Registrar Callbacks GLUT
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshapeFunc); // Registrar callback de redimensionamiento
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idleFunc);       // Para animación continua
    // Registrar función de limpieza al salir
    atexit(cleanup);

    // Inicializar GLEW (después de crear ventana GLUT)
#ifdef _WIN32
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        cerr << "Error inicializando GLEW: " << glewGetErrorString(glewErr) << endl;
        return 1;
    }
    if (!GLEW_VERSION_2_0) { // Verificar versión mínima si necesitas shaders más adelante
        cerr << "OpenGL 2.0 no soportado." << endl;
        // return 1; // O continuar si no usas shaders aún
    }
#endif

    // Inicializar Biblioteca de Imágenes (¡IMPORTANTE!)
    if (!initializeImageLibrary()) {
        cerr << "Fallo al inicializar la biblioteca de imágenes. Las texturas no funcionarán." << endl;
        // Puedes decidir si continuar o salir
        // return 1;
    }

    // Cargar Configuración desde XML
    if (!readXMLConfig(configFile)) {
        cerr << "Fallo al cargar el archivo de configuración: " << configFile << endl;
        return 1;
    }

    // Ajustar tamaño de ventana si se leyó del XML
    glutReshapeWindow(window.width, window.height);


    // --- Configuración Inicial de OpenGL ---
    glEnable(GL_DEPTH_TEST); // Habilitar test de profundidad
    glEnable(GL_CULL_FACE);  // Opcional: Habilitar culling (ocultar caras traseras)
    glFrontFace(GL_CCW);     // Definir orden de vértices para caras frontales (Counter-Clockwise)
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Color de fondo oscuro

    // Habilitar normalización/reescalado de normales (¡Importante para escalas!)
    glEnable(GL_RESCALE_NORMAL); // Más eficiente que GL_NORMALIZE si las escalas son uniformes [cite: 2]

    // Configurar luz ambiente global (si quieres que el ambiente del material funcione sin luz ambiente por fuente)
    // float global_ambient[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Según nota [cite: 5]
    // glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    // O usar un valor más bajo si prefieres que la luz ambiente venga de las fuentes
    float low_global_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, low_global_ambient);


    // NO habilitar GL_COLOR_MATERIAL si usas glMaterialfv
    // glDisable(GL_COLOR_MATERIAL); // Asegurarse de que esté deshabilitado

    // Habilitar texturizado 2D globalmente puede ser conveniente
    // glEnable(GL_TEXTURE_2D); // O manejarlo dentro de renderModelWithVBOs

    // --- Iniciar Bucle Principal GLUT ---
    if (DEBUG) cerr << "Iniciando bucle principal de GLUT..." << endl;
    glutMainLoop();

    return 0; // Aunque nunca debería llegar aquí porque glutMainLoop no retorna
}