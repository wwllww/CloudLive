#include "previewlabel.h"
#include <QLabel>
#include <QMouseEvent>
#include <QDialog>
#include <QDebug>
#include "LogDeliver.h"
#include "configoper.h"

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif

enum MousePos
{
    InLeft = 0,
    InTop,
    InRight,
    InBottom,
    InLeftTop,
    InRightTop,
    InLeftBottom,
    InRightBottom,
    InCenter,
    InOut
};

PreviewLabel::PreviewLabel(QDialog *parent) :
    QLabel(parent)
{
    m_Parent = (ButelLive*)parent;
    setAttribute(Qt::WA_PaintOnScreen,true);
    setMouseTracking(true);
    m_CurrentLevel = -1;


    QAction* pActMoveUp = new QAction(tr("move up"),this);
    connect(pActMoveUp,SIGNAL(triggered(bool)),parent,SLOT(on_m_pBtnScnMtrlMoveUp_clicked()));

    QAction* pActMoveDown = new QAction(tr("move down"),this);
    connect(pActMoveDown,SIGNAL(triggered(bool)),parent,SLOT(on_m_pBtnScnMtrlMoveDown_clicked()));

    QAction* pActMoveTop = new QAction(tr("move top"),this);
    connect(pActMoveTop,SIGNAL(triggered(bool)),parent,SLOT(on_m_pBtnScnMtrlMoveTop_clicked()));

    QAction* pActMoveBottom = new QAction(tr("move bottom"),this);
    connect(pActMoveBottom,SIGNAL(triggered(bool)),parent,SLOT(on_m_pBtnScnMtrlMoveBottom_clicked()));

    m_EditMenu.setStyleSheet(m_Parent->m_MenuSceneEdit.styleSheet());
    m_EditMenu.clear();
    m_EditMenu.addAction(pActMoveUp);
    m_EditMenu.addAction(pActMoveDown);
    m_EditMenu.addAction(pActMoveTop);
    m_EditMenu.addAction(pActMoveBottom);

    m_bIsMove = false;
}

