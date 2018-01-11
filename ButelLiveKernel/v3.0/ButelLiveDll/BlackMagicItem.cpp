#include "BlackMagicItem.h"
#include <QHBoxLayout>
CBlackMagicItem::CBlackMagicItem(QWidget *parent,QString name,int Index):
    QWidget(parent)
{
    m_LabelName.setText(name);
    m_Index = Index;
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    setFixedHeight(30);

    m_LabelName.setFixedSize(250,30);
    m_LdtAnotherName.setFixedHeight(30);
    m_RdBtnInput.setFixedHeight(30);
    m_RdBtnOutput.setFixedHeight(30);
    m_RdBtnInput.setText(tr("Input"));
    m_RdBtnOutput.setText(tr("Output"));
    m_LabelName.setAlignment(Qt::AlignCenter);

    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(&m_LabelName);
//    hBoxLayout->addSpacing(20);
    hBoxLayout->addWidget(&m_LdtAnotherName);
//    hBoxLayout->addSpacing(20);
    hBoxLayout->addWidget(&m_RdBtnInput);
    hBoxLayout->addSpacing(20);
    hBoxLayout->addWidget(&m_RdBtnOutput);
    hBoxLayout->setContentsMargins(0,0,0,0);
//    hBoxLayout->setSpacing(20);

    hBoxLayout->setAlignment(&m_LabelName,Qt::AlignCenter);
    hBoxLayout->setAlignment(&m_LdtAnotherName,Qt::AlignCenter);
    hBoxLayout->setAlignment(&m_RdBtnInput,Qt::AlignRight);
    hBoxLayout->setAlignment(&m_RdBtnOutput,Qt::AlignLeft);

    setLayout(hBoxLayout);

    connect(&m_LdtAnotherName,SIGNAL(textChanged(QString)),parent,SLOT(OnBlackMagicChanged()));
    connect(&m_RdBtnInput,SIGNAL(clicked(bool)),parent,SLOT(OnBlackMagicChanged()));
    connect(&m_RdBtnOutput,SIGNAL(clicked(bool)),parent,SLOT(OnBlackMagicChanged()));
}
CBlackMagicItem::~CBlackMagicItem()
{

}
