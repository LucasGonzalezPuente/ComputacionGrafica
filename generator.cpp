#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm> // Para replace, remove_if
#include <sstream>   // Para istringstream/stringstream
#include <limits>    // Para numeric_limits
#include <functional> // Para std::function en generateBox
#include <stdexcept> // Para std::out_of_range, std::invalid_argument
#include <cctype>    // Para ::isspace

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// --- Estructuras Auxiliares ---
struct Point3D {
    float x, y, z;
};

struct Point2D {
    float u, v;
};

// --- Funciones Auxiliares ---

// Normalizar un vector 3D
void normalize(Point3D& p) {
    float length = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    // Evitar división por cero o valores muy pequeños
    if (length > numeric_limits<float>::epsilon()) {
        p.x /= length;
        p.y /= length;
        p.z /= length;
    }
    else {
        // Keep direction if possible, otherwise default to Y up
        // This case shouldn't happen for valid geometry normals usually.
        if (p.x == 0 && p.y == 0 && p.z == 0) {
            p.y = 1.0f; // Default normal if zero vector
        }
        // If it's non-zero but too small, normalization might fail,
        // but the direction is likely preserved. Handle as needed.
    }
}


// Producto Vectorial
Point3D crossProduct(const Point3D& a, const Point3D& b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}

// Escribir vértices, normales, coordenadas de textura y *índices* en un archivo .3d
// *** CRITICAL FIX: writeModelFile MUST write indices for engine's glDrawElements ***
// *** The format interpreted by engine.cpp's loadModel requires V, N, T to be parallel arrays ***
// *** and 'faces' contains indices into these arrays.                       ***
void writeModelFile(const string& filename,
    const vector<Point3D>& vertices,
    const vector<Point3D>& normals,
    const vector<Point2D>& texCoords,
    const vector<unsigned int>& faces) // Added faces parameter
{
    ofstream outFile(filename);
    if (!outFile) {
        cerr << "Error: No se pudo abrir el archivo de salida: " << filename << endl;
        return;
    }
    // Use scientific notation for higher precision and consistency
    outFile << std::scientific;
    outFile.precision(std::numeric_limits<float>::max_digits10);


    // --- IMPORTANT: Interpretation based on engine.cpp's loadModel ---
    // It reads numVertices, then V data. Reads numNormals, then N data (expects N==V).
    // Reads numTexCoords, then T data (expects T==V). Reads numFaces, then index data.
    // This means V, N, T are parallel arrays indexed by the 'faces' data.

    if (vertices.empty()) {
        cerr << "Error: No vertices to write for " << filename << endl;
        return;
    }
    if (normals.size() != vertices.size()) {
        cerr << "Warning: Number of normals (" << normals.size() << ") doesn't match vertices (" << vertices.size() << ") for " << filename << ". Writing normals anyway." << endl;
        // Proceed, but the engine might behave unexpectedly if loadModel assumes sizes match
    }
    if (texCoords.size() != vertices.size()) {
        cerr << "Warning: Number of texCoords (" << texCoords.size() << ") doesn't match vertices (" << vertices.size() << ") for " << filename << ". Writing texCoords anyway." << endl;
        // Proceed with caution
    }


    // 1. Write number of vertices
    outFile << vertices.size() << endl;

    // 2. Write vertices
    for (const auto& v : vertices) {
        outFile << v.x << " " << v.y << " " << v.z << endl;
    }

    // 3. Write number of normals
    outFile << normals.size() << endl;
    // 4. Write normals
    for (const auto& n : normals) {
        outFile << n.x << " " << n.y << " " << n.z << endl;
    }

    // 5. Write number of texture coordinates
    outFile << texCoords.size() << endl;
    // 6. Write texture coordinates
    for (const auto& t : texCoords) {
        outFile << t.u << " " << t.v << endl;
    }

    // 7. Write number of faces (triangles)
    if (faces.empty()) {
        cerr << "Warning: No faces (indices) provided for " << filename << ". Writing 0 faces." << endl;
        outFile << 0 << endl;
    }
    else {
        if (faces.size() % 3 != 0) {
            cerr << "Error: Number of indices (" << faces.size() << ") is not a multiple of 3 for " << filename << "." << endl;
            outFile << 0 << endl; // Write 0 faces to signal error downstream
        }
        else {
            outFile << faces.size() / 3 << endl; // Number of TRIANGLES
            // 8. Write face indices
            for (size_t i = 0; i < faces.size(); ++i) {
                outFile << faces[i] << ((i % 3 == 2) ? "" : " "); // Space between indices in a face
                if (i % 3 == 2) outFile << endl; // Newline after each face
            }
        }
    }


    cout << "Modelo guardado en " << filename << endl;
    cout << "  Vertices: " << vertices.size() << endl;
    cout << "  Normales: " << normals.size() << endl;
    cout << "  TexCoords: " << texCoords.size() << endl;
    cout << "  Caras (Triangulos): " << (faces.size() / 3) << endl;
    cout << "  Indices Totales: " << faces.size() << endl;

    outFile.close();
}


// --- Data Generation Helper ---
// Instead of directly pushing into vectors in each function,
// we use intermediate storage and build indices.
struct VertexData {
    vector<Point3D> vertices;
    vector<Point3D> normals;
    vector<Point2D> texCoords;
    vector<unsigned int> indices; // Renamed from faces for clarity
};

