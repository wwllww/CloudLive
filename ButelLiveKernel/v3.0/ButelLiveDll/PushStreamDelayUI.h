#ifndef PUSHSTREAMDELAY_H
#define PUSHSTREAMDELAY_H
#include <QString>
#include <QDialog>
#include "ui_PushStreamDelayUI.h"
#include "ButelLive.h"
namespace Ui {
class PushStreamDelayUI;
}
class PushStreamDelayUI: public QDialog
{
    Q_OBJECT
public:
    PushStreamDelayUI(QDialog *parent = 0);
    ~PushStreamDelayUI();
    void SetInfo(const QString& info);
private slots:

    void on_m_pBtnExit_clicked();

private:
    Ui::PushStreamDelayUI *ui;
    ButelLive* m_pParent;
};

#endif // PUSHSTREAMDELAY_H
