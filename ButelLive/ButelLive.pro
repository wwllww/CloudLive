#-------------------------------------------------
#
# Project created by QtCreator 2016-08-24T09:30:35
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ButelLive
TEMPLATE = app

QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /debug /opt:ref

INCLUDEPATH += D:/work/comLib\
               D:/work/Log_writer\

LIBS += D:/work/ButelLive/UltraliveApi.lib\
        D:/work/ButelLive/Log_writer.lib\

SOURCES += main.cpp\
        ButelLive.cpp \
    titlebar.cpp \
    pgmdisply.cpp \
    previewlabel.cpp \
    InputName.cpp \
    SceneItem.cpp \
    MyMessageBox.cpp \
    MaterialItem.cpp \
    Login.cpp \
    ConfigOper.cpp \
    CNetInerface.cpp \
    IniFile.cpp \
    ChannelAndProgram.cpp \
    PreviewItem.cpp \
    cslider.cpp \
    PlayControl.cpp \
    previewitemdisarea.cpp \
    SettingUI.cpp \
    SceneMateralItem.cpp \
    PositionSizeSet.cpp \
    LiveDislplayArea.cpp \
    SetShortcutKey.cpp

HEADERS  += ButelLive.h \
    titlebar.h \
    pgmdisplay.h \
    previewlabel.h \
    InputName.h \
    SceneItem.h \
    MyMessageBox.h \
    MaterialItem.h \
    Login.h \
    ConfigOper.h \
    CNetInerface.h \
    ChannelAndProgram.h \
    PreviewItem.h \
    cslider.h \
    PlayControl.h \
    previewitemdisarea.h \
    SettingUI.h \
    SceneMateralItem.h\
    PositionSizeSet.h \
    LiveDislplayArea.h \
    SetShortcutKey.h

FORMS    += ButelLive.ui \
    InputName.ui \
    MyMessageBox.ui \
    Login.ui \
    ChannelAndProgram.ui \
    SettingUI.ui \
    PositionSizeSet.ui \
    SetShortcutKey.ui

RESOURCES += \
    resource.qrc

RC_FILE = myapp.rc

TRANSLATIONS    +=  ButelLive_zh.ts
