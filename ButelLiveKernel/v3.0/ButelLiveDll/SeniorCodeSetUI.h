#ifndef SENIORCODESET_H
#define SENIORCODESET_H
#include "SettingUI.h"
#include "ui_SeniorCodeSetUI.h"

namespace Ui {
class SeniorCodeSetUI;
}

class SeniorCodeSetUI: public QDialog
{
    Q_OBJECT
public:
    SeniorCodeSetUI(QDialog *parent = 0,bool bFirstLive = true);
    ~SeniorCodeSetUI();

    SettingUI* m_pParent;
private slots:
    void on_m_pBtnOk_clicked();
    void on_m_pBtnCancel_clicked();
    void on_m_pCombCPUPreset_currentIndexChanged(int index);

    void on_m_pCombCodeCfgFile_activated(const QString &arg1);

    void on_m_pSBoxKeyFrameInterval_valueChanged(int arg1);

    void on_m_pSBoxBFrameNum_valueChanged(int arg1);

private:
    Ui::SeniorCodeSetUI *ui;
    bool m_bFirstLive;

};

#endif // SENIORCODESET_H