void PreviewLabel::mousePressEvent(QMouseEvent *event)
{
    if(!m_Parent->m_bIsPreview && !m_Parent->m_bIsLiving && !m_Parent->m_bIsRecording)
        return;
    if(m_Parent->GetSceneCurrentRow() < 0) return;
    m_bIsMove = false;
    if(event->buttons() == Qt::LeftButton)
    {
        if(!m_Parent->m_bIsEditing) return;
        m_bIsCut = false;
        m_bPressed = true;
        Vect v;
        QPoint gPos  = event->globalPos();
        QPoint realPos = this->mapFromGlobal(gPos);   //鼠标当前相对于窗体的坐标
        m_pStart = realPos;                           // 记录鼠标按下位置
        v.x = realPos.x();
        v.y = realPos.y();
        m_vStart = m_Parent->MapWindowToFramePos(v);

        int iCurScene = m_Parent->GetSceneCurrentRow();

        if(m_Parent->m_SelLevel >= 0 && m_Parent->m_SelLevel < m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count())
        {
           int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].id;
           bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].bIsAgent;
           if(bIsAgent)
           {
               SLiveSelectStream(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,false);
           }
           else
           {
               SLiveSelectStream(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,false);
           }
        }

        qDebug() << m_Parent->m_SelLevel;
        for(int i = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count() - 1; i >= 0; i--)
        {
            if(!m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bRender)
                continue;
            if(i > m_Parent->m_SelLevel && m_Parent->m_SelLevel != -1)
                continue;
            int streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
            int streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].height;

            int left = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].left;
            int top = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].top;

            if(left <  m_vStart.x && m_vStart.x < left + 10 && top+10 <  m_vStart.y && m_vStart.y < top + streamH-10)
                mousePos = InLeft;
            else if(left+streamW-10 < m_vStart.x && m_vStart.x < left+streamW && top+10 <  m_vStart.y && m_vStart.y < top + streamH-10)
                mousePos = InRight;
            else if(left+10 < m_vStart.x && m_vStart.x < left+streamW-10 && top <  m_vStart.y && m_vStart.y < top + 10)
                mousePos  = InTop;
            else if(left+10 < m_vStart.x && m_vStart.x < left+streamW-10 && top+ streamH-10 <  m_vStart.y && m_vStart.y < top + streamH)
                mousePos  = InBottom;
            else if(left <  m_vStart.x && m_vStart.x < left + 10 && top <  m_vStart.y && m_vStart.y < top + 10)
                mousePos = InLeftTop;
            else if(left+streamW-10 < m_vStart.x && m_vStart.x < left+streamW && top <  m_vStart.y && m_vStart.y < top + 10)
                mousePos = InRightTop;
            else if(left <  m_vStart.x && m_vStart.x < left + 10 && top+ streamH-10 <  m_vStart.y && m_vStart.y < top + streamH)
                mousePos = InLeftBottom;
            else if(left+streamW-10 < m_vStart.x && m_vStart.x < left+streamW && top+ streamH-10 <  m_vStart.y && m_vStart.y < top + streamH)
                mousePos = InRightBottom;
            else if(left+10 < m_vStart.x && m_vStart.x < left+streamW-10 && top+10 <  m_vStart.y && m_vStart.y < top + streamH-10)
                mousePos = InCenter;
            else
            {
                mousePos = InOut;
                m_CurrentLevel = -1;
                m_Parent->m_SelLevel = -1;
                if(0 == i)
                   m_Parent->SetSelectIndexSource(-1);
                continue;
            }
            qDebug() << "mousePos = " << mousePos << "m_CurrentLevel = " << m_CurrentLevel;
            if(left <  m_vStart.x && m_vStart.x < left + streamW)
                bX = true;
            if(top <  m_vStart.y && m_vStart.y < top + streamH)
                bY = true;
            if(bX && bY)
            {
                m_CurrentLevel = i;
                m_Parent->m_SelLevel = i;
                m_Parent->SetSelectIndexSource(i);
                int iCurScene = m_Parent->GetSceneCurrentRow();
                int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].id;

                bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsAgent;
                m_bIsCut = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsCut;
                if(bIsAgent)
                {
                    SLiveSelectStream(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,true);
                }
                else
                {
                    SLiveSelectStream(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,true);
                }
                m_streamPos = QPoint(m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].left,m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].top);
                m_streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
                m_streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].height;
                m_streamCutPos = QPoint(m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutLeft,m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutTop);
                m_streamCutW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutWidth;
                m_streamCutH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutHeight;
                m_bIsMove = false;
                break;
            }
        }
    }
    else if(event->buttons() == Qt::RightButton)
    {
        Vect v;
        QPoint gPos  = event->globalPos();
        QPoint realPos = this->mapFromGlobal(gPos);   //鼠标当前相对于窗体的坐标
        m_pStart = realPos;                           // 记录鼠标按下位置
        v.x = realPos.x();
        v.y = realPos.y();
        m_vStart = m_Parent->MapWindowToFramePos(v);

        if(m_Parent->m_bIsEditing)
        {
            int iCurScene = m_Parent->GetSceneCurrentRow();
            if(m_Parent->m_SelLevel >= 0 && m_Parent->m_SelLevel < m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count())
            {
               int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].id;
               bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].bIsAgent;
               if(bIsAgent)
               {
                   SLiveSelectStream(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,false);
               }
               else
               {
                   SLiveSelectStream(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,false);
               }
            }
        }

        int iCurScene = m_Parent->GetSceneCurrentRow();
        int iNum = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count();

        for(int i = iNum - 1; i >= 0; i--)
        {
            if(!m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bRender)
             {
                if(0 == i)
                {
                    m_Parent->m_SelLevel = -1;
                    m_Parent->SetSelectSourceIndex(-1);
                    m_Parent->m_PreviewMenu.exec(QCursor::pos());
                    break;
                }
                else
                {
                    continue;
                }
            }
            int streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
            int streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].height;
            int left = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].left;
            int top = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].top;
            bool bx = false;
            bool by = false;
            if(left <  m_vStart.x && m_vStart.x < left + streamW)
                bx = true;
            if(top <  m_vStart.y && m_vStart.y < top + streamH)
                by = true;
            if(bx && by/* && m_Parent->m_SelLevel ==  i*/)
            {
                m_Parent->m_SelLevel = i;
                m_Parent->SetSelectSourceIndex(iNum - i - 1);
                if(m_Parent->m_bIsEditing)
                {
                    int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].id;
                    bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].bIsAgent;
                    if(bIsAgent)
                    {
                        SLiveSelectStream(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,true);
                    }
                    else
                    {
                        SLiveSelectStream(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,true);
                    }
                }
                m_Parent->m_PreviewInSourceMenu.exec(QCursor::pos());
                break;
            }
            else if(0 == i && (!bx || !by))
            {
                m_Parent->m_SelLevel = -1;
                m_Parent->SetSelectSourceIndex(-1);
                m_Parent->m_PreviewMenu.exec(QCursor::pos());
                break;
            }
        }
    }
    else
    {
        event->ignore();
    }
}

