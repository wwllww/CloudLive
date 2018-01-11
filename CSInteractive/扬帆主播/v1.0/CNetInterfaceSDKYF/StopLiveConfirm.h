#ifndef STOPLIVECONFIRM_H
#define STOPLIVECONFIRM_H
#include <QString>
#include <QDialog>
#include "ui_StopLiveConfirm.h"

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
};

#endif // STOPLIVECONFIRM_H
