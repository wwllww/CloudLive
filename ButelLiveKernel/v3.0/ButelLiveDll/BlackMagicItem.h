#ifndef CBLACKMAGICITEM_H
#define CBLACKMAGICITEM_H

#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>


class CBlackMagicItem: public QWidget
{
    Q_OBJECT
public:
    CBlackMagicItem(QWidget *parent = 0,QString name = 0,int Index = 0);
    ~CBlackMagicItem();

public:

    QLabel              m_LabelName;
    QLineEdit           m_LdtAnotherName;
    QRadioButton        m_RdBtnInput;
    QRadioButton        m_RdBtnOutput;
    int                 m_Index;              //当前项的索引号

};

#endif // CBLACKMAGICITEM_H
