#ifndef LOGLEVEL_H
#define LOGLEVEL_H
#include <QString>
#include <QDialog>
#include "ui_SetLogLevelUI.h"
#include "ButelLive.h"

//enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL,ON_BUTTON_EXIT};
//enum {BUTTON_YES = 1,BUTTON_NO,BUTTON_EXIT};

namespace Ui {
class SetLogLevelUI;
}
class SetLogLevelUI: public QDialog
{
    Q_OBJECT
public:
    SetLogLevelUI(QDialog *parent = 0);
    ~SetLogLevelUI();
private slots:
    void on_m_pBtnOk_clicked();
    void on_m_pBtnCancel_clicked();

private:
    Ui::SetLogLevelUI *ui;
};

#endif // LOGLEVEL_H
