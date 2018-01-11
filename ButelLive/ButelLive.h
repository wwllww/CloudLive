#ifndef BUTELLIVE_H
#define BUTELLIVE_H

#include <QMainWindow>
#include <QDialog>
#include <QMenu>
#include <QAction>
#include <QListWidgetItem>
#include <QVector>
#include <QResizeEvent>

#include <Windows.h>

#include "SceneItem.h"
#include "SLiveApi.h"
#include "PlayControl.h"
#include "CNetInerface.h"

namespace Ui {
class ButelLive;
}

//鼠标位置，用于窗口伸缩使用
#define PADDING 2
enum Direction { UP=0, DOWN=1, LEFT, RIGHT, LEFTTOP, LEFTBOTTOM, RIGHTBOTTOM, RIGHTTOP, NONE };

enum SourceType { Dianbo=0,Tupian=1, Shexiang,Hudong,Datetime,AudioCapture,AgentSource,Text,Live,MonitorCapture,WindowCapture,ProcTopWindow};


struct Vect
{
    float x;
    float y;
};

struct tagChannelInfo;

typedef struct ConfigureInfo
{
    uint64_t InstanceID;
    uint64_t StreamID;
    QString  data;

}ConfigureInfo;

typedef struct DeviceInfo
{
    QString ShowName;
    QString DeviceName;
    QString DeviceID;
}DeviceInfo;

struct DeviceInitInfo
{
    QString m_InputAudio;
    QString m_InputVideo;
    QString m_OutputAudio;
    QString m_OutputVideo;
    bool    m_bUseCamera;
    bool    m_bUseMic;
    int     m_EncodeFormat;
};


//源信息
struct StreamInfo
{
    QString       ClassName;
    uint64_t      streamID1;
    uint64_t      streamID2;
    QString       SourceName;
    int           level;
    QPoint        Position;
    int           width;
    int           height;
};

struct LinkSourceInfo
{
    int           id; //关联素材索引
    int           top;
    int           left;
    int           width;
    int           height;
    bool          bRender;
    bool          bIsAgent;
    bool          bIsCut;
    int           cutLeft;
    int           cutTop;
    int           cutWidth;
    int           cutHeight;
};

//场景信息
struct SceneInfo
{
    QString                              name;              //场景名
    QVector<LinkSourceInfo>              m_LinkMateralVec;  //关联的素材信息列表
    bool                                 bIsReserve;
    QString                              shortcutKeys;
};

//素材信息
struct MateralInfo
{
    QString       name;               //素材名
    int           type;               //类型
    QString       data;
    QString       source;             //文件名
    uint64_t      streamID1;
    uint64_t      streamID2;
};

class ButelLive : public QDialog
{
    Q_OBJECT

public:
    explicit ButelLive(tagChannelInfo* channelInfo,QWidget *parent = 0);
    ~ButelLive();

    void SLiveApiInit();
    void MenuInit();
    void region(const QPoint &cursorGlobalPoint);
    void SetItemColor(int iIndex,bool bIsSelected);
    void SetMaterialItemColor(int iIndex,bool bIsSelected);
    void SetSceneMaterialItemColor(int iIndex,bool bIsSelected);
    void SetSceneCurrentRow(int iIndex);
    int  GetSceneCurrentRow();
    void SetSceneIndexNumber();
    void SetMaterialCurrentRow(int iIndex);
    int  GetMaterialCurrentRow();
    void SetMaterialIndexNumber();
    void SetSceneMaterialCurrentRow(int iIndex);
    int  GetSceneMaterialCurrentRow();

    void SetPreviewCurrentRow(int iIndex);
    int  GetPreviewCurrentRow();

    void ResizeRenderFrame();
    void ResizeRenderFrame_2();

    void AddMateralToScene(int materal,int scene);
    int  GetPreviewSourceLinkMateralNo(int iPreNo);
    int  GetMateralSourceLinkPreviewNo(int iMatNo);
    void InitDeviceList();
    Vect MapWindowToFramePos(Vect& mousePos);
    Vect MoveMapWindowToFramePos(Vect& mousePos);
    void AddPreviewFromMateralNo(int iMatNo,bool bIsAgent = false,bool bIsRender = true);

