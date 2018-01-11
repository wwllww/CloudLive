#include "previewlabel.h"
#include <QLabel>
#include <QMouseEvent>
#include <QDialog>
#include "LogDeliver.h"

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
}

void PreviewLabel::mousePressEvent(QMouseEvent *event)
{

    if(m_Parent->GetSceneCurrentRow() < 0) return;
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

        for(int i = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count() - 1; i >= 0; i--)
        {
            if(!m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bRender) continue;
//            if(i > m_Parent->m_SelLevel && m_Parent->m_SelLevel != -1)continue;
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



//                SLiveStream(m_Parent->Instance,m_Parent->m_StreamVec.at(i).streamID2,true);

                qDebug() << "mousePos = " << mousePos << "m_CurrentLevel = " << m_CurrentLevel;

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
    else if(event->buttons() == Qt::RightButton)
    {
        Vect v;
        QPoint gPos  = event->globalPos();
        QPoint realPos = this->mapFromGlobal(gPos);   //鼠标当前相对于窗体的坐标
        m_pStart = realPos;                           // 记录鼠标按下位置
        v.x = realPos.x();
        v.y = realPos.y();
        m_vStart = m_Parent->MapWindowToFramePos(v);

        int iCurScene = m_Parent->GetSceneCurrentRow();
        int iNum = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.count();

        for(int i = iNum - 1; i >= 0; i--)
        {
            if(!m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[i].bRender) continue;
//            QPoint streamPos = m_Parent->m_StreamVec.at(i).Position;
//            int streamW = m_Parent->m_StreamVec.at(i).width;
//            int streamH = m_Parent->m_StreamVec.at(i).height;
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
                m_Parent->m_PreviewInSourceMenu.exec(QCursor::pos());
                break;
            }
            else if(0 == i && (!bx || !by))
            {
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

void PreviewLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_Parent->m_bIsEditing) return;
    bX = false;
    bY = false;

    if(m_bPressed)
    {
        m_bPressed = false;
        if(m_CurrentLevel == -1) return;

        VideoArea Area;
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
        if(!streamInfo.bIsCut)
            streamInfo.bIsCut = m_bIsCut;
        qDebug() << "************************************";
        qDebug() << "streamInfo.cutLeft = " <<streamInfo.cutLeft << " streamInfo.cutTop = "<< streamInfo.cutTop;
        qDebug() << "m_streamCutW = " << m_streamCutW << "m_streamCutH = " << m_streamCutH;

        Area.left = streamInfo.left;
        Area.top = streamInfo.top;
        Area.width = m_streamW;
        Area.height = m_streamH;

        //判断是否需要吸附边缘
//        if(Area.left < 50 && Area.top < 50)
//        {
//            Area.left = 0;
//            Area.top = 0;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else if((Area.left > m_Parent->m_pixWidth - m_streamW -50) && Area.top < 50)
//        {
//            Area.left = m_Parent->m_pixWidth - m_streamW;
//            Area.top = 0;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else if((Area.top > m_Parent->m_pixHeight - m_streamH -50) && Area.left < 50)
//        {
//            Area.top = m_Parent->m_pixHeight - m_streamH;
//            Area.left = 0;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else if((Area.top > m_Parent->m_pixHeight - m_streamH -50) && (Area.left > m_Parent->m_pixWidth - m_streamW -50))
//        {
//            Area.top = m_Parent->m_pixHeight - m_streamH;
//            Area.left = m_Parent->m_pixWidth - m_streamW;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else if(Area.left < 50)
//        {
//            Area.left = 0;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else if(Area.top < 50)
//        {
//            Area.top = 0;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }

//        else if(Area.left > m_Parent->m_pixWidth - m_streamW -50)
//        {
//            Area.left = m_Parent->m_pixWidth - m_streamW;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else if(Area.top > m_Parent->m_pixHeight - m_streamH -50)
//        {
//            Area.top = m_Parent->m_pixHeight - m_streamH;
//            streamInfo.Position = QPoint(Area.left,Area.top);
//            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_StreamVec.at(m_CurrentLevel).streamID2,&Area) < 0)
//            {
//                return;
//            }
//            else
//            {
//                m_Parent->m_StreamVec.replace(m_CurrentLevel,streamInfo);
//            }
//        }
//        else
//        {
            m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec.replace(m_CurrentLevel,streamInfo);
//        }

        m_CurrentLevel = -1;
        m_bIsCut = false;
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
    if(!m_Parent->m_bIsPreview && !m_Parent->m_bIsLiving) return;
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
            qDebug() << "############################";
            qDebug() << "streamCutW = " << streamCutW << "streamCutH = " << streamCutH;
            qDebug() << "CutLeft = " << CutLeft << "CutTop = " << CutTop;

            float scal;
            UINT streamWidth,streamHeight;

            int iMatIndex = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].id;

//            if(SLiveGetStreamSize(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,&streamWidth,&streamHeight) < 0)
//            {
//                return;
//            }
//            else
//            {
//                scal = (float)streamWidth/streamHeight;
//            }

//            scal = 1;

            if(mousePos == InRight)
            {
                if(streamW+moveW <= 20)
                    return;
                Area.width = streamW + moveW;
//                Area.height = Area.width/scal;
                setCursor(Qt::SizeHorCursor);

//                AreaCut.width = streamCutW + moveW;
            }
            else if(mousePos == InLeft)
            {
                if(streamW-moveW <= 20)
                    return;
                Area.left += moveW;
                Area.width = streamW - moveW;
//                Area.height = Area.width/scal;
                setCursor(Qt::SizeHorCursor);
            }
            else if(mousePos == InBottom)
            {
                if(streamH+moveH <= 20)
                    return;
                Area.height = streamH + moveH;
//                Area.width = Area.height*scal;
                setCursor(Qt::SizeVerCursor);
            }
            else if(mousePos == InTop)
            {
                if(streamH-moveH <= 20)
                    return;
                Area.top += moveH;
                Area.height = streamH - moveH;
//                Area.width = Area.height*scal;
                setCursor(Qt::SizeVerCursor);
            }
            else if(mousePos == InLeftTop)
            {
                if(streamW-moveW <= 20 || streamH-moveH <= 20)
                    return;
                Area.width = streamW - moveW;
                Area.height = streamH - moveH;
//                Area.height = Area.width/scal;
                Area.top = Area.top - (Area.height - streamH);
                Area.left += moveW;
                setCursor(Qt::SizeFDiagCursor);
            }
            else if(mousePos == InLeftBottom)
            {
                if(streamW-moveW <= 20 || streamH+moveH <= 20)
                    return;
                Area.width = streamW - moveW;
                Area.height = streamH + moveH;
                Area.left += moveW;
//                Area.height = Area.width/scal;
                setCursor(Qt::SizeBDiagCursor);
            }
            else if(mousePos == InRightTop)
            {
                if(streamW+moveW <= 20 || streamH-moveH <= 20)
                    return;
                Area.width = streamW + moveW;
                Area.height = streamH - moveH;
//                    Area.height = Area.width/scal;
                Area.top = Area.top - (Area.height - streamH);
                setCursor(Qt::SizeBDiagCursor);
            }
            else if(mousePos == InRightBottom)
            {
                if(streamW+moveW <= 20 || streamH+moveH <= 20)
                    return;
                Area.width = streamW + moveW;
                Area.height = streamH + moveH;
//                    Area.height = Area.width/scal;
                setCursor(Qt::SizeFDiagCursor);
            }
            else if(mousePos == InCenter)
            {
                Area.left = left+moveW;
                Area.top = top+moveH;
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
                bool bIsCut = m_Parent->m_SceneList[iCurScene]->m_LinkMateralVec[m_CurrentLevel].bIsCut;
                if(bIsAgent)
                {
                    if(m_Parent->m_bAltKeyPressed)
                    {   
                        if(Area.width > AreaCut.width || Area.height > AreaCut.height)
                            return;

                        SLiveSetCropping(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,Area.left - AreaCut.left,Area.top-AreaCut.top,(AreaCut.width-Area.width)-(Area.left-AreaCut.left),(AreaCut.height-Area.height)-(Area.top-AreaCut.top));
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;
                        m_bIsCut = true;
                    }
                    else
                    {
//                        if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
//                        {
//                            return;
//                        }
//                        m_streamPos = QPoint(Area.left,Area.top);
//                        m_streamW = Area.width;
//                        m_streamH = Area.height;
                        if(bIsCut)
                        {
                            if(mousePos == InRight)
                            {
                                AreaCut.width = streamCutW + moveW;
                            }
                            else if(mousePos == InLeft)
                            {
                                AreaCut.left += moveW;
                                AreaCut.width = streamCutW - moveW;
                            }
                            else if(mousePos == InBottom)
                            {
                                AreaCut.height = streamCutH + moveH;
                            }
                            else if(mousePos == InTop)
                            {
                                AreaCut.top += moveH;
                                AreaCut.height = streamCutH - moveH;
                            }
                            else if(mousePos == InLeftTop)
                            {
                                AreaCut.width = streamCutW - moveW;
                                AreaCut.height = streamCutH - moveH;
                                AreaCut.top += moveH;
                                AreaCut.left += moveW;
                            }
                            else if(mousePos == InLeftBottom)
                            {
                                AreaCut.width = streamCutW - moveW;
                                AreaCut.height = streamCutH + moveH;
                                AreaCut.left += moveW;
                                setCursor(Qt::SizeBDiagCursor);
                            }
                            else if(mousePos == InRightTop)
                            {
                                AreaCut.width = streamCutW + moveW;
                                AreaCut.height = streamCutH - moveH;
                                AreaCut.top = AreaCut.top += moveH;
                                setCursor(Qt::SizeBDiagCursor);
                            }
                            else if(mousePos == InRightBottom)
                            {
                                AreaCut.width = streamCutW + moveW;
                                AreaCut.height = streamCutH + moveH;
                                setCursor(Qt::SizeFDiagCursor);
                            }
                            else if(mousePos == InCenter)
                            {
                                AreaCut.left = CutLeft+moveW;
                                AreaCut.top = CutTop+moveH;
                                setCursor(Qt::SizeAllCursor);
                            }
                            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,&AreaCut) < 0)
                            {
                                return;
                            }
                        }
                        else
                        {
                            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
                            {
                                return;
                            }
                            AreaCut.left = Area.left;
                            AreaCut.top = Area.top;
                            AreaCut.width = Area.width;
                            AreaCut.height = Area.height;
                        }
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;

                        m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
                        m_streamCutW = AreaCut.width;
                        m_streamCutH = AreaCut.height;
                    }
                }
                else
                {
                    if(m_Parent->m_bAltKeyPressed)
                    {
                        if(Area.width > AreaCut.width || Area.height > AreaCut.height)
                            return;

                        qDebug() << "Area = " << Area.left << Area.top << Area.width << Area.height;
                        qDebug() << "AreaCut = " << AreaCut.left << AreaCut.top << AreaCut.width << AreaCut.height;


                        qDebug() << Area.left - AreaCut.left << Area.top-AreaCut.top << (AreaCut.width-Area.width)-(Area.left-AreaCut.left) << (AreaCut.height-Area.height)-(Area.top-AreaCut.top);
                        SLiveSetCropping(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,Area.left - AreaCut.left,Area.top-AreaCut.top,(AreaCut.width-Area.width)-(Area.left-AreaCut.left),(AreaCut.height-Area.height)-(Area.top-AreaCut.top));
                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;
                        qDebug() << "--------------------------------------------------";
                        qDebug() << "m_streamCutW = " << m_streamCutW << "m_streamCutH = " << m_streamCutH;
                        m_bIsCut = true;
                    }
                    else
                    {
                        if(bIsCut)
                        {
                            if(mousePos == InRight)
                            {
                                AreaCut.width = streamCutW + moveW;
                            }
                            else if(mousePos == InLeft)
                            {
                                AreaCut.left += moveW;
                                AreaCut.width = streamCutW - moveW;
                            }
                            else if(mousePos == InBottom)
                            {
                                AreaCut.height = streamCutH + moveH;
                            }
                            else if(mousePos == InTop)
                            {
                                AreaCut.top += moveH;
                                AreaCut.height = streamCutH - moveH;
                            }
                            else if(mousePos == InLeftTop)
                            {
                                AreaCut.width = streamCutW - moveW;
                                AreaCut.height = streamCutH - moveH;
                                AreaCut.top += moveH;
                                AreaCut.left += moveW;
                            }
                            else if(mousePos == InLeftBottom)
                            {
                                AreaCut.width = streamCutW - moveW;
                                AreaCut.height = streamCutH + moveH;
                                AreaCut.left += moveW;
                                setCursor(Qt::SizeBDiagCursor);
                            }
                            else if(mousePos == InRightTop)
                            {
                                AreaCut.width = streamCutW + moveW;
                                AreaCut.height = streamCutH - moveH;
                                AreaCut.top = AreaCut.top += moveH;
                                setCursor(Qt::SizeBDiagCursor);
                            }
                            else if(mousePos == InRightBottom)
                            {
                                AreaCut.width = streamCutW + moveW;
                                AreaCut.height = streamCutH + moveH;
                                setCursor(Qt::SizeFDiagCursor);
                            }
                            else if(mousePos == InCenter)
                            {
                                AreaCut.left = CutLeft+moveW;
                                AreaCut.top = CutTop+moveH;
                                setCursor(Qt::SizeAllCursor);
                            }
                            qDebug() << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$";
                            qDebug() << "Area.width =" << Area.width << "AreaCut.width = " << AreaCut.width;
                            qDebug() << "Area.height =" << Area.height << "AreaCut.height = " << AreaCut.height;



                            qDebug() << AreaCut.left << AreaCut.top << AreaCut.width << AreaCut.height << bIsCut;
                            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,&AreaCut) < 0)
                            {
                                return;
                            }
                        }
                        else
                        {
                            qDebug() << "++++++++++++++++++++++++++++++++++++++";
                            qDebug() << Area.left << Area.top << Area.width << Area.height << bIsCut;
                            qDebug() << Area.CropLeft << Area.CropTop << Area.CropRight << Area.CropBottom << bIsCut;
                            if(SLiveUpdateStreamPosition(m_Parent->Instance,m_Parent->m_MateralList[iMatIndex]->streamID1,&Area) < 0)
                            {
                                return;
                            }
                            AreaCut.left = Area.left;
                            AreaCut.top = Area.top;
                            AreaCut.width = Area.width;
                            AreaCut.height = Area.height;
                        }


                        m_streamPos = QPoint(Area.left,Area.top);
                        m_streamW = Area.width;
                        m_streamH = Area.height;

                        m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
                        m_streamCutW = AreaCut.width;
                        m_streamCutH = AreaCut.height;

//                        float fcal = (float)m_Parent->m_pixWidth / m_Parent->m_pixHeight;
//                        if((m_streamW/m_streamH) < fcal)
//                        {
//                            if(mousePos == InLeft || mousePos == InLeftTop || mousePos == InLeftBottom)
//                                m_streamCutPos = QPoint(AreaCut.left - (fcal * m_streamH - m_streamCutW),AreaCut.top);
//                            else
//                                m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
//                            m_streamCutW = fcal * m_streamH;
//                            m_streamCutH = m_streamH;
//                        }
//                        else if((m_streamW/m_streamH) > fcal)
//                        {
//                            if(mousePos == InTop || mousePos == InLeftTop || mousePos == InRightTop)
//                                m_streamCutPos = QPoint(AreaCut.left,AreaCut.top - (m_streamCutW / fcal - m_streamCutH));
//                            else
//                                m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
//                            m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
//                            m_streamCutW = m_streamW;
//                            m_streamCutH = m_streamCutW / fcal;
//                        }
//                        else
//                        {
//                            m_streamCutPos = QPoint(AreaCut.left,AreaCut.top);
//                            m_streamCutW = m_streamW;
//                            m_streamCutH = m_streamH;
//                        }
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

