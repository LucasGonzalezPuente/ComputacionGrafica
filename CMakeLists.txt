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

# --- BEGIN DEVIL CONFIGURATION ---
# Establecer ruta a la carpeta base de DevIL (contenedora de include y lib)
set(DEVIL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}) # Asume include y lib están junto a CMakeLists.txt
set(DEVIL_INCLUDE_DIR ${DEVIL_BASE_DIR}/include)

# Verificar existencia de cabeceras
if(NOT EXISTS "${DEVIL_INCLUDE_DIR}/IL/il.h")
    message(FATAL_ERROR "Cabecera DevIL (il.h) no encontrada en ${DEVIL_INCLUDE_DIR}")
endif()

# Determinar arquitectura y construir rutas a lib/dll
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(DEVIL_ARCH "x64")
    set(DEVIL_ARCH_SUBDIR "x64") # Subcarpeta para la arquitectura
else()
    set(DEVIL_ARCH "x86")
    set(DEVIL_ARCH_SUBDIR "x86") # Subcarpeta para la arquitectura
endif()
message(STATUS "Configurando DevIL para arquitectura: ${DEVIL_ARCH}")

# Subcarpeta de configuración (asumiendo Release) - AJUSTA SI USAS Debug
set(DEVIL_CONFIG_SUBDIR "Release")

# Rutas a las carpetas específicas de lib y dll
set(DEVIL_LIBRARY_DIR ${DEVIL_BASE_DIR}/lib/${DEVIL_ARCH_SUBDIR}/${DEVIL_CONFIG_SUBDIR})
set(DEVIL_DLL_DIR ${DEVIL_LIBRARY_DIR}) # DLLs están en la misma carpeta que las LIBs en este caso

# Nombres de archivo (AJUSTA si son diferentes)
set(DEVIL_LIB_NAME "DevIL.lib")
set(DEVIL_DLL_NAME "DevIL.dll")
set(ILU_LIB_NAME "ILU.lib")
set(ILU_DLL_NAME "ILU.dll")
set(ILUT_LIB_NAME "ILUT.lib")
set(ILUT_DLL_NAME "ILUT.dll")

# Construir rutas completas a las librerías
set(DEVIL_LIBRARY ${DEVIL_LIBRARY_DIR}/${DEVIL_LIB_NAME})
set(ILU_LIBRARY ${DEVIL_LIBRARY_DIR}/${ILU_LIB_NAME})
set(ILUT_LIBRARY ${DEVIL_LIBRARY_DIR}/${ILUT_LIB_NAME})

# Verificar existencia de librerías .lib
if(NOT EXISTS "${DEVIL_LIBRARY}")
    message(FATAL_ERROR "Librería DevIL (${DEVIL_LIB_NAME}) no encontrada para ${DEVIL_ARCH}/${DEVIL_CONFIG_SUBDIR} en ${DEVIL_LIBRARY_DIR}")
endif()
# Descomenta estas verificaciones si usas y tienes ILU/ILUT
# if(NOT EXISTS "${ILU_LIBRARY}")
#     message(WARNING "Librería ILU (${ILU_LIB_NAME}) no encontrada en ${DEVIL_LIBRARY_DIR}")
#     set(ILU_LIBRARY "")
# endif()
# if(NOT EXISTS "${ILUT_LIBRARY}")
#     message(WARNING "Librería ILUT (${ILUT_LIB_NAME}) no encontrada en ${DEVIL_LIBRARY_DIR}")
#     set(ILUT_LIBRARY "")
# endif()

# Añadir directorio de inclusión de DevIL
target_include_directories(engine PRIVATE ${DEVIL_INCLUDE_DIR})

# Enlazar librerías DevIL con el motor
if(EXISTS "${DEVIL_LIBRARY}")
    target_link_libraries(engine PRIVATE ${DEVIL_LIBRARY})
    message(STATUS "Enlazando con: ${DEVIL_LIBRARY}")
endif()
# Descomenta si usas y verificaste ILU/ILUT
# if(EXISTS "${ILU_LIBRARY}")
#     target_link_libraries(engine PRIVATE ${ILU_LIBRARY})
#     message(STATUS "Enlazando con: ${ILU_LIBRARY}")
# endif()
# if(EXISTS "${ILUT_LIBRARY}")
#     target_link_libraries(engine PRIVATE ${ILUT_LIBRARY})
#     message(STATUS "Enlazando con: ${ILUT_LIBRARY}")
# endif()

