#ifndef SDISETUI_H
#define SDISETUI_H
#include <QString>
#include <QDialog>
#include <QString>
#include <QVector>
#include "ui_SDIConfigureUI.h"
#include "ButelLive.h"

//struct SDIDeviceInfo
//{
//    QString name;
//    int     id;
//};
namespace Ui {
class SDIConfigureUI;
}
class SDIConfigureUI: public QDialog
{
    Q_OBJECT
public:
    SDIConfigureUI(QDialog *parent = 0);
    ~SDIConfigureUI();
    void DeviceListInit();
private slots:
    void on_m_pBtnOk_clicked();
    void on_m_pBtnCancel_clicked();

private:
    Ui::SDIConfigureUI *ui;
    ButelLive* m_pParent;
//    QVector<SDIDeviceInfo> m_SDIDeviceVec;
//    QStringList            m_ModeList;
};

#endif // SDISETUI_H