// Add triangle to VertexData
void addTriangle(VertexData& data,
    Point3D p1, Point3D n1, Point2D t1,
    Point3D p2, Point3D n2, Point2D t2,
    Point3D p3, Point3D n3, Point2D t3)
{
    unsigned int baseIndex = data.vertices.size();
    data.vertices.push_back(p1); data.normals.push_back(n1); data.texCoords.push_back(t1);
    data.vertices.push_back(p2); data.normals.push_back(n2); data.texCoords.push_back(t2);
    data.vertices.push_back(p3); data.normals.push_back(n3); data.texCoords.push_back(t3);
    data.indices.push_back(baseIndex);
    data.indices.push_back(baseIndex + 1);
    data.indices.push_back(baseIndex + 2);
}


// --- Funciones de Generación de Primitivas (Actualizadas) ---

// Plano (en XZ, centrado en el origen)
void generatePlane(float size, int divisions, const string& filename) {
    VertexData data;
    float halfSize = size / 2.0f;
    float step = size / divisions;
    Point3D normal = { 0.0f, 1.0f, 0.0f }; // Normal hacia arriba para plano XZ

    for (int i = 0; i < divisions; ++i) {
        for (int j = 0; j < divisions; ++j) {
            float x1 = -halfSize + i * step;
            float z1 = -halfSize + j * step;
            float x2 = x1 + step;
            float z2 = z1 + step;

            float u1 = (float)i / divisions;
            float v1 = (float)j / divisions;
            float u2 = (float)(i + 1) / divisions;
            float v2 = (float)(j + 1) / divisions;

            // Vertices del quad
            Point3D p1 = { x1, 0.0f, z1 }; Point2D t1 = { u1, v1 };
            Point3D p2 = { x2, 0.0f, z1 }; Point2D t2 = { u2, v1 };
            Point3D p3 = { x1, 0.0f, z2 }; Point2D t3 = { u1, v2 };
            Point3D p4 = { x2, 0.0f, z2 }; Point2D t4 = { u2, v2 };


            // Triángulo 1 (p1, p2, p4) - CCW looking from +Y
            addTriangle(data, p1, normal, t1, p2, normal, t2, p4, normal, t4);
            // Triángulo 2 (p1, p4, p3) - CCW looking from +Y
            addTriangle(data, p1, normal, t1, p4, normal, t4, p3, normal, t3);
        }
    }

    writeModelFile(filename, data.vertices, data.normals, data.texCoords, data.indices);
}


