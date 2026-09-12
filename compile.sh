qmake -project "QT+=core gui widgets opengl openglwidgets" "QMAKE_CXXFLAGS+= -fopenmp -Wall" "LIBS += -lGL -lGLU -fopenmp"
qmake
make
