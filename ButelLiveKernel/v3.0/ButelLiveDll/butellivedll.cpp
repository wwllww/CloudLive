#include "ButelLiveDll.h"
#include "ButelLive.h"
#include <QTranslator>
int CreatButelLiveInstance(bool bChat,ChannelInfo* channelInfoList,int channelCount,int currentChannelIndex)
{
    if(!channelInfoList || channelCount <= 0 || currentChannelIndex < 0)
    {
        return -1;
    }

    QTranslator* translator = new QTranslator;
    translator->load("ButelLiveDll_CN.qm");
    qApp->installTranslator(translator);

    ButelLive* pButelLiveUi = new ButelLive(bChat,channelInfoList,channelCount,currentChannelIndex);
    pButelLiveUi->showMaximized();
    pButelLiveUi->SLiveApiInit();
    return 0;
}