// Caja (centrada en el origen)
void generateBox(float size, int divisions, const string& filename) {
    VertexData data;
    float halfSize = size / 2.0f;
    float step = size / divisions;

    // Función lambda para añadir un quad dividido
    auto addFace = [&](function<Point3D(float, float)> posFunc, Point3D normal, function<Point2D(float, float)> texFunc) {
        for (int i = 0; i < divisions; ++i) {
            for (int j = 0; j < divisions; ++j) {
                // Use normalized coords [0,1] for both position and texture mapping generation funcs
                float u_norm1 = (float)i / divisions;
                float v_norm1 = (float)j / divisions;
                float u_norm2 = (float)(i + 1) / divisions;
                float v_norm2 = (float)(j + 1) / divisions;


                Point3D p1 = posFunc(u_norm1, v_norm1);
                Point3D p2 = posFunc(u_norm2, v_norm1);
                Point3D p3 = posFunc(v_norm1, u_norm1); // Swapped for consistency? No, use u,v directly
                Point3D p_tl = posFunc(u_norm1, v_norm1); // Top-left param pair
                Point3D p_tr = posFunc(u_norm2, v_norm1); // Top-right param pair
                Point3D p_bl = posFunc(u_norm1, v_norm2); // Bottom-left param pair
                Point3D p_br = posFunc(u_norm2, v_norm2); // Bottom-right param pair


                Point2D t_tl = texFunc(u_norm1, v_norm1);
                Point2D t_tr = texFunc(u_norm2, v_norm1);
                Point2D t_bl = texFunc(u_norm1, v_norm2);
                Point2D t_br = texFunc(u_norm2, v_norm2);


                // Triángulo 1: top-left, top-right, bottom-right (CCW depends on face normal)
                // Triángulo 2: top-left, bottom-right, bottom-left (CCW depends on face normal)
                // Adjust vertex order based on normal to ensure CCW winding when viewed from outside
                if (normal.y > 0.9f) { // Top face (+Y) looking down: p_tl, p_tr, p_br | p_tl, p_br, p_bl (OK)
                    addTriangle(data, p_tl, normal, t_tl, p_tr, normal, t_tr, p_br, normal, t_br);
                    addTriangle(data, p_tl, normal, t_tl, p_br, normal, t_br, p_bl, normal, t_bl);
                }
                else if (normal.y < -0.9f) { // Bottom face (-Y) looking up: need reversed order: p_tl, p_bl, p_br | p_tl, p_br, p_tr
                    addTriangle(data, p_tl, normal, t_tl, p_bl, normal, t_bl, p_br, normal, t_br);
                    addTriangle(data, p_tl, normal, t_tl, p_br, normal, t_br, p_tr, normal, t_tr);
                }
                else if (normal.x > 0.9f) { // Right face (+X) looking from -X: p_tl, p_bl, p_br | p_tl, p_br, p_tr
                    addTriangle(data, p_tl, normal, t_tl, p_bl, normal, t_bl, p_br, normal, t_br);
                    addTriangle(data, p_tl, normal, t_tl, p_br, normal, t_br, p_tr, normal, t_tr);
                }
                else if (normal.x < -0.9f) { // Left face (-X) looking from +X: p_tl, p_tr, p_br | p_tl, p_br, p_bl
                    addTriangle(data, p_tl, normal, t_tl, p_tr, normal, t_tr, p_br, normal, t_br);
                    addTriangle(data, p_tl, normal, t_tl, p_br, normal, t_br, p_bl, normal, t_bl);
                }
                else if (normal.z > 0.9f) { // Front face (+Z) looking from -Z: p_tl, p_tr, p_br | p_tl, p_br, p_bl
                    addTriangle(data, p_tl, normal, t_tl, p_tr, normal, t_tr, p_br, normal, t_br);
                    addTriangle(data, p_tl, normal, t_tl, p_br, normal, t_br, p_bl, normal, t_bl);
                }
                else { // Back face (-Z) looking from +Z: p_tl, p_bl, p_br | p_tl, p_br, p_tr
                    addTriangle(data, p_tl, normal, t_tl, p_bl, normal, t_bl, p_br, normal, t_br);
                    addTriangle(data, p_tl, normal, t_tl, p_br, normal, t_br, p_tr, normal, t_tr);
                }
            }
        }
        };


    // Define faces using normalized u,v parameters [0,1]
    // Cara +Y (Arriba) normal=(0,1,0)
    addFace([&](float u, float v) { return Point3D{ -halfSize + u * size, halfSize, -halfSize + v * size }; },
        { 0, 1, 0 },
        [](float u, float v) { return Point2D{ u, 1.0f - v }; }); // Y is up, map v = 0 to top
    // Cara -Y (Abajo) normal=(0,-1,0)
    addFace([&](float u, float v) { return Point3D{ -halfSize + u * size, -halfSize, halfSize - v * size }; },
        { 0, -1, 0 },
        [](float u, float v) { return Point2D{ u, 1.0f - v }; }); // Y is down, map v = 0 to "top" of bottom face
    // Cara +X (Derecha) normal=(1,0,0)
    addFace([&](float u, float v) { return Point3D{ halfSize, halfSize - u * size, -halfSize + v * size }; },
        { 1, 0, 0 },
        [](float u, float v) { return Point2D{ v, u }; }); // Map u to Y, v to Z
    // Cara -X (Izquierda) normal=(-1,0,0)
    addFace([&](float u, float v) { return Point3D{ -halfSize, halfSize - u * size, halfSize - v * size }; },
        { -1, 0, 0 },
        [](float u, float v) { return Point2D{ v, u }; }); // Map u to Y, v to Z
    // Cara +Z (Frente) normal=(0,0,1)
    addFace([&](float u, float v) { return Point3D{ -halfSize + u * size, halfSize - v * size, halfSize }; },
        { 0, 0, 1 },
        [](float u, float v) { return Point2D{ u, v }; }); // Map u to X, v to Y
    // Cara -Z (Atrás) normal=(0,0,-1)
    addFace([&](float u, float v) { return Point3D{ halfSize - u * size, halfSize - v * size, -halfSize }; },
        { 0, 0, -1 },
        [](float u, float v) { return Point2D{ u, v }; }); // Map u to X, v to Y


    writeModelFile(filename, data.vertices, data.normals, data.texCoords, data.indices);
}


// Esfera (centrada en el origen)
void generateSphere(float radius, int slices, int stacks, const string& filename) {
    VertexData data;
    float deltaPhi = 2 * M_PI / slices; // Ángulo azimutal (horizontal, around Y)
    float deltaTheta = M_PI / stacks;   // Ángulo polar (vertical, from +Y)

    for (int i = 0; i < stacks; ++i) {
        float theta1 = i * deltaTheta;       // Angle from +Y axis for upper edge of stack
        float theta2 = (i + 1) * deltaTheta; // Angle from +Y axis for lower edge of stack

        for (int j = 0; j < slices; ++j) {
            float phi1 = j * deltaPhi;       // Azimuth angle for left edge of slice
            float phi2 = (j + 1) * deltaPhi; // Azimuth angle for right edge of slice

            // Calculate vertices of the quad for this segment
            // Top-left
            Point3D p1 = { radius * sin(theta1) * cos(phi1), radius * cos(theta1), radius * sin(theta1) * sin(phi1) };
            // Top-right
            Point3D p2 = { radius * sin(theta1) * cos(phi2), radius * cos(theta1), radius * sin(theta1) * sin(phi2) };
            // Bottom-left
            Point3D p3 = { radius * sin(theta2) * cos(phi1), radius * cos(theta2), radius * sin(theta2) * sin(phi1) };
            // Bottom-right
            Point3D p4 = { radius * sin(theta2) * cos(phi2), radius * cos(theta2), radius * sin(theta2) * sin(phi2) };

            // Calculate normals (for sphere centered at origin, normals are normalized vertex positions)
            Point3D n1 = p1; normalize(n1);
            Point3D n2 = p2; normalize(n2);
            Point3D n3 = p3; normalize(n3);
            Point3D n4 = p4; normalize(n4);

            // Calculate texture coordinates (spherical mapping)
            // u = longitude/(2*PI), v = latitude/PI (latitude from North pole)
            float u1 = phi1 / (2 * M_PI);
            float u2 = phi2 / (2 * M_PI);
            float v1 = theta1 / M_PI; // v=0 at North Pole, v=1 at South Pole
            float v2 = theta2 / M_PI;

            Point2D t1 = { u1, v1 };
            Point2D t2 = { u2, v1 };
            Point2D t3 = { u1, v2 };
            Point2D t4 = { u2, v2 };

            // Add triangles (ensure CCW winding order when viewed from outside)
            // Top triangle (p1, p3, p4) - forms bottom part of the quad strip
            if (i != 0) { // Don't create triangles for the top cap degenerate edge
                addTriangle(data, p1, n1, t1, p3, n3, t3, p4, n4, t4);
            }
            // Bottom triangle (p1, p4, p2) - forms top part of the quad strip
            if (i != (stacks - 1)) { // Don't create triangles for the bottom cap degenerate edge
                addTriangle(data, p1, n1, t1, p4, n4, t4, p2, n2, t2);
            }
        }
    }

    writeModelFile(filename, data.vertices, data.normals, data.texCoords, data.indices);
}