    void SetSelectSourceIndex(int index);           //根据列表中对应行点击选中画面
    void SetSelectIndexSource(int index);           //根据画面点击选择列表中对应行

   bool InitConfig(const QString& jsonFile);
 //   bool SaveConfig(const QString& jsonFile);
    bool AddResource();
    bool SceneIsExist(const QString& name);
    bool MateralIsExist(const QString& name);

    void SetAudioPara();

public slots:
    void OnBtnToPreview();
    void OnPushStreamFPSChanged(int FPS, PushStauts *pPushStatus);
    void OnConfigResourceInit();

    void OnMoveStreamDowm(bool);
    void OnMoveStreamUp(bool);
    void OnMoveStreamTop(bool);
    void OnMoveStreamBottom(bool);

    void OnActFitScreen();
    void OnActResetSize();
    void OnActSetPosSize();
    void OnActResetCut();
    void OnActCenter();
    void OnActHCenter();
    void OnActVCenter();
    void OnActMoveLeftEdge();
    void OnActMoveTopEdge();
    void OnActMoveRightEdge();
    void OnActMoveBottomEdge();
    void OnPreiveFullScreen();
    void OnLiveAudioDBValueChanged(float LeftDb,float RightDb);

signals:

    void PushStreamFPSChanged(int FPS, PushStauts *pPushStatus);
    void ConfigResourceInit();
    void LiveAudioDBValueChanged(float LeftDb,float RightDb);

private slots:
    void on_m_pBtnScene_clicked();
    void on_m_pBtnMaterial_clicked();
    void ShowMenu();
    void OnQuickChange(bool);
    void on_m_pBtnAddMaterial_clicked();
    void on_m_pBtnAddScene_clicked();
    void OnSceneEditClicked();
    void OnSceneArrowClicked();
    void OnMaterialEditClicked();
    void OnSceneMaterialEditClicked();
    void OnManagerRename();
    void OnManagerSet();

    void on_m_pBtnDelScene_clicked();
    void on_m_pBtnMoveUp_clicked();
    void on_m_pBtnMoveDown_clicked();

    void on_m_pBtnReturnScene_clicked();

    void OnExportScene();
    void OnImportScene();

    void OnActSetSceneShortcut();
    void OnActSceneCopy();
    void OnActRename();
    void OnActReserve();
    void OnMoveToTop();
    void OnMoveToLow();

    void OnMaterialAddToCurrentScene();
    void OnMaterialRename();
    void OnMaterialSet();

    void OnRequestSource();
    void OnPictureSource();
    void OnVideoCatchSource();
    void OnInteractConnectSource();
    void OnDateTimeSource();
    void OnAudioCatchSource();
    void OnAgentSource();
    void OnTextSource();
    void OnMonitorCapture();
    void OnWindowCapture();
    void OnProcTopWindow();
    void OnLiveSource();

    void on_m_pBtnDelMaterial_clicked();

    void on_m_pBtnListen_clicked();

    void on_m_pLWScene_itemEntered(QListWidgetItem *item);

    void on_m_pBtnScnMtrlAdd_clicked();

    void on_m_pBtnLocalPreview_clicked();

    void on_m_pBtnToLive_clicked();

    void on_m_pLWScene_currentRowChanged(int currentRow);

    void on_m_pBtnScnMtrlDel_clicked();

    void on_m_pBtnStartLive_clicked();

    void on_m_pBtnEditMode_clicked();


    void on_m_pBtnScnMtrlMoveUp_clicked();

    void on_m_pBtnScnMtrlMoveDown_clicked();

    void on_m_pBtnScnMtrlMoveTop_clicked();

    void on_m_pBtnScnMtrlMoveBottom_clicked();

    void on_m_pLWSceneMateralManage_currentRowChanged(int currentRow);

