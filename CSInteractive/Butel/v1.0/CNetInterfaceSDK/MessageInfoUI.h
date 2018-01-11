#ifndef MESSAGEINFO_H
#define MESSAGEINFO_H
#include <QString>
#include <QDialog>
#include "ui_MessageInfoUI.h"
namespace Ui {
class MessageInfoUI;
}
class PushStreamDelayUI: public QDialog
{
    Q_OBJECT
public:
    MessageInfoUI(QDialog *parent = 0);
    ~MessageInfoUI();
    void SetInfo(const QString& info);
private slots:

    void on_m_pBtnExit_clicked();

private:
    Ui::MessageInfoUI *ui;
};

#endif // MESSAGEINFO_H