// Cono (base en XZ, punta en +Y)
void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
    VertexData data;
    float deltaAlpha = 2 * M_PI / slices;
    Point3D baseCenter = { 0.0f, 0.0f, 0.0f };
    Point3D tip = { 0.0f, height, 0.0f };
    Point3D baseNormal = { 0.0f, -1.0f, 0.0f };

    // 1. Generate Base Circle (disk in XZ plane at Y=0)
    for (int i = 0; i < slices; ++i) {
        float alpha1 = i * deltaAlpha;
        float alpha2 = (i + 1) * deltaAlpha;

        Point3D p1 = { radius * cos(alpha1), 0.0f, radius * sin(alpha1) };
        Point3D p2 = { radius * cos(alpha2), 0.0f, radius * sin(alpha2) };

        // Texture coordinates for the base (polar mapping)
        Point2D t_center = { 0.5f, 0.5f };
        Point2D t1 = { 0.5f + 0.5f * cos(alpha1), 0.5f + 0.5f * sin(alpha1) };
        Point2D t2 = { 0.5f + 0.5f * cos(alpha2), 0.5f + 0.5f * sin(alpha2) };


        // Add base triangle (Center, P2, P1) - ensures CCW when viewed from below (-Y)
        addTriangle(data, baseCenter, baseNormal, t_center, p2, baseNormal, t2, p1, baseNormal, t1);
    }

    // 2. Generate Lateral Surface (stacks)
    float L = sqrt(radius * radius + height * height); // Slant height (generatrix length)
    float normalYComponent = radius / L; // Y component of lateral normal is constant (cos of angle between generatrix and base)
    float normalXZMagnitude = height / L; // Magnitude of XZ component (sin of angle)


    for (int j = 0; j < stacks; ++j) {
        float h1 = (float)j / stacks * height;         // Height of lower edge of stack
        float h2 = (float)(j + 1) / stacks * height;   // Height of upper edge of stack
        float r1 = radius * (1.0f - (float)j / stacks); // Radius at lower edge
        float r2 = radius * (1.0f - (float)(j + 1) / stacks); // Radius at upper edge

        for (int i = 0; i < slices; ++i) {
            float alpha1 = i * deltaAlpha;
            float alpha2 = (i + 1) * deltaAlpha;

            // Vertices of the quad for this segment
            Point3D p1 = { r1 * cos(alpha1), h1, r1 * sin(alpha1) }; // Bottom-left
            Point3D p2 = { r1 * cos(alpha2), h1, r1 * sin(alpha2) }; // Bottom-right
            Point3D p3 = { r2 * cos(alpha1), h2, r2 * sin(alpha1) }; // Top-left
            Point3D p4 = { r2 * cos(alpha2), h2, r2 * sin(alpha2) }; // Top-right

            // Normals for the lateral surface
            // Normal vector points outwards and slightly upwards.
            // XZ component is along the radius vector, Y component is constant.
            Point3D n1 = { normalXZMagnitude * cos(alpha1), normalYComponent, normalXZMagnitude * sin(alpha1) }; normalize(n1);
            Point3D n2 = { normalXZMagnitude * cos(alpha2), normalYComponent, normalXZMagnitude * sin(alpha2) }; normalize(n2);
            Point3D n3 = n1; // Normal is the same along the generatrix
            Point3D n4 = n2; // Normal is the same along the generatrix


            // Texture coordinates for the lateral surface
            // u corresponds to angle (alpha), v corresponds to height/stack
            float u1 = (float)i / slices;
            float u2 = (float)(i + 1) / slices;
            float v1 = (float)j / stacks; // v=0 at base, v=1 near tip
            float v2 = (float)(j + 1) / stacks;
            Point2D t1 = { u1, v1 };
            Point2D t2 = { u2, v1 };
            Point2D t3 = { u1, v2 };
            Point2D t4 = { u2, v2 };

            // Add triangles for the lateral surface quad (ensure CCW when viewed from outside)
            // Triangle 1 (p1, p2, p4) - Bottom-left, Bottom-right, Top-right
            addTriangle(data, p1, n1, t1, p2, n2, t2, p4, n4, t4);
            // Triangle 2 (p1, p4, p3) - Bottom-left, Top-right, Top-left
            addTriangle(data, p1, n1, t1, p4, n4, t4, p3, n3, t3);
        }
    }

    writeModelFile(filename, data.vertices, data.normals, data.texCoords, data.indices);
}


