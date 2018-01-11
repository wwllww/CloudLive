#ifndef POSITIONSIZESET_H
#define POSITIONSIZESET_H
#include "ButelLive.h"
#include "ui_PositionSizeSet.h"

namespace Ui {
class PositionSizeSetUI;
}

class PositionSizeSetUI: public QDialog
{
    Q_OBJECT
public:
    PositionSizeSetUI(ButelLive *parent = 0,VideoArea *Area = 0);
    ~PositionSizeSetUI();
private slots:

    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();

    void on_m_pBtnSetGlbCoord_clicked();

private:
    Ui::PositionSizeSetUI *ui;
    ButelLive*            m_pParent;
};

#endif // POSITIONSIZESET_H
