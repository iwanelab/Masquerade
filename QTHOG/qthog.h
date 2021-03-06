#ifndef QTHOG_H
#define QTHOG_H
#include <QtGui/QMainWindow>
#include "ui_qthog.h"
#include "PTM/TMatrixUtility.h"
#include "RegionData.h"
#include "utilities.h"
#include "Constants.h"
#include <opencv.hpp>
#include "gpu.hpp"
#include <QTimer>

//test code
#include <QApplication>
// secuwatcherMask用
#include <windows.h>
#include "IDetector.h"

// マスク処理用
#include "EnmaskImageAccessor.h"
#include "ProjectionImageAccessor.h"

#define _CRTDBG_MAP_ALLOC

class AviManager;
class DlgFaceGrid;
class Calculator;
class msImageProcessor;
class DlgBatchProcess;
class IdleWatcher;

class QtHOG : public QMainWindow
{
	Q_OBJECT

public:
	QtHOG(QWidget *parent = 0, Qt::WFlags flags = 0);
	~QtHOG();

private:
	// Iniファイルへ
	// qthog内用のパラメーター
	int m_PredictSearchRange;		// PREDICT_SEARCH_RANGE
	int m_InitialSearchRange;		// INITIAL_SEARCH_RANGE
	int m_ResizeWidth;				// RESIZE_WIDTH
	int m_ResizeHeight;				// RESIZE_HEIGHT

	//GPU
	// なぜかcv::gpu::gpiMatのコンストラクタで戻ってこなくなるため未使用にする。（動くときもある？）
	//const bool useGPU = cv::gpu::getCudaEnabledDeviceCount() > 0;
	//bool m_useGPU;					// USE_GPU

	//jpeg Q
	int m_JpegQuality;				// JPEG_QUALITY
	bool m_OutputMaskLog;			// OUTPUT_MASK_LOG

	double m_FaceThreshold;			// FACE_THRESHOLD

	// 検出処理タイプ
	Constants::DetectMethod m_detectMethod;

	// イメージ展開方法
	Constants::ImageType m_imageType;
	// Iniファイル ここまで

	Ui::QtHOGClass ui;
	AviManager *m_pAviManager;
	StructData *m_pStructData;
	Calculator *m_pCalculator;

	DlgFaceGrid *m_pDlgFaceGrid;
	DlgBatchProcess *m_pDlgBatch;
	Constants::Mode m_mode;
	std::vector<SRegion::Type> m_TargetType;
	cv::Point m_selectedRegion[2];
	StructData::iterator<SRegion> m_curRegion;
	int m_curCamera;		// kosshy add 20131022
	int m_curFrame;
	std::set<int> m_selectedItems;
	int m_lastSelected;
	int m_targetObject;
	bool m_bMidDrag;
	double m_colorThresh;
	std::vector<float> m_decisionPlane;
	bool m_bCancel;
	QIcon m_iconClock;
	QIcon m_iconWarning;
	QIcon m_iconDone;
	QIcon m_iconDisc;
	int m_lockonTarget;
	IdleWatcher *m_pIdleWatcher;
	QTime *m_pTime;
	cv::PCA *m_pPCA;
	int m_blurIntensityForFaces;
	int m_blurIntensityForPlates;

	// ステータスバー
	QLabel m_executeStatus;
	QLabel m_selectedStatus;

	// マスク結果
	bool m_bOutputMaskingLog;

	// test カスケード分類機
	cv::CascadeClassifier cascade;


	// 外部認識処理
	TCHAR m_childProcName[MAX_PATH];
	// 認識処理クラス
	IDetector *m_detector;

	// オブジェクトファイル名
	QString m_objectfile;

