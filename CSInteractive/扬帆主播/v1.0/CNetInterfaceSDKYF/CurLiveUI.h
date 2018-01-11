#ifndef CURLIVEUI_H
#define CURLIVEUI_H
#include <QString>
#include <QDialog>
#include "ui_CurLiveUI.h"

namespace Ui {
class CurLiveUI;
}
class CurLiveUI: public QDialog
{
    Q_OBJECT
public:
    CurLiveUI(QDialog *parent = 0,QString title = 0,QString info = 0);
    ~CurLiveUI();

private slots:
    void on_m_pBtnContinue_clicked();

    void on_m_pBtnCreate_clicked();

private:
    Ui::CurLiveUI *ui;
};

#endif // CURLIVEUI_H
