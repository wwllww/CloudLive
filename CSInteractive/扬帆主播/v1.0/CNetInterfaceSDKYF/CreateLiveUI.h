#ifndef CREATELIVEUI_H
#define CREATELIVEUI_H
#include <QString>
#include <QDialog>
#include "ui_CreateLiveUI.h"

namespace Ui {
class CreateLiveUI;
}
class CreateLiveUI: public QDialog
{
    Q_OBJECT
public:
    CreateLiveUI(QDialog *parent = 0,QString title = 0,QString channelName = 0,QString programName = 0);
    ~CreateLiveUI();

    void InitFileUpload();
    void SetUploadGetAddr(char* token);
    void StartFileUpload(char* pFileUploadUrl, char* pFileProcUrl);
    void ReuploadFile(char* pCreator, char* pHost, char* pRange, char* pCID);
    void DrawImage();


private slots:

    void on_m_pBtnUpload_clicked();

    void on_m_pBtnGoLive_clicked();

protected:
    void keyPressEvent(QKeyEvent *event);

signals:
    void ProgressChanged(int,QString);
    void UploadFailed();

public slots:
    void OnProgressChanged(int iVal,QString juzhaoDownLoadUrl);
    void OnUploadFailed();

private:
    Ui::CreateLiveUI *ui;

    QString m_FileName;
    QString m_JuzhaoDownLoadUrl;
    void* Handle = NULL; //上传句柄
    char* Url = NULL;
    char* AppId = NULL;
    char* PictureUrl = NULL;
    char* Token;
    int   m_iLiveType;
    bool  m_bPullIn;
};

#endif // CREATELIVEUI_H
