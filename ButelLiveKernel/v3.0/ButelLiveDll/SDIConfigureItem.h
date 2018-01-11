#ifndef CSDICONFIGUREITEM_H
#define CSDICONFIGUREITEM_H

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>


class CSDIConfigureItem: public QWidget
{
    Q_OBJECT

public:
    CSDIConfigureItem(QWidget *parent = 0,QString name = 0,int Index = 0);
    ~CSDIConfigureItem();
public:
    QLabel              m_LabelName;
    QComboBox           m_CombOutputSource;
    QComboBox           m_CombDeviceStandard;
    QCheckBox           m_ChkState;
    int                 m_Index;              //当前项的索引号
};

#endif // CSDICONFIGUREITEM_H
