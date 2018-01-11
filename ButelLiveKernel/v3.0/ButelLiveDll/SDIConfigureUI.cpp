#include "SDIConfigureUI.h"
#include <QDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QListWidgetItem>
#include <QStringList>
#include "SDIConfigureItem.h"
#include "SLiveApi.h"
#include "LogDeliver.h"
#include "configoper.h"

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif
void ConvertGBKToUtf8(std::string& utf8, const char *strGBK);

SDIConfigureUI::SDIConfigureUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::SDIConfigureUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    m_pParent = (ButelLive*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("BlackMagic.Output.Setting"));
    ui->m_pLWSDIDevice->setFocusPolicy(Qt::NoFocus);
    DeviceListInit();
}

SDIConfigureUI::~SDIConfigureUI()
{
    delete ui;
}

void SDIConfigureUI::on_m_pBtnOk_clicked()
{
    int iCount = ui->m_pLWSDIDevice->count();
    __SDIOut* pSDIOut = (__SDIOut*)malloc(iCount * sizeof(__SDIOut));
    __SDIOut* pTmpSDIOut = pSDIOut;
    for(int i = 0; i < iCount; i++,pTmpSDIOut++)
    {
        CSDIConfigureItem* pSDICfgItem = (CSDIConfigureItem*)ui->m_pLWSDIDevice->itemWidget(ui->m_pLWSDIDevice->item(i));
        pTmpSDIOut->Id = m_pParent->m_SDIDeviceVec[i].id;
        pTmpSDIOut->bEnable = pSDICfgItem->m_ChkState.isChecked();
        pTmpSDIOut->SourceName = (char*)malloc(strlen(pSDICfgItem->m_CombOutputSource.currentText().toLocal8Bit().data()) + 1);
        strcpy((char*)pTmpSDIOut->SourceName,pSDICfgItem->m_CombOutputSource.currentText().toLocal8Bit().data());
        pTmpSDIOut->Format = (char*)malloc(strlen(pSDICfgItem->m_CombDeviceStandard.currentText().toLocal8Bit().data()) + 1);
        strcpy((char*)pTmpSDIOut->Format,pSDICfgItem->m_CombDeviceStandard.currentText().toLocal8Bit().data());

        m_pParent->m_ArrSDIOut[i].bEnable = pTmpSDIOut->bEnable;
        m_pParent->m_ArrSDIOut[i].Id = m_pParent->m_SDIDeviceVec[i].id;
        if(m_pParent->m_ArrSDIOut[i].SourceName)
        {
            free((char*)m_pParent->m_ArrSDIOut[i].SourceName);
            m_pParent->m_ArrSDIOut[i].SourceName = NULL;
        }
        if(m_pParent->m_ArrSDIOut[i].Format)
        {
            free((char*)m_pParent->m_ArrSDIOut[i].Format);
            m_pParent->m_ArrSDIOut[i].Format = NULL;
        }
        m_pParent->m_ArrSDIOut[i].SourceName = (char*)malloc(strlen(pSDICfgItem->m_CombOutputSource.currentText().toLocal8Bit().data()) + 1);
        m_pParent->m_ArrSDIOut[i].Format = (char*)malloc(strlen(pSDICfgItem->m_CombDeviceStandard.currentText().toLocal8Bit().data()) + 1);
        strcpy((char*)m_pParent->m_ArrSDIOut[i].SourceName,pSDICfgItem->m_CombOutputSource.currentText().toLocal8Bit().data());
        strcpy((char*)m_pParent->m_ArrSDIOut[i].Format,pSDICfgItem->m_CombDeviceStandard.currentText().toLocal8Bit().data());
    }
    if(SLiveSetBlackMagicOut(pSDIOut) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveSetBlackMagicOut failed!", __FUNCTION__);
    }
    pTmpSDIOut = pSDIOut;
    for(int i = 0; i < iCount; i++,pTmpSDIOut++)
    {
        free((char*)pTmpSDIOut->SourceName);
        free((char*)pTmpSDIOut->Format);
    }
    free(pSDIOut);
    pSDIOut = NULL;

    done(ON_BUTTON_OK);
}

void SDIConfigureUI::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
}

