# Compilador
CXX = g++

# Opciones de compilación
CXXFLAGS = -Wall -std=c++11

# Nombre de los ejecutables
GENERATOR_TARGET = generator
ENGINE_TARGET = engine

# Archivos fuente
GENERATOR_SRCS = generator.cpp
ENGINE_SRCS = engine.cpp

# Archivos objeto (se generan automáticamente)
GENERATOR_OBJS = $(GENERATOR_SRCS:.cpp=.o)
ENGINE_OBJS = $(ENGINE_SRCS:.cpp=.o)

# Bibliotecas necesarias para el engine (OpenGL, GLUT, TinyXML)
LIBS = -lGL -lGLU -lglut -ltinyxml

# Regla por defecto (compila ambos ejecutables)
all: $(GENERATOR_TARGET) $(ENGINE_TARGET)

# Regla para compilar el generador
$(GENERATOR_TARGET): $(GENERATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $(GENERATOR_TARGET) $(GENERATOR_OBJS)

# Regla para compilar el engine
$(ENGINE_TARGET): $(ENGINE_OBJS)
	$(CXX) $(CXXFLAGS) -o $(ENGINE_TARGET) $(ENGINE_OBJS) $(LIBS)

# Regla para compilar cada archivo .cpp en un archivo .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regla para limpiar los archivos generados
clean:
	rm -f $(GENERATOR_OBJS) $(ENGINE_OBJS) $(GENERATOR_TARGET) $(ENGINE_TARGET)

# Indica que "clean" no es un archivo
.PHONY: clean