// --- Funciones para Superficies de Bézier ---

// Leer archivo .patch
// *** FIX for the teapot.patch reading error ***
bool readPatchFile(const string& filepath, vector<vector<int>>& patchIndices, vector<Point3D>& controlPoints) {
    ifstream patchFile(filepath);
    if (!patchFile) {
        cerr << "Error: No se pudo abrir el archivo de patch: " << filepath << endl;
        return false;
    }

    string line;
    int numPatches = 0;
    int numControlPointsExpected = 0; // Use a different name

    // 1. Leer número de patches
    if (!getline(patchFile, line)) { /* Handle EOF / empty file */ return false; }
    try {
        numPatches = stoi(line);
    }
    catch (const std::exception& e) { /* Handle parse error */ cerr << "Error reading numPatches: " << e.what() << endl; return false; }
    if (numPatches <= 0) { /* Handle invalid value */ cerr << "Invalid numPatches: " << numPatches << endl; return false; }

    patchIndices.clear(); // Clear previous data
    patchIndices.resize(numPatches);

    // 2. Leer los índices de cada patch
    for (int i = 0; i < numPatches; ++i) {
        if (!getline(patchFile, line)) { /* Handle unexpected EOF */ cerr << "EOF while reading indices for patch " << i << endl; return false; }
        stringstream ss_indices(line);
        string indice_str;
        vector<int> current_patch_indices;
        while (getline(ss_indices, indice_str, ',')) {
            indice_str.erase(remove_if(indice_str.begin(), indice_str.end(), ::isspace), indice_str.end());
            if (indice_str.empty()) continue; // Skip empty segments potentially caused by trailing commas
            try {
                current_patch_indices.push_back(stoi(indice_str));
            }
            catch (const std::exception& e) { /* Handle parse error */ cerr << "Error parsing index '" << indice_str << "' for patch " << i << ": " << e.what() << endl; return false; }
        }
        if (current_patch_indices.size() != 16) {
            cerr << "Warning: Patch " << i << " has " << current_patch_indices.size() << " indices, expected 16." << endl;
            // Decide if this is fatal. For now, allow it but it might crash later.
            // return false; // Uncomment to make it fatal
        }
        patchIndices[i] = current_patch_indices;
    }

    // 3. Leer número de puntos de control
    if (!getline(patchFile, line)) { /* Handle unexpected EOF */ cerr << "EOF while expecting numControlPoints." << endl; return false; }
    try {
        numControlPointsExpected = stoi(line);
    }
    catch (const std::exception& e) { /* Handle parse error */ cerr << "Error reading numControlPoints: " << e.what() << endl; return false; }
    if (numControlPointsExpected <= 0) { /* Handle invalid value */ cerr << "Invalid numControlPoints: " << numControlPointsExpected << endl; return false; }

    // 4. Leer los puntos de control *** FIX THE READING LOOP ***
    controlPoints.clear(); // Clear previous data
    controlPoints.reserve(numControlPointsExpected); // Reserve space

    cout << "[DEBUG readPatchFile] Expecting " << numControlPointsExpected << " control points." << endl;

    int pointsRead = 0;
    while (pointsRead < numControlPointsExpected && getline(patchFile, line)) // Check pointsRead AND getline success
    {
        // Trim leading/trailing whitespace from the line itself
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty()) { // Skip empty lines if any exist in the file
            // cerr << "[DEBUG readPatchFile] Skipping empty line at index " << pointsRead << endl;
            continue;
        }

        // cout << "[DEBUG readPatchFile] Reading point " << pointsRead << ": Line='" << line << "'" << endl; // Verbose debug

        stringstream ss_coords(line);
        string coord_str;
        vector<float> coords;
        bool readError = false;
        while (getline(ss_coords, coord_str, ',')) {
            coord_str.erase(remove_if(coord_str.begin(), coord_str.end(), ::isspace), coord_str.end());
            if (coord_str.empty()) { // Handle potential empty segments like "1.0,,2.0"
                cerr << "[ERROR readPatchFile] Empty coordinate segment found for point " << pointsRead << ". Line: '" << line << "'" << endl;
                readError = true;
                break;
            }
            try {
                coords.push_back(stof(coord_str));
            }
            catch (const std::exception& e) {
                cerr << "[ERROR readPatchFile] Failed stof() for '" << coord_str << "' (point " << pointsRead << "). Line: '" << line << "'. Error: " << e.what() << endl;
                readError = true;
                break;
            }
        }

        if (readError) {
            // Optionally decide if one error should stop the whole process
            // return false; // Uncomment to make it fatal
            // If we continue, this point is skipped, and the count will be wrong.
            // Let's make it fatal to ensure correctness.
            return false;
        }


        if (coords.size() != 3) {
            cerr << "[ERROR readPatchFile] Point " << pointsRead << " has " << coords.size() << " coordinates, expected 3. Line: '" << line << "'" << endl;
            return false; // Make this fatal
        }

        controlPoints.push_back({ coords[0], coords[1], coords[2] });
        pointsRead++;
    }

    // After the loop, check if we read the expected number *and* if there was an error reading the last line (e.g., EOF before reading enough points)
    cout << "[DEBUG readPatchFile] Finished reading loop. Points actually read: " << pointsRead << endl;

    if (pointsRead != numControlPointsExpected) {
        cerr << "[ERROR readPatchFile] Read " << pointsRead << " control points, but expected " << numControlPointsExpected << "." << endl;
        // Check stream state for clues
        if (patchFile.eof()) {
            cerr << "  Reason: Reached end-of-file prematurely." << endl;
        }
        else if (patchFile.fail()) {
            cerr << "  Reason: Stream failed (possibly bad format in the last lines)." << endl;
        }
        else if (!patchFile.good()) {
            cerr << "  Reason: Stream is not good (unknown error)." << endl;
        }
        return false; // Indicate failure if counts don't match
    }


    // One final check: are there more non-empty lines after reading all expected points?
    while (getline(patchFile, line)) {
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (!line.empty()) {
            cerr << "[WARNING readPatchFile] Extra non-empty line found after reading all expected points: '" << line << "'" << endl;
            // This isn't necessarily an error, but good to know.
            break; // Stop checking after the first extra line.
        }
    }


    cout << "[DEBUG readPatchFile] Successfully read " << numPatches << " patches and " << controlPoints.size() << " control points." << endl;
    return true;
}

