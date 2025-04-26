#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm> // Add this for replace function
#include <sstream>   // Add this for istringstream

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// Estructura para representar un vértice
struct Vertex {
    float x, y, z;
};

// Función para generar un plano
void generatePlane(float size, int divisions, const string& filename) {
    ofstream file(filename); // Abrir el archivo de salida

    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        return;
    }

    // Calcular el número de vértices
    int numVertices = (divisions + 1) * (divisions + 1);
    file << numVertices << endl; // Escribir el número de vértices

    // Generar los vértices
    for (int i = 0; i <= divisions; i++) {
        for (int j = 0; j <= divisions; j++) {
            float x = -size / 2 + i * (size / divisions); // Coordenada X
            float z = -size / 2 + j * (size / divisions); // Coordenada Z
            file << x << " 0 " << z << endl; // Coordenada Y es 0 (plano XZ)
        }
    }

    // Calcular el número de caras (triángulos)
    int numFaces = divisions * divisions * 2;
    file << numFaces << endl; // Escribir el número de caras

    // Generar las caras (triángulos)
    for (int i = 0; i < divisions; i++) {
        for (int j = 0; j < divisions; j++) {
            int v1 = i * (divisions + 1) + j; // Vértice superior izquierdo
            int v2 = v1 + 1;                  // Vértice superior derecho
            int v3 = v1 + divisions + 1;      // Vértice inferior izquierdo
            int v4 = v2 + divisions + 1;      // Vértice inferior derecho

            // Primer triángulo (v1, v2, v3)
            file << v1 << " " << v2 << " " << v3 << endl;

            // Segundo triángulo (v2, v4, v3)
            file << v2 << " " << v4 << " " << v3 << endl;
        }
    }

    file.close(); // Cerrar el archivo
    cout << "Archivo generado: " << filename << endl;
}

// Función para generar una caja
void generateBox(float size, int divisions, const string& filename) {
    ofstream file(filename);

    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        return;
    }

    // Calcular el número de vértices
    int numVertices = (divisions + 1) * (divisions + 1) * 6; // 6 caras
    file << numVertices << endl;

    // Generar los vértices para cada cara
    for (int face = 0; face < 6; face++) {
        for (int i = 0; i <= divisions; i++) {
            for (int j = 0; j <= divisions; j++) {
                float x, y, z;
                switch (face) {
                    case 0: // Cara frontal (Z = size/2)
                        x = -size / 2 + i * (size / divisions);
                        y = -size / 2 + j * (size / divisions);
                        z = size / 2;
                        break;
                    case 1: // Cara trasera (Z = -size/2)
                        x = -size / 2 + i * (size / divisions);
                        y = -size / 2 + j * (size / divisions);
                        z = -size / 2;
                        break;
                    case 2: // Cara derecha (X = size/2)
                        x = size / 2;
                        y = -size / 2 + i * (size / divisions);
                        z = -size / 2 + j * (size / divisions);
                        break;
                    case 3: // Cara izquierda (X = -size/2)
                        x = -size / 2;
                        y = -size / 2 + i * (size / divisions);
                        z = -size / 2 + j * (size / divisions);
                        break;
                    case 4: // Cara superior (Y = size/2)
                        x = -size / 2 + i * (size / divisions);
                        y = size / 2;
                        z = -size / 2 + j * (size / divisions);
                        break;
                    case 5: // Cara inferior (Y = -size/2)
                        x = -size / 2 + i * (size / divisions);
                        y = -size / 2;
                        z = -size / 2 + j * (size / divisions);
                        break;
                }
                file << x << " " << y << " " << z << endl;
            }
        }
    }

    // Calcular el número de caras (triángulos)
    int numFaces = divisions * divisions * 12; // 6 caras * 2 triángulos por cuadrado
    file << numFaces << endl;

    // Generar las caras (triángulos)
    for (int face = 0; face < 6; face++) {
        int offset = face * (divisions + 1) * (divisions + 1);
        for (int i = 0; i < divisions; i++) {
            for (int j = 0; j < divisions; j++) {
                int v1 = offset + i * (divisions + 1) + j;
                int v2 = v1 + 1;
                int v3 = v1 + divisions + 1;
                int v4 = v2 + divisions + 1;

                // Primer triángulo (v1, v2, v3)
                file << v1 << " " << v2 << " " << v3 << endl;

                // Segundo triángulo (v2, v4, v3)
                file << v2 << " " << v4 << " " << v3 << endl;
            }
        }
    }

    file.close();
    cout << "Archivo generado: " << filename << endl;
}

