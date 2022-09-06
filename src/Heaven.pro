TEMPLATE = subdirs
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += ordered

CONFIG += -std=c99
QMAKE_CFLAGS += -std=c99

SUBDIRS = lib/server/Heaven-Server.pro \
          lib/client/Heaven-Client.pro \
          examples/Examples.pro
