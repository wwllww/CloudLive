#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H
#include <QString>
#include <QDialog>
#include "ui_MyMessageBox.h"
#include "ButelLive.h"

enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL};
enum {BUTTON_YES = 1,BUTTON_NO};

namespace Ui {
class MyMessageBoxUI;
}
class MyMessageBox: public QDialog
{
    Q_OBJECT
public:
    MyMessageBox(QDialog *parent = 0,QString title = 0,QString info = 0,int buttons = 0);
    ~MyMessageBox();
private slots:
    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();

private:
    Ui::MyMessageBoxUI *ui;
};

#endif // MESSAGEBOX_H