// Calculate Bernstein polynomial basis function B_i^3(t)
float getBernstein(float t, int i) {
    // Using the explicit formulas for degree 3:
    switch (i) {
    case 0: return (1 - t) * (1 - t) * (1 - t);       // (1-t)^3
    case 1: return 3 * t * (1 - t) * (1 - t);     // 3t(1-t)^2
    case 2: return 3 * t * t * (1 - t);         // 3t^2(1-t)
    case 3: return t * t * t;                   // t^3
    default: return 0.0f;                   // Invalid index
    }
}

// Calculate matrices B(u) and B(v) [as row vectors for easier multiplication later]
void getBernsteinMatrices(float u, float v, float B_u[4], float B_v[4]) {
    for (int i = 0; i < 4; ++i) {
        B_u[i] = getBernstein(u, i);
        B_v[i] = getBernstein(v, i);
    }
}

// Obtain point P(u,v) on the Bezier surface patch
Point3D getBezierPoint(float u, float v, const vector<Point3D>& allControlPoints, const vector<int>& patchIdx) {
    Point3D point = { 0.0f, 0.0f, 0.0f };
    float B_u[4], B_v[4];
    getBernsteinMatrices(u, v, B_u, B_v);

    if (patchIdx.size() != 16) {
        throw std::runtime_error("Invalid patch index count in getBezierPoint");
    }

    for (int row = 0; row < 4; ++row) { // Corresponds to u dimension (B_u)
        for (int col = 0; col < 4; ++col) { // Corresponds to v dimension (B_v)
            int controlPointIndex = patchIdx[row * 4 + col];

            // BOUNDS CHECK (CRITICAL)
            if (controlPointIndex < 0 || controlPointIndex >= allControlPoints.size()) {
                cerr << "[FATAL ERROR] Control point index " << controlPointIndex
                    << " is out of bounds (0.." << allControlPoints.size() - 1 << ") in getBezierPoint." << endl;
                throw std::out_of_range("Control point index out of range in getBezierPoint");
            }

            const Point3D& P_ij = allControlPoints[controlPointIndex];
            float weight = B_u[row] * B_v[col];
            point.x += P_ij.x * weight;
            point.y += P_ij.y * weight;
            point.z += P_ij.z * weight;
        }
    }
    return point;
}

// Calculate derivative of Bernstein polynomial basis function B'_i^3(t)
float getDerivativeBernstein(float t, int i) {
    // Using the explicit formulas for derivatives of degree 3 basis functions:
    switch (i) {
    case 0: return -3 * (1 - t) * (1 - t);        // -3(1-t)^2
    case 1: return 3 * (1 - t) * (1 - t) - 6 * t * (1 - t); // 3(1-t)^2 - 6t(1-t) = 3(1-t)(1-3t)
    case 2: return 6 * t * (1 - t) - 3 * t * t;     // 6t(1-t) - 3t^2 = 3t(2-3t)
    case 3: return 3 * t * t;                 // 3t^2
    default: return 0.0f;                   // Invalid index
    }
}

// Calculate matrices B'(u) and B'(v) [as row vectors]
void getDerivativeBernsteinMatrices(float u, float v, float B_prime_u[4], float B_prime_v[4]) {
    for (int i = 0; i < 4; ++i) {
        B_prime_u[i] = getDerivativeBernstein(u, i);
        B_prime_v[i] = getDerivativeBernstein(v, i);
    }
}


// Obtain tangent vector dP/du at (u,v)
Point3D getBezierTangentU(float u, float v, const vector<Point3D>& allControlPoints, const vector<int>& patchIdx) {
    Point3D tangentU = { 0.0f, 0.0f, 0.0f };
    float B_prime_u[4], B_v[4];
    getDerivativeBernsteinMatrices(u, v, B_prime_u, B_v); // Need B'(u)
    getBernsteinMatrices(u, v, B_prime_u, B_v);          // Need B(v) -- reuse second parameter

    if (patchIdx.size() != 16) {
        throw std::runtime_error("Invalid patch index count in getBezierTangentU");
    }


    for (int row = 0; row < 4; ++row) { // Corresponds to u dimension (B'_u)
        for (int col = 0; col < 4; ++col) { // Corresponds to v dimension (B_v)
            int controlPointIndex = patchIdx[row * 4 + col];
            // BOUNDS CHECK (CRITICAL)
            if (controlPointIndex < 0 || controlPointIndex >= allControlPoints.size()) {
                throw std::out_of_range("Control point index out of range in getBezierTangentU");
            }
            const Point3D& P_ij = allControlPoints[controlPointIndex];
            float weight = B_prime_u[row] * B_v[col]; // Use B'(u) * B(v)
            tangentU.x += P_ij.x * weight;
            tangentU.y += P_ij.y * weight;
            tangentU.z += P_ij.z * weight;
        }
    }
    return tangentU;
}

