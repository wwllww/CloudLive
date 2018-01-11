#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QPoint>
#include <QDesktopWidget>
#include <QRect>
#include "titlebar.h"
#include "ButelLive.h"

#ifdef Q_OS_WIN
#pragma comment(lib, "user32.lib")
#include <qt_windows.h>
#endif

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    m_IsChildWnd = false;
    setFixedHeight(50);

    m_pIconLabel = new QLabel(this);
    m_pTitleLabel = new QLabel(this);
    m_pMinimizeButton = new QPushButton(this);
    m_pMaximizeButton = new QPushButton(this);
    m_pCloseButton = new QPushButton(this);
    m_pMenuButton = new QPushButton(this);

    m_pIconLabel->setScaledContents(true);

    m_pTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_pMinimizeButton->setFixedSize(25, 25);
    m_pMaximizeButton->setFixedSize(25, 25);
    m_pIconLabel->setFixedSize(25,25);
    m_pTitleLabel->setFixedHeight(25);
    m_pCloseButton->setFixedSize(25, 25);
    m_pMenuButton->setFixedSize(25, 25);

    m_pTitleLabel->setObjectName("whiteLabel");
    m_pMinimizeButton->setObjectName("minimizeButton");
    m_pMaximizeButton->setObjectName("maximizeButton");
    m_pCloseButton->setObjectName("closeButton");
    m_pMenuButton->setObjectName("MenuButton");

    m_pMinimizeButton->setToolTip("Minimize");
    m_pMaximizeButton->setToolTip("Maximize");
    m_pCloseButton->setToolTip("Close");
    m_pMenuButton->setToolTip("Menu");

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->addWidget(m_pIconLabel);
    pLayout->addWidget(m_pTitleLabel);
    pLayout->addWidget(m_pMenuButton);
    pLayout->addWidget(m_pMinimizeButton);
    pLayout->addWidget(m_pMaximizeButton);
    pLayout->addWidget(m_pCloseButton);
    pLayout->setContentsMargins(0, 0, 5, 0);
    setLayout(pLayout);

    connect(m_pMinimizeButton, SIGNAL(clicked(bool)), this, SLOT(onClicked()));
    connect(m_pMaximizeButton, SIGNAL(clicked(bool)), this, SLOT(onClicked()));
    connect(m_pCloseButton, SIGNAL(clicked(bool)), this, SLOT(onClicked()));
    connect(m_pMenuButton, SIGNAL(clicked(bool)), this, SLOT(onClicked()));
    connect(this, SIGNAL(clickedMenu()), parent, SLOT(ShowMenu()));
}

TitleBar::~TitleBar()
{

}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(!m_IsChildWnd)
        emit m_pMaximizeButton->clicked();
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (ReleaseCapture())
    {
        QWidget *pWindow = this->parentWidget();
        if (pWindow)
        {
           SendMessage(HWND(pWindow->winId()), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
        }
    }
       event->ignore();
#else
#endif
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (ReleaseCapture())
    {
        QWidget *pWindow = this->parentWidget();
        if((event->globalPos().y()<= 0)&&(!m_IsChildWnd))
        {
            pWindow->showMaximized();
        }
        if((event->globalPos().x()<= 0)&&(!m_IsChildWnd))
        {
            QRect rect;
            int iWidth = ((QApplication::desktop()->width())/2);
            int iHeihgt = (QApplication::desktop()->height()-40);
            rect.setCoords(0,0,iWidth,iHeihgt);
            pWindow->setGeometry(rect);
        }
    }
       event->ignore();
#else
#endif
}

bool TitleBar::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::WindowTitleChange:
    {
        QWidget *pWidget = qobject_cast<QWidget *>(obj);
        if (pWidget)
        {
            m_pTitleLabel->setText(pWidget->windowTitle());
            return true;
        }
    }
    case QEvent::WindowIconChange:
    {
        QWidget *pWidget = qobject_cast<QWidget *>(obj);
        if (pWidget)
        {
            QIcon icon = pWidget->windowIcon();
            m_pIconLabel->setPixmap(icon.pixmap(m_pIconLabel->size()));
            return true;
        }
    }
    case QEvent::WindowStateChange:
    case QEvent::Resize:
        updateMaximize();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void TitleBar::onClicked()
{
    QPushButton *pButton = qobject_cast<QPushButton *>(sender());
    QWidget *pWindow = this->parentWidget();
    {
        if (pButton == m_pMinimizeButton)
        {
            pWindow->showMinimized();
        }
        else if (pButton == m_pMaximizeButton)
        {
            pWindow->isMaximized() ? pWindow->showNormal() : pWindow->showMaximized();
        }
        else if (pButton == m_pCloseButton)
        {
            pWindow->close();
        }
        else if (pButton == m_pMenuButton)
        {
            emit clickedMenu();
        }
    }
}

void TitleBar::updateMaximize()
{
    QWidget *pWindow = this->window();
    if (pWindow->isTopLevel())
    {
        bool bMaximize = pWindow->isMaximized();
        if (bMaximize)
        {
            m_pMaximizeButton->setToolTip(tr("Restore"));
            m_pMaximizeButton->setProperty("maximizeProperty", "restore");
        }
        else
        {
            m_pMaximizeButton->setProperty("maximizeProperty", "maximize");
            m_pMaximizeButton->setToolTip(tr("Maximize"));
        }

        m_pMaximizeButton->setStyle(QApplication::style());
    }
}