    void on_m_pSldLeftCtr_sliderReleased();

    void on_m_pSldRightCtr_sliderReleased();

    void on_m_pSldPGMCtr_sliderReleased();

    void on_m_pBtnReset_clicked();

    void on_m_pBtnPreviewTo_clicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    Ui::ButelLive *ui;
    void UiInit();

    QMenu                  m_MainMenu;
    QMenu*                 m_pMenuFile;
    QMenu*                 m_pMenuLive;
    QMenu*                 m_pMenuAbout;
    QMenu*                 m_pMenuQuence;
    QMenu*                 m_pMenuManagerSquence;
    QMenu*                 m_pMenuManagerPosSize;

    bool                        m_IsLeftPressDown;  // 判断左键是否按下
    QPoint                      m_DragPosition;     // 窗口移动拖动时需要记住的点
    Direction                   m_Dir;              // 窗口大小改变时，记录改变方向

public:
    bool                        m_bAltKeyPressed;
    tagChannelInfo*             m_pChannelInfo;
    QMenu*                      m_pMenuPreview;
    QMenu                       m_PreviewMenu;        //投影窗口没有点中源右键弹出菜单
    QMenu                       m_PreviewInSourceMenu;        //投影窗口点中源右键弹出菜单
    QMenu                       m_MenuSceneEdit;      //场景编辑菜单
    QMenu                       m_MenuMaterialAdd;    //素材添加菜单
    QMenu                       m_MenuMaterialEdit;   //素材编辑菜单
    QMenu                       m_MenuSceneMaterialEdit;   //场景素材管理编辑菜单
    QListWidgetItem*            m_pLWSceneItem;
    QListWidgetItem*            m_pLWMaterialItem;
    QListWidgetItem*            m_pLWSceneMaterialItem;
    QList<bool>                 m_ItemIsSelectedList;
    QList<bool>                 m_MaterialItemIsSelectedList;
    QList<bool>                 m_SceneMaterialItemIsSelectedList;
    int                              m_iSceneNum;           // 场景编号，初始化场景名使用（如：场景1）
    int                              m_iMateralNum;         // 素材编号，初始化素材名使用（如：素材1）
    int                              m_iDianboNum;
    int                              m_iHudongNum;
    int                              m_iShexiangNum;
    int                              m_iTupianNum;
    int                              m_iDateTimeNum;
    int                              m_iTextNum;
    int                              m_iLiveNum;
    int                              m_iAudioCaptureNum;
    int                              m_iMonitorCaptureNum;
    int                              m_iWindowCaptureNum;
    int                              m_iProcTopWindowNum;
    int                              m_iAgentNum;
    bool                             m_bIsListen;
    bool                             m_bIsPreview;
    bool                             m_bIsLiving;
    bool                             m_bIsEditing;
    bool                             m_bIsFullScreenPreview;
    bool                             m_bPlayLocal;
    QList<MateralInfo*>              m_AgentSourceList;
    QList<MateralInfo*>              m_MateralList;
    QList<SceneInfo*>                m_SceneList;
    QVector<ConfigureInfo>           m_ConfigVec;
    int                              m_pixWidth = 1280;
    int                              m_pixHeight = 720;
    int                              m_pixMiniWidth = 560;
    int                              m_pixMiniHeight = 315;
    int                              outputCX;
    int                              outputCY;
    uint64_t                         Instance;
    uint64_t                         Instance_2;
    float                            renderFrameOffsetx;
    float                            renderFrameOffsety;
    float                            renderFrameSizex;
    float                            renderFrameSizey;
    QVector<uint64_t>                m_VideoInstanceVec;
    QVector<uint64_t>                m_AgentInstanceVec;
    int                              m_CurrentSceneNo;         //记录场景号，切换场景时使用
    QVector<DeviceInfo>              VideoList;
    QStringList                      AudioRenderList;
    QStringList                      AudioCaptureList;
    QStringList                      DisplayDeviceList;
    int                              m_SelLevel = -1;
};

#endif // BUTELLIVE_H
