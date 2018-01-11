#include "InputName.h"
#include "MyMessageBox.h"
#include <QDialog>
#include <QLabel>

//enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL};

InputNameUI::InputNameUI(ButelLive *parent,int type) :
    QDialog(parent),
    ui(new Ui::InputNameUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    m_pParent = parent;
    m_type = type;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("Please enter a name"));
}

InputNameUI::~InputNameUI()
{
    delete ui;
}

void InputNameUI::on_m_pBtnOk_clicked()
{
//    done(ON_BUTTON_OK);
    if(m_type == Scene)
    {
        if(m_pParent->SceneIsExist(getInputText()))
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(getInputText()),BUTTON_YES);
            message.exec();
        }
        else
        {
            close();
        }
    }
    else if(m_type == Materal)
    {
        if(m_pParent->MateralIsExist(getInputText()))
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(getInputText()),BUTTON_YES);
            message.exec();
        }
        else
        {
            close();
        }
    }
}

void InputNameUI::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
}
void InputNameUI::setInputText(QString text)
{
    ui->m_pLineEditInputName->setText(text);
}

QString InputNameUI::getInputText()
{
    return ui->m_pLineEditInputName->text();
}