// Obtain tangent vector dP/dv at (u,v)
Point3D getBezierTangentV(float u, float v, const vector<Point3D>& allControlPoints, const vector<int>& patchIdx) {
    Point3D tangentV = { 0.0f, 0.0f, 0.0f };
    float B_u[4], B_prime_v[4];
    getBernsteinMatrices(u, v, B_u, B_prime_v);           // Need B(u)
    getDerivativeBernsteinMatrices(u, v, B_u, B_prime_v); // Need B'(v) -- reuse second parameter

    if (patchIdx.size() != 16) {
        throw std::runtime_error("Invalid patch index count in getBezierTangentV");
    }

    for (int row = 0; row < 4; ++row) { // Corresponds to u dimension (B_u)
        for (int col = 0; col < 4; ++col) { // Corresponds to v dimension (B'_v)
            int controlPointIndex = patchIdx[row * 4 + col];
            // BOUNDS CHECK (CRITICAL)
            if (controlPointIndex < 0 || controlPointIndex >= allControlPoints.size()) {
                throw std::out_of_range("Control point index out of range in getBezierTangentV");
            }
            const Point3D& P_ij = allControlPoints[controlPointIndex];
            float weight = B_u[row] * B_prime_v[col]; // Use B(u) * B'(v)
            tangentV.x += P_ij.x * weight;
            tangentV.y += P_ij.y * weight;
            tangentV.z += P_ij.z * weight;
        }
    }
    return tangentV;
}


// Obtain normal vector N(u,v) = normalize(dP/du x dP/dv)
Point3D getBezierNormal(float u, float v, const vector<Point3D>& allControlPoints, const vector<int>& patchIdx) {
    Point3D tangentU = getBezierTangentU(u, v, allControlPoints, patchIdx);
    Point3D tangentV = getBezierTangentV(u, v, allControlPoints, patchIdx);
    Point3D normal = crossProduct(tangentU, tangentV);
    normalize(normal); // Normalize the result
    // Ensure consistent orientation (e.g., always pointing "out") if necessary,
    // though cross product order usually handles this if patches are defined consistently.
    return normal;
}


// Get texture coordinate for Bezier point (simple mapping u,v -> u,v)
Point2D getBezierTextureCoord(float u, float v) {
    return { u, v };
}


// Generar superficie de Bézier teselada
void generateBezierSurface(const string& patchFilePath, const string& filename, int tessellation) {
    vector<vector<int>> patchIndices;
    vector<Point3D> controlPoints;

    cout << "--- Generating Bezier Surface ---" << endl;
    cout << "Patch File: " << patchFilePath << endl;
    cout << "Tessellation: " << tessellation << endl;
    cout << "Output File: " << filename << endl;

    // --- Read Patch File ---
    if (!readPatchFile(patchFilePath, patchIndices, controlPoints)) {
        cerr << "Error: Failed to read or parse patch file: " << patchFilePath << endl;
        return; // Stop generation if reading failed
    }
    cout << "Successfully loaded patch data." << endl;


    VertexData data; // Accumulate data for all patches
    float step = 1.0f / tessellation;
    int numPatches = patchIndices.size();

    cout << "Processing " << numPatches << " patches..." << endl;

    for (int p = 0; p < numPatches; ++p) {
        const auto& currentPatchIndices = patchIndices[p];

        // Check if patch has correct number of indices (should be done in readPatchFile ideally)
        if (currentPatchIndices.size() != 16) {
            cerr << "Warning: Skipping patch " << p << " due to incorrect index count (" << currentPatchIndices.size() << " != 16)." << endl;
            continue;
        }

        // Tessellate this patch
        for (int i = 0; i < tessellation; ++i) { // Iterate over u steps
            for (int j = 0; j < tessellation; ++j) { // Iterate over v steps
                float u1 = (float)i * step;
                float v1 = (float)j * step;
                float u2 = (float)(i + 1) * step;
                float v2 = (float)(j + 1) * step;

                try { // Wrap Bezier calculations in try-catch for safety
                    // Calculate the 4 points of the quad on the Bezier surface
                    Point3D p_tl = getBezierPoint(u1, v1, controlPoints, currentPatchIndices); // Top-left (u1,v1)
                    Point3D p_tr = getBezierPoint(u2, v1, controlPoints, currentPatchIndices); // Top-right (u2,v1)
                    Point3D p_bl = getBezierPoint(u1, v2, controlPoints, currentPatchIndices); // Bottom-left (u1,v2)
                    Point3D p_br = getBezierPoint(u2, v2, controlPoints, currentPatchIndices); // Bottom-right (u2,v2)

                    // Calculate normals at the 4 corners
                    Point3D n_tl = getBezierNormal(u1, v1, controlPoints, currentPatchIndices);
                    Point3D n_tr = getBezierNormal(u2, v1, controlPoints, currentPatchIndices);
                    Point3D n_bl = getBezierNormal(u1, v2, controlPoints, currentPatchIndices);
                    Point3D n_br = getBezierNormal(u2, v2, controlPoints, currentPatchIndices);

                    // Calculate texture coordinates at the 4 corners
                    Point2D t_tl = getBezierTextureCoord(u1, v1);
                    Point2D t_tr = getBezierTextureCoord(u2, v1);
                    Point2D t_bl = getBezierTextureCoord(u1, v2);
                    Point2D t_br = getBezierTextureCoord(u2, v2);

                    // Add the two triangles forming the quad (ensure CCW winding)
                    // Triángulo 1: Top-Left, Bottom-Left, Bottom-Right (p_tl, p_bl, p_br)
                    addTriangle(data, p_tl, n_tl, t_tl, p_bl, n_bl, t_bl, p_br, n_br, t_br);
                    // Triángulo 2: Top-Left, Bottom-Right, Top-Right (p_tl, p_br, p_tr)
                    addTriangle(data, p_tl, n_tl, t_tl, p_br, n_br, t_br, p_tr, n_tr, t_tr);

                }
                catch (const std::exception& e) {
                    cerr << "Error during Bezier calculation for patch " << p << " at (u,v)=(" << u1 << "," << v1 << "): " << e.what() << endl;
                    // Decide how to handle: skip quad, skip patch, or stop entirely?
                    // Stopping entirely is safest if index errors occur.
                    return;
                }
            }
        }
        // Optional: Print progress per patch
        // if ((p + 1) % 10 == 0 || p == numPatches - 1) {
        //     cout << "  Processed patch " << (p + 1) << "/" << numPatches << endl;
        // }
    }

    cout << "Finished processing patches. Writing to file..." << endl;
    writeModelFile(filename, data.vertices, data.normals, data.texCoords, data.indices);
    cout << "--- Bezier Surface Generation Complete ---" << endl;
}