// Función para generar una esfera
void generateSphere(float radius, int slices, int stacks, const string& filename) {
    ofstream file(filename);

    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        return;
    }

    // Calcular el número de vértices
    int numVertices = (stacks + 1) * (slices + 1);
    file << numVertices << endl;

    // Generar los vértices
    for (int i = 0; i <= stacks; i++) {
        float phi = M_PI * i / stacks; // Ángulo vertical
        for (int j = 0; j <= slices; j++) {
            float theta = 2 * M_PI * j / slices; // Ángulo horizontal
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            file << x << " " << y << " " << z << endl;
        }
    }

    // Calcular el número de caras (triángulos)
    int numFaces = stacks * slices * 2;
    file << numFaces << endl;

    // Generar las caras (triángulos)
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int v1 = i * (slices + 1) + j;
            int v2 = v1 + 1;
            int v3 = v1 + slices + 1;
            int v4 = v2 + slices + 1;

            // Primer triángulo (v1, v2, v3)
            file << v1 << " " << v2 << " " << v3 << endl;

            // Segundo triángulo (v2, v4, v3)
            file << v2 << " " << v4 << " " << v3 << endl;
        }
    }

    file.close();
    cout << "Archivo generado: " << filename << endl;
}

// Función para generar un cono
void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
    ofstream file(filename);

    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        return;
    }

    // Calcular el número de vértices
    int numVertices = (stacks + 1) * (slices + 1) + 1; // +1 para el vértice superior
    file << numVertices << endl;

    // Generar los vértices del cono (base en y=0, vértice en y=height)
    for (int i = 0; i <= stacks; i++) {
        float y = i * (height / stacks); // Base en y=0, vértice en y=height
        float currentRadius = radius * (1 - (float)i / stacks);
        for (int j = 0; j <= slices; j++) {
            float theta = 2 * M_PI * j / slices;
            float x = currentRadius * cos(theta);
            float z = currentRadius * sin(theta);
            file << x << " " << y << " " << z << endl;
        }
    }

    // Vértice superior
    file << "0 " << height << " 0" << endl;

    // Calcular el número de caras (triángulos)
    int numFaces = stacks * slices * 2 + slices; // +slices para los triángulos de la base
    file << numFaces << endl;

    // Generar las caras (triángulos)
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int v1 = i * (slices + 1) + j;
            int v2 = v1 + 1;
            int v3 = v1 + slices + 1;
            int v4 = v2 + slices + 1;

            // Primer triángulo (v1, v2, v3)
            file << v1 << " " << v2 << " " << v3 << endl;

            // Segundo triángulo (v2, v4, v3)
            file << v2 << " " << v4 << " " << v3 << endl;
        }
    }

    // Triángulos de la base
    int baseVertex = numVertices - 1; // Vértice superior
    for (int j = 0; j < slices; j++) {
        int v1 = j;
        int v2 = (j + 1) % slices;
        file << v1 << " " << v2 << " " << baseVertex << endl;
    }

    file.close();
    cout << "Archivo generado: " << filename << endl;
}

//Bezier//

// Add these helper functions near the top of the file, after the Vertex struct
Vertex newVertex(float x, float y, float z) {
    Vertex v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

float getX(const Vertex& v) { return v.x; }
float getY(const Vertex& v) { return v.y; }
float getZ(const Vertex& v) { return v.z; }

Vertex bezier(float u, float v, const vector<Vertex>& controlPoints, const vector<int>& patchIndices) {
    Vertex result = { 0, 0, 0 };

    // Bernstein basis functions for u and v
    float bu[4], bv[4];
    float u1 = 1 - u;
    float v1 = 1 - v;

    bu[0] = u1 * u1 * u1;
    bu[1] = 3 * u * u1 * u1;
    bu[2] = 3 * u * u * u1;
    bu[3] = u * u * u;

    bv[0] = v1 * v1 * v1;
    bv[1] = 3 * v * v1 * v1;
    bv[2] = 3 * v * v * v1;
    bv[3] = v * v * v;

    // Calculate the point on the Bezier surface
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int index = patchIndices[i * 4 + j];
            const Vertex& cp = controlPoints[index];

            float basis = bu[i] * bv[j];
            result.x += cp.x * basis;
            result.y += cp.y * basis;
            result.z += cp.z * basis;
        }
    }

    return result;
}

