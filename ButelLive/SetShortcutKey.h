#ifndef SHORTCUT_H
#define SHORTCUT_H
#include "ButelLive.h"
#include "ui_SetShortcutKey.h"

class ButelLive;
namespace Ui {
class SetShortcutKeyUI;
}

class SetShortcutKeyUI: public QDialog
{
    Q_OBJECT
public:
    SetShortcutKeyUI(ButelLive *parent = 0,QString shorcut = 0);
    ~SetShortcutKeyUI();
    void setInputText(QString text);
    QString getInputText();
private slots:
    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();
    void on_pushButton_clicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
private:
    Ui::SetShortcutKeyUI *ui;
public:    
    ButelLive* m_pParent;
    QString    m_ShortcutKeys;
};

#endif // SHORTCUT_H
