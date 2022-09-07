TEMPLATE = lib
CONFIG -= qt

CONFIG += -std=c99
QMAKE_CFLAGS += -std=c99

DEFINES += Heaven-Server

DESTDIR = $$PWD/../../../build

QMAKE_CXXFLAGS_DEBUG *= -O
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#LIBS += -L/usr/local/lib/x86_64-linux-gnu -lwayland-server -lEGL -lGL -lGLESv2 -linput -ludev -lpthread -lXcursor -lxkbcommon -lSOIL
#INCLUDEPATH += /usr/include/libdrm ./classes ./classes/private ./globals/Wayland ./globals/XdgShell /usr/local/include


# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../common/Heaven-Common.h \
    Heaven-Server-Private.h \
    Heaven-Server.h

SOURCES += \
    ../common/Heaven-Common.c \
    Heaven-Server-Private.c \
    Heaven-Server.c

