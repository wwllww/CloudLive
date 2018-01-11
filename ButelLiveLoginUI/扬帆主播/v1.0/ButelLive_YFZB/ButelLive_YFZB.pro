#-------------------------------------------------
#
# Project created by QtCreator 2017-10-16T15:01:31
#
#-------------------------------------------------

QT       += core gui
QT       += concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ButelLive
TEMPLATE = app

QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /debug /opt:ref


INCLUDEPATH += D:/work/comLib\
               D:/work/Log_writer\

LIBS += D:/work/ButelLive_YFZB/CNetInterfaceSDK.lib\
        D:/work/ButelLive_YFZB/ButelLiveDll.lib\

SOURCES += main.cpp\
        Login.cpp \
        ChannelAndProgram.cpp \
        MyMessageBox.cpp \
        ConfigOper.cpp \
        IniFile.cpp \
        titlebar.cpp \
        Worker.cpp \
        ChannelItem.cpp

HEADERS  += Login.h \
         ChannelAndProgram.h \
         MyMessageBox.h \
         ConfigOper.h \
         titlebar.h \
         Worker.h \
         ChannelItem.h

FORMS    += Login.ui \
         ChannelAndProgram.ui \
         MyMessageBox.ui
RESOURCES += \
    resource.qrc

RC_FILE = myapp.rc

TRANSLATIONS    +=  ButelLive_CN.ts