void generateBezierSurface(const string& patchFilePath, const string& outputFileName, int tessellation) {
    ifstream patchFile(patchFilePath);
    if (!patchFile.is_open()) {
        cerr << "Error al abrir el archivo de entrada: " << patchFilePath << endl;
        return;
    }

    ofstream outputFile(outputFileName);
    if (!outputFile.is_open()) {
        cerr << "Error al abrir el archivo de salida: " << outputFileName << endl;
        return;
    }

    // Read patch file
    string line;
    getline(patchFile, line);
    int numPatches = stoi(line);

    vector<vector<int>> patches(numPatches);
    for (int i = 0; i < numPatches; ++i) {
        getline(patchFile, line);
        replace(line.begin(), line.end(), ',', ' ');
        istringstream iss(line);
        vector<int> indices(16);
        for (int j = 0; j < 16; ++j) {
            iss >> indices[j];
        }
        patches[i] = indices;
    }

    getline(patchFile, line);
    int numControlPoints = stoi(line);

    vector<Vertex> controlPoints(numControlPoints);
    for (int i = 0; i < numControlPoints; ++i) {
        getline(patchFile, line);
        replace(line.begin(), line.end(), ',', ' ');
        istringstream iss(line);
        float x, y, z;
        iss >> x >> y >> z;
        controlPoints[i] = newVertex(x, y, z);
    }

    // Generate vertices
    vector<Vertex> vertices;
    float step = 1.0f / tessellation;

    for (auto& patch : patches) {
        for (float u = 0; u <= 1.0f; u += step) {
            for (float v = 0; v <= 1.0f; v += step) {
                Vertex p = bezier(u, v, controlPoints, patch);
                vertices.push_back(p);
            }
        }
    }

    // Write vertices to file
    outputFile << vertices.size() << endl;
    for (const auto& v : vertices) {
        outputFile << v.x << " " << v.y << " " << v.z << endl;
    }

    // Generate faces (triangles)
    int pointsPerPatch = (tessellation + 1) * (tessellation + 1);
    int numFaces = numPatches * tessellation * tessellation * 2;
    outputFile << numFaces << endl;

    for (int p = 0; p < numPatches; p++) {
        int patchOffset = p * pointsPerPatch;
        for (int i = 0; i < tessellation; i++) {
            for (int j = 0; j < tessellation; j++) {
                int v1 = patchOffset + i * (tessellation + 1) + j;
                int v2 = v1 + 1;
                int v3 = v1 + tessellation + 1;
                int v4 = v2 + tessellation + 1;

                // First triangle
                outputFile << v1 << " " << v2 << " " << v3 << endl;
                // Second triangle
                outputFile << v2 << " " << v4 << " " << v3 << endl;
            }
        }
    }

    patchFile.close();
    outputFile.close();
    cout << "Archivo generado: " << outputFileName << endl;
}

int main(int argc, char* argv[]) {
    // Verificar los argumentos de la línea de comandos
    if (argc < 2) {
        cerr << "Uso: generator <primitive> <params> <output_file>" << endl;
        cerr << "Primitivas disponibles: plane, box, sphere, cone, bezier" << endl;
        return 1;
    }

    // Verificar la primitiva
    if (strcmp(argv[1], "plane") == 0) {
        if (argc != 5) {
            cerr << "Uso: generator plane <size> <divisions> <output_file>" << endl;
            return 1;
        }
        float size = stof(argv[2]);
        int divisions = stoi(argv[3]);
        string filename = argv[4];
        generatePlane(size, divisions, filename);
    }
    else if (strcmp(argv[1], "box") == 0) {
        if (argc != 5) {
            cerr << "Uso: generator box <size> <divisions> <output_file>" << endl;
            return 1;
        }
        float size = stof(argv[2]);
        int divisions = stoi(argv[3]);
        string filename = argv[4];
        generateBox(size, divisions, filename);
    }
    else if (strcmp(argv[1], "sphere") == 0) {
        if (argc != 6) {
            cerr << "Uso: generator sphere <radius> <slices> <stacks> <output_file>" << endl;
            return 1;
        }
        float radius = stof(argv[2]);
        int slices = stoi(argv[3]);
        int stacks = stoi(argv[4]);
        string filename = argv[5];
        generateSphere(radius, slices, stacks, filename);
    }
    else if (strcmp(argv[1], "cone") == 0) {
        if (argc != 7) {
            cerr << "Uso: generator cone <radius> <height> <slices> <stacks> <output_file>" << endl;
            return 1;
        }
        float radius = stof(argv[2]);
        float height = stof(argv[3]);
        int slices = stoi(argv[4]);
        int stacks = stoi(argv[5]);
        string filename = argv[6];
        generateCone(radius, height, slices, stacks, filename);
    }
    else if (strcmp(argv[1], "bezier") == 0) {
        if (argc != 5) {
            cerr << "Uso: generator bezier <patch_file> <tessellation> <output_file>" << endl;
            return 1;
        }
        string patchFile = argv[2];
        int tessellation = stoi(argv[3]);
        string filename = argv[4];
        generateBezierSurface(patchFile, filename, tessellation);
    }
    else {
        cerr << "Primitiva no reconocida: " << argv[1] << endl;
        return 1;
    }

    return 0;
}