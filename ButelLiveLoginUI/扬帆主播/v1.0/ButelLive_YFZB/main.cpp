#include "Login.h"
#include <QApplication>
#include <QTranslator>
#include <QFont>
#include <QLibrary>
#include "CHttpInterfaceSDK.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    typedef int (*pLoadPlugin)();
    typedef int (*pUnloadPlugin)();
    typedef int (*pCheckOut)(HWND parent);

    QLibrary updateDll( "Update.dll" );

    if(updateDll.load())
    {
        pLoadPlugin pLoadFun = (pLoadPlugin)updateDll.resolve("LoadPlugin");
        if ( pLoadFun )
        {
            int iRet = pLoadFun();
            if(!iRet)
            {
                pCheckOut pCheckFun = (pCheckOut)updateDll.resolve("CheckOutUpdate");
                if ( pCheckFun )
                {
                    pCheckFun(0);
                }
                pUnloadPlugin pUnloadFun = (pUnloadPlugin)updateDll.resolve("UnloadPlugin");
                if ( pUnloadFun )
                {
                    pUnloadFun();
                }
            }
        }
        updateDll.unload();
    }

    QObject::connect(&a, &QApplication::lastWindowClosed,[=](){
        BLiveAppReleaseChannelList();
        BLiveAppReleaseSeverNodeList();
    });
    QFont font;
    font.setFamily("微软雅黑");
    a.setFont(font);
    QTranslator translator;
    translator.load("ButelLive_CN.qm");
    a.installTranslator(&translator);
    LoginUI w;
    w.show();
    return a.exec();
}
