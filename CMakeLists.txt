cmake_minimum_required(VERSION 3.10...3.25)

# Nombre del proyecto
PROJECT(CG_Project)

# Habilitar el uso de carpetas en IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set(CMAKE_CXX_STANDARD 11) # Forzar el uso de C++11

# Archivos fuente del generador y del motor
set(GENERATOR_SRCS generator.cpp)
set(ENGINE_SRCS engine.cpp)

# Crear ejecutables para el generador y el motor
add_executable(generator ${GENERATOR_SRCS})
add_executable(engine ${ENGINE_SRCS})

# Buscar OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL no encontrado!")
endif()

# Configuración para Windows
if(WIN32)
    message(STATUS "Sistema operativo: Windows")

    # Configurar la ruta a las bibliotecas GLUT y GLEW
    set(TOOLKITS_FOLDER "" CACHE PATH "Ruta a la carpeta de Toolkits (GLUT y GLEW)")

    # Verificar si GLUT está instalado
    if(NOT EXISTS "${TOOLKITS_FOLDER}/glut/GL/glut.h" OR NOT EXISTS "${TOOLKITS_FOLDER}/glut/glut32.lib")
        message(FATAL_ERROR "GLUT no encontrado en ${TOOLKITS_FOLDER}/glut")
    endif()

    # Verificar si GLEW está instalado
    if(NOT EXISTS "${TOOLKITS_FOLDER}/glew/GL/glew.h" OR NOT EXISTS "${TOOLKITS_FOLDER}/glew/glew32.lib")
        message(FATAL_ERROR "GLEW no encontrado en ${TOOLKITS_FOLDER}/glew")
    endif()

    # Incluir directorios y enlazar bibliotecas
    include_directories(${TOOLKITS_FOLDER}/glut ${TOOLKITS_FOLDER}/glew)
    target_link_libraries(generator ${OPENGL_LIBRARIES})
    target_link_libraries(engine ${OPENGL_LIBRARIES} 
                          ${TOOLKITS_FOLDER}/glut/glut32.lib 
                          ${TOOLKITS_FOLDER}/glew/glew32.lib)

    # Copiar DLLs al directorio de salida
    if(EXISTS "${TOOLKITS_FOLDER}/glut/glut32.dll" AND EXISTS "${TOOLKITS_FOLDER}/glew/glew32.dll")
        file(COPY ${TOOLKITS_FOLDER}/glut/glut32.dll DESTINATION ${CMAKE_BINARY_DIR})
        file(COPY ${TOOLKITS_FOLDER}/glew/glew32.dll DESTINATION ${CMAKE_BINARY_DIR})
    endif()

    # Establecer el proyecto de inicio en Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)

# Configuración para Linux y macOS
else()
    message(STATUS "Sistema operativo: Linux/macOS")

    # Buscar GLUT
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    link_directories(${GLUT_LIBRARY_DIRS})
    add_definitions(${GLUT_DEFINITIONS})

    if(NOT GLUT_FOUND)
        message(FATAL_ERROR "GLUT no encontrado!")
    endif()

    # Buscar GLEW (solo en Linux)
    if(NOT APPLE)
        find_package(GLEW REQUIRED)
        include_directories(${GLEW_INCLUDE_DIRS})
        link_libraries(${GLEW_LIBRARIES})
        target_link_libraries(engine ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES})
    else()
        target_link_libraries(engine ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
    endif()

    # Enlazar bibliotecas para el motor
    target_link_libraries(engine ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
endif()

# Ruta a TinyXML2
set(TINYXML2_DIR ${CMAKE_SOURCE_DIR}/GivenFiles/toolkits)
set(TINYXML2_SOURCES 
    ${TINYXML2_DIR}/tinyxml2.cpp
)

# Incluir TinyXML2 en el motor
target_include_directories(engine PRIVATE ${TINYXML2_DIR})
target_sources(engine PRIVATE ${TINYXML2_SOURCES})

# Configuración adicional para el generador
target_link_libraries(generator)

# Mensaje de finalización
message(STATUS "Configuración de CMake completada.")