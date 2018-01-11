#include "PositionSizeSet.h"
#include <QDialog>
#include <QLabel>
#include "configoper.h"

//enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL};

PositionSizeSetUI::PositionSizeSetUI(ButelLive *parent,VideoArea *AreaCut) :
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
    ui->m_pWidthEdt->setText(QString::number(AreaCut->width));
    ui->m_pHeightEdt->setText(QString::number(AreaCut->height));
    ui->m_pXMoveEdt->setText(QString::number(AreaCut->left));
    ui->m_pYMoveEdt->setText(QString::number(AreaCut->top));

    ui->m_pLeftTangentEdt->setText(QString::number(AreaCut->CropLeft));
    ui->m_pTopTangentEdt->setText(QString::number(AreaCut->CropTop));
    ui->m_pRightTangentEdt->setText(QString::number(AreaCut->CropRight));
    ui->m_pBottomTangentEdt->setText(QString::number(AreaCut->CropBottom));

    QRegExp regx("^[0-9]+$");
    QValidator *validator = new QRegExpValidator(regx,this);
    ui->m_pWidthEdt->setValidator(validator);
    ui->m_pHeightEdt->setValidator(validator);
    QRegExp regx1("^-?[0-9][0-9]+$");
    QValidator *validator1 = new QRegExpValidator(regx1,this);
    ui->m_pXMoveEdt->setValidator(validator1);
    ui->m_pYMoveEdt->setValidator(validator1);


    int iCurScene = m_pParent->GetSceneCurrentRow();
    int index = m_pParent->m_SelLevel;
    bool bKeepRatio = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bKeepRatio;

    ui->m_pChkBoxKeepRatio->setChecked(bKeepRatio);
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
//    ConfigOper::instance()->m_bKeepRatio = ui->m_pChkBoxKeepRatio->isChecked();
//    ConfigOper::instance()->WriteCfgFile();
    bool bKeepRatio = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bKeepRatio = ui->m_pChkBoxKeepRatio->isChecked();
    VideoArea AreaCut;
    AreaCut.left = ui->m_pXMoveEdt->text().toInt();
    AreaCut.top = ui->m_pYMoveEdt->text().toInt();
    AreaCut.width = ui->m_pWidthEdt->text().toInt();
    AreaCut.height = ui->m_pHeightEdt->text().toInt();
    AreaCut.CropLeft = ui->m_pLeftTangentEdt->text().toInt();
    AreaCut.CropTop = ui->m_pTopTangentEdt->text().toInt();
    AreaCut.CropRight = ui->m_pRightTangentEdt->text().toInt();
    AreaCut.CropBottom = ui->m_pBottomTangentEdt->text().toInt();

    bool bIsAgent = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    int iMatIndex = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    if(!bIsAgent)
    {
        if(SLiveUpdateStreamPosition(m_pParent->Instance,m_pParent->m_MateralList[iMatIndex]->streamID1,&AreaCut,/*ConfigOper::instance()->m_bKeepRatio*/bKeepRatio) < 0)
        {
            return;
        }
        else
        {
            if(SLiveSetCropping(m_pParent->Instance,m_pParent->m_MateralList[iMatIndex]->streamID1,AreaCut.CropLeft,AreaCut.CropTop,AreaCut.CropRight,AreaCut.CropBottom) < 0)
            {
                return;
            }
        }

    }
    else
    {
        if(SLiveUpdateStreamPosition(m_pParent->Instance,m_pParent->m_AgentSourceList[iMatIndex]->streamID1,&AreaCut,/*ConfigOper::instance()->m_bKeepRatio*/bKeepRatio) < 0)
        {
            return;
        }
        else
        {
            if(SLiveSetCropping(m_pParent->Instance,m_pParent->m_AgentSourceList[iMatIndex]->streamID1,AreaCut.CropLeft,AreaCut.CropTop,AreaCut.CropRight,AreaCut.CropBottom) < 0)
            {
                return;
            }
        }
    }  
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].left = AreaCut.left + AreaCut.CropLeft;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].top = AreaCut.top + AreaCut.CropTop;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].width = AreaCut.width - AreaCut.CropLeft - AreaCut.CropRight;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].height = AreaCut.height - AreaCut.CropTop - AreaCut.CropBottom;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].cutLeft = AreaCut.left;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].cutTop = AreaCut.top;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].cutWidth = AreaCut.width;
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].cutHeight = AreaCut.height;
    if(AreaCut.CropLeft != 0 || AreaCut.CropTop != 0 || AreaCut.CropRight != 0 || AreaCut.CropBottom != 0)
        m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsCut = true;
    else
        m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsCut = false;
    close();
}

