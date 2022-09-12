TEMPLATE = subdirs
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += ordered

CONFIG += -std=c99
QMAKE_CFLAGS += -std=c99

SUBDIRS = lib/Heaven-Server.pro \
          lib/Heaven-Client.pro \
          lib/Heaven-Compositor.pro\
          examples/Examples.pro
