#-------------------------------------------------
#
# Project created by QtCreator 2015-05-09T16:41:37
#
#-------------------------------------------------

QT       += core
QT       += network
QT       -= gui

QMAKE_CXXFLAGS += -std=c++0x

TARGET = libwebsocket-test

DESTDIR = release

OBJECTS_DIR=bin

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
           ClientSocketHandler.cpp

HEADERS += ClientSockethandler.h

INCLUDEPATH += $$PWD/../lib

QMAKE_CLEAN += -r $${PWD}/$${DESTDIR}

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/externalLib/release/ -lwebsocket
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/externalLib/debug/ -lwebsocket
else:unix: LIBS += -L$$PWD/externalLib/ -lwebsocket

INCLUDEPATH += $$PWD/externalLib
DEPENDPATH += $$PWD/externalLib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/externalLib/release/ -lhttpdecoder
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/externalLib/debug/ -lhttpdecoder
else:unix: LIBS += -L$$PWD/externalLib/ -lhttpdecoder

INCLUDEPATH += $$PWD/externalLib
DEPENDPATH += $$PWD/externalLib
