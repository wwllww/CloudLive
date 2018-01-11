#include "PositionSizeSet.h"
#include <QDialog>
#include <QLabel>

enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL};

PositionSizeSetUI::PositionSizeSetUI(ButelLive *parent,VideoArea *Area) :
    QDialog(parent),
    ui(new Ui::PositionSizeSetUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    m_pParent = parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("PositionSizeSet"));
    ui->m_pWidthEdt->setText(QString::number(Area->width));
    ui->m_pHeightEdt->setText(QString::number(Area->height));
    ui->m_pXMoveEdt->setText(QString::number(Area->left));
    ui->m_pYMoveEdt->setText(QString::number(Area->top));

    QRegExp regx("[1-9][0-9]+$");
    QValidator *validator = new QRegExpValidator(regx,this);
    ui->m_pWidthEdt->setValidator(validator);
    ui->m_pHeightEdt->setValidator(validator);
    ui->m_pXMoveEdt->setValidator(validator);
    ui->m_pYMoveEdt->setValidator(validator);
}

PositionSizeSetUI::~PositionSizeSetUI()
{
    delete ui;
}


void PositionSizeSetUI::on_m_pBtnOk_clicked()
{
    int iCurScene = m_pParent->GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_pParent->m_SelLevel;
    VideoArea Area;
    Area.left = ui->m_pXMoveEdt->text().toInt();
    Area.top = ui->m_pYMoveEdt->text().toInt();
    Area.width = ui->m_pWidthEdt->text().toInt();
    Area.height = ui->m_pHeightEdt->text().toInt();
    bool bIsAgent = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    int iMatIndex = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    if(!bIsAgent)
    {
        if(SLiveUpdateStreamPosition(m_pParent->Instance,m_pParent->m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].width = Area.width;
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].height = Area.height;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(m_pParent->Instance,m_pParent->m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].width = Area.width;
             m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].height = Area.height;
        }
    }
    close();
}

void PositionSizeSetUI::on_m_pBtnCancel_clicked()
{
    close();
}

void PositionSizeSetUI::on_m_pBtnSetGlbCoord_clicked()
{

}