void SDIConfigureUI::DeviceListInit()
{
//    char* DevicesList;
//    if(SLiveGetBlackMagicDevices(&DevicesList) < 0)
//    {
//        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveGetBlackMagicDevices failed!", __FUNCTION__);
//        return;
//    }
//    else
//    {
//        std::string utf;
//        ConvertGBKToUtf8(utf,DevicesList);
//        QString val(utf.c_str());
//        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
//        if(json_root_doc.isNull())
//            return;
//        QJsonObject json_root_obj = json_root_doc.object();
//        QJsonValue devicelist_value = json_root_obj.value(QString("DevicesList"));
//        if(devicelist_value.isArray())
//        {
//            QJsonArray devicelist_arr = devicelist_value.toArray();
//            for(int i = 0; i < devicelist_arr.count(); i++)
//            {
//                SDIDeviceInfo deviceInfo;
//                QJsonValue devicelist_arr_item =  devicelist_arr.at(i);
//                QJsonObject devicelist_arr_item_obj = devicelist_arr_item.toObject();

//                QJsonValue devicelist_arr_item_Name_value = devicelist_arr_item_obj.take("Name");
//                QString Name = devicelist_arr_item_Name_value.toString();

//                QJsonValue devicelist_arr_item_ID_value = devicelist_arr_item_obj.take("DeviceID");
//                int ID = devicelist_arr_item_ID_value.toInt();

//                deviceInfo.name = Name;
//                deviceInfo.id = ID;
//                m_SDIDeviceVec.append(deviceInfo);
//            }
//        }
//        SLiveFreeMemory(DevicesList);
//    }

//    char *DisplayModeList;
//    if(SLiveGetBlackMagicDisplayMode(&DisplayModeList) < 0)
//    {
//        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveGetBlackMagicDisplayMode failed!", __FUNCTION__);
//        return;
//    }
//    else
//    {
//        std::string utf;
//        ConvertGBKToUtf8(utf,DisplayModeList);
//        QString val(utf.c_str());
//        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
//        if(json_root_doc.isNull())
//            return;
//        QJsonObject json_root_obj = json_root_doc.object();
//        QJsonValue modelist_value = json_root_obj.value(QString("DisplayModeList"));
//        if(modelist_value.isArray())
//        {
//            QJsonArray modelist_arr = modelist_value.toArray();
//            for(int i = 0; i < modelist_arr.count(); i++)
//            {
//                QString modeVal = modelist_arr[i].toString();
//                m_ModeList.append(modeVal);
//            }
//        }
//        SLiveFreeMemory(DisplayModeList);
//    }


//    m_pParent->InitDeviceList();
//    m_pParent->InitSDIDeviceList();
    QStringList sourceList;
    sourceList.append(tr("PGM"));
    for(int i = 0; i < m_pParent->m_MateralList.count(); i++)
    {
        if(m_pParent->m_MateralList[i]->type == Dianbo || m_pParent->m_MateralList[i]->type == Shexiang || m_pParent->m_MateralList[i]->type == Hudong || m_pParent->m_MateralList[i]->type == Live)
        {
            sourceList.append(m_pParent->m_MateralList[i]->name);
        }
    }

    //根据遍历出的设备列表信息动态加载
    for(int i = 0; i < m_pParent->m_SDIDeviceVec.count(); i++)
    {
        CSDIConfigureItem* pSDICfgItem = new CSDIConfigureItem(this,m_pParent->m_SDIDeviceVec[i].name,i);
        QListWidgetItem* pLWItem = new QListWidgetItem(ui->m_pLWSDIDevice);

        pSDICfgItem->m_CombDeviceStandard.addItems(m_pParent->m_ModeList);
        pSDICfgItem->m_CombOutputSource.addItems(sourceList);

        Log::writeMessage(LOG_RTSPSERV, 1, "%s m_SceneCfgSDIcount = %d", __FUNCTION__,m_pParent->m_SceneCfgSDIcount);
        if(m_pParent->m_SceneCfgSDIcount > 0)
        {
            pSDICfgItem->m_ChkState.setChecked(m_pParent->m_ArrSDIOut[i].bEnable);
            Log::writeMessage(LOG_RTSPSERV, 1, "%s SourceName = %s", __FUNCTION__,m_pParent->m_ArrSDIOut[i].SourceName);
            Log::writeMessage(LOG_RTSPSERV, 1, "%s Format = %s", __FUNCTION__,m_pParent->m_ArrSDIOut[i].Format);

            std::string utf;
            ConvertGBKToUtf8(utf,m_pParent->m_ArrSDIOut[i].SourceName);
            QString val(utf.c_str());

            pSDICfgItem->m_CombOutputSource.setCurrentText(val);
            pSDICfgItem->m_CombDeviceStandard.setCurrentText(m_pParent->m_ArrSDIOut[i].Format);

            if(0 == i && !ConfigOper::instance()->m_AnotherName1.isEmpty())
                pSDICfgItem->m_LabelName.setText(ConfigOper::instance()->m_AnotherName1);
            else if(1 == i && !ConfigOper::instance()->m_AnotherName2.isEmpty())
                pSDICfgItem->m_LabelName.setText(ConfigOper::instance()->m_AnotherName2);
            else if(2 == i && !ConfigOper::instance()->m_AnotherName3.isEmpty())
                pSDICfgItem->m_LabelName.setText(ConfigOper::instance()->m_AnotherName3);
            else if(3 == i && !ConfigOper::instance()->m_AnotherName4.isEmpty())
                pSDICfgItem->m_LabelName.setText(ConfigOper::instance()->m_AnotherName4);

        }
//        else
//        {
//            m_pParent->m_ArrSDIOut[i].bEnable = true;
//            pSDICfgItem->m_ChkState.setChecked(true);
//        }

        pLWItem->setSizeHint(QSize(0,50));
        ui->m_pLWSDIDevice->addItem(pLWItem);
        ui->m_pLWSDIDevice->setItemWidget(pLWItem,pSDICfgItem);
    }
    m_pParent->m_SceneCfgSDIcount = m_pParent->m_SDIDeviceVec.count();
}