void PreviewLabel::SetSourceCropData()
{
//    if(m_bPressed)
//    {
        if(m_CurrentLevel == -1) return;
        LinkSourceInfo streamInfo;
        int iCurScene = m_Parent->GetSceneCurrentRow();
        streamInfo = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel];
        qDebug() << "*********************************************";
        qDebug() << m_streamPos.x() << m_streamPos.y() << m_streamW << m_streamH;
        qDebug() << m_streamCutPos.x() << m_streamCutPos.y() << m_streamCutW << m_streamCutH;
        streamInfo.left = m_streamPos.x();
        streamInfo.top = m_streamPos.y();
        streamInfo.width = m_streamW;
        streamInfo.height = m_streamH;

        streamInfo.cutLeft = m_streamCutPos.x();
        streamInfo.cutTop = m_streamCutPos.y();
        streamInfo.cutWidth = m_streamCutW;
        streamInfo.cutHeight = m_streamCutH;

        m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.replace(m_CurrentLevel,streamInfo);
//    }

}

void PreviewLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_Parent->m_bIsEditing) return;
    bX = false;
    bY = false;
    m_Parent->m_bAltKeyPressed = false;

    if(m_bPressed)
    {
        m_bPressed = false;
        if(m_CurrentLevel != -1)
        {
            LinkSourceInfo streamInfo;
            int iCurScene = m_Parent->GetSceneCurrentRow();
            streamInfo = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel];
            streamInfo.left = m_streamPos.x();
            streamInfo.top = m_streamPos.y();
            streamInfo.width = m_streamW;
            streamInfo.height = m_streamH;

            streamInfo.cutLeft = m_streamCutPos.x();
            streamInfo.cutTop = m_streamCutPos.y();
            streamInfo.cutWidth = m_streamCutW;
            streamInfo.cutHeight = m_streamCutH;
//            if(!streamInfo.bIsCut)
                streamInfo.bIsCut = m_bIsCut;
//            streamInfo.bIsCut = m_bIsCut;
            qDebug() << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
            qDebug() << "streamInfo.left top width height" << streamInfo.left << streamInfo.top << streamInfo.width << streamInfo.height;
            qDebug() << "streamInfo.cutLeft cutTop cutWidth cutHeight" << streamInfo.cutLeft << streamInfo.cutTop << streamInfo.cutWidth << streamInfo.cutHeight;

            m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.replace(m_CurrentLevel,streamInfo);
            m_CurrentLevel = -1;
            m_bIsCut = false;
        }
        if(!m_bIsMove)
        {
            int iCurScene = m_Parent->GetSceneCurrentRow();
            if(m_Parent->m_SelLevel >= 0 && m_Parent->m_SelLevel < m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count())
            {
               int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].id;
               bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].bIsAgent;
               if(bIsAgent)
               {
                   SLiveSelectStream(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,false);
               }
               else
               {
                   SLiveSelectStream(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,false);
               }
            }
            for(int i = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count() - 1; i >= 0; i--)
            {
                if(!m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bRender) continue;

                int streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
                int streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].height;

                int left = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].left;
                int top = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].top;

                if(left <  m_vStart.x && m_vStart.x < left + 10 && top+10 <  m_vStart.y && m_vStart.y < top + streamH-10)
                    mousePos = InLeft;
                else if(left+streamW-10 < m_vStart.x && m_vStart.x < left+streamW && top+10 <  m_vStart.y && m_vStart.y < top + streamH-10)
                    mousePos = InRight;
                else if(left+10 < m_vStart.x && m_vStart.x < left+streamW-10 && top <  m_vStart.y && m_vStart.y < top + 10)
                    mousePos  = InTop;
                else if(left+10 < m_vStart.x && m_vStart.x < left+streamW-10 && top+ streamH-10 <  m_vStart.y && m_vStart.y < top + streamH)
                    mousePos  = InBottom;
                else if(left <  m_vStart.x && m_vStart.x < left + 10 && top <  m_vStart.y && m_vStart.y < top + 10)
                    mousePos = InLeftTop;
                else if(left+streamW-10 < m_vStart.x && m_vStart.x < left+streamW && top <  m_vStart.y && m_vStart.y < top + 10)
                    mousePos = InRightTop;
                else if(left <  m_vStart.x && m_vStart.x < left + 10 && top+ streamH-10 <  m_vStart.y && m_vStart.y < top + streamH)
                    mousePos = InLeftBottom;
                else if(left+streamW-10 < m_vStart.x && m_vStart.x < left+streamW && top+ streamH-10 <  m_vStart.y && m_vStart.y < top + streamH)
                    mousePos = InRightBottom;
                else if(left+10 < m_vStart.x && m_vStart.x < left+streamW-10 && top+10 <  m_vStart.y && m_vStart.y < top + streamH-10)
                    mousePos = InCenter;
                else
                {
                    mousePos = InOut;
                    m_CurrentLevel = -1;
                    m_Parent->m_SelLevel = -1;
                    continue;
                }
                qDebug() << "mousePos = " << mousePos << "m_CurrentLevel = " << m_CurrentLevel;
                if(left <  m_vStart.x && m_vStart.x < left + streamW)
                    bX = true;
                if(top <  m_vStart.y && m_vStart.y < top + streamH)
                    bY = true;
                if(bX && bY)
                {
                    m_CurrentLevel = i;
                    m_Parent->m_SelLevel = i;
                    m_Parent->SetSelectIndexSource(i);
                    int iCurScene = m_Parent->GetSceneCurrentRow();
                    int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].id;

                    bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsAgent;
                    if(bIsAgent)
                    {
                        SLiveSelectStream(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,true);
                    }
                    else
                    {
                        SLiveSelectStream(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,true);
                    }
                    m_streamPos = QPoint(m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].left,m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].top);
                    m_streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
                    m_streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].height;
                    m_streamCutPos = QPoint(m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutLeft,m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutTop);
                    m_streamCutW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutWidth;
                    m_streamCutH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].cutHeight;
                    break;
                }
            }
        }
    }
    else
    {
        event->ignore();
    }
}
void PreviewLabel::mouseMoveEvent(QMouseEvent *event)
{
    if(!m_Parent->m_bIsEditing) return;
    int iCurScene = m_Parent->GetSceneCurrentRow();
    if(iCurScene < 0/* || !m_Parent->m_bIsPreview || m_Parent->m_SelLevel < 0*/) return;
    if(!m_Parent->m_bIsPreview && !m_Parent->m_bIsLiving && !m_Parent->m_bIsRecording) return;
    if(m_Parent->m_SelLevel < 0) return;
    QPoint gPos  = event->globalPos();
    QPoint realPos = this->mapFromGlobal(gPos);
    Vect v;
    v.x = realPos.x();
    v.y = realPos.y();
    m_vEnd = m_Parent->MoveMapWindowToFramePos(v);
    int streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].width;
    int streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].height;
    int left = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].left;
    int top = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_Parent->m_SelLevel].top;
    Vect vOffset;
    vOffset.x = m_Parent->renderFrameOffsetx;
    vOffset.y = m_Parent->renderFrameOffsety;
    vOffset = m_Parent->MoveMapWindowToFramePos(vOffset);
    m_vEnd.x -= vOffset.x;
    m_vEnd.y -= vOffset.y;
    if(left <  m_vEnd.x && m_vEnd.x < left+ 10 && top+10 <  m_vEnd.y && m_vEnd.y < top + streamH-10)
        setCursor(Qt::SizeHorCursor);
    else if(left+streamW-10 < m_vEnd.x && m_vEnd.x < left+streamW && top+10 <  m_vEnd.y && m_vEnd.y < top + streamH-10)
        setCursor(Qt::SizeHorCursor);
    else if(left+10 < m_vEnd.x && m_vEnd.x < left+streamW-10 && top <  m_vEnd.y && m_vEnd.y < top + 10)
        setCursor(Qt::SizeVerCursor);
    else if(left+10 < m_vEnd.x && m_vEnd.x < left+streamW-10 && top+ streamH-10 <  m_vEnd.y && m_vEnd.y < top + streamH)
        setCursor(Qt::SizeVerCursor);
    else if(left <  m_vEnd.x && m_vEnd.x < left + 10 && top <  m_vEnd.y && m_vEnd.y < top + 10)
        setCursor(Qt::SizeFDiagCursor);
    else if(left+streamW-10 < m_vEnd.x && m_vEnd.x < left+streamW && top <  m_vEnd.y && m_vEnd.y < top + 10)
        setCursor(Qt::SizeBDiagCursor);
    else if(left <  m_vEnd.x && m_vEnd.x < left + 10 && top+ streamH-10 <  m_vEnd.y && m_vEnd.y < top + streamH)
        setCursor(Qt::SizeBDiagCursor);
    else if(left+streamW-10 < m_vEnd.x && m_vEnd.x < left+streamW && top+ streamH-10 <  m_vEnd.y && m_vEnd.y < top + streamH)
        setCursor(Qt::SizeFDiagCursor);
    else if(left+10 < m_vEnd.x && m_vEnd.x < left+streamW-10 && top+10 <  m_vEnd.y && m_vEnd.y < top + streamH-10)
        setCursor(Qt::SizeAllCursor);
    else
        setCursor(Qt::ArrowCursor);
    if(event->buttons() == Qt::LeftButton)
    {
        if(m_bPressed)
        {
            if(m_CurrentLevel == -1) return;
            Vect v;
            QPoint gPos  = event->globalPos();
            QPoint realPos = this->mapFromGlobal(gPos);
            m_pEnd = realPos;
            int iCurScene = m_Parent->GetSceneCurrentRow();
            int streamW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].width;
            int streamH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].height;
            int left = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].left;
            int top = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].top;
            int streamCutW = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].cutWidth;
            int streamCutH = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].cutHeight;
            int CutLeft = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].cutLeft;
            int CutTop = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].cutTop;
            m_bIsCut = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bIsCut;
            v.x = realPos.x()- m_pStart.x();
            v.y = realPos.y()- m_pStart.y();
            Vect vEnd = m_Parent->MoveMapWindowToFramePos(v);
            int moveW = vEnd.x;
            int moveH = vEnd.y;
            VideoArea Area;
            Area.left = left;
            Area.top = top;
            Area.width = streamW;
            Area.height = streamH;
            Area.CropLeft = left - CutLeft;
            Area.CropTop = top - CutTop;
            Area.CropRight = (streamCutW-streamW)-(left-CutLeft);
            Area.CropBottom = (streamCutH-streamH)-(top-CutTop);
            VideoArea AreaCut;
            AreaCut.left = CutLeft;
            AreaCut.top = CutTop;
            AreaCut.width = streamCutW;
            AreaCut.height = streamCutH;
            AreaCut.CropLeft = left - CutLeft;
            AreaCut.CropTop = top - CutTop;
            AreaCut.CropRight = (streamCutW-streamW)-(left-CutLeft);
            AreaCut.CropBottom = (streamCutH-streamH)-(top-CutTop);
            float scal,scalCrop;
            UINT streamWidth,streamHeight;
            int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].id;
            bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bIsAgent;
            bool bIsCut = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bIsCut;
            if(!m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bRender)
                return;
            if(bIsAgent)
            {
                if(SLiveGetStreamSize(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,&streamWidth,&streamHeight) < 0)
                {
                    return;
                }
                else
                {
                    scal = (float)streamWidth/streamHeight; //源的原始比例
                }
            }
            else
            {
                if(SLiveGetStreamSize(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,&streamWidth,&streamHeight) < 0)
                {
                    return;
                }
                else
                {
                    scal = (float)streamWidth/streamHeight;
                }
            }
            if(bIsCut)
                scalCrop = (float)streamW/streamH;  //剪裁后的比例
                //scalCrop = scal;
            else
                scalCrop = scal;

            if(mousePos == InRight)
            {
                if(streamW+moveW <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.width = streamW + moveW;
                    AreaCut.width = streamCutW + moveW;
                    Area.height = Area.width/scalCrop;
                    AreaCut.height = AreaCut.width/scal;
                    qDebug() << Area.width << AreaCut.width << Area.height << AreaCut.height << scal << scalCrop;
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.width = streamW + moveW;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.width = streamW + moveW;
                    AreaCut.width = streamCutW + moveW;
                }
                setCursor(Qt::SizeHorCursor);
            }
            else if(mousePos == InLeft)
            {
                if(streamW-moveW <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.left += moveW;
                    Area.width = streamW - moveW;
                    AreaCut.left += moveW;
                    AreaCut.width = streamCutW - moveW;
                    Area.height = Area.width/scalCrop;
                    AreaCut.height = AreaCut.width/scal;
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.left += moveW;
                    Area.width = streamW - moveW;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.left += moveW;
                    Area.width = streamW - moveW;
                    AreaCut.left += moveW;
                    AreaCut.width = streamCutW - moveW;
                }
                setCursor(Qt::SizeHorCursor);
            }
            else if(mousePos == InBottom)
            {
                if(streamH+moveH <= 20)
                    return;
                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.height = streamH + moveH;
                    AreaCut.height = streamCutH + moveH;
                    Area.width = Area.height*scalCrop;
                    AreaCut.width = AreaCut.height*scal;
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.height = streamH + moveH;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.height = streamH + moveH;
                    AreaCut.height = streamCutH + moveH;
                }
                setCursor(Qt::SizeVerCursor);
            }
            else if(mousePos == InTop)
            {
                if(streamH-moveH <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.top += moveH;
                    Area.height = streamH - moveH;
                    AreaCut.top += moveH;
                    AreaCut.height = streamCutH - moveH;
                    Area.width = Area.height*scalCrop;
                    AreaCut.width = AreaCut.height*scal;
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.top += moveH;
                    Area.height = streamH - moveH;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.top += moveH;
                    Area.height = streamH - moveH;
                    AreaCut.top += moveH;
                    AreaCut.height = streamCutH - moveH;
                }
                setCursor(Qt::SizeVerCursor);
            }
            else if(mousePos == InLeftTop)
            {
                if(streamW-moveW <= 20 || streamH-moveH <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.left += moveW;
                    Area.width = streamW - moveW;

                    AreaCut.left += moveW;
                    AreaCut.width = streamCutW - moveW;
                    Area.height = Area.width/scalCrop;
                    AreaCut.height = AreaCut.width/scal;

                    Area.top = Area.top -(Area.height - streamH);
                    AreaCut.top = AreaCut.top -(AreaCut.height - streamCutH);
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.left += moveW;
                    Area.top += moveH;
                    Area.width = streamW - moveW;
                    Area.height = streamH - moveH;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.left += moveW;
                    Area.width = streamW - moveW;
                    AreaCut.left += moveW;
                    AreaCut.width = streamCutW - moveW;
                    Area.height = streamH - moveH;
                    AreaCut.height = streamCutH - moveH;

                    Area.top = Area.top -(Area.height - streamH);
                    AreaCut.top = AreaCut.top -(AreaCut.height - streamCutH);
                }

                setCursor(Qt::SizeFDiagCursor);
            }
            else if(mousePos == InLeftBottom)
            {
                if(streamW-moveW <= 20 || streamH+moveH <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.width = streamW - moveW;
                    Area.left += moveW;
                    AreaCut.width = streamCutW - moveW;
                    AreaCut.left += moveW;
                    Area.height = Area.width/scalCrop;
                    AreaCut.height = AreaCut.width/scal;
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.left += moveW;
                    Area.width = streamW - moveW;
                    Area.height = Area.width/scalCrop;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.width = streamW - moveW;
                    Area.left += moveW;
                    AreaCut.width = streamCutW - moveW;
                    AreaCut.left += moveW;
                    Area.height = streamH + moveH;
                    AreaCut.height = streamCutH + moveH;
                }

                setCursor(Qt::SizeBDiagCursor);
            }
            else if(mousePos == InRightTop)
            {
                if(streamW+moveW <= 20 || streamH-moveH <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {

                    Area.width = streamW + moveW;
                    AreaCut.width = streamCutW + moveW;
                    Area.height = Area.width/scalCrop;
                    AreaCut.height = AreaCut.width/scal;

                    Area.top = Area.top - (Area.height - streamH);
                    AreaCut.top = AreaCut.top - (AreaCut.height - streamCutH);
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.top += moveH;
                    Area.height = streamH - moveH;
                    Area.width = streamW + moveW;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.top += moveH;
                    AreaCut.top += moveH;
                    Area.width = streamW + moveW;
                    AreaCut.width = streamCutW + moveW;
                    Area.height = streamH - moveH;
                    AreaCut.height = streamCutH - moveH;
                }
                setCursor(Qt::SizeBDiagCursor);
            }
            else if(mousePos == InRightBottom)
            {
                if(streamW+moveW <= 20 || streamH+moveH <= 20)
                    return;

                if(!m_Parent->m_bShiftKeyPressed && !m_Parent->m_bAltKeyPressed && !bIsCut)
                {
                    Area.width = streamW + moveW;
                    AreaCut.width = streamCutW + moveW;
                    Area.height = Area.width/scalCrop;
                    AreaCut.height = AreaCut.width/scal;
                }
                else if(m_Parent->m_bAltKeyPressed && !m_Parent->m_bShiftKeyPressed)
                {
                    Area.width = streamW + moveW;
                    Area.height = Area.width/scalCrop;
                }
                else if((!m_Parent->m_bAltKeyPressed && m_Parent->m_bShiftKeyPressed) || bIsCut)
                {
                    Area.width = streamW + moveW;
                    AreaCut.width = streamCutW + moveW;
                    Area.height = streamH + moveH;
                    AreaCut.height = streamCutH + moveH;
                }

                setCursor(Qt::SizeFDiagCursor);
            }
            else if(mousePos == InCenter)
            {
                Area.left = left+moveW;
                Area.top = top+moveH;
                AreaCut.left = CutLeft+moveW;
                AreaCut.top = CutTop+moveH;
                setCursor(Qt::SizeAllCursor);
            }
            else if(mousePos == InOut)
            {
                setCursor(Qt::ArrowCursor);
            }
            if(m_CurrentLevel >= 0 && m_CurrentLevel <  m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count())
            {
                int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].id;
                bool bIsAgent = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bIsAgent;
                bool bKeepRatio = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bKeepRatio;
                if(bIsAgent)
                {
                    if(m_Parent->m_bAltKeyPressed && mousePos != InCenter)
                    {   
                        if(Area.top < AreaCut.top || Area.left < AreaCut.left || (Area.top + Area.height) > (AreaCut.top + AreaCut.height) || (Area.left + Area.width) > (AreaCut.left + AreaCut.width))
                        {
                            setCursor(Qt::ArrowCursor);
                            if(Area.top < AreaCut.top/* || (Area.top + Area.height) > (AreaCut.top + AreaCut.height)*/)
                            {
                                Area.top = AreaCut.top;
                                Area.height = AreaCut.height - Area.CropBottom;
                                AreaCut.CropTop = 0;
                            }
                            else if((Area.top + Area.height) > (AreaCut.top + AreaCut.height))
                            {
//                                Area.top = AreaCut.top;
                                Area.height = AreaCut.height - AreaCut.CropTop;
//                                AreaCut.CropTop = 0;
                                AreaCut.CropBottom = 0;
                            }
                            if(Area.left < AreaCut.left /*|| (Area.left + Area.width) > (AreaCut.left + AreaCut.width)*/)
                            {
                                Area.left = AreaCut.left;
                                Area.width = AreaCut.width - AreaCut.CropRight;
                                AreaCut.CropLeft = 0;
                            }
                            else if((Area.left + Area.width) > (AreaCut.left + AreaCut.width))
                            {
//                                Area.left = AreaCut.left;
                                Area.width = AreaCut.width - AreaCut.CropLeft;
//                                AreaCut.CropLeft = 0;
                                AreaCut.CropRight = 0;
                            }
                            m_streamPos = QPoint(Area.left,Area.top);
                            m_streamW = Area.width;
                            m_streamH = Area.height;
                            m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
                            m_streamCutW = AreaCut.width;
                            m_streamCutH = AreaCut.height;
                            SLiveSetCropping(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,Area.left - AreaCut.left,Area.top-AreaCut.top,(AreaCut.width-Area.width)-(Area.left-AreaCut.left),(AreaCut.height-Area.height)-(Area.top-AreaCut.top));
                            if(AreaCut.width == Area.width && AreaCut.height == Area.height)
                            {
                                m_bIsCut = false;
                            }

                            return;
                        }

                        SLiveSetCropping(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,Area.left - AreaCut.left,Area.top-AreaCut.top,(AreaCut.width-Area.width)-(Area.left-AreaCut.left),(AreaCut.height-Area.height)-(Area.top-AreaCut.top));
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;
                        m_bIsCut = true;
                        m_bIsMove = true;
                    }
                    else
                    {

                        AreaCut.CropLeft = Area.left - AreaCut.left;
                        AreaCut.CropTop = Area.top - AreaCut.top;
                        AreaCut.CropRight = (AreaCut.width-Area.width)-(Area.left-AreaCut.left);
                        AreaCut.CropBottom = (AreaCut.height-Area.height)-(Area.top-AreaCut.top);
                        if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,&AreaCut,/*ConfigOper::instance()->m_bKeepRatio*/bKeepRatio) < 0)
                        {
                            return;
                        }
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;
                        m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
                        m_streamCutW = AreaCut.width;
                        m_streamCutH = AreaCut.height;
                        m_bIsMove = true;
                    }
                }
                else
                {
                    if(m_Parent->m_bAltKeyPressed && mousePos != InCenter)
                    {
                        if(Area.top < AreaCut.top || Area.left < AreaCut.left || (Area.top + Area.height) > (AreaCut.top + AreaCut.height) || (Area.left + Area.width) > (AreaCut.left + AreaCut.width))
                        {
                            setCursor(Qt::ArrowCursor);

                            if(Area.top < AreaCut.top/* || (Area.top + Area.height) > (AreaCut.top + AreaCut.height)*/)
                            {
                                Area.top = AreaCut.top;
                                Area.height = AreaCut.height - Area.CropBottom;
                                AreaCut.CropTop = 0;
                            }
                            else if((Area.top + Area.height) > (AreaCut.top + AreaCut.height))
                            {
//                                Area.top = AreaCut.top;
                                Area.height = AreaCut.height - AreaCut.CropTop;
//                                AreaCut.CropTop = 0;
                                AreaCut.CropBottom = 0;
                            }
                            if(Area.left < AreaCut.left /*|| (Area.left + Area.width) > (AreaCut.left + AreaCut.width)*/)
                            {
                                Area.left = AreaCut.left;
                                Area.width = AreaCut.width - AreaCut.CropRight;
                                AreaCut.CropLeft = 0;
                            }
                            else if((Area.left + Area.width) > (AreaCut.left + AreaCut.width))
                            {
//                                Area.left = AreaCut.left;
                                Area.width = AreaCut.width - AreaCut.CropLeft;
//                                AreaCut.CropLeft = 0;
                                AreaCut.CropRight = 0;
                            }
                            m_streamPos = QPoint(Area.left,Area.top);
                            m_streamW = Area.width;
                            m_streamH = Area.height;
                            m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
                            m_streamCutW = AreaCut.width;
                            m_streamCutH = AreaCut.height;
                            SLiveSetCropping(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,Area.left - AreaCut.left,Area.top-AreaCut.top,(AreaCut.width-Area.width)-(Area.left-AreaCut.left),(AreaCut.height-Area.height)-(Area.top-AreaCut.top));
                            if(AreaCut.width == Area.width && AreaCut.height == Area.height)
                            {
                                m_bIsCut = false;
                            }
                            return;
                        }
                        SLiveSetCropping(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,Area.left - AreaCut.left,Area.top-AreaCut.top,(AreaCut.width-Area.width)-(Area.left-AreaCut.left),(AreaCut.height-Area.height)-(Area.top-AreaCut.top));
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;
                        m_bIsCut = true;
                        m_bIsMove = true;
                    }
                    else
                    {
                        AreaCut.CropLeft = Area.left - AreaCut.left;
                        AreaCut.CropTop = Area.top - AreaCut.top;
                        AreaCut.CropRight = (AreaCut.width-Area.width)-(Area.left-AreaCut.left);
                        AreaCut.CropBottom = (AreaCut.height-Area.height)-(Area.top-AreaCut.top);
                        if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,&AreaCut,/*ConfigOper::instance()->m_bKeepRatio*/bKeepRatio) < 0)
                        {
                            return;
                        }
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;
                        m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
                        m_streamCutW = AreaCut.width;
                        m_streamCutH = AreaCut.height;
                        m_bIsMove = true;
                    }
                }
            }               
        }
    }
    else {
        event->ignore();
    }
}

void PreviewLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    //m_Parent->OnStreamMaxDisplay(true);
}

QPaintEngine* PreviewLabel::paintEngine()const
{
    return 0;
}
void PreviewLabel::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        if(m_Parent->m_bIsFullScreenPreview)
        {
            m_Parent->OnPreiveFullScreen();
        }
    }
    else if(event->key() == Qt::Key_Alt)
    {
        this->SetSourceCropData();
        m_Parent->m_bAltKeyPressed = true;
    }
}
void PreviewLabel::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Alt)
    {
        if(!this->m_bPressed)
            m_Parent->m_bAltKeyPressed = false;
    }
}

