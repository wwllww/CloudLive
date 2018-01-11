#ifndef PREVIEWLABEL_H
#define PREVIEWLABEL_H

#include "ButelLive.h"
#include "SLiveApi.h"
#include <QLabel>
//投影窗口类
class PreviewLabel : public QLabel
{
    Q_OBJECT
public:
    explicit PreviewLabel(QDialog *parent = 0);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    QPaintEngine* paintEngine()const;
signals:
    void ButtonRightRelease();
public:
    ButelLive*      m_Parent;
    bool            m_bPressed;
    QPoint          m_pStart;
    QPoint          m_pEnd;
    Vect            m_vStart;
    Vect            m_vEnd;
    int             m_CurrentLevel;
    bool            bX;
    bool            bY;

    int mousePos = -1;

    QPoint          m_streamPos;
    int             m_streamW;
    int             m_streamH;

    QPoint          m_streamCutPos;
    int             m_streamCutW;
    int             m_streamCutH;
    bool            m_bIsCut;

    bool            m_bIsMove;

    QMenu           m_EditMenu;

};

#endif // PREVIEWLABEL_H
