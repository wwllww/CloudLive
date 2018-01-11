#ifndef CCHANNELITEM_H
#define CCHANNELITEM_H
#include <QLabel>
#include <QRadioButton>
class CChannelItem: public QWidget
{
    Q_OBJECT
public:
    CChannelItem(QWidget *parent = 0,QString name = 0,int index = 0);
    void SetDefault();
public:
    QLabel               m_LabelDefault;
    QLabel               m_LabelName;
    QRadioButton         m_RdBtnSel;
    int                 m_index;
signals:
    void EnterButelLive();
public slots:
    void OnRdBtnSelClicked();
};

#endif // CCHANNELITEM_H
