#-------------------------------------------------
#
# Project created by QtCreator 2016-08-24T09:30:35
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ButelLiveDll
TEMPLATE = lib

QMAKE_CFLAGS_RELEASE += -MD \
                         MT

QMAKE_CXXFLAGS_RELEASE += -MD \
                           MT

DEFINES += CBUTELLIVEDLLAPI_EXPORTS \
           CNETZTV


QMAKE_LFLAGS_RELEASE += /MAP
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /debug /opt:ref

INCLUDEPATH += D:/work/comLib\
               D:/work/Log_writer\

LIBS += D:/work/ButelLiveDll/UltraliveApi.lib\
        D:/work/ButelLiveDll/Log_writer.lib\
        D:/work/ButelLiveDll/CNetInterfaceSDK.lib

SOURCES +=\
        ButelLive.cpp \
    titlebar.cpp \
    pgmdisply.cpp \
    previewlabel.cpp \
    InputName.cpp \
    SceneItem.cpp \
    MyMessageBox.cpp \
    MaterialItem.cpp \
    ConfigOper.cpp \
    IniFile.cpp \
    PreviewItem.cpp \
    cslider.cpp \
    PlayControl.cpp \
    previewitemdisarea.cpp \
    SettingUI.cpp \
    SceneMateralItem.cpp \
    PositionSizeSet.cpp \
    LiveDislplayArea.cpp \
    SetShortcutKey.cpp \
    SetLogLevelUI.cpp \
    PushStreamDelayUI.cpp \
    SDIConfigureUI.cpp \
    SDIConfigureItem.cpp \
    BlackMagicItem.cpp \
    VideoListUI.cpp \
    SeniorCodeSetUI.cpp \
    AboutUI.cpp \
    ButelLiveDll.cpp \
    MessageInfoUI.cpp \
    CMessageBox.cpp
    

HEADERS  += ButelLive.h \
    titlebar.h \
    pgmdisplay.h \
    previewlabel.h \
    InputName.h \
    SceneItem.h \
    MyMessageBox.h \
    MaterialItem.h \
    ConfigOper.h \
    PreviewItem.h \
    cslider.h \
    PlayControl.h \
    previewitemdisarea.h \
    SettingUI.h \
    SceneMateralItem.h\
    PositionSizeSet.h \
    LiveDislplayArea.h \
    SetShortcutKey.h \
    SetLogLevelUI.h \
    PushStreamDelayUI.h \
    SDIConfigureUI.h \
    SDIConfigureItem.h \
    BlackMagicItem.h \
    VideoListUI.h \
    SeniorCodeSetUI.h \
    ButelLiveDll.h \
    AboutUI.h \
    MessageInfoUI.h \
    CMessageBox.h

FORMS    += ButelLive.ui \
    InputName.ui \
    MyMessageBox.ui \
    SettingUI.ui \
    PositionSizeSet.ui \
    SetShortcutKey.ui \
    SetLogLevelUI.ui \
    PushStreamDelayUI.ui \
    SDIConfigureUI.ui \
    VideoListUI.ui \
    SeniorCodeSetUI.ui \
    AboutUI.ui \
    MessageInfoUI.ui \
    CMessageBox.ui

RESOURCES += \
    resource.qrc

RC_FILE = myapp.rc

TRANSLATIONS    +=  ButelLiveDll_CN.ts