# Copiar DLLs de DevIL al directorio de salida (para ejecución)
set(DEVIL_DLL ${DEVIL_DLL_DIR}/${DEVIL_DLL_NAME})
set(ILU_DLL ${DEVIL_DLL_DIR}/${ILU_DLL_NAME})
set(ILUT_DLL ${DEVIL_DLL_DIR}/${ILUT_DLL_NAME})

# Definir directorios de salida para Debug y Release
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Opcional: poner ejecutables en bin/
set(DEBUG_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Debug) # Ajusta si tu IDE usa otros nombres
set(RELEASE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Release) # Ajusta si tu IDE usa otros nombres

# Crear directorios si no existen (buena práctica)
file(MAKE_DIRECTORY ${DEBUG_OUTPUT_DIR})
file(MAKE_DIRECTORY ${RELEASE_OUTPUT_DIR})

if(EXISTS "${DEVIL_DLL}")
    file(COPY ${DEVIL_DLL} DESTINATION ${DEBUG_OUTPUT_DIR})
    file(COPY ${DEVIL_DLL} DESTINATION ${RELEASE_OUTPUT_DIR})
    message(STATUS "Copiando ${DEVIL_DLL_NAME} a ${DEBUG_OUTPUT_DIR} y ${RELEASE_OUTPUT_DIR}")
endif()
# Descomenta si usas y tienes ILU/ILUT DLLs
# if(EXISTS "${ILU_DLL}")
#     file(COPY ${ILU_DLL} DESTINATION ${DEBUG_OUTPUT_DIR})
#     file(COPY ${ILU_DLL} DESTINATION ${RELEASE_OUTPUT_DIR})
#     message(STATUS "Copiando ${ILU_DLL_NAME} a directorios de salida")
# endif()
# if(EXISTS "${ILUT_DLL}")
#     file(COPY ${ILUT_DLL} DESTINATION ${DEBUG_OUTPUT_DIR})
#     file(COPY ${ILUT_DLL} DESTINATION ${RELEASE_OUTPUT_DIR})
#     message(STATUS "Copiando ${ILUT_DLL_NAME} a directorios de salida")
# endif()
# --- END DEVIL CONFIGURATION ---

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
    target_link_libraries(engine PRIVATE # Cambiado a PRIVATE si solo engine lo necesita
                          ${OPENGL_LIBRARIES}
                          ${TOOLKITS_FOLDER}/glut/glut32.lib
                          ${TOOLKITS_FOLDER}/glew/glew32.lib)

    # Copiar DLLs al directorio de salida
    # NOTA: La copia de DLLs de DevIL ya se hizo arriba
    if(EXISTS "${TOOLKITS_FOLDER}/glut/glut32.dll" AND EXISTS "${TOOLKITS_FOLDER}/glew/glew32.dll")
        file(COPY ${TOOLKITS_FOLDER}/glut/glut32.dll DESTINATION ${DEBUG_OUTPUT_DIR})
        file(COPY ${TOOLKITS_FOLDER}/glut/glut32.dll DESTINATION ${RELEASE_OUTPUT_DIR})
        file(COPY ${TOOLKITS_FOLDER}/glew/glew32.dll DESTINATION ${DEBUG_OUTPUT_DIR})
        file(COPY ${TOOLKITS_FOLDER}/glew/glew32.dll DESTINATION ${RELEASE_OUTPUT_DIR})
    endif()

    # Establecer el proyecto de inicio en Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)

# Configuración para Linux y macOS
else()
    message(STATUS "Sistema operativo: Linux/macOS")

    # Buscar GLUT
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    # link_directories(${GLUT_LIBRARY_DIRS}) # target_link_libraries es preferido
    add_definitions(${GLUT_DEFINITIONS})

    if(NOT GLUT_FOUND)
        message(FATAL_ERROR "GLUT no encontrado!")
    endif()

    # Buscar GLEW (solo en Linux)
    if(NOT APPLE)
        find_package(GLEW REQUIRED)
        include_directories(${GLEW_INCLUDE_DIRS})
        # link_libraries(${GLEW_LIBRARIES}) # target_link_libraries es preferido
        target_link_libraries(engine PRIVATE ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES})
    else()
        # En macOS, GLEW no suele ser necesario o se enlaza diferente
        target_link_libraries(engine PRIVATE ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
    endif()

    # Las librerías de DevIL ya fueron enlazadas arriba con target_link_libraries
    # NO es necesario añadir ${DEVIL_LIBRARY} aquí de nuevo si ya se hizo

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
# Si generator necesita DevIL, habría que añadir target_link_libraries(generator PRIVATE ${DEVIL_LIBRARY}) aquí también
target_link_libraries(generator)

# Mensaje de finalización
message(STATUS "Configuración de CMake completada.")