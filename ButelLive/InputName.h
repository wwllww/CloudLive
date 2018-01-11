#ifndef INPUTNAME_H
#define INPUTNAME_H
#include "ButelLive.h"
#include "ui_InputName.h"

class ButelLive;
namespace Ui {
class InputNameUI;
}
enum{Scene = 0,Materal = 1};
class InputNameUI: public QDialog
{
    Q_OBJECT
public:
    InputNameUI(ButelLive *parent = 0,int type = 0);
    ~InputNameUI();
    void setInputText(QString text);
    QString getInputText();
private slots:
    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();

private:
    Ui::InputNameUI *ui;
public:    
    ButelLive* m_pParent;
    int        m_type;
};

#endif // INPUTNAME_H
