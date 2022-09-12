TEMPLATE = lib
CONFIG -= qt

CONFIG += -std=c99
QMAKE_CFLAGS += -std=c99

DEFINES += Heaven-Server

DESTDIR = $$PWD/../../build

QMAKE_CXXFLAGS_DEBUG *= -O
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Heaven-Common.h \
    Heaven-Server.h

SOURCES += \
    Heaven-Common.c \
    Heaven-Server.c

