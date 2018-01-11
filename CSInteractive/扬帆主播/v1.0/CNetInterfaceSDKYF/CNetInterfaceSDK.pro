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
    CurLiveUI.cpp \
    CreateLiveUI.cpp \
    titlebar.cpp \
    StopLiveConfirm.cpp \
    MyMessageBox.cpp

HEADERS += CNetInterfaceSDK.h \
    CHttpInterfaceSDK.h \
    CurLiveUI.h \
    CreateLiveUI.h \
    titlebar.h \
    StopLiveConfirm.h \
    MyMessageBox.h

FORMS    += CurLiveUI.ui \
         CreateLiveUI.ui \
         StopLiveConfirm.ui \
         MyMessageBox.ui

LIBS += D:/work/CNetInterfaceSDKYF/Log_writer.lib \
        D:/work/CNetInterfaceSDKYF/RedCDNUploadSDK.lib

RESOURCES += \
    resource.qrc

TRANSLATIONS    +=  CNetInterfaceSDK_CN.ts