// --- Función Principal ---

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso:" << endl;
        cerr << "  generator plane <size> <divisions> <output_file>" << endl;
        cerr << "  generator box <size> <divisions> <output_file>" << endl;
        cerr << "  generator sphere <radius> <slices> <stacks> <output_file>" << endl;
        cerr << "  generator cone <radius> <height> <slices> <stacks> <output_file>" << endl;
        cerr << "  generator bezier <patch_file> <tessellation> <output_file>" << endl;
        return 1;
    }

    string primitiveType = argv[1];
    string filename = argv[argc - 1]; // El último argumento es siempre el archivo de salida

    try { // Envolver la mayoría de las conversiones y llamadas en un try-catch
        if (primitiveType == "plane") {
            if (argc != 5) throw invalid_argument("Uso: generator plane <size> <divisions> <output_file>");
            float size = stof(argv[2]);
            int divisions = stoi(argv[3]);
            if (divisions <= 0) throw invalid_argument("Divisions must be positive.");
            generatePlane(size, divisions, filename);
        }
        else if (primitiveType == "box") {
            if (argc != 5) throw invalid_argument("Uso: generator box <size> <divisions> <output_file>");
            float size = stof(argv[2]);
            int divisions = stoi(argv[3]);
            if (divisions <= 0) throw invalid_argument("Divisions must be positive.");
            generateBox(size, divisions, filename);
        }
        else if (primitiveType == "sphere") {
            if (argc != 6) throw invalid_argument("Uso: generator sphere <radius> <slices> <stacks> <output_file>");
            float radius = stof(argv[2]);
            int slices = stoi(argv[3]);
            int stacks = stoi(argv[4]);
            if (radius <= 0 || slices <= 2 || stacks <= 1) throw invalid_argument("Invalid sphere parameters (radius>0, slices>2, stacks>1).");
            generateSphere(radius, slices, stacks, filename);
        }
        else if (primitiveType == "cone") {
            if (argc != 7) throw invalid_argument("Uso: generator cone <radius> <height> <slices> <stacks> <output_file>");
            float radius = stof(argv[2]);
            float height = stof(argv[3]);
            int slices = stoi(argv[4]);
            int stacks = stoi(argv[5]);
            if (radius <= 0 || height <= 0 || slices <= 2 || stacks <= 0) throw invalid_argument("Invalid cone parameters (radius/height>0, slices>2, stacks>0).");
            generateCone(radius, height, slices, stacks, filename);
        }
        else if (primitiveType == "bezier") {
            if (argc != 5) throw invalid_argument("Uso: generator bezier <patch_file> <tessellation> <output_file>");
            string patchFile = argv[2];
            int tessellation = stoi(argv[3]);
            if (tessellation <= 0) throw invalid_argument("Tessellation must be positive.");
            // La llamada a generateBezierSurface maneja sus propios errores internos (lectura, cálculo)
            generateBezierSurface(patchFile, filename, tessellation);
        }
        else {
            throw invalid_argument("Error: Tipo de primitiva desconocido '" + primitiveType + "'");
        }
    }
    // Catch specific exceptions from stof/stoi
    catch (const invalid_argument& e) {
        cerr << "Error: Invalid argument provided. " << e.what() << endl;
        return 1;
    }
    catch (const out_of_range& e) {
        cerr << "Error: Numerical argument out of range. " << e.what() << endl;
        return 1;
    }
    // Catch general exceptions (including those potentially thrown by Bezier functions)
    catch (const exception& e) {
        cerr << "An unexpected error occurred: " << e.what() << endl;
        return 1;
    }
    catch (...) {
        cerr << "An unknown error occurred." << endl;
        return 1;
    }


    return 0; // Éxito
}