	// 平面展開処理
	CProjectionImageAccessor m_projectionImage;

signals:
	void sgSelectionChanged();
	void sgImageChanged(const std::vector<int> &changedImages, bool bSelect);

private slots:
	void on_actionQuit_triggered();
	void on_actionRead_Image_triggered();
	void on_sliderFrame_valueChanged(int);
	void on_sliderScale_valueChanged(int);
	void on_checkBoxAutoScale_clicked(bool);
	void on_actionWrite_Objects_triggered();
	void on_actionRead_Objects_triggered();
	void on_menuTest_aboutToShow();
	void on_checkBoxFace_clicked();
	void on_checkBoxPlate_clicked();
	void on_actionFace_CV_Tracking_triggered();
	void on_actionShow_Faces_triggered();
	void on_actionExport_Masked_Images_triggered(); //FileMenu -> ExportMaskedImages
	void on_actionDetect_Faces_triggered(); // DetectMenu -> DetectFaces
	void on_actionClear_Objects_triggered();
	void on_actionShowBatch_Dialog_triggered();
	void on_actionForce_Auto_Save_triggered();
	void on_actionSet_Blur_Intensity_For_Faces_triggered();
	void on_actionSet_Blur_Intensity_For_Plates_triggered();
	void on_actionAbout_Masquerade_triggered();
	void on_actionSelect_All_triggered();	void on_pushButtonManualDetect_clicked(bool bChecked);

	void on_editFrame_returnPressed();

	void onImageViewClicked(QGraphicsSceneMouseEvent *);
	void onImageViewReleased(QGraphicsSceneMouseEvent *);

	void onSelectionChanged();
	void onItemChanged();
	void onItemDeleted(const std::set<int> &deleteSet);
	void onItemDoubleClicked(int);
	void onItemDoubleClicked(int, int, bool);

	void onWheelEvent(QWheelEvent * event);
	void onAutoTrackingRequired(int, int);

	void onScrollBack();
	void onScrollForward();
	void onManualDetect();
	void onAutoTrackingRequiredByR();
	void onAutoBackwardTrackingRequiredByE();	//kosshy add 20131004
	void onAutoMultiTrackingRequiredByF();		//kosshy add 20130924
	void onIdle(bool bForce = false);

private:
	void keyPressEvent(QKeyEvent *event);
	void resizeEvent(QResizeEvent *event);
	void setFrame(int frame, bool bUpdate = false, bool bKey = false);
	void detect_Panoramic(int frame);
	void detect_Perspective(int frame);
	void detect(cv::Mat image, std::vector<cv::Rect> &found, std::vector<SRegion::Type> &types);

	void skinLearning();
	void manualDetect();

	bool readSingleGauss();
	bool objectTracking(int startFrame, int endFrame);
	void selectFaces(bool bCtrl);
	void readAvi(QString fileName);
	bool predictTrackingRect(QPointF predictPos, int frame, int object, int searchRange, CvRect &newRect);
	bool doSelectFaces(int frame, cv::Point p1, cv::Point p2, std::vector<int> &faces, int &lastSelected);
	bool getPredictRect(int object, int frame, int width, int height, CvRect &predictRect);
	void setManualTarget(int object, int frame);
	bool exportMaskedZIC(const QString &fileName, int startFrame, int endFrame, int resizeWidth = -1, int resizeHeight = -1);
	bool exportMaskedIZIC(const QString &fileName, int startFrame, int endFrame, int resizeWidth = -1, int resizeHeight = -1);	// yamazaki add 20170504
	bool exportMaskedIZICUsingGPUMasking(const QString &fileName, int startFrame, int endFrame, int resizeWidth = -1, int resizeHeight = -1);	// yamazaki add 20170504
	bool exportMaskedIZICUsingCPUMasking(const QString &fileName, int startFrame, int endFrame, int resizeWidth = -1, int resizeHeight = -1);	// yamazaki add 20170504

	std::vector<cv::Rect_<double>> getEquirectangularRectList(cv::Rect_<double> rectOrg, double width, double height);

	//multiからの呼び出し用
	void partTracking(int, int);

	//カメラチェンジ
	void onCameraChange();
	void setCameraNo(int cameraNo);

	//パラメータをIniFileから読み込む
	void InitializeParameters();
	void getInitialFileName(wchar_t *filepath);

	// 選択数ステータス表示
	void updateSelectNum();

	void clickType(bool isChecked, SRegion::Type type);

};

#ifndef NOLICENSE
#endif

class MyApplication : public QApplication
{
public:
    MyApplication(int& argc, char** argv);
    bool notify(QObject* receiver, QEvent* event); 

#ifndef NOLICENSE
#endif
};

#endif // QTHOG_H
