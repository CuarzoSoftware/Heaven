TEMPLATE = app
CONFIG += console c++17
CONFIG -= qt

DESTDIR = $$PWD/../../../build

QMAKE_CXXFLAGS_DEBUG *= -O
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

LIBS += -L$$PWD/../../../build -lHeaven-Compositor

QMAKE_LFLAGS += -Wl,-rpath=.

TARGET = Compositor

SOURCES += \
    main.c
