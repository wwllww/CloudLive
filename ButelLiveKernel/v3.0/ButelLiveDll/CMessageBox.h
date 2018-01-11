#ifndef CMESSAGEBOX_H
#define CMESSAGEBOX_H
#include <QString>
#include <QDialog>
#include "ui_CMessageBox.h"
#include "ButelLive.h"
namespace Ui {
class CMessageBoxUI;
}
class CMessageBox: public QDialog
{
    Q_OBJECT
public:
    CMessageBox(QDialog *parent = 0,QString title = 0,QString info = 0);
    ~CMessageBox();
    void SetInfo(const QString& info);
private slots:

    void on_m_pBtnYes_clicked();

    void on_m_pBtnNO_clicked();

    void on_m_pBtnCancel_clicked();

private:
    Ui::CMessageBoxUI *ui;
};

#endif // CMESSAGEBOX_H
