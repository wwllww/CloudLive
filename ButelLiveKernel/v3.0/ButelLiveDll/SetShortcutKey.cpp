#include "SetShortcutKey.h"
#include "MyMessageBox.h"
#include <QDialog>
#include <QLabel>

SetShortcutKeyUI::SetShortcutKeyUI(ButelLive *parent,QString shorcut) :
    QDialog(parent),
    ui(new Ui::SetShortcutKeyUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    m_pParent = parent;
    m_ShortcutKeys = shorcut;
    setModal(true);
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("Scene shortcut key"));
    ui->m_pLineEditInputName->setText(shorcut);
    ui->m_pLineEditInputName->setInputMethodHints(Qt::ImhHiddenText);
    ui->m_pLineEditInputName->installEventFilter(this);
}

SetShortcutKeyUI::~SetShortcutKeyUI()
{
    delete ui;
}

bool SetShortcutKeyUI::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->m_pLineEditInputName)
    {
        if(event->type() == QEvent::InputMethod)
        {
            if(!hasFocus())
            {
                QInputMethodEvent* iEvent = dynamic_cast<QInputMethodEvent*>(event);
                iEvent->setCommitString("");
                return true;
            }
         }
         else if(event->type() == QEvent::KeyPress)
         {
             QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
             if (keyEvent->modifiers() == Qt::ControlModifier )
             {
                 QString shotcutKeys = "Ctrl+";
                 switch (keyEvent->key()) {
                 case Qt::Key_0:
                     shotcutKeys += "0";
                     break;
                 case Qt::Key_1:
                     shotcutKeys += "1";
                     break;
                 case Qt::Key_2:
                     shotcutKeys += "2";
                     break;
                 case Qt::Key_3:
                     shotcutKeys += "3";
                     break;
                 case Qt::Key_4:
                     shotcutKeys += "4";
                     break;
                 case Qt::Key_5:
                     shotcutKeys += "5";
                     break;
                 case Qt::Key_6:
                     shotcutKeys += "6";
                     break;
                 case Qt::Key_7:
                     shotcutKeys += "7";
                     break;
                 case Qt::Key_8:
                     shotcutKeys += "8";
                     break;
                 case Qt::Key_9:
                     shotcutKeys += "9";
                     break;
                 case Qt::Key_A:
                     shotcutKeys += "A";
                     break;
                 case Qt::Key_B:
                     shotcutKeys += "B";
                     break;
                 case Qt::Key_C:
                     shotcutKeys += "C";
                     break;
                 case Qt::Key_D:
                     shotcutKeys += "D";
                     break;
                 case Qt::Key_E:
                     shotcutKeys += "E";
                     break;
                 case Qt::Key_F:
                     shotcutKeys += "F";
                     break;
                 case Qt::Key_G:
                     shotcutKeys += "G";
                     break;
                 case Qt::Key_H:
                     shotcutKeys += "H";
                     break;
                 case Qt::Key_I:
                     shotcutKeys += "I";
                     break;
                 case Qt::Key_J:
                     shotcutKeys += "J";
                     break;
                 case Qt::Key_K:
                     shotcutKeys += "K";
                     break;
                 case Qt::Key_L:
                     shotcutKeys += "L";
                     break;
                 case Qt::Key_M:
                     shotcutKeys += "M";
                     break;
                 case Qt::Key_N:
                     shotcutKeys += "N";
                     break;
                 case Qt::Key_O:
                     shotcutKeys += "O";
                     break;
                 case Qt::Key_P:
                     shotcutKeys += "P";
                     break;
                 case Qt::Key_Q:
                     shotcutKeys += "Q";
                     break;
                 case Qt::Key_R:
                     shotcutKeys += "R";
                     break;
                 case Qt::Key_S:
                     shotcutKeys += "S";
                     break;
                 case Qt::Key_T:
                     shotcutKeys += "T";
                     break;
                 case Qt::Key_U:
                     shotcutKeys += "U";
                     break;
                 case Qt::Key_V:
                     shotcutKeys += "V";
                     break;
                 case Qt::Key_W:
                     shotcutKeys += "W";
                     break;
                 case Qt::Key_X:
                     shotcutKeys += "X";
                     break;
                 case Qt::Key_Y:
                     shotcutKeys += "Y";
                     break;
                 case Qt::Key_Z:
                     shotcutKeys += "Z";
                     break;
                 default:
                     shotcutKeys = ui->m_pLineEditInputName->text();
                     break;
                 }
                 ui->m_pLineEditInputName->setText(shotcutKeys);
             }
             return true;
         }
         else
         {
             return false;
         }
    }
    else
    {
         return QDialog::eventFilter(watched, event);
    }
}

void SetShortcutKeyUI::on_m_pBtnOk_clicked()
{
 //   done(ON_BUTTON_OK);
    QString strShortcut = getInputText();
    QString curSceneShortcut = m_pParent->m_SceneList[m_pParent->GetSceneCurrentRow()]->shortcutKeys;
    //如果修改前的快捷键和修改后的快捷键一样，直接取消操作
    if(curSceneShortcut == strShortcut)
    {
        done(ON_BUTTON_CANCEL);
    }
    else
    {
        bool bShortcutExist = false;
        for(int i = 0; i < m_pParent->actions().count(); i++)
        {
            if(m_pParent->actions().at(i)->shortcut().toString() == strShortcut)
            {
                bShortcutExist = true;
                break;
            }
        }
        if(bShortcutExist)
        {
            MyMessageBox message(this, tr("error"), tr("The shortcut key is already occupied!"),BUTTON_YES);
            message.exec();
        }
        else
        {
            done(ON_BUTTON_OK);
        }
    }
}

void SetShortcutKeyUI::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
}
void SetShortcutKeyUI::setInputText(QString text)
{
    ui->m_pLineEditInputName->setText(text);
}

QString SetShortcutKeyUI::getInputText()
{
    return ui->m_pLineEditInputName->text();
}

void SetShortcutKeyUI::on_pushButton_clicked()
{
    ui->m_pLineEditInputName->clear();
}
