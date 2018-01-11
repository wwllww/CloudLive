#ifndef LOGIN_H
#define LOGIN_H
#include "ButelLive.h"
#include "ui_Login.h"
#include <QSettings>
#include "ChannelAndProgram.h"
class ChannelAndProgramUI;
namespace Ui {
class LoginUI;
}

class LoginUI: public QDialog
{
    Q_OBJECT
public:
    LoginUI(QWidget *parent = Q_NULLPTR);
    virtual ~LoginUI();
    void LoginStateInfo(int state);           //登录信息
    void AuthorizeStateInfo(int state);    //鉴权信息
    void OperAccept();
private slots:
    void on_m_pBtnLogin_clicked();
    void on_m_pBtnUserNameDel_clicked();
protected:
    void keyPressEvent(QKeyEvent *event);
private:
    Ui::LoginUI *ui;
    std::wstring m_WUserName;
    std::wstring m_WPassWord;
    std::wstring m_WToken;
//    QSettings*   m_pSettingConf;
    ChannelAndProgramUI* m_pChannelUI;


};

#endif // LOGIN_H
