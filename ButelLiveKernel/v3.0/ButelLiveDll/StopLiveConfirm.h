#ifndef STOPLIVECONFIRM_H
#define STOPLIVECONFIRM_H
#include <QString>
#include <QDialog>
#include "ui_StopLiveConfirm.h"
#include "ButelLive.h"

//enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL,ON_BUTTON_EXIT};
//enum {BUTTON_YES = 1,BUTTON_NO,BUTTON_EXIT};

namespace Ui {
class StopLiveConfirmUI;
}
class StopLiveConfirmUI: public QDialog
{
    Q_OBJECT
public:
    StopLiveConfirmUI(QDialog *parent = 0);
    ~StopLiveConfirmUI();
private slots:
    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();


private:
    Ui::StopLiveConfirmUI *ui;
    ButelLive* m_pParent;
};

#endif // STOPLIVECONFIRM_H
