#-------------------------------------------------
#
# Project created by QtCreator 2017-07-31T14:47:11
#
#-------------------------------------------------

QT       += network

QT       += core gui


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CNetInterfaceSDK
TEMPLATE = lib

QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /debug /opt:ref

DEFINES += CNETINTERFACESDK_LIBRARY \
           CNETINTERFACEAPI_EXPORTS

INCLUDEPATH += D:/work/Log_writer\

SOURCES += CNetInterfaceSDK.cpp \
    CHttpInterfaceSDK.cpp \
    StopLiveConfirm.cpp \
    titlebar.cpp


HEADERS += CNetInterfaceSDK.h \
    CHttpInterfaceSDK.h \
    StopLiveConfirm.h \
    titlebar.h

FORMS    += StopLiveConfirm.ui

LIBS += D:/work/CNetInterfaceSDK/Log_writer.lib

TRANSLATIONS    +=  CNetInterfaceSDK_CN.ts

