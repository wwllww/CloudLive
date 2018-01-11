#ifndef ABOUTUI_H
#define ABOUTUI_H
#include <QDialog>
#include "ui_AboutUI.h"

namespace Ui {
class AboutUI;
}
class AboutUI: public QDialog
{
    Q_OBJECT
public:
    AboutUI(QDialog *parent = 0,QString title = 0,QString version = 0);
    ~AboutUI();
private slots:
    void on_m_pBtnOk_clicked();

private:
    Ui::AboutUI *ui;
};

#endif // ABOUTUI_H
