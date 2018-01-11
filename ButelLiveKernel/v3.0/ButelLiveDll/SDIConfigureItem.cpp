#include "SDIConfigureItem.h"
#include <QHBoxLayout>

CSDIConfigureItem::CSDIConfigureItem(QWidget *parent,QString name,int Index):
    QWidget(parent)
{
    m_LabelName.setText(name);
    m_Index = Index;
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    setFixedHeight(30);

    m_LabelName.setFixedHeight(30);
    m_CombOutputSource.setFixedHeight(30);
    m_CombDeviceStandard.setFixedHeight(30);
    m_ChkState.setFixedHeight(30);

    m_ChkState.setStyleSheet("QCheckBox::indicator:unchecked"
                             "{"
                             "border-image: url(:/images/slider_off.png);"
                             "color:rgb(0,0,0);"
                             "width:34px;"
                             "height:22px;"
                             "}"
                             "QCheckBox::indicator:checked"
                             "{"
                             "border-image: url(:/images/slider_on.png);"
                             "color:rgb(0,0,0);"
                             "width:34px;"
                             "height:22px;"
                             "}"
                             );

    QString combStyle = "QComboBox::drop-down"
                            "{"
                            "border-image: url(:images/arrow_down.png);"
                            "width:10px;"
                            "height:10px;"
                            "subcontrol-origin: padding;"
                            "subcontrol-position: center right;"
                            "margin: 14 8px;"
                            "}"
                            "QComboBox"
                            "{"
                            "height:20px;"
                            "border: 1px solid #dddddd;"
                            "background-color: rgb(221,221,221);"
                            "color: #2a2a2a;"
                            "border-radius: 4px; "
                            "}";
    m_CombOutputSource.setStyleSheet(combStyle);
    m_CombDeviceStandard.setStyleSheet(combStyle);

    m_LabelName.setAlignment(Qt::AlignCenter);

    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(&m_LabelName);
    hBoxLayout->addWidget(&m_CombOutputSource);
    hBoxLayout->addWidget(&m_CombDeviceStandard);
    hBoxLayout->addWidget(&m_ChkState);
    hBoxLayout->setContentsMargins(0,0,0,0);
    hBoxLayout->setSpacing(30);
    hBoxLayout->setAlignment(&m_ChkState,Qt::AlignCenter);
    setLayout(hBoxLayout);
}
CSDIConfigureItem::~CSDIConfigureItem()
{

}
