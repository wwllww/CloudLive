#include "VideoListUI.h"
#include <QDialog>
#include <QLabel>
VideoListUI::VideoListUI(QStringList fileList,QDialog *parent,int index):
    QDialog(parent),
    ui(new Ui::VideoListUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    m_pParent = (ButelLive*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("Choose file to play"));
    ui->m_pLWPlayList->addItems(fileList);

//    ui->m_pLWPlayList->setStyleSheet("QListWidget{border:1px solid gray; color:black; }"
//                               "QListWidget::Item{padding-top:20px; padding-bottom:4px; }"
//                               "QListWidget::Item:hover{background:skyblue;padding-top:20px;padding-bottom:4px; }"
//                               "QListWidget::item:selected{background:rgba(47,160,190,255); color:white; }"
//                               "QListWidget::item:selected:!active{border-width:0px; background:rgba(47,160,190,255); }"
//                               );

    ui->m_pLWPlayList->setStyleSheet("QListWidget{border:1px solid gray; color:black; }"
                               "QListWidget::Item:hover{background:skyblue;}"
                               "QListWidget::item:selected{background:rgba(47,160,190,255); color:white; }"
                               "QListWidget::item:selected:!active{border-width:0px; background:rgba(47,160,190,255); }"
                               );
    ui->m_pLWPlayList->setCurrentRow(index);
}

VideoListUI::~VideoListUI()
{
    delete ui;
}

void VideoListUI::on_m_pBtnOk_clicked()
{
    if(ui->m_pLWPlayList->currentRow() < 0)
        return;
    emit ui->m_pLWPlayList->itemDoubleClicked(NULL);
    done(ON_BUTTON_OK);
}

void VideoListUI::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
}

void VideoListUI::on_m_pLWPlayList_itemDoubleClicked(QListWidgetItem *item)
{
    int iIndex = ui->m_pLWPlayList->currentRow();
    int iCurPrev = m_pParent->GetPreviewCurrentRow();
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(iCurPrev);
    if(iLinkMatIndex == -1) return;

    QString strJson_obj = "{\"DeskTopSetting\" : 1,\"CurrentIndex\" :%1}";
    strJson_obj = strJson_obj.arg(iIndex);
    if(SLiveUpdateStream(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,strJson_obj.toLocal8Bit().data()) < 0)
    {
        return;
    }
    close();
}