void PositionSizeSetUI::on_m_pBtnCancel_clicked()
{
    close();
}

void PositionSizeSetUI::on_m_pBtnSetGlbCoord_clicked()
{
    int iCurScene = m_pParent->GetSceneCurrentRow();
    int index = m_pParent->m_SelLevel;
//    ConfigOper::instance()->m_bKeepRatio = ui->m_pChkBoxKeepRatio->isChecked();
//    ConfigOper::instance()->WriteCfgFile();
    bool bKeepRatio = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bKeepRatio = ui->m_pChkBoxKeepRatio->isChecked();
    VideoArea AreaCut;
    AreaCut.left = ui->m_pXMoveEdt->text().toInt();
    AreaCut.top = ui->m_pYMoveEdt->text().toInt();
    AreaCut.width = ui->m_pWidthEdt->text().toInt();
    AreaCut.height = ui->m_pHeightEdt->text().toInt();
    AreaCut.CropLeft = ui->m_pLeftTangentEdt->text().toInt();
    AreaCut.CropTop = ui->m_pTopTangentEdt->text().toInt();
    AreaCut.CropRight = ui->m_pRightTangentEdt->text().toInt();
    AreaCut.CropBottom = ui->m_pBottomTangentEdt->text().toInt();

    bool bIsAgent = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    int iMatIndex = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    if(!bIsAgent)
    {
        if(SLiveUpdateStreamPosition(m_pParent->Instance,m_pParent->m_MateralList[iMatIndex]->streamID1,&AreaCut,/*ConfigOper::instance()->m_bKeepRatio*/bKeepRatio) < 0)
        {
            return;
        }
        else
        {
            if(SLiveSetCropping(m_pParent->Instance,m_pParent->m_MateralList[iMatIndex]->streamID1,AreaCut.CropLeft,AreaCut.CropTop,AreaCut.CropRight,AreaCut.CropBottom) < 0)
            {
                return;
            }
        }
        for(int i = 0; i < m_pParent->m_SceneList.count(); i++)
        {
            for(int j = 0; j < m_pParent->m_SceneList[i]->m_LinkMateralVec.count(); j++)
            {
                if(!m_pParent->m_SceneList[i]->m_LinkMateralVec[j].bIsAgent && m_pParent->m_SceneList[i]->m_LinkMateralVec[j].id == iMatIndex)
                {
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].left = AreaCut.left + AreaCut.CropLeft;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].top = AreaCut.top + AreaCut.CropTop;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].width = AreaCut.width - AreaCut.CropLeft - AreaCut.CropRight;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].height = AreaCut.height - AreaCut.CropTop - AreaCut.CropBottom;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].cutLeft = AreaCut.left;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].cutTop = AreaCut.top;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].cutWidth = AreaCut.width;
                    m_pParent->m_SceneList[i]->m_LinkMateralVec[j].cutHeight = AreaCut.height;
                    if(AreaCut.CropLeft != 0 || AreaCut.CropTop != 0 || AreaCut.CropRight != 0 || AreaCut.CropBottom != 0)
                        m_pParent->m_SceneList[i]->m_LinkMateralVec[j].bIsCut = true;
                    else
                        m_pParent->m_SceneList[i]->m_LinkMateralVec[j].bIsCut = false;
                }
            }
        }
        close();
    }
    else
    {
        on_m_pBtnOk_clicked();
    }
}
