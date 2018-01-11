#include "Worker.h"
#include "CHttpInterfaceSDK.h"

#include <QThread>
extern ChannelInfo* g_pChannelList;
extern int          g_channelCount;

Worker::Worker(QObject *parent) : QObject(parent)
{

}

void Worker::process()
{
    void* thread_id = QThread::currentThreadId();
    BLiveAppGetChannelList(&g_pChannelList,&g_channelCount);
    emit this->finished();
}
