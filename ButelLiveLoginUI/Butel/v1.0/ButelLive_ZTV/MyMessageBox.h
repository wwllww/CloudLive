#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H
#include <QString>
#include <QDialog>
#include "ui_MyMessageBox.h"
//#include "ButelLive.h"

enum {ON_BUTTON_CANCEL = 0,ON_BUTTON_OK,ON_BUTTON_EXIT};
enum {BUTTON_YES = 1,BUTTON_NO = 2};

namespace Ui {
class MyMessageBoxUI;
}
class MyMessageBox: public QDialog
{
    Q_OBJECT
public:
    MyMessageBox(QDialog *parent = 0,QString title = 0,QString info = 0,int buttons = 0);
    ~MyMessageBox();
    void SetInfo(const QString& info);
private slots:
    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();

private:
    Ui::MyMessageBoxUI *ui;
//    ButelLive* m_pParent;
};

#endif // MESSAGEBOX_H
