#ifndef VIDEOLIST_H
#define VIDEOLIST_H
#include <QString>
#include <QDialog>
#include "ui_VideoListUI.h"
#include "ButelLive.h"
class ButelLive;
namespace Ui {
class VideoListUI;
}
class VideoListUI: public QDialog
{
    Q_OBJECT
public:
    VideoListUI(QStringList fileList,QDialog *parent = 0,int index = -1);
    ~VideoListUI();

private slots:
    void on_m_pBtnOk_clicked();
    void on_m_pBtnCancel_clicked();

    void on_m_pLWPlayList_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::VideoListUI *ui;
    ButelLive*  m_pParent;
};

#endif // VIDEOLIST_H
