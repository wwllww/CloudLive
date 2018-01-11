#ifndef CHANNELANDPROGRAN_H
#define CHANNELANDPROGRAN_H
//#include "ButelLive.h"
#include "ui_ChannelAndProgram.h"
//#include "CNetInerface.h"
#include "Login.h"
class LoginUI;
namespace Ui {
class ChannelAndProgramUI;
}

class ChannelAndProgramUI: public QDialog
{
    Q_OBJECT
public:
    ChannelAndProgramUI(QWidget *parent = Q_NULLPTR);
    virtual ~ChannelAndProgramUI();
    void AddChannelItem(QString name);
    void AddProgramItem(QString name);
    void SetInformation(QString infor);
    void ClearAllChannelItem();
signals:
    void GetChannelListFinished();
private slots:

    void on_m_pBtnEnter_clicked();

    void on_m_pBtnBack_clicked();

    void on_m_pCombChannel_currentIndexChanged(int index);

    void OnGetChannelListFinished();
protected:
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::ChannelAndProgramUI *ui;
    int m_iCurrentChannelIndex;

public:
    LoginUI*    m_pLoginUi;
};

#endif // CHANNELANDPROGRAN_H
