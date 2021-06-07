#pragma warning(disable: 4819)

#include "windows.h"
#include "atlstr.h"
#include "qthog.h"
#include "AviManager.h"
#include "psapi.h"

#include <QFileDialog>
#include <QDebug>
#include <fstream>
#include <set>
#include <vector>
#include <QGraphicsSceneMouseEvent>
#include <shlobj.h>

#include "DlgFaceGrid.h"
#include "DlgBatchProcess.h"
#include "Calculator.h"
#include <fstream>
#include <QInputDialog>
#include <opencv.hpp>
#include "HogUtils.h"
#include <highgui.h>
#include <vfw.h>
#include "IdleWatcher.h"
#include <QTime>
#include "detector.h"

#include <QMessageBox>
#include <QUuid>

//testcode
#include <exception>
#include <QElapsedTimer>

#include "FileUtility.h"


QtHOG::QtHOG(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags),
	m_pAviManager(new AviManager),
	m_pCalculator(new Calculator),
	m_pDlgFaceGrid(new DlgFaceGrid(this)),
	m_pDlgBatch(new DlgBatchProcess(this)),
	m_mode(Constants::ModeNone),
	m_curRegion(StructData::iterator<SRegion>()),
	m_pStructData(new StructData),
	m_curCamera(0),
	m_curFrame(-1),
	m_lastSelected(-1),
	m_targetObject(-1),
	m_bMidDrag(false),
	m_colorThresh(2.0),
	m_svm(0),
	m_numberSvm(0),
	m_bCancel(false),
	m_lockonTarget(-1),
	m_pIdleWatcher(new IdleWatcher(this)),
	m_pTime(new QTime),
	m_pPCA(0),
	m_blurIntensityForFaces(8),
	m_blurIntensityForPlates(4),
	m_bOutputMaskingLog( false ),
	m_detectMethod(Constants::DetectMethod::HOG),
	m_imageType(Constants::ImageType::PANORAMIC)
{
	// ライセンスファイル名取得
	{
		WCHAR exePath[MAX_PATH];
		WCHAR drive[_MAX_DRIVE];
		WCHAR dir[_MAX_DIR];
		WCHAR fname[_MAX_FNAME];
		::GetModuleFileName(NULL, exePath, MAX_PATH);
		_wsplitpath(exePath, drive, dir, fname, NULL);
		PWSTR pszPath;
		::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath);

		WCHAR szPathName[MAX_PATH];
		_swprintf(szPathName, L"%s\\%s", pszPath, fname);
		if (!PathFileExistsW(szPathName))
			CreateDirectoryW(szPathName, NULL);
		_swprintf(szPathName, L"%s\\License.dat", szPathName);

		m_pAviManager->setLicenseFilename(szPathName);
	}
	InitializeParameters();
	//if (m_useGPU)
	//	m_useGPU = cv::gpu::getCudaEnabledDeviceCount();
	ui.setupUi(this);

	Qt::WindowFlags flg = Qt::Dialog;
	flg |= Qt::CustomizeWindowHint;
	flg |= Qt::WindowTitleHint;
	m_pDlgFaceGrid->setStructData(m_pStructData);
	m_pDlgFaceGrid->setAviManager(m_pAviManager);
	m_pDlgFaceGrid->setSelectedItems(&m_selectedItems);
	m_pDlgFaceGrid->setLastSelected(&m_lastSelected);


	ui.imageView->setStructData(m_pStructData);
	ui.imageView->setParentDialog((QWidget*)this);
	ui.imageView->setSelectedItems(&m_selectedItems);
	ui.imageView->setLastSelected(&m_lastSelected);

	m_pCalculator->setStructData(m_pStructData);

	connect(ui.imageView, SIGNAL(sgClicked(QGraphicsSceneMouseEvent *)), this, SLOT(onImageViewClicked(QGraphicsSceneMouseEvent *)));
	connect(ui.imageView, SIGNAL(sgReleased(QGraphicsSceneMouseEvent *)), this, SLOT(onImageViewReleased(QGraphicsSceneMouseEvent *)));

	connect(m_pDlgFaceGrid, SIGNAL(sgSelectionChanged()), this, SLOT(onSelectionChanged()));
	connect(m_pDlgFaceGrid, SIGNAL(sgItemDoubleClicked(int)), this, SLOT(onItemDoubleClicked(int)));
	connect(m_pDlgFaceGrid, SIGNAL(sgItemDoubleClicked(int, int, bool)), this, SLOT(onItemDoubleClicked(int, int, bool)));
	connect(m_pDlgFaceGrid, SIGNAL(sgAutoTrackingRequired(int, int)), this, SLOT(onAutoTrackingRequired(int, int)));

	connect(m_pDlgFaceGrid, SIGNAL(sgScrollBack()), this, SLOT(onScrollBack()));
	connect(m_pDlgFaceGrid, SIGNAL(sgScrollForward()), this, SLOT(onScrollForward()));
	connect(m_pDlgFaceGrid, SIGNAL(sgManualDetect()), this, SLOT(onManualDetect()));
	connect(m_pDlgFaceGrid, SIGNAL(sgRkeyPressed()), this, SLOT(onAutoTrackingRequiredByR()));

	connect(this, SIGNAL(sgSelectionChanged()), m_pDlgFaceGrid, SLOT(onSelectionChanged()));
	connect(this, SIGNAL(sgImageChanged(const std::vector<int> &, bool)), m_pDlgFaceGrid, SLOT(onImageChanged(const std::vector<int> &, bool)));

	connect(ui.imageView, SIGNAL(sgImageChanged(const std::vector<int> &, bool)), m_pDlgFaceGrid, SLOT(onImageChanged(const std::vector<int> &, bool)));
	connect(ui.imageView, SIGNAL(sgItemChanged()), this, SLOT(onItemChanged()));
	connect(ui.imageView, SIGNAL(sgItemDeleted(const std::set<int> &)), this, SLOT(onItemDeleted(const std::set<int> &)));
	connect(ui.imageView, SIGNAL(sgWheelEvent(QWheelEvent *)), this, SLOT(onWheelEvent(QWheelEvent *)));

	m_pStructData->retain();
	//m_pStructData->release();

	ui.pushButtonManualDetect->setIcon(QIcon(":/Icon/AddIcon"));
	
	QValidator *validator = new QIntValidator(0, 10000, this);
	ui.editFrame->setValidator(validator);

	setWindowIcon(QIcon(":/Icon/AppIcon"));
	setWindowTitle(QFileInfo( QCoreApplication::applicationFilePath() ).baseName());

	m_iconClock = QIcon(":/Icon/Clock");
	m_iconWarning = QIcon(":/Icon/Warning");
	m_iconDone = QIcon(":/Icon/Done");
	m_iconDisc = QIcon(":/Icon/Disc");

	// ステータスバーの設定
	ui.statusBar->addWidget(&m_executeStatus, 1);
	ui.statusBar->addWidget(&m_selectedStatus, 0);

	connect(m_pIdleWatcher, SIGNAL(sgIdle()), this, SLOT(onIdle()));
	qApp->installEventFilter(m_pIdleWatcher);
	m_pTime->start();

	// secuwatcherMask実行
	if (m_detectMethod == Constants::DetectMethod::SECUWATCHER)
	{
		TCHAR drive[_MAX_DRIVE];
		TCHAR dir[_MAX_DIR];
		TCHAR filename[_MAX_FNAME];
		TCHAR ext[_MAX_FNAME];
		// 起動中のsecuwatcherMaskを停止する。
		DWORD processIds[1024];
		DWORD processNum;
		if (EnumProcesses(processIds, sizeof(processIds), &processNum))
		{
			for (DWORD idNo = 0; idNo < processNum; ++idNo)
			{
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, processIds[idNo]);
				if (hProcess != NULL)
				{
					TCHAR processName[MAX_PATH] = {0};
					DWORD size = MAX_PATH;
					if (QueryFullProcessImageName(hProcess, NULL, processName, &size))
					{
						_tsplitpath(processName, NULL, NULL, filename, ext);
						_tmakepath_s(processName, NULL, NULL, filename, ext);
						if (_tcsicmp(processName, m_childProcName) == 0)
						{	// 同一ファイル名発見
							TerminateProcess(hProcess, -1);
						}
					}
		            CloseHandle(hProcess);
				}
			}
		}

		// secuwatcherMaskを実行する。
		TCHAR childProcPath[MAX_PATH];
		TCHAR exePath[MAX_PATH];
		::GetModuleFileName(NULL, exePath, MAX_PATH);
		_tsplitpath(exePath, drive, dir, NULL, NULL);
		_tmakepath_s(childProcPath, drive, dir, m_childProcName, NULL);
		if (!PathFileExists(childProcPath))
		{
			MessageBox(NULL, _T("not exist secuwatcherMask process file"), _T("Masquerade"), MB_OK);
			return;
		}
		m_secuwatcherMask.setWindowHandle(this->winId());
		if (!m_secuwatcherMask.initChildProcess(childProcPath))
		{
			MessageBox(NULL, _T("execute process error"), _T("Masquerade"), MB_OK);
			return;
		}
	}
}

QtHOG::~QtHOG()
{
	delete m_pAviManager;
	delete m_pCalculator;
	//delete m_pDlgSkinColor;
	//delete m_pSkinClassifier;
	//delete m_pImageProcessor;

	m_pStructData->release();

	if (m_svm)
		delete m_svm;
	if (m_pPCA)
		delete m_pPCA;

	delete m_pTime;
}

void QtHOG::on_actionQuit_triggered()
{
	qApp->quit();
}

void QtHOG::on_actionRead_Image_triggered()
{
	QString selFilter = "";
	QString dirName = QDir::currentPath();
	if (!m_pAviManager->getCurFileName().isEmpty())
	{
		QFileInfo fileInfo(m_pAviManager->getCurFileName());
		dirName = fileInfo.absolutePath();
	}
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Open Movie"), dirName,
													tr("Movie file (*.zic *.izic);;All files (*.*)"), &selFilter
													/*QFileDialog::DontUseNativeDialog*/);

	if (fileName.isEmpty())
		return;
	QFileInfo fileInfo(fileName);
	if(fileInfo.exists() == false)
	{
		return;
	}

	// 初期化関数が分からないため、とりあえずここに入れておく

	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->hide();
	m_pStructData->clearAll();
	m_TargetType.clear();
	m_TargetType.push_back(SRegion::TypeManual);
	m_TargetType.push_back(SRegion::TypeHog);
	if (ui.checkBoxFace->isChecked())
		m_TargetType.push_back(SRegion::TypeFace);
	if (ui.checkBoxPlate->isChecked())
		m_TargetType.push_back(SRegion::TypePlate);
	m_pDlgFaceGrid->setTargetType(&m_TargetType);
	ui.imageView->setTargetType(&m_TargetType);

	readAvi(fileName);
}

void QtHOG::readAvi(QString fileName)
{
	int frameCount = m_pAviManager->openAvi(fileName);
	ui.sliderFrame->setEnabled(frameCount > 0);

	if (frameCount >= 0)
	{
		ui.sliderFrame->setRange(0, frameCount - 1);
		setFrame(0, true);
		m_curRegion = StructData::iterator<SRegion>();
		resizeEvent(0);

		m_pDlgFaceGrid->setAviName(fileName);
		
		setWindowIcon(QIcon(":/Icon/AppIcon"));
		setWindowTitle(QFileInfo(QCoreApplication::applicationFilePath() ).baseName() + " : " + QFileInfo(fileName).fileName());
	}
}

void QtHOG::onSelectionChanged()
{
	if (m_selectedItems.size() == 1 && *m_selectedItems.begin() >= 0)
	{
		StructData::iterator<SRegion> itr;
		itr = m_pStructData->regionFitAt(*m_selectedItems.begin());
		if (itr != m_pStructData->regionEnd())
			m_lockonTarget = itr->object;
	}
	else
		m_lockonTarget = -1;

	setFrame(ui.sliderFrame->value(), true);
	//ui.imageView->setFrame(ui.sliderFrame->value());
}

void QtHOG::onItemChanged()
{
	ui.imageView->setFrame(ui.sliderFrame->value());

	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->updateView();
}

void QtHOG::onItemDeleted(const std::set<int> &deleteSet)
{
	ui.imageView->setFrame(ui.sliderFrame->value());

	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->eraseItems(deleteSet);
	updateSelectNum();
}

void QtHOG::setFrame(int frame, bool bUpdate, bool bKey)
{
	if (frame == m_curFrame && !bUpdate)
		return;

	if (frame < 0 || frame >= m_pAviManager->getFrameCount())
		return;

	StructData::iterator<SRegion> itr = m_pStructData->regionEnd();
	if (m_lockonTarget >= 0)
		itr = m_pStructData->regionFitAt(m_curCamera, frame, m_lockonTarget);

	m_pStructData->mutex.lock();
	ui.imageView->setFrame(frame);
	ui.imageView->setImage(m_pAviManager->getImage(frame));
	m_pStructData->mutex.unlock();

	m_curFrame = frame;

	int editFrame = ui.editFrame->displayText().toInt();
	if (editFrame != frame)
		ui.editFrame->setText(QString::number(frame));

	if (ui.sliderFrame->value() != frame)
		ui.sliderFrame->setValue(frame);

	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->setFrame(frame);
	
	if (itr != m_pStructData->regionEnd())
	{
		CvRect &r = itr->rect;
		ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);
	}

	updateSelectNum();

	ui.imageView->updateView();

	if (bKey)
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void QtHOG::on_sliderFrame_valueChanged(int frame)
{
	ui.sliderFrame->blockSignals( true );
	
	setFrame(frame);

	ui.sliderFrame->blockSignals( false );
}

// 縮小版テスト
void QtHOG::on_actionDetect_HOG_triggered()
{
	int frame = ui.sliderFrame->value();

	//detectHOG(frame);

	setFrame(frame, true);
	m_curRegion = StructData::iterator<SRegion>();
}

void QtHOG::on_actionDetect_Faces_triggered()
{	
	//人検出、顔検出でGPUを利用する
//	bool useGPU = false;
	switch (m_detectMethod)
	{
	case Constants::DetectMethod::HOG:
		if (!m_svm)
		{
			if (!readSVMData())
				return;
		}
		break;
	case Constants::DetectMethod::SECUWATCHER:
		if (!m_secuwatcherMask.setParameters(m_detectParam))
		{
			MessageBox(NULL, _T("secuwatcherMask initialize error"), _T("Masquerade"), MB_OK);
			return;
		}
		break;
	}

	
	QString msg = QString("Initializing...");
	m_executeStatus.setText(msg);

	//計測タイマー
	QElapsedTimer elTimer;
	qDebug() << QElapsedTimer::clockType();

	m_bCancel = false;

	// 映像を１フレームずつ回しているのこのループ
	for (int frame = 0; frame < m_pAviManager->getFrameCount(); ++frame)
	{
		if (m_bCancel)
			break;

		msg = QString("Detecting in (%1/%2)...").arg(frame).arg(m_pAviManager->getFrameCount());
		m_executeStatus.setText(msg);
		qApp->processEvents();
#ifdef DEBUG
		TCHAR err[32];
		_stprintf_s(err, _T("Frame %d\n"), frame);
		OutputDebugString(err);
#endif

		switch (m_imageType)
		{
		case Constants::ImageType::PANORAMIC:
			//計測対象１
			detect_Panoramic(frame);
			//計測対象２
			detectFaceInFrame(frame);
			break;
		case Constants::ImageType::PERSPECTIVE:
			//計測対象１
			detect_Perspective(frame);
			//計測対象２
			detectFaceInFrame(frame);
			break;
		default:
			return;
		}


		setFrame(frame, true);
		m_curRegion = StructData::iterator<SRegion>();
	}
	m_executeStatus.clear();
	//m_executeStatus.clear();
}

// 未使用なので削除
//void QtHOG::detecterCascade(int frame)
//{
//	int width = m_pAviManager->getWidth();
//	int height = m_pAviManager->getHeight();
//
//	cv::Mat frameImage;
//
//	m_pStructData->mutex.lock();
//	cv::Mat frameMat = m_pAviManager->getImage(frame);
//	cv::cvtColor(frameMat, frameImage, CV_BGR2GRAY);
//	m_pStructData->mutex.unlock();
//
//	double scale = 2.0;
//	cv::Mat smallImg(cv::saturate_cast<int>(frameMat.rows/scale), cv::saturate_cast<int>(frameMat.cols/scale), CV_8UC1);
//	cv::resize(frameImage, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR);
//	cv::equalizeHist(smallImg, smallImg);
//
//	std::vector<cv::Rect> faces;
//	cascade.detectMultiScale(
//		smallImg,
//		faces,
//		1.05f,
//		3,
//		CV_HAAR_SCALE_IMAGE,//CV_HAAR_DO_CANNY_PRUNING,//
//		cv::Size(8/scale, 8/scale)
//		);
//
//	for(std::vector<cv::Rect>::const_iterator itr = faces.begin();
//		itr != faces.end();
//		itr++)
//	{
//		CvRect r;
//		r.x = itr->x * scale;
//		r.y = itr->y * scale;
//		r.width = itr->width * scale;
//		r.height = itr->height * scale;
//
//		m_pStructData->insertRegion(m_curCamera, frame, -1, r, SRegion::TypeFace);
//	}
//	setFrame(frame, true);
//	m_curRegion = StructData::iterator<SRegion>();
//}
//
//void QtHOG::detecterSvm(int frame)
//{
//	cv::Mat m;
//	IplImage *src, *src_color, *src_tmp;
//
//	m_pStructData->mutex.lock();
//	src_color = m_pAviManager->getImage(frame);
//	src = cvCreateImage( cvGetSize(src_color), IPL_DEPTH_8U, CV_BGRA2BGR );
//	cvCvtColor(src_color,src,CV_BGR2GRAY);
//	m_pStructData->mutex.unlock();
//
//	//src が gray
//	CvMat mat;
//	cvInitMatHeader(&mat, 1, WidthClip*HeightClip, CV_32FC1, NULL);
//	int tw = src->width;
//	int th = src->height;
//	float a[WidthClip*HeightClip];
//
//	//サンプルどおり
//	float ret = -1.0;
//	int stepx = 3;
//	int stepy = 3;
//	int width = WidthClip;
//	int height= HeightClip;
//	int iterateLimit = 9;
//	float steps = 1.1;
//	for(int iterate = 0; iterate < iterateLimit; iterate++)
//	{
//		src_tmp = cvCreateImage(
//			cvSize( (int)(tw / steps), (int)(th / steps) ), IPL_DEPTH_8U, 1);
//		cvResize (src, src_tmp);
//
//		tw = src_tmp->width;
//		th = src_tmp->height;
//		for (int sy = 0; sy <= src_tmp->height - height; sy += stepy)
//		{
//			for (int sx = 0; sx <= src_tmp->width - width; sx += stepx)
//			{
//				for (int i = 0; i < height; i++)
//				{
//					for (int j = 0; j < width; j++)
//					{
//						a[i * width + j] =
//							float ((int) ((unsigned char) (src_tmp->imageData[(i + sy) * src_tmp->widthStep + (j + sx)])) / 255.0);
//					}
//				}
//
//				//
//				cvSetData(&mat, a, sizeof (float) * WidthClip*HeightClip);
//				ret = m_svm->predict(&mat);
//				if( (int)ret == 1 )
//				{
//					qDebug() << "score :" << ret;
//				}
//			}
//		}
//
//		steps += 0.1;//0.1は変数にしないとだめ
//	}
//}
//
void QtHOG::detect_Panoramic(int frame)
{
	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	m_pStructData->mutex.lock();
	cv::Mat frameMat = m_pAviManager->getImage(frame);

	cv::Mat resizeImage;
	cv::resize(frameMat, resizeImage, cv::Size(m_ResizeWidth, m_ResizeHeight));
//	cv::Mat halfImage = resizeImage(cv::Rect(0, ResizeHalfHeight, ResizeWidth, ResizeHalfHeight));
	m_pStructData->mutex.unlock();

	float scale = (float)m_ResizeWidth / (float)frameMat.size().width;

		std::vector<SRegion::Type> types;
	std::vector<cv::Rect> found;
	switch (m_detectMethod)
	{
	case Constants::DetectMethod::HOG:
		detectHOG(resizeImage, found);
		break;
	case Constants::DetectMethod::SECUWATCHER:
		detectSecuwatcher(resizeImage, found, types);
		break;
	}
	//if (m_useGPU)
	//{
	//	OutputDebugString(_T("detectHOG with useGPU\n"));
	//	cv::gpu::GpuMat gpu_img(resizeImage.size().height, resizeImage.size().width, CV_8UC1);
	//	cv::gpu::HOGDescriptor hog;
	//	hog.setSVMDetector( cv::gpu::HOGDescriptor::getPeopleDetector64x128());

	//	gpu_img.upload(resizeImage);
	//	hog.detectMultiScale(gpu_img, found);
	//	gpu_img.release();
	//}
	//else
	//{
	//	cv::HOGDescriptor hog;
	//	hog.setSVMDetector( cv::HOGDescriptor::getDefaultPeopleDetector() );

	//	hog.detectMultiScale(resizeImage, found);
	//}

	StructData::iterator<SRegion> itr;
	for (int i = 0; i < found.size(); ++i)
	{
		CvRect r;
		r.x = found[i].x / scale;
		r.y = found[i].y / scale;
		r.width = found[i].width / scale;
		r.height = found[i].height / scale;

		if (r.x < 0) r.x = 0;
		if (r.y < 0) r.y = 0;
		if (r.x + r.width >= width) r.width = width - 1 - r.x;
		if (r.y + r.height >= height) r.height = height - 1 - r.y;

		switch (m_detectMethod)
		{
		case Constants::DetectMethod::HOG:
			itr = m_pStructData->insertRegion(m_curCamera, frame, -1, r, SRegion::TypeHog);
			break;
		case Constants::DetectMethod::SECUWATCHER:
			itr = m_pStructData->insertRegion(m_curCamera, frame, -1, r, types[i]);
			break;
		}
	}
	//kosshyadd
	/*int doDivideImageStep = 80;	// 外から調整できるように
	if( !(frame%doDivideImageStep) )
	{
		int wclip = 64;
		int hclip = 128;
		int wstep = 32;
		int hstep = 32;
		for(int hs = 0; hs <= height - hclip; hs+=hstep)
		{
			for(int ws = 0; ws <= width - wclip; ws+=wstep)
			{
				CvRect rect;
				rect.x = ws;
				rect.y = hs;
				rect.width = wclip;
				rect.height = hclip;

				itr = m_pStructData->insertRegion(0, frame, -1, rect, SRegion::TypeHog);
			}
		}
	}*/
	//kosshyaddend
	setFrame(frame, true);
	m_curRegion = StructData::iterator<SRegion>();
}
void QtHOG::detect_Perspective(int frame)
{
	
#ifdef DEBUG
	OutputDebugString(_T("detect_Perspective\n"));
#endif
//	cv::Mat frameImage;

	//int proj_type_list[4] = {MOVIELOADER_CUBE_FRONT, MOVIELOADER_CUBE_REAR, MOVIELOADER_CUBE_LEFT, MOVIELOADER_CUBE_RIGHT};
	//MathUtil::Mat3d proj_rot_list[4];
	//ILCore::Math::CMatrix33<double> proj_mat_list[4] = {ILCore::IZIC::projectionMatrix[0], ILCore::IZIC::projectionMatrix[1], ILCore::IZIC::projectionMatrix[2], ILCore::IZIC::projectionMatrix[3]};
	//for (int proj_type = 0;proj_type < 4; proj_type++)
	//{
	//	for (int x=0; x<3; x++)
	//		for (int y=0; y<3; y++)
	//			proj_rot_list[proj_type][x][y] = proj_mat_list[proj_type][y][x];		// 転置して代入、ILCoreからPTMへ変換
	//}
	cv::Mat frameMat;
	for (int proj_type = 0;proj_type < 4; proj_type++)
	{
		m_pStructData->mutex.lock();
		frameMat = m_pAviManager->getImage(frame, 0, proj_type);
		m_pStructData->mutex.unlock();
		//cv::Mat dbgImg;
		//cv::resize(frameMat, dbgImg, cv::Size(), 0.3, 0.3);
		//cv::imshow("test", dbgImg);
		//cv::waitKey();

		std::vector<SRegion::Type> types;
		std::vector<cv::Rect> found;
		switch (m_detectMethod)
		{
		case Constants::DetectMethod::HOG:
			detectHOG(frameMat, found);
			break;
		case Constants::DetectMethod::SECUWATCHER:
			detectSecuwatcher(frameMat, found, types);
			break;
		}
		//if (m_useGPU)
		//{
		//	OutputDebugString(_T("detectHOG_Perspective with useGPU\n"));
		//	cv::gpu::GpuMat gpu_img(resizeImage.size().height, resizeImage.size().width, CV_8UC1);
		//	cv::gpu::HOGDescriptor hog;
		//	hog.setSVMDetector( cv::gpu::HOGDescriptor::getPeopleDetector64x128());

		//	gpu_img.upload(resizeImage);
		//	hog.detectMultiScale(gpu_img, found);
		//	gpu_img.release();
		//}
		//else
		//{
		//	cv::HOGDescriptor hog;
		//	hog.setSVMDetector( cv::HOGDescriptor::getDefaultPeopleDetector() );

		//	hog.detectMultiScale(resizeImage, found);
		//}

//#ifdef DEBUG
//		cv::Mat panoimage = m_pAviManager->getImage(frame, 0, MOVIELOADER_SPHERE);
//#endif

		StructData::iterator<SRegion> itr;
		for (int i = 0; i < found.size(); ++i)
		{
			RECT panoRect, cubeRect;
			cubeRect.left = found[i].x; cubeRect.top = found[i].y;
			cubeRect.right = found[i].x + found[i].width; cubeRect.bottom = found[i].y + found[i].height;
			if (!m_projectionImage.cube2PanoRect(	&cubeRect, m_pAviManager->getPerspectiveWidth(), m_pAviManager->getPerspectiveHeight(), m_pAviManager->getFov(), proj_type,
												&panoRect, m_pAviManager->getWidth(), m_pAviManager->getHeight()))
				continue;
			CvRect r = cvRect(panoRect.left, panoRect.top, panoRect.right - panoRect.left, panoRect.bottom - panoRect.top);
			switch (m_detectMethod)
			{
			case Constants::DetectMethod::HOG:
				itr = m_pStructData->insertRegion(m_curCamera, frame, -1, r, SRegion::TypeHog);
				break;
			case Constants::DetectMethod::SECUWATCHER:
				itr = m_pStructData->insertRegion(m_curCamera, frame, -1, r, types[i]);
				break;
			}
		}
//#ifdef DEBUG
//		if (found.size() > 0)
//		{
//			for (int i = 0; i < found.size(); ++i)
//				cv::rectangle(frameMat,cvPoint(found[i].x,found[i].y),cvPoint(found[i].x + found[i].width, found[i].y + found[i].height),CV_RGB(255,0,0));
//			cv::namedWindow("detect", cv::WINDOW_AUTOSIZE);
//			cv::imshow("detect", frameMat);
//			cv::namedWindow("panoramic", cv::WINDOW_AUTOSIZE);
//			cv::imshow("panoramic", panoimage);
//			cv::waitKey(0);
//			cv::destroyAllWindows();
//		}
//#endif
	}
	//kosshyaddend
	setFrame(frame, true);
	m_curRegion = StructData::iterator<SRegion>();
}
void QtHOG::detectHOG(cv::Mat image, std::vector<cv::Rect> &found)
{
	cv::Mat frameImage;
	cv::cvtColor(image, frameImage, CV_BGR2GRAY);
	//if (m_useGPU)
	//{
	//	OutputDebugString(_T("detectHOG with useGPU\n"));
	//	cv::gpu::GpuMat gpu_img(frameImage.size().height, frameImage.size().width, CV_8UC1);
	//	cv::gpu::HOGDescriptor hog;
	//	hog.setSVMDetector( cv::gpu::HOGDescriptor::getPeopleDetector64x128());

	//	gpu_img.upload(frameImage);
	//	hog.detectMultiScale(gpu_img, found);
	//	gpu_img.release();
	//}
	//else
	{
		cv::HOGDescriptor hog;
		hog.setSVMDetector( cv::HOGDescriptor::getDefaultPeopleDetector() );

		hog.detectMultiScale(frameImage, found);
	}
	//float scaleX = (float)m_ResizeWidth / (float)image.size().width;
	//float scaleY = (float)m_ResizeHeight / (float)image.size().height;
	//for (std::vector<cv::Rect>::iterator itRect = found.begin(); itRect != found.end(); itRect++)
	//{
	//	itRect->x = itRect->x / scaleX;
	//	itRect->y = itRect->y / scaleY;
	//	itRect->width = itRect->width / scaleX;
	//	itRect->height = itRect->height / scaleY;
	//}
}

void QtHOG::detectSecuwatcher(cv::Mat image, std::vector<cv::Rect> &found, std::vector<SRegion::Type> &types)
{
	std::vector<secuwatcher_access::detectedData> detectDatas;
	m_secuwatcherMask.detect(image, detectDatas);
	for (std::vector<secuwatcher_access::detectedData>::iterator detectData = detectDatas.begin(); detectData != detectDatas.end(); detectData++)
	{
		cv::Rect rectData = cvRect(detectData->rectData.x, detectData->rectData.y, detectData->rectData.width, detectData->rectData.height);

		if (m_detectParam.model[detectData->modelType].valid)
		{
			switch (detectData->modelType)
			{
			case secuwatcher_access::MODEL_PERSON:
				types.push_back(SRegion::TypeFace);
				found.push_back(rectData);
				break;
			case secuwatcher_access::MODEL_FACE:
				types.push_back(SRegion::TypeFace);
				found.push_back(rectData);
				break;
			case secuwatcher_access::MODEL_PLATE:
				types.push_back(SRegion::TypePlate);
				found.push_back(rectData);
				break;
			}
		}
	}
}

void QtHOG::on_sliderScale_valueChanged(int value)
{
	QMatrix scaleMat(float(value) / 100, 0, 0, float(value) / 100, 0, 0);
	ui.imageView->setMatrix(scaleMat);
	ui.labelScale->setText(QString::number(value));
}

void QtHOG::resizeEvent(QResizeEvent *event)
{
	if (!ui.checkBoxAutoScale->isChecked())
		return;

	if (m_pAviManager->getFrameCount() < 1)
		return;

	float scale = ui.imageView->adjustScale();
	ui.labelScale->setText(QString::number((int)(scale * 100)));
	ui.sliderScale->setValue((int)(scale * 100));
}

void QtHOG::onWheelEvent ( QWheelEvent * event )
{
	 int numDegrees = event->delta() / 8;
     float numSteps = numDegrees;// / 15;
	 ui.sliderScale->setValue(ui.sliderScale->value() * (1 + numSteps / 100));
}

void QtHOG::on_checkBoxAutoScale_clicked(bool bCheck)
{
	ui.sliderScale->setEnabled(!bCheck);
	resizeEvent(0);
}

void QtHOG::on_actionWrite_Objects_triggered()
{
	// オブジェクトデータ書き出し
	QString selFilter = "";
	QString defaultName = QDir::currentPath();
	if (!m_objectfile.isEmpty())
	{
		QFileInfo fileInfo(m_objectfile);
		defaultName = fileInfo.absoluteFilePath();
	}
	QString fileName = QFileDialog::getSaveFileName(this,
													tr("Write region data"), defaultName,
													tr("Region file (*.rgn)"), &selFilter
													);

	if (fileName.isEmpty())
		return;

	m_pStructData->writeRegions(fileName.toLocal8Bit(), m_pAviManager->getWidth(), m_pAviManager->getHeight());
}

void QtHOG::on_actionRead_Objects_triggered()
{
	// オブジェクトデータの読込み
	QString selFilter = "";
	QString dirName = QDir::currentPath();
	if (!m_objectfile.isEmpty())
	{
		QFileInfo fileInfo(m_objectfile);
		dirName = fileInfo.absolutePath();
	}
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read region data"), dirName,
													tr("Region file (*.rgn);;All files (*.*)"), &selFilter
													);

	if (fileName.isEmpty())
		return;

	m_objectfile = fileName;
	m_pStructData->readRegions(fileName.toLocal8Bit(), m_pAviManager->getWidth(), m_pAviManager->getHeight());

	updateSelectNum();
	setFrame(ui.sliderFrame->value(), true);
	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->updateView();
}

//void QtHOG::on_comboBoxMode_currentIndexChanged(int idx)
//{
//	/*
//	m_mode = (Constants::Mode)idx;
//
//	if (m_mode == Constants::ModeSkinLearning)
//	{
//		m_pDlgSkinColor->show();
//	}
//	else
//		m_pDlgSkinColor->hide();
//		*/
//}
//
void QtHOG::on_actionShow_Skin_Color_triggered()
{
	//m_pDlgSkinColor->show();
}


void QtHOG::on_pushButtonManualDetect_clicked(bool bChecked)
{
	if (bChecked)
	{
		m_mode = Constants::ModeManualDetect;
		QCursor cursor;
		cursor.setShape(Qt::WhatsThisCursor);
		setCursor(cursor);
		if (m_pDlgFaceGrid->isVisible())
			m_pDlgFaceGrid->setWaitRegion();
	}
	else
	{
		m_mode = Constants::ModeNone;
		ui.pushButtonManualDetect->setIcon(QIcon(":/Icon/AddIcon"));
		m_targetObject = -1;
		unsetCursor();
		if (m_pDlgFaceGrid->isVisible())
			m_pDlgFaceGrid->unsetWaitRegion();
	}
}

//void QtHOG::on_comboBoxType_currentIndexChanged(int idx)
//{
//	m_detectType = (Constants::DetectType)idx;
//	//m_pDlgFaceGrid->setDetectType(m_detectType);
//
//	ui.imageView->setDetectMode(m_detectType);
//	setFrame(ui.sliderFrame->value(), true);
//}

void QtHOG::onImageViewClicked(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (mouseEvent->buttons() == Qt::MidButton)
		m_bMidDrag = true;

	m_selectedRegion[0].x = mouseEvent->scenePos().x() + 0.5;
	m_selectedRegion[0].y = mouseEvent->scenePos().y() + 0.5;
}

bool QtHOG::doSelectFaces(int frame, cv::Point p1, cv::Point p2, std::vector<int> &faces, int &lastSelected)
{
	faces.clear();
	lastSelected = -1;

	// 顔領域のリスト作成
	StructData::iterator<SRegion> it;
	//int frame = ui.sliderFrame->value();
	//QRect selection(m_selectedRegion[0].x, m_selectedRegion[0].y,
	//				m_selectedRegion[1].x - m_selectedRegion[0].x, m_selectedRegion[1].y - m_selectedRegion[0].y);

	QRect selection(p1.x, p1.y, p2.x - p1.x, p2.y - p1.y);


	bool bPoint = (selection.width() < 3 && selection.height() < 3);
	int cx = (p1.x + p2.x) / 2;
	int cy = (p1.y + p2.y) / 2;
	int minDist = -1, dx, dy;
#ifdef _DEBUG
	{
		char msg[256];
		sprintf(msg, "center[x = %04d, y = %04d]\n", cx, cy);
		OutputDebugStringA(msg);
	}
#endif

	for (it = m_pStructData->regionBegin(m_curCamera, frame); it != m_pStructData->regionEnd(); ++it)
	{
		if (std::find(m_TargetType.begin(), m_TargetType.end(), it->type) == m_TargetType.end())
			continue;

#ifdef _DEBUG
		char msg[256];
		sprintf(msg, "Rect[x = %04d-%04d, y = %04d-%04d]\n", it->rect.x, it->rect.x + it->rect.width, it->rect.y, it->rect.y + it->rect.height);
		OutputDebugStringA(msg);
#endif
		QRect candidate(it->rect.x, it->rect.y, it->rect.width, it->rect.height);
		int cx2 = it->rect.x + it->rect.width / 2;
		int cy2 = it->rect.y + it->rect.height / 2;

		if ((!bPoint && candidate.intersects(selection)) || (bPoint && candidate.contains(cx, cy)))
		{
			faces.push_back(it.getID());

			dx = cx - cx2;
			dy = cy - cy2;
			int dist = dx * dx + dy * dy;
			if (minDist < 0 || dist < minDist)
			{
				minDist = dist;
				lastSelected = it.getID();
			}
		}
	}
	updateSelectNum();

	return bPoint;
}

void QtHOG::selectFaces(bool bCtrl)
{
	if (!bCtrl)
		m_selectedItems.clear();

	std::vector<int> faces;
	int lastSelected;

	int frame = ui.sliderFrame->value();
	bool bPoint = doSelectFaces(frame, m_selectedRegion[0], m_selectedRegion[1], faces, lastSelected);

	if (lastSelected >= 0)
	{
		if (faces.size() == 1)
		{
			if (m_selectedItems.find(lastSelected) == m_selectedItems.end())
			{
				m_selectedItems.insert(lastSelected);
				m_lastSelected = lastSelected;
			}
			else
			{
				m_selectedItems.erase(lastSelected);
				if (m_selectedItems.empty())
					m_lastSelected = -1;
				else
					m_lastSelected = *(m_selectedItems.begin());
			}
		}
		else
		{
			if (!bPoint)
			{
				for (unsigned int i = 0; i < faces.size(); ++i)
					m_selectedItems.insert(faces[i]);
			}
			else
				m_selectedItems.insert(lastSelected);
			m_lastSelected = lastSelected;
		}
	}

	m_lockonTarget = -1;
	setFrame(ui.sliderFrame->value(), true);
	emit sgSelectionChanged();
}

void QtHOG::onImageViewReleased(QGraphicsSceneMouseEvent *mouseEvent)
{
	cv::Point pos;
	pos.x = mouseEvent->scenePos().x() + 0.5;
	pos.y = mouseEvent->scenePos().y() + 0.5;

	if (pos.x < m_selectedRegion[0].x)
	{
		m_selectedRegion[1].x = m_selectedRegion[0].x;
		m_selectedRegion[0].x = pos.x;
	}
	else
	{
		m_selectedRegion[1].x = pos.x;
	}

	if (pos.y < m_selectedRegion[0].y)
	{
		m_selectedRegion[1].y = m_selectedRegion[0].y;
		m_selectedRegion[0].y = pos.y;
	}
	else
	{
		m_selectedRegion[1].y = pos.y;
	}

	if (m_selectedRegion[0].x < 0)
		m_selectedRegion[0].x = 0;
	if (m_selectedRegion[0].y < 0)
		m_selectedRegion[0].y = 0;

	if (m_selectedRegion[1].x >= m_pAviManager->getWidth())
		m_selectedRegion[1].x = m_pAviManager->getWidth() - 1;
	if (m_selectedRegion[1].y >= m_pAviManager->getHeight())
		m_selectedRegion[1].y = m_pAviManager->getHeight() - 1;

	int w = m_selectedRegion[1].x -  m_selectedRegion[0].x;
	int h = m_selectedRegion[1].y -  m_selectedRegion[0].y;

	if (m_bMidDrag)
	{
		if (w < 5 && h < 5)
		{
			ui.sliderScale->setEnabled(false);
			ui.checkBoxAutoScale->setCheckState(Qt::Checked);
			resizeEvent(0);
		}
		else
		{
			ui.sliderScale->setEnabled(true);
			ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);

			CvRect r;
			r.x = m_selectedRegion[0].x;
			r.y = m_selectedRegion[0].y;
			r.width = w;
			r.height = h;

			float scale = ui.imageView->adjustScale(r.width, r.height);
			ui.labelScale->setText(QString::number((int)(scale * 100)));
			ui.sliderScale->setValue((int)(scale * 100));

			ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);
		}
		m_bMidDrag = false;
		return;
	}

	if (false)//m_svm)
	{
		// SVMのテスト
		int frame = ui.sliderFrame->value();
		cv::Mat grayImage;
		cv::Mat frameImage = m_pAviManager->getImage(frame);
		cv::cvtColor(frameImage, grayImage, CV_BGR2GRAY);

		cv::Rect r;
		r.x = m_selectedRegion[0].x;
		r.y = m_selectedRegion[0].y;
		r.width = w;
		r.height = h;

		std::vector<float> feat;
		Detector::getHoG(grayImage(r), feat);

		CvMat m;
		cvInitMatHeader (&m, 1, feat.size(), CV_32FC1, &(feat[0]));

		if (m_svm->predict(&m))
			qDebug() << "Found!";
		else
			qDebug() << "not found";

		return;
	}

	Constants::Mode mode;
	if (GetKeyState(VK_CONTROL) < 0)
		mode = Constants::ModeNone;
	else if (GetKeyState(VK_SHIFT) < 0)
		mode = Constants::ModeManualDetect;
	else
		mode = m_mode;

	switch (mode)
	{
	case Constants::ModeSkinLearning:
		{
			if (w <= 0 || h <= 0)
				return;
			skinLearning();
			break;
		}
	case Constants::ModeNone:
		{
			selectFaces(mouseEvent->modifiers() & Qt::ControlModifier);
			break;
		}
	case Constants::ModeManualDetect:
		{
			std::vector<int> faces;
			int lastSelected;
			int frame = ui.sliderFrame->value();
			bool bPoint = doSelectFaces(frame, m_selectedRegion[0], m_selectedRegion[1], faces, lastSelected);

			if (!bPoint || faces.empty() || m_targetObject < 0)
			{
				// 手動で領域追加
				manualDetect();
			}
			else
			{
				int frame = ui.sliderFrame->value();
				int minDist = -1, sourceID;
				StructData::iterator<SRegion> itr;
				for (itr = m_pStructData->regionBegin(m_targetObject); itr != m_pStructData->regionEnd(); ++itr)
				{
					int dist = abs(itr->frame - frame);
					if (minDist < 0 || dist < minDist)
					{
						sourceID = itr.getID();
						minDist = dist;
					}
				}
				m_selectedItems.clear();
				m_selectedItems.insert(sourceID);
				m_lastSelected = sourceID;

				std::vector<int>::iterator its;
				for (its = faces.begin(); its != faces.end(); ++its)
					m_selectedItems.insert(*its);

				emit sgSelectionChanged();

				m_pDlgFaceGrid->execMerge();
			}

			// kosshy wrote:
			// この下に毎回無効化があるのでコメントアウト
			// トグルタイプに
			onManualDetect();
			//ui.comboBoxMode->setItemText(Constants::ModeManualDetect, "Manual Detect");
			//m_mode = Constants::ModeNone;
			//ui.comboBoxMode->setCurrentIndex(Constants::ModeNone);
			//ui.pushButtonManualDetect->setChecked(false);
			//ui.pushButtonManualDetect->setIcon(QIcon(":/Icon/AddIcon"));
			//m_targetObject = -1;
			//unsetCursor();
			//if (m_pDlgFaceGrid->isVisible())
			//	m_pDlgFaceGrid->unsetWaitRegion();
			
			break;
		}
	}
}

void QtHOG::skinLearning()
{
	/*
	int w = m_selectedRegion[1].x -  m_selectedRegion[0].x;
	int h = m_selectedRegion[1].y -  m_selectedRegion[0].y;

	m_pStructData->mutex.lock();
	IplImage *img = m_pAviManager->getImage(ui.sliderFrame->value());
	m_pStructData->mutex.unlock();

	qDebug() << m_selectedRegion[0].x << m_selectedRegion[0].y << w << h;

	cvSetImageROI(img, cvRect(m_selectedRegion[0].x, m_selectedRegion[0].y, w, h));

	IplImage *region = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 3);
	cvCopy(img, region);
	cvResetImageROI(img);

	cvNamedWindow("region");
	cvShowImage("region", region);

	const int HalfKernelSize = 1;//3;
	const int KernelSize = HalfKernelSize * 2 + 1;

	// 共分散行列
	int ww = region->width - 2 * HalfKernelSize;
	int hh = region->height - 2 * HalfKernelSize;

	std::vector<PTM::TMatrixRow<2, 2, double> > covMat(ww * hh);
	std::vector<PTM::TVector<2, double> > meanRg(ww * hh);

	uchar *pixel;

	int skinCls = -1;
	for (int y = 0; y < region->height; ++y)
	{
		for (int x = 0; x < region->width; ++x)
		{
			pixel = (uchar*)(region->imageData) + y * region->widthStep + 3 * x;
			float b = *(pixel++);
			float g = *(pixel++);
			float r = *pixel;

			//skinCls = m_pSkinClassifier->addClusterSample(r, g, b, skinCls);
		}
	}
	m_pDlgSkinColor->updateView();


	cvReleaseImage(&region);
	*/
}

void QtHOG::manualDetect()
{
	//ui.comboBoxMode->setItemText(Constants::ModeManualDetect, "Manual Detect");
	int target = m_targetObject;
	m_targetObject = -1;
	//unsetCursor();
	//ui.comboBoxMode->setCurrentIndex(MANUAL_DETECT);

	StructData::iterator<SRegion> it;
	int frame = ui.sliderFrame->value();

	CvRect newRect;
	newRect.x = m_selectedRegion[0].x;
	newRect.y = m_selectedRegion[0].y;
	newRect.width = m_selectedRegion[1].x - m_selectedRegion[0].x;
	newRect.height = m_selectedRegion[1].y - m_selectedRegion[0].y;

	if (newRect.width < 5 || newRect.height < 5)
		return;

	it = m_pStructData->insertRegion(m_curCamera, frame, target, newRect, SRegion::TypeManual);

	if (target >= 0)
		m_pStructData->setRegionFlags(it, SRegion::FlagTracked | SRegion::FlagManual);

	m_selectedItems.clear();
	m_selectedItems.insert(it.getID());
	m_lastSelected = it.getID();

	setFrame(ui.sliderFrame->value(), true);
	
	std::vector<int> changedImages(1, it.getID());
	emit sgImageChanged(changedImages, true);
	//emit sgSelectionChanged();

	if (m_pDlgFaceGrid->isVisible())
	{
		m_pDlgFaceGrid->unsetWaitRegion();
		m_pDlgFaceGrid->updateView();
	}
	updateSelectNum();
}

StructData::iterator<SRegion> QtHOG::searchNextObject(int frame)
{
	if (m_pAviManager->getFrameCount() < 1)
		return StructData::iterator<SRegion>();

	int curFrame = frame;
	while (true)
	{
		StructData::iterator<SRegion> it;

		for (it = m_pStructData->regionBegin(m_curCamera, curFrame); it != m_pStructData->regionEnd(); ++it)
		{
			if (it->type == SRegion::TypeHog)
				return it;
		}

		++curFrame;
		if (curFrame > m_pAviManager->getFrameCount() - 1)
			curFrame = 0;

		if (curFrame == frame)
			return StructData::iterator<SRegion>();
	}
}

void QtHOG::on_actionNext_Objects_triggered()
{
	if (m_curRegion == m_pStructData->regionEnd())
	{
		int frame = ui.sliderFrame->value();

		m_curRegion = searchNextObject(frame);
		if (m_curRegion == m_pStructData->regionEnd())
			return;

		setFrame(m_curRegion->frame);
		//ui.sliderFrame->setValue(m_curRegion->frame);
	}
	else
	{
		++m_curRegion;
		for (;m_curRegion != m_pStructData->regionEnd(); ++m_curRegion)
		{
			if (m_curRegion->type == SRegion::TypeHog)
				break;
		}

		if (m_curRegion == m_pStructData->regionEnd())
		{
			int frame = ui.sliderFrame->value();

			m_curRegion = searchNextObject(frame + 1);
			if (m_curRegion == m_pStructData->regionEnd())
				return;
			setFrame(m_curRegion->frame);
		}
	}

	ui.sliderScale->setEnabled(true);
	ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);

	qDebug() << m_curRegion->camera << m_curRegion->frame << m_curRegion->object;
	qDebug() << m_curRegion->rect.x;

	CvRect &r = m_curRegion->rect;
	float scale = ui.imageView->adjustScale(r.width, r.height);
	ui.labelScale->setText(QString::number((int)(scale * 100)));
	ui.sliderScale->setValue((int)(scale * 100));

	ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);
}

void QtHOG::on_menuTest_aboutToShow()
{
	//ui.actionDetect_Face->setEnabled(m_curRegion != m_pStructData->regionEnd() && m_pSkinClassifier->isSingleGaussAvailable());
}
void QtHOG::on_checkBoxFace_clicked()
{
	clickType(ui.checkBoxFace->isChecked(), SRegion::TypeFace);
}

void QtHOG::on_checkBoxPlate_clicked()
{
	clickType(ui.checkBoxPlate->isChecked(), SRegion::TypePlate);
}

void QtHOG::clickType(bool isChecked, SRegion::Type type)
{
	std::vector<SRegion::Type>::iterator itType = std::find(m_TargetType.begin(), m_TargetType.end(), type);
	if (isChecked && itType == m_TargetType.end())
		m_TargetType.push_back(type);
	if (!isChecked && itType != m_TargetType.end())
		m_TargetType.erase(itType);
	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->updateTarget();
	setFrame(ui.sliderFrame->value(), true);
}

static float calcMdist(const PTM::TVector<2, double> &v1, const PTM::TVector<2, double> &v2,
				const PTM::TMatrixRow<2, 2, double> &m)
{
	PTM::TMatrixRow<2, 2, double> mInv = m.inv();

	PTM::TVector<2, double> dv = v2 - v1;
	PTM::TVector<2, double> mv = mInv * dv;

	double b = PTM::dot(dv, mv);

	return sqrt(b);
}

//未使用
void QtHOG::on_actionSkin_Detect_triggered()
{
}

bool rectsOverlap(const CvRect &r1, const CvRect &r2, float threshRatio = 0.8)
{
	CvRect r3;
	r3.x = r1.x < r2.x ? r1.x : r2.x;
	r3.y = r1.y < r2.y ? r1.y : r2.y;
	
	int r1_x2 = r1.x + r1.width;
	int r1_y2 = r1.y + r1.height;
	int r2_x2 = r2.x + r2.width;
	int r2_y2 = r2.y + r2.height;

	r3.width = r1_x2 > r2_x2 ? r1_x2 - r3.x : r2_x2 - r3.x;
	r3.height = r1_y2 > r2_y2 ? r1_y2 - r3.y : r2_y2 - r3.y;

	float r1_area = r1.width * r1.height;
	float r2_area = r2.width * r2.height;
	float r3_area = r3.width * r3.height;

	return ((r1_area / r3_area) > threshRatio || (r2_area / r3_area) > threshRatio);
}

//未使用
void QtHOG::on_actionDetect_Face_triggered()
{
}

void QtHOG::detectFaceInFrame(int frame)
{
#ifdef DEBUG
	OutputDebugString(_T("detectFaceInFrame\n"));
#endif

	m_pStructData->mutex.lock();
	cv::Mat image = m_pAviManager->getImage(frame);
	m_pStructData->mutex.unlock();

	StructData::iterator<SRegion> itr;
	std::vector<CvRect> faceRects;

	for (itr = m_pStructData->regionBegin(m_curCamera, frame); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->type != SRegion::TypeHog)
			continue;

		CvRect rect = itr->rect;
		cv::Point facePos;
		
		bool detectFace = false;

		//if(m_useGPU == true)
		//{
		//	detectFace = detectFaceInRegion7(m_svm, image, rect, facePos);
		//}
		//else
		{
			detectFace = detectFaceInRegion7NoGPU(m_svm, image, rect, facePos);
		}

		if (detectFace == true)
		{
			m_pStructData->setRegionFlags(itr, SRegion::FlagHasFace);

			int faceSize = rect.width / 8;	// test
			CvRect faceRect;
			faceRect.x = rect.x + facePos.x - faceSize;
			faceRect.y = rect.y + facePos.y - faceSize;
			faceRect.width = 2 * faceSize;
			faceRect.height = 2 * faceSize;

			faceRects.push_back(faceRect);
		}

	}

	std::vector<std::set<int> > regionSets;
	std::vector<std::set<int> >::iterator itSets;
	std::set<int>::iterator its;

	for (unsigned int j = 0; j < faceRects.size(); ++j)
	{
		std::set<int> tmpSet;
		tmpSet.insert(j);
		for (unsigned int k = j + 1; k < faceRects.size(); ++k)
		{
			if (rectsOverlap(faceRects[j], faceRects[k]))
				tmpSet.insert(k);
		}
		// 既存のグループとの統合
		bool bUnion = false;
		for (itSets = regionSets.begin(); itSets != regionSets.end(); ++itSets)
		{
			for (its = tmpSet.begin(); its != tmpSet.end(); ++its)
			{
				if (itSets->find(*its) != itSets->end())
				{
					bUnion = true;
					break;
				}
			}
			if (bUnion)
			{
				for (its = tmpSet.begin(); its != tmpSet.end(); ++its)
					itSets->insert(*its);
				break;
			}
		}
		if (!bUnion)
			regionSets.push_back(tmpSet);
	}
	
	for (itSets = regionSets.begin(); itSets != regionSets.end(); ++itSets)
	{
		CvRect maxRect;
		int maxArea = -1;
		for (its = itSets->begin(); its != itSets->end(); ++its)
		{
			CvRect &r = faceRects[*its];
			if (r.width * r.height > maxArea)
			{
				maxArea = r.width * r.height;
				maxRect = r;
			}
		}

		itr = m_pStructData->insertRegion(m_curCamera, frame, -1, maxRect, SRegion::TypeFace);

		itr->cov.clear();// = cov;
		itr->u.clear();// = u;
	}
	updateSelectNum();
}

//未使用
void QtHOG::on_actionSkin_Sampling_triggered()
{

}

struct STrail
{
	int lastFrame;
	int life;
	CvRect rect;
	double depth;
	MathUtil::Vec3d frameTrans;
	MathUtil::Mat3d frameRot;
	MathUtil::Vec3d firstRay;
	MathUtil::Vec3d pos;

	MathUtil::Mat2d cov;
	MathUtil::Vec2d u;

	SRegion::Type type;

	STrail() : life(1), depth(-1) {}
};

bool QtHOG::readSingleGauss()
{
	/*
	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read single gauss data"), QDir::currentPath(),
													tr("Sample gauss data(*.sgl);;All files (*.*)"), &selFilter);

	if (fileName.isEmpty())
		return false;

	//m_pSkinClassifier->readSingleGauss(fileName.toLocal8Bit());
	m_pDlgSkinColor->updateView();
	*/

	return true;
}

void QtHOG::on_actionRead_SVM_Data_triggered()
{
	if (m_svm)
		delete m_svm;

	readSVMData();
}

bool QtHOG::readSVMData()
{
	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read SVM data"), QDir::currentPath(),
													tr("SVM data(*.xml);;All files (*.*)"), &selFilter);

	if (fileName.isEmpty())
		return false;

	if (!m_svm)
	{
		m_svm = new CvSVM;
		setCursor(Qt::WaitCursor);
		m_svm->load (fileName.toLocal8Bit());
/*
		m_decisionPlane.resize(Constants::HogDim + 1, 0);
		// SVMからパラメータを取得する
		std::vector<float> vecParams(Constants::HogDim, 0);
	
		CvMat m;
		cvInitMatHeader (&m, 1, Constants::HogDim, CV_32FC1, NULL);

		cvSetData (&m, &vecParams[0], sizeof(float) * Constants::HogDim);
		m_decisionPlane[Constants::HogDim] = m_svm->predict(&m, true);
		//double rho = m_svm->predict(&m, true);

		for (int i = 0; i < Constants::HogDim; ++i)
		{
			vecParams[i] = 1.0f;
			cvSetData (&m, &vecParams[0], sizeof(float) * Constants::HogDim);
			m_decisionPlane[i] = m_svm->predict(&m, true) - m_decisionPlane[Constants::HogDim];
			vecParams[i] = 0.0f;
		}
*/		
		unsetCursor();
	}

	return true;
}

void QtHOG::on_actionFace_Tracking_triggered()
{
	/*
	if (!m_pSkinClassifier->isSingleGaussAvailable())
	{
		if (!readSingleGauss())
			return;
		if (!m_pSkinClassifier->isSingleGaussAvailable())
			return;
	}
	*/
	PTM::TVector<2, double> u;
	PTM::TMatrixRow<2, 2, double> cov;

	std::map<int, STrail> trails;

	int startFrame = 0;
	int endFrame = m_pAviManager->getFrameCount() - 1;
	const int SearchRange = 70;	// temp
	const double CorrThresh = 0.6;	// temp
	//const double ColorThresh = -2.0;	// temp
	const int NumImageCache = 20;
	const int InitialLife = 3;

	// 先頭フレームの処理
	m_pStructData->mutex.lock();
	cv::Mat frameImage = m_pAviManager->getImage(startFrame);
	m_pStructData->mutex.unlock();
	std::map<int, cv::Mat> grayImages;
	cv::cvtColor(frameImage, grayImages[0], CV_BGR2GRAY);

	StructData::iterator<SRegion> itr;
	for (itr = m_pStructData->regionBegin(m_curCamera, startFrame); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
			continue;

		// 新規に追跡を開始
		trails[itr->object].lastFrame = startFrame;
		trails[itr->object].rect = itr->rect;
		trails[itr->object].life = InitialLife;
		trails[itr->object].type = itr->type;
	}

	for (unsigned int frame = startFrame + 1; frame <= endFrame; ++frame)
	{
		qDebug() << "frame" << frame;

		m_pStructData->mutex.lock();
		cv::Mat frameImage = m_pAviManager->getImage(frame);
		m_pStructData->mutex.unlock();
		//cv::Mat grayImage;
		cv::cvtColor(frameImage, grayImages[frame], CV_BGR2GRAY);

		std::map<int, STrail>::iterator itTrail;
		std::set<int> eraseSet;
		for (itTrail = trails.begin(); itTrail != trails.end(); ++itTrail)
		{
			CvRect &sourceRect = itTrail->second.rect;
			int cx = sourceRect.x + sourceRect.width / 2;
			int cy = sourceRect.y + sourceRect.height / 2;

			if (cx < 50 || cx > frameImage.cols - 50 || cy < 50 || cy > frameImage.rows - 50)
			{
				eraseSet.insert(itTrail->first);
				continue;
			}

			int minX = cx - SearchRange;
			int minY = cy - SearchRange;
			int maxX = cx + SearchRange;
			int maxY = cy + SearchRange;

			if (minX < 0)
				minX = 0;
			if (minY < 0)
				minY = 0;
			if (maxX >= frameImage.cols)
				maxX = frameImage.cols - 1;
			if (maxY >= frameImage.rows)
				maxY = frameImage.rows - 1;

			cv::Rect searchRoi(minX, minY, maxX - minX, maxY - minY);
			cv::Mat searchWindow = grayImages[frame](searchRoi);
			
			cv::Rect patchRoi(sourceRect);
			cv::Mat patch = grayImages[itTrail->second.lastFrame](patchRoi);

			cv::Mat searchResult;
			cv::matchTemplate(searchWindow, patch, searchResult, CV_TM_CCOEFF_NORMED);

			//double maxVal;
			//cv::Point maxLoc;
			//cv::minMaxLoc(searchResult, 0, &maxVal, 0, &maxLoc);

			std::vector<cv::Point> candidates;
			//std::vector<double> scores;
			std::vector<std::pair<int, int> > scoreSortList;

			const int NonMaxSize = 20;
			//int w = itTrail->second.patch.cols;
			//int h = itTrail->second.patch.rows;

			//StructData::iterator<SRegion> itPrev;
			//itPrev = m_pStructData->regionFitAt(itTrail->second.lastRegion);

			for (int y = 0; y < (searchResult.rows / NonMaxSize); ++y)
			{
				for (int x = 0; x < (searchResult.cols / NonMaxSize); ++x)
				{
					cv::Mat window(searchResult, cv::Rect(x * NonMaxSize, y * NonMaxSize, NonMaxSize, NonMaxSize));
					double maxCorr;
					cv::Point maxPos;
					cv::minMaxLoc(window, 0, &maxCorr, 0, &maxPos);

					if (maxCorr < CorrThresh)
						continue;

					int xx = searchRoi.x + maxPos.x + x * NonMaxSize;
					int yy = searchRoi.y + maxPos.y + y * NonMaxSize;

					if (xx < 50 || xx > frameImage.cols - 50 || yy < 50 || yy > frameImage.rows - 50)
						continue;

					// Married-Match テスト
					int minX = xx - SearchRange;
					int minY = yy - SearchRange;
					int maxX = xx + SearchRange;
					int maxY = yy + SearchRange;

					if (minX < 0)
						minX = 0;
					if (minY < 0)
						minY = 0;
					if (maxX >= frameImage.cols)
						maxX = frameImage.cols - 1;
					if (maxY >= frameImage.rows)
						maxY = frameImage.rows - 1;

					//qDebug() << minX << minY << maxX << maxY;

					cv::Rect reverse_searchRoi(minX, minY, maxX - minX, maxY - minY);
					cv::Mat reverse_searchWindow = grayImages[itTrail->second.lastFrame](reverse_searchRoi);

					//qDebug() << "----- 1 -----";

					//if (frame == 236)
					//	qDebug() << xx << yy << sourceRect.x << sourceRect.y << sourceRect.width << sourceRect.height;

					cv::Rect sourceRoi(xx, yy, sourceRect.width, sourceRect.height);

					//if (frame == 236)
					//qDebug() << "----- 2 -----";

					cv::Mat sourceMat = grayImages[frame](sourceRoi);

					//if (frame == 236)
					//qDebug() << "----- 3 -----";

					cv::matchTemplate(reverse_searchWindow, sourceMat, searchResult, CV_TM_CCOEFF_NORMED);

					//double maxVal;
					//cv::Point maxLoc;
					cv::minMaxLoc(searchResult, 0, 0, 0, &maxPos);

					double dx = sourceRect.x - (reverse_searchRoi.x + maxPos.x);
					double dy = sourceRect.y - (reverse_searchRoi.y + maxPos.y);

					if (dx * dx + dy * dy > 400)	// temp
						continue;

					// 肌色で絞り込む
					const int ColorMargin = 10;

					minX = xx - ColorMargin;
					minY = yy - ColorMargin;
					maxX = xx + sourceRect.width + ColorMargin;
					maxY = yy + sourceRect.height + ColorMargin;

					if (minX < 0)
						minX = 0;
					if (minY < 0)
						minY = 0;
					if (maxX >= frameImage.cols)
						maxX = frameImage.cols - 1;
					if (maxY >= frameImage.rows)
						maxY = frameImage.rows - 1;

					CvRect colorRect;
					colorRect.x = minX;
					colorRect.y = minY;
					colorRect.width = maxX - minX;
					colorRect.height = maxY - minY;
					
					cv::Mat distMat(colorRect.height, colorRect.width, CV_32FC1);
					uchar *pixel;
					for (int y = 0; y < colorRect.height; ++y)
					{
						uchar *pixelBase = (uchar*)(frameImage.data) + (colorRect.y + y) * frameImage.step;

						for (int x = 0; x < colorRect.width; ++x)
						{
							//pixel = (uchar*)(image->imageData) + (y + rect.y) * region->widthStep + 3 * x;
							pixel = pixelBase + 3 * (colorRect.x + x);

							float b = *(pixel++);
							float g = *(pixel++);
							float r = *pixel;

							PTM::TVector<2, double> ts;
							//double /*t, s,*/ l;
							//m_pSkinClassifier->convRGB2TSL(r, g, b, ts[0], ts[1], l);
					
							//ts[0] = log(t * 100);	ts[1] = log(s * 100);
		
							distMat.at<float>(y, x) = -calcMdist(u, ts, cov);
						}
					}
					cvSmooth(&CvMat(distMat), &CvMat(distMat), 2, 11);
					
					double maxColorDist;
					cv::minMaxLoc(distMat, 0, &maxColorDist, 0, &maxPos);

					if (maxColorDist < -m_colorThresh)
						continue;

					double corrScore = (maxCorr - CorrThresh) / (1 - CorrThresh);
					double colorScore = (maxColorDist + m_colorThresh) / (0 + m_colorThresh);

					candidates.push_back(cv::Point(colorRect.x + maxPos.x - sourceRect.width / 2, colorRect.y + maxPos.y - sourceRect.height / 2));
					//scores.push_back(corrScore + corrScore);

					scoreSortList.push_back(std::make_pair( static_cast<int>((corrScore + corrScore)) * 10000, static_cast<int>(candidates.size() - 1)));
				}
			}
			std::sort(scoreSortList.rbegin(), scoreSortList.rend());
			qDebug() << "candidates.size" << candidates.size();


			//if (maxVal > CorrThresh)	// 本当は複数の候補から色と組み合わせて選ぶ
			if (!candidates.empty())
			{
				// トラッキング成功
				StructData::iterator<SRegion> itr;//, itPrev;

				// イメージの更新
				CvRect rect;
				rect.x = candidates[0].x;
				rect.y = candidates[0].y;
				rect.width = sourceRect.width;
				rect.height = sourceRect.height;

				// 既存のTrailと重複をチェック
				std::map<int, STrail>::iterator foundTrail = trails.end();
				std::map<int, STrail>::iterator itCheck;
				for (itCheck = trails.begin(); itCheck != trails.end(); ++itCheck)
				{
					if (itCheck->second.lastFrame != frame)
						continue;

					if (rectsOverlap(rect, itCheck->second.rect, 0.7f))
					{
						foundTrail = itCheck;
						break;
					}
				}

				// 新規Region登録
				if (foundTrail == trails.end())
				{
					itr = m_pStructData->insertRegion(m_curCamera, frame, itTrail->first, rect, itTrail->second.type);

					m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);

					itTrail->second.lastFrame = frame;
					itTrail->second.life = InitialLife;
					itTrail->second.rect = rect;
				}
			}

			--itTrail->second.life;
			if (itTrail->second.life <= 0)
				eraseSet.insert(itTrail->first);
		}	// for (itTrail = trails.begin(); itTrail != trails.end(); ++itTrail)

		// トラッキングされなかったTrailの削除
		std::set<int>::iterator its;
		for (its = eraseSet.begin(); its != eraseSet.end(); ++its)
			trails.erase(*its);

		// Trailの更新
		StructData::iterator<SRegion> itr, itNext, itNew;
		std::set<int> integratedSet;
		for (itr = m_pStructData->regionBegin(m_curCamera, frame); itr != m_pStructData->regionEnd();)
		{
			itNext = itr;
			++itNext;

			if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
			{
				itr = itNext;
				continue;
			}
			if (m_pStructData->checkRegionFlags(itr, SRegion::FlagTracked))
			{
				itr = itNext;
				continue;
			}

			if (integratedSet.find(itr.getID()) != integratedSet.end())
				continue;

			if (trails.find(itr->object) != trails.end())
			{
				//qDebug() << "********************************** not here **************************";
				trails[itr->object].lastFrame = frame;//itr.getID();
				trails[itr->object].life = InitialLife;
				trails[itr->object].rect = itr->rect;
			}
			else
			{
				// 既存のTrailと重複があるか確認
				std::map<int, STrail>::iterator foundTrail = trails.end();
				for (itTrail = trails.begin(); itTrail != trails.end(); ++itTrail)
				{
					if (itTrail->second.lastFrame != frame)
						continue;

					if (rectsOverlap(itr->rect, itTrail->second.rect, 0.7f))
					{
						foundTrail = itTrail;
						break;
					}
				}

				if (foundTrail != trails.end())
				{
					foundTrail->second.lastFrame = frame;//itr.getID();
					foundTrail->second.life = InitialLife;
					foundTrail->second.rect = itr->rect;
					foundTrail->second.type = itr->type;

					m_pStructData->eraseObject(itr->object);

					itNew = m_pStructData->insertRegion(m_curCamera, frame, foundTrail->first, foundTrail->second.rect, foundTrail->second.type);
					//m_pStructData->setRegionFlags(itNew, SRegion::RegionFlag::TRACKED);
					integratedSet.insert(itNew.getID());
				}
				else
				{
					// 新規に追跡を開始
					trails[itr->object].lastFrame = frame;//itr.getID();
					trails[itr->object].life = InitialLife;
					trails[itr->object].rect = itr->rect;
					trails[itr->object].type = itr->type;
				}
			}
			itr = itNext;
		}	// for (itr = m_pStructData->regionBegin(0, frame); itr != m_pStructData->regionEnd();)

		// 古い画像キャッシュのクリア
		while (grayImages.size() > NumImageCache)
			grayImages.erase(grayImages.begin());
	}	// for (unsigned int frame = startFrame + 1; frame <= endFrame; ++frame)
}

void QtHOG::on_actionRead_ICV_triggered()
{
	return;
	
	// ICVファイルの読込み
	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read ICV data"), QDir::currentPath(),
													tr("ICV data file (*.icv)"), &selFilter
													);

	if (fileName.isEmpty())
		return;

	//m_pStructData->importICV(fileName.toLocal8Bit());
}

bool calcS(MathUtil::Vec3d p0, MathUtil::Vec3d p1, MathUtil::Vec3d d0, MathUtil::Vec3d d1, double &s0, double &s1)
{
    double a = 1.0, b = -d0 *d1, c = d0 * d1, d = -1.0;
        double p = d1 * p0 - d1 * p1;
        double q = d0 * p0 - d0 * p1;
	double det = a * d - b * c;
	if (fabs(det) < 1.0e-5)
		return false;

	s1 = (d * p - b * q) / det;
	s0 = (a * q - p * c) / det;
	return true;
}
bool QtHOG::objectTracking(int startFrame, int endFrame)
{	

	std::map<int, STrail> trails;

	int searchRange;
	const double CorrThresh = 0.6;	// temp

	const int NumImageCache = 20;
	const int InitialLife = 3;

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	// 先頭フレームの処理
	m_pStructData->mutex.lock();
	cv::Mat frameImage = m_pAviManager->getImage(startFrame);
	m_pStructData->mutex.unlock();
	std::map<int, cv::Mat> grayImages;
	cv::cvtColor(frameImage, grayImages[startFrame], CV_BGR2GRAY);

	StructData::iterator<SRegion> itr;


	for (itr = m_pStructData->regionBegin(m_curCamera, startFrame); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
			continue;

		// 新規に追跡を開始
		trails[itr->object].lastFrame = startFrame;
		trails[itr->object].rect = itr->rect;
		trails[itr->object].life = InitialLife;
		trails[itr->object].type = itr->type;
	}

	int vf = (endFrame >= startFrame) ? 1 : -1;
	for (int frame = startFrame + vf; frame != endFrame + vf; frame += vf)
	{
		if (m_bCancel)
			break;

		QString msg = QString("Tracking in %1 (to %2)...").arg(frame).arg(endFrame); 
		m_executeStatus.setText(msg);
		qApp->processEvents();

		m_pStructData->mutex.lock();
		cv::Mat frameImage = m_pAviManager->getImage(frame);
		m_pStructData->mutex.unlock();
		cv::cvtColor(frameImage, grayImages[frame], CV_BGR2GRAY);

		std::map<int, STrail>::iterator itTrail;
		std::set<int> eraseSet;
		for (itTrail = trails.begin(); itTrail != trails.end(); ++itTrail)
		{
			// 既出オブジェクトのチェック
			itr = m_pStructData->regionFitAt(m_curCamera, frame, itTrail->first);
			if (itr != m_pStructData->regionEnd())
			{
				itTrail->second.lastFrame = frame;
				itTrail->second.life = InitialLife;
				itTrail->second.rect = itr->rect;
				itTrail->second.type = itr->type;

				continue;
			}

			CvRect &sourceRect = itTrail->second.rect;

			CvRect predictRect;

			if (getPredictRect(itTrail->first, frame, width, height, predictRect))
				searchRange = m_PredictSearchRange;//20;//50;
			else
				searchRange = m_InitialSearchRange;//80;//150;

			CvPoint c = MathUtil::rectCenter(predictRect);

			if (c.x < 50 || c.x > frameImage.cols - 50 || c.y < 50 || c.y > frameImage.rows - 50)
			{
				eraseSet.insert(itTrail->first);
				continue;
			}

			int minX = c.x - searchRange;
			int minY = c.y - searchRange;
			int maxX = c.x + searchRange;
			int maxY = c.y + searchRange;

			if (minX < 0)
				minX = 0;
			if (minY < 0)
				minY = 0;
			if (maxX >= frameImage.cols)
				maxX = frameImage.cols - 1;
			if (maxY >= frameImage.rows)
				maxY = frameImage.rows - 1;

			cv::Rect searchRoi(minX, minY, maxX - minX, maxY - minY);
			cv::Mat searchWindow = grayImages[frame](searchRoi);
			
			cv::Mat searchClone = searchWindow.clone();

			int marginWidth = sourceRect.width / 5;
			int marginHeight = sourceRect.height / 5;

			CvRect newRect;
			newRect.x = sourceRect.x + marginWidth;
			newRect.y = sourceRect.y + marginHeight;
			newRect.width = sourceRect.width - 2 * marginWidth;
			newRect.height = sourceRect.height - 2 * marginHeight;

			cv::Rect patchRoi(newRect);
			cv::Mat patch = grayImages[itTrail->second.lastFrame](patchRoi);

			cv::Mat searchResult;
			cv::matchTemplate(searchClone, patch, searchResult, CV_TM_CCOEFF_NORMED);

			double maxCorr;
			cv::Point maxPos;
			cv::minMaxLoc(searchResult, 0, &maxCorr, 0, &maxPos);

			cv::Point foundPos;
			foundPos.x = searchRoi.x + maxPos.x;
			foundPos.y = searchRoi.y + maxPos.y;

			// CPU版 //
			// SVM でチェックする
			if (itTrail->second.type == SRegion::TypeFace)
			{
				int portraitSize = sourceRect.width * 2;
				float scale = (float)HogUtils::RESIZE_X * 2 / (float)portraitSize;

				cv::Mat searchResize;
				cv::resize(searchClone, searchResize, cv::Size(searchClone.size().width * scale, searchClone.size().height * scale));
				cv::HOGDescriptor hog(cv::Size(96, 96), cv::Size(16, 16), cv::Size(8, 8), cv::Size(8, 8), 9, -1, 0.2, false, 1);
				
				while (true)
				{
					cv::minMaxLoc(searchResult, 0, &maxCorr, 0, &maxPos);

					if (maxCorr < CorrThresh)
						break;

					cv::Rect foundRect((maxPos.x - sourceRect.width * 0.5 - marginWidth) * scale, (maxPos.y - sourceRect.height * 0.5  - marginHeight) * scale,
										HogUtils::RESIZE_X * 2, HogUtils::RESIZE_Y * 2);

					if (foundRect.x < 0)
					{
						searchResult.at<float>(maxPos.y, maxPos.x) = 0;
						continue;
					}
					if (foundRect.y < 0)
					{
						searchResult.at<float>(maxPos.y, maxPos.x) = 0;
						continue;
					}
					if (foundRect.x >= searchResize.size().width - foundRect.width)
					{
						searchResult.at<float>(maxPos.y, maxPos.x) = 0;
						continue;
					}
					if (foundRect.y >= searchResize.size().height - foundRect.height)
					{
						searchResult.at<float>(maxPos.y, maxPos.x) = 0;
						continue;
					}

					cv::Mat img = searchResize(foundRect);
					std::vector<float> d;
					hog.compute(img, d, cv::Size(8, 8), cv::Size(0, 0));
					cv::Mat descriptors(1, Constants::HogDim, CV_32FC1, &d[0]);

					float score = m_svm->predict(descriptors, true);
					if(score <= m_FaceThreshold)
					{
						foundPos.x = searchRoi.x + maxPos.x;
						foundPos.y = searchRoi.y + maxPos.y;
						break;
					}

					searchResult.at<float>(maxPos.y, maxPos.x) = 0;
				}	// while (true)
			}

			foundPos.x -= marginWidth;
			foundPos.y -= marginHeight;

			if (foundPos.x < 0)
				foundPos.x = 0;
			if (foundPos.y < 0)
				foundPos.y = 0;
			if (foundPos.x >= width - sourceRect.width)
				foundPos.x = width - sourceRect.width - 1;
			if (foundPos.y >= height - sourceRect.height)
				foundPos.y = height - sourceRect.height - 1;


			if (maxCorr > CorrThresh)// && !bColorReject)//!candidates.empty())
			{
				// トラッキング成功
				StructData::iterator<SRegion> itr;//, itPrev;

				// イメージの更新
				CvRect rect;
				rect.x = foundPos.x;//candidates[scoreSortList[0].second].x;
				rect.y = foundPos.y;//candidates[scoreSortList[0].second].y;
				rect.width = sourceRect.width;
				rect.height = sourceRect.height;

				// 既存のTrailと重複をチェック
				std::map<int, STrail>::iterator foundTrail = trails.end();
				std::map<int, STrail>::iterator itCheck;
				for (itCheck = trails.begin(); itCheck != trails.end(); ++itCheck)
				{
					if (itCheck->second.lastFrame != frame)
						continue;

					if (rectsOverlap(rect, itCheck->second.rect, 0.7f))
					{
						foundTrail = itCheck;
						break;
					}
				}

				// 新規Region登録
				if (foundTrail == trails.end())
				{
					itr = m_pStructData->insertRegion(m_curCamera, frame, itTrail->first, rect, itTrail->second.type);

					m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);


					itTrail->second.lastFrame = frame;
					itTrail->second.life = InitialLife;
					itTrail->second.rect = rect;
						//}
					//}
				}
			}	// if (maxCorr > CorrThresh && !bColorReject)//!candidates.empty())

			--itTrail->second.life;
			if (itTrail->second.life <= 0)
				eraseSet.insert(itTrail->first);
		}	// for (itTrail = trails.begin(); itTrail != trails.end(); ++itTrail)

		// トラッキングされなかったTrailの削除
		std::set<int>::iterator its;
		for (its = eraseSet.begin(); its != eraseSet.end(); ++its)
			trails.erase(*its);

		// Trailの更新
		StructData::iterator<SRegion> itr, itNext, itNew;
		std::set<int> integratedSet;
		for (itr = m_pStructData->regionBegin(m_curCamera, frame); itr != m_pStructData->regionEnd();)
		{
			itNext = itr;
			++itNext;

			if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
			{
				itr = itNext;
				continue;
			}
			if (m_pStructData->checkRegionFlags(itr, SRegion::FlagTracked))
			{
				itr = itNext;
				continue;
			}

			if (integratedSet.find(itr.getID()) != integratedSet.end())
			{
				itr = itNext;
				continue;
			}

			if (trails.find(itr->object) != trails.end())
			{
				//qDebug() << "********************************** not here **************************";
				trails[itr->object].lastFrame = frame;//itr.getID();
				trails[itr->object].life = InitialLife;
				trails[itr->object].rect = itr->rect;

			}
			else
			{
				// 既存のTrailと重複があるか確認
				std::map<int, STrail>::iterator foundTrail = trails.end();
				for (itTrail = trails.begin(); itTrail != trails.end(); ++itTrail)
				{
					if (itTrail->second.lastFrame != frame)
						continue;

					if (rectsOverlap(itr->rect, itTrail->second.rect, 0.7f))
					{
						foundTrail = itTrail;
						break;
					}
				}

				if (foundTrail != trails.end())
				{
					foundTrail->second.lastFrame = frame;//itr.getID();
					foundTrail->second.life = InitialLife;
					foundTrail->second.rect = itr->rect;
					foundTrail->second.type = itr->type;

					m_pStructData->eraseObject(itr->object);

					itNew = m_pStructData->insertRegion(m_curCamera, frame, foundTrail->first, foundTrail->second.rect, foundTrail->second.type);
					//m_pStructData->setRegionFlags(itNew, SRegion::RegionFlag::TRACKED);
					integratedSet.insert(itNew.getID());
				}
				else
				{
					// 新規に追跡を開始
					trails[itr->object].lastFrame = frame;//itr.getID();
					trails[itr->object].life = InitialLife;
					trails[itr->object].rect = itr->rect;
					trails[itr->object].type = itr->type;

				}
			}
			itr = itNext;
		}	// for (itr = m_pStructData->regionBegin(0, frame); itr != m_pStructData->regionEnd();)

		// 古い画像キャッシュのクリア
		while (grayImages.size() > NumImageCache)
		{
			if (vf > 0)
				grayImages.erase(grayImages.begin()->first);
			else
				grayImages.erase(grayImages.rbegin()->first);
		}
	}	// for (unsigned int frame = startFrame + 1; frame <= endFrame; ++frame)
	m_executeStatus.clear();

	return !m_bCancel;
}


void QtHOG::on_actionFace_CV_Tracking_triggered()
{	

	if (!m_svm)
	{
		if (!readSVMData())
			return;
	}

	//映像が読み込まれていないなら処理できない
	if( m_pAviManager->getFrameCount() == 0 )
	{
		return;
	}

	m_bCancel = false;
	if (!objectTracking(0, m_pAviManager->getFrameCount() - 1))
		return;

	if (!objectTracking(m_pAviManager->getFrameCount() - 1, 0))
		return;
}

void QtHOG::on_actionShow_skin_score_triggered()
{
}

void QtHOG::on_actionShow_Faces_triggered()
{
	//m_pDlgFaces->setFrame(ui.sliderFrame->value());
	//m_pDlgFaces->show();
	m_pDlgFaceGrid->setAviName(m_pAviManager->getCurFileName());
	m_pDlgFaceGrid->setFrame(ui.sliderFrame->value());
	m_pDlgFaceGrid->show();
}

void QtHOG::onItemDoubleClicked(int id)
{
	StructData::iterator<SRegion> itr;
	itr = m_pStructData->regionFitAt(id);

	setFrame(itr->frame);

	CvRect &r = itr->rect;
	float scale = ui.imageView->adjustScale(r.width * 8, r.height * 8);
	ui.labelScale->setText(QString::number((int)(scale * 100)));
	ui.sliderScale->setValue((int)(scale * 100));
	
	ui.sliderScale->setEnabled(true);
	ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
	ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);

	m_lockonTarget = itr->object;

	m_selectedItems.clear();
	m_selectedItems.insert(id);

	emit sgSelectionChanged(); 
}

bool QtHOG::getPredictRect(int object, int frame, int width, int height, CvRect &predictRect)
{
	StructData::iterator<SRegion> itr, itr1, itr2;
	StructData::reverse_iterator<SRegion> itrr;
	//CvRect predictRect;

	//int width = m_pAviManager->getWidth();
	//int height = m_pAviManager->getHeight();

	itr = m_pStructData->regionBegin(object);
	itrr = m_pStructData->regionRBegin(object);

	int firstFrame = itr->frame;
	int lastFrame = itrr->frame;

	if (lastFrame - firstFrame < 1)
	{
		itr = m_pStructData->regionBegin(object);
		predictRect = itr->rect;

		return false;
	}
	else
	{
		// RANSACで対象物平面の法線を推定
		std::vector<MathUtil::Vec3d> regionDirs, hypNormals;
		for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
		{
			if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
				continue;
		
			CvPoint c = MathUtil::rectCenter(itr->rect);
			regionDirs.push_back(MathUtil::convertPix2Dir(c.x, c.y, width, height));
		}
		int numSamples = regionDirs.size();

		while (hypNormals.size() < numSamples)
		{
			int d0, d1;
			d1 = d0 = rand() % regionDirs.size();
			do
			{
				d1 = rand() % regionDirs.size();
			}
			while (d0 == d1);
			
			if (d1 < d0)
			{
				// swap
				int swp = d0;
				d0 = d1;
				d1 = swp;
			}
			hypNormals.push_back(PTM::cross(regionDirs[d0], regionDirs[d1]).unit());
		}
		int maxIndex;
		float maxScore = -1;
		for (int i = 0; i < hypNormals.size(); ++i)
		{
			// 採点
			float score = 0;
			for (int j = 0; j < regionDirs.size(); ++j)
			{
				float inp = fabs(regionDirs[j].dot(hypNormals[i]));
				if (inp < 0.174)	// cos80deg
					score += 1.0f - inp;
			}
			if (score > maxScore)
			{
				maxScore = score;
				maxIndex = i;
			}
		}

		// 位置を予測
		if (frame < firstFrame)
		{
			itr1 = m_pStructData->regionEnd();
			for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
			{
				if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
					continue;

				if (itr1 == m_pStructData->regionEnd())
					itr1 = itr;
				else
				{
					itr2 = itr;
					break;
				}		
			}
		}
		else if (frame > lastFrame)
		{
			itr2 = m_pStructData->regionEnd();
			for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
			{
				if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
					continue;

				if (itr2 == m_pStructData->regionEnd())
					itr2 = itr;
				else
				{
					itr1 = itr2;
					itr2 = itr;
				}		
			}
		}
		else
		{
			int prevDist(-1), nextDist(-1);
			for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
			{
				if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
					continue;

				if (itr->frame <= frame)
				{
					int dist = frame - itr->frame;
					if (prevDist < 0 || dist < prevDist)
					{
						prevDist = dist;
						itr1 = itr;
					}
				}
				else
				{
					itr2 = itr;
					break;
				}		
			}
		}

		CvPoint c = MathUtil::rectCenter(itr1->rect);
		MathUtil::Vec3d p0 = MathUtil::convertPix2Dir(c.x, c.y, width, height);

		c = MathUtil::rectCenter(itr2->rect);
		MathUtil::Vec3d p1 = MathUtil::convertPix2Dir(c.x, c.y, width, height);

		MathUtil::Vec3d n = PTM::cross(p0, p1);

		if (n.norm() > 0.017)	// sin1deg
		{
			double angle = asin(n.norm()) / (double)(itr2->frame - itr1->frame);
			n.unitize();

			// 法線方向の補正
			n = 0.2 * n + 0.8 * hypNormals[maxIndex];
			n.unitize();

			MathUtil::Mat3d rot;
			PTM::init_rot(rot, (double)(frame - itr1->frame) * angle, n);

			MathUtil::Vec2d predictPos = MathUtil::convertDir2Pix(rot * p0, width, height);

			predictRect = itr1->rect;

			predictRect.x = predictPos[0] - predictRect.width / 2;
			predictRect.y = predictPos[1] - predictRect.height / 2;

			if (predictRect.x < 0)
				predictRect.x = 0;
			if (predictRect.y < 0)
				predictRect.y = 0;
			if (predictRect.x >= width - predictRect.width)
				predictRect.x = width - predictRect.width - 1;
			if (predictRect.y >= height - predictRect.height)
				predictRect.y = height - predictRect.height - 1;
		}
		else
		{
			predictRect = itr1->rect;
		}

		return true;
	}
}

void QtHOG::setManualTarget(int object, int frame)
{
	int minDist = -1, sourceID;
	StructData::iterator<SRegion> itr;
	m_targetObject = object;

	QCursor cursor;
	cursor.setShape(Qt::WhatsThisCursor);
	setCursor(cursor);
	if (m_pDlgFaceGrid->isVisible())
		m_pDlgFaceGrid->setWaitRegion();

	for (itr = m_pStructData->regionBegin(m_targetObject); itr != m_pStructData->regionEnd(); ++itr)
	{
		int dist = abs(itr->frame - frame);
		if (minDist < 0 || dist < minDist)
		{
			sourceID = itr.getID();
			minDist = dist;
		}
	}
	itr = m_pStructData->regionFitAt(sourceID);
	
	m_pStructData->mutex.lock();
	cv::Mat frameImage = m_pAviManager->getImage(itr->frame);
	m_pStructData->mutex.unlock();

	qDebug() << ui.pushButtonManualDetect->width() << ui.pushButtonManualDetect->height() << itr->rect.width << itr->rect.height;

	float scale;
	if (itr->rect.width > itr->rect.height)
		scale = (float)(ui.pushButtonManualDetect->width() - 4) / (float)itr->rect.width;
	else
		scale = (float)(ui.pushButtonManualDetect->height() - 4) / (float)itr->rect.height;

	cv::Mat buttonImage = frameImage(itr->rect);

	cv::cvtColor(buttonImage, buttonImage, CV_BGR2RGB);
	QImage tmp(buttonImage.data, buttonImage.cols, buttonImage.rows, buttonImage.step, QImage::Format_RGB888);

	ui.pushButtonManualDetect->setIconSize(QSize(ui.pushButtonManualDetect->width() - 8, ui.pushButtonManualDetect->height() - 8));
	ui.pushButtonManualDetect->setIcon(QIcon(QPixmap::fromImage(tmp)));

	ui.pushButtonManualDetect->setChecked(true);
}

void QtHOG::onItemDoubleClicked(int object, int frame, bool bCtrl)
{
	if (!m_svm)
	{
		if (!readSVMData())
			return;
	}

	StructData::iterator<SRegion> itr;
	CvRect predictRect;

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	int searchRange;
	if (getPredictRect(object, frame, width, height, predictRect))
		searchRange = m_PredictSearchRange;//20;//30;
	else
		searchRange = m_InitialSearchRange;//80;//60;

	if( ui.checkBoxAutoFocus->isChecked() )
	{
		float scale = ui.imageView->adjustScale(predictRect.width * 8, predictRect.height * 8);
		ui.labelScale->setText(QString::number((int)(scale * 100)));
		ui.sliderScale->setValue((int)(scale * 100));

		ui.sliderScale->setEnabled(true);
		ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
	}

	QPointF predictPos(predictRect.x + predictRect.width / 2, predictRect.y + predictRect.height / 2);

	if( ui.checkBoxAutoFocus->isChecked() )
	{
		ui.imageView->centerOn(predictPos);
	}

	if (bCtrl)
	{
		// マニュアル検出モードに切り替え
		setManualTarget(object, frame);
		m_mode = Constants::ModeManualDetect;
	}
	else
	{
		// 自動トラッキング処理
		//qDebug() << "before predictTrackingRectWithSVM";

		CvRect trackingRect;
		//if (predictTrackingRect(predictPos, frame, object, searchRange, trackingRect))
		if (predictTrackingRectWithSVM(predictPos, frame, object, searchRange, trackingRect))
		{
			//qDebug() << "after predictTrackingRectWithSVM";


			std::vector<int> faces;
			int lastSelected;

			cv::Point point(trackingRect.x + trackingRect.width / 2, trackingRect.y + trackingRect.height / 2);
			doSelectFaces(frame, point, point, faces, lastSelected);
			int trackTarget = -1;
			for (std::vector<int>::iterator itf = faces.begin(); itf != faces.end(); ++itf)
			{
				itr = m_pStructData->regionFitAt(*itf);
				if (rectsOverlap(itr->rect, trackingRect, 0.7f))
				{
					trackTarget = itr.getID();
					break;
				}
			}

			itr = m_pStructData->insertRegion(m_curCamera, frame, object, trackingRect, m_pStructData->regionBegin(object)->type);

			m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);

			m_selectedItems.clear();
			m_selectedItems.insert(itr.getID());
			m_lastSelected = itr.getID();

			setFrame(frame, true);

			if (trackTarget > 0)
			{
				m_selectedItems.insert(trackTarget);
				m_lastSelected = trackTarget;
				m_pDlgFaceGrid->execMerge();
			}
			emit sgSelectionChanged();
		}
		else
		{
			// 失敗したのでマニュアル検出モードに切り替え
			ui.imageView->showIcon();

			setFrame(frame, true);

			setManualTarget(object, frame);
			m_mode = Constants::ModeManualDetect;
		}
	}
}

bool QtHOG::predictTrackingRectWithSVM(QPointF predictPos, int frame, int object, int searchRange, CvRect &newRect)
{	
	if (!m_svm)
		return false;

	// 最寄のソース画像を取得
	StructData::iterator<SRegion> itr, itSource;
	CvRect sourceRect;
	int sourceFrame;
	int frameDist = -1;
	const double CorrThresh = 0.6;//0.7;

	itSource = m_pStructData->regionEnd();
	for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (std::find(m_TargetType.begin(), m_TargetType.end(), itr->type) == m_TargetType.end())
			continue;

		int dist = abs(itr->frame - frame);
		if (frameDist < 0 || dist < frameDist)
		{
			frameDist = dist;
			itSource = itr;
		}
	}
	if (itSource == m_pStructData->regionEnd())
		return false;

	//kosshy wrote
	// この先のソースを読んでみたら、
	// イメージ（ターゲットになるもの）のサイズより
	// テンプ（探索もと）のサイズが大きいケースがままあったので
	// そこを修正する

	//根拠分からないけど割る５らしい
	int widthMargin = itSource->rect.width * 0.2f;// 5;
	int heightMargin = itSource->rect.height * 0.2f;// 5;

	/*
	sourceRect.x = itSource->rect.x + widthMargin;
	sourceRect.width = itSource->rect.width - 2 * widthMargin;
	sourceRect.y = itSource->rect.y + heightMargin;
	sourceRect.height = itSource->rect.height - 2 * heightMargin;
	*/
	// 探索元だったので縮める処理
	sourceRect.x = itSource->rect.x + widthMargin;
	sourceRect.y = itSource->rect.y + heightMargin;
	sourceRect.width = itSource->rect.width - (widthMargin < 1);
	sourceRect.height = itSource->rect.height - (widthMargin < 1);
	sourceFrame = itSource->frame;

	m_pStructData->mutex.lock();
	cv::Mat frameImage = m_pAviManager->getImage(sourceFrame);
	m_pStructData->mutex.unlock();
	int width = frameImage.cols;
	int height = frameImage.rows;

	if( sourceRect.x < 0 ) sourceRect.x = 0;
	if( sourceRect.y < 0 ) sourceRect.y = 0;
	int chkwidth = sourceRect.x + sourceRect.width;
	int chkHeight = sourceRect.y + sourceRect.height;
	if( chkwidth >= width ) sourceRect.width -= (chkwidth - width + 1);
	if( chkHeight >= height ) sourceRect.height -= (chkHeight - height + 1);

	cv::Mat sourceGrayImage(height, width, CV_8UC1), targetGrayImage(height, width, CV_8UC1);
	cv::cvtColor(frameImage, sourceGrayImage, CV_BGR2GRAY);

	m_pStructData->mutex.lock();
	cv::Mat targetFrameImage = m_pAviManager->getImage(frame);
	m_pStructData->mutex.unlock();
	cv::cvtColor(targetFrameImage, targetGrayImage, CV_BGR2GRAY);

	int minX = predictPos.x() - searchRange;
	int minY = predictPos.y() - searchRange;
	int maxX = predictPos.x() + searchRange;
	int maxY = predictPos.y() + searchRange;

	if (minX < 0)
		minX = 0;
	if (minY < 0)
		minY = 0;
	if (maxX >= width)
		maxX = width - 1;
	if (maxY >= height)
		maxY = height - 1;

	cv::Rect searchRoi(minX, minY, maxX - minX, maxY - minY);
	if( searchRoi.width < sourceRect.width )
	{
		searchRoi.width = sourceRect.width;
	}
	if( searchRoi.height < sourceRect.height )
	{
		searchRoi.height = sourceRect.height;
	}

	cv::Mat searchWindow = targetGrayImage(searchRoi);
	cv::Mat searchClone = searchWindow.clone();// kosshy wrote:このクローン意味ある？

	cv::Rect patchRoi(sourceRect);
	cv::Mat patch = sourceGrayImage(patchRoi);

	/////ここでひっかかるならフォーマットか何かの指定間違えてる
	if( *(searchClone.size.p) < *(patch.size.p) )
	{
		qDebug() << "ileagle : predictTrackingRectWithSVM";
		return false;
	}
	cv::Mat searchResult;
	cv::matchTemplate(searchClone, patch, searchResult, CV_TM_CCOEFF_NORMED);

	double maxCorr;
	cv::Point maxPos;
	cv::minMaxLoc(searchResult, 0, &maxCorr, 0, &maxPos);

	if (maxCorr < CorrThresh)
		return false;

	int xx = searchRoi.x + maxPos.x;
	int yy = searchRoi.y + maxPos.y;

	newRect = sourceRect;
	newRect.x = searchRoi.x + maxPos.x;
	newRect.y = searchRoi.y + maxPos.y;


	if (std::find(m_TargetType.begin(), m_TargetType.end(), Constants::TypeFace) != m_TargetType.end())
	{
		int portraitSize = itSource->rect.width * 2;//最初からfloatにすればいいのに
		if( portraitSize == 0 ) portraitSize = 1;
		float scale = (float)HogUtils::RESIZE_X * 2 / (float)portraitSize;

		cv::Mat searchResize;
		cv::resize(searchClone, searchResize, cv::Size(searchClone.size().width * scale, searchClone.size().height * scale));

		cv::HOGDescriptor hog(cv::Size(96, 96), cv::Size(16, 16), cv::Size(8, 8), cv::Size(8, 8), 9, -1, 0.2, false, 1);
		cv::Mat img;

		while (true)
		{
			cv::minMaxLoc(searchResult, 0, &maxCorr, 0, &maxPos);

			if (maxCorr < CorrThresh)
				return false;

			cv::Rect foundRect((maxPos.x - itSource->rect.width * 0.5 - widthMargin) * scale, (maxPos.y - itSource->rect.height * 0.5 - heightMargin) * scale,
								HogUtils::RESIZE_X * 2, HogUtils::RESIZE_Y * 2);

			if (foundRect.x < 0)
			{
				searchResult.at<float>(maxPos.y, maxPos.x) = 0;
				continue;
			}
			if (foundRect.y < 0)
			{
				searchResult.at<float>(maxPos.y, maxPos.x) = 0;
				continue;
			}
			if (foundRect.x >= searchResize.size().width - foundRect.width)
			{
				searchResult.at<float>(maxPos.y, maxPos.x) = 0;
				continue;
			}
			if (foundRect.y >= searchResize.size().height - foundRect.height)
			{
				searchResult.at<float>(maxPos.y, maxPos.x) = 0;
				continue;
			}

			img = searchResize(foundRect);

			std::vector<float> d;
			hog.compute(img, d, cv::Size(8, 8), cv::Size(0, 0));
			
			cv::Mat descriptors(1, Constants::HogDim, CV_32FC1, &d[0]);

			float score = m_svm->predict(descriptors, true);
			if (score <= m_FaceThreshold)
			{
				newRect = sourceRect;
				newRect.x = searchRoi.x + maxPos.x;
				newRect.y = searchRoi.y + maxPos.y;
				break;
			}

			searchResult.at<float>(maxPos.y, maxPos.x) = 0;
		}

	}

	newRect.x -= widthMargin;
	newRect.y -= heightMargin;

	if (newRect.x < 0)
		newRect.x = 0;
	if (newRect.y < 0)
		newRect.y = 0;
	if (newRect.x >= width - newRect.width)
		newRect.x = width - newRect.width - 1;
	if (newRect.y >= height - newRect.height)
		newRect.y = height - newRect.height - 1;

	return true;//!bColorReject;
}

void QtHOG::on_actionRead_Project_File_triggered()
{
	return;

	m_pStructData->clearAll();

	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read project file"), QDir::currentPath(),
													tr("project file (*.fcp);;All files (*.*)"), &selFilter,
													QFileDialog::ReadOnly);

	if (fileName.isEmpty())
		return;

	QFile file;
	file.setFileName(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		if (line.section('.', -1) == "avi")
		{
			readAvi(line);
		}
		else if (line.section('.', -1) == "rgn")
		{
			m_pStructData->readRegions(line.toLocal8Bit(), m_pAviManager->getWidth(), m_pAviManager->getHeight());
		}
		else if (line.section('.', -1) == "icv")
		{
			//m_pStructData->importICV(line.toLocal8Bit());
		}
	}
	file.close();
}

void QtHOG::onAutoTrackingRequired(int object, int mode)
{

	// とりあえずセーブ
	onIdle(true);
	if (!m_svm)
	{
		if (!readSVMData())
			return;
	}

	StructData::iterator<SRegion> itr;
	StructData::reverse_iterator<SRegion> itrr;
	CvRect predictRect;

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	int searchRange;

	itr = m_pStructData->regionBegin(object);
	itrr = m_pStructData->regionRBegin(object);

	int firstFrame = itr->frame;
	int lastFrame = itrr->frame;

	std::set<int> redundantSet;

	// 欠損フレームでのトラッキング
	if (mode & DlgFaceGrid::ModeMiddle)
	{
		for (int frame = firstFrame; frame <= lastFrame; ++frame)
		{
			itr = m_pStructData->regionFitAt(m_curCamera, frame, object);
			if (itr != m_pStructData->regionEnd())
				continue;

			if (getPredictRect(object, frame, width, height, predictRect))
				searchRange = m_PredictSearchRange;//50;
			else
				searchRange = m_InitialSearchRange;//150;

			QPointF predictPos(predictRect.x + predictRect.width / 2, predictRect.y + predictRect.height / 2);

			// 自動トラッキング処理
			CvRect trackingRect;
			if (predictTrackingRectWithSVM(predictPos, frame, object, searchRange, trackingRect))
			{
				// 既存領域との重複をチェック
				std::vector<int> faces;
				int lastSelected;

				cv::Point point(trackingRect.x + trackingRect.width / 2, trackingRect.y + trackingRect.height / 2);
				doSelectFaces(frame, point, point, faces, lastSelected);
				StructData::iterator<SRegion> itTarget = m_pStructData->regionEnd();
				for (std::vector<int>::iterator itf = faces.begin(); itf != faces.end(); ++itf)
				{
					itr = m_pStructData->regionFitAt(*itf);
					if (rectsOverlap(itr->rect, trackingRect, 0.6f))
					{
						itTarget = itr;
						break;
					}
				}

				// 新規登録
				itr = m_pStructData->insertRegion(m_curCamera, frame, object, trackingRect, itTarget->type);

				m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);

				if (itTarget != m_pStructData->regionEnd())
				{
					redundantSet.insert(itTarget.getID());
					redundantSet.insert(itr.getID());
				}

				setFrame(frame);
				CvRect &r = itr->rect;
				float scale = ui.imageView->adjustScale(r.width * 8, r.height * 8);
				ui.labelScale->setText(QString::number((int)(scale * 100)));
				ui.sliderScale->setValue((int)(scale * 100));
				
				ui.sliderScale->setEnabled(true);
				ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
				ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);
				qApp->processEvents();
			}
			else
			{
				setFrame(frame);
				//CvRect &r = itr->rect;
				float scale = ui.imageView->adjustScale(predictRect.width * 8, predictRect.height * 8);
				ui.labelScale->setText(QString::number((int)(scale * 100)));
				ui.sliderScale->setValue((int)(scale * 100));
				
				ui.sliderScale->setEnabled(true);
				ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
				ui.imageView->centerOn(predictPos);
				qApp->processEvents();
			}

			if (m_pDlgFaceGrid->isEscape())
				break;
		}
	}

	// 前方トラッキング
	int lostCount;
	if (!m_pDlgFaceGrid->isEscape() && (mode & DlgFaceGrid::ModeBefore))
	{
		lostCount = 0;
		for (int frame = firstFrame - 1; frame >= 0; --frame)
		{
			itr = m_pStructData->regionFitAt(m_curCamera, frame, object);
			if (itr != m_pStructData->regionEnd())
				continue;

			if (getPredictRect(object, frame, width, height, predictRect))
				searchRange = m_PredictSearchRange;
			else
				searchRange = m_InitialSearchRange;

			QPointF predictPos(predictRect.x + predictRect.width / 2, predictRect.y + predictRect.height / 2);

			// 自動トラッキング処理
			CvRect trackingRect;
			if (predictTrackingRectWithSVM(predictPos, frame, object, searchRange, trackingRect))
			{
				// 既存領域との重複をチェック
				std::vector<int> faces;
				int lastSelected;

				cv::Point point(trackingRect.x + trackingRect.width / 2, trackingRect.y + trackingRect.height / 2);
				doSelectFaces(frame, point, point, faces, lastSelected);
				StructData::iterator<SRegion> itTarget = m_pStructData->regionEnd();
				for (std::vector<int>::iterator itf = faces.begin(); itf != faces.end(); ++itf)
				{
					itr = m_pStructData->regionFitAt(*itf);
					if (rectsOverlap(itr->rect, trackingRect, 0.6f))
					{
						itTarget = itr;
						break;
					}
				}

				// 新規登録
				itr = m_pStructData->insertRegion(m_curCamera, frame, object, trackingRect, itTarget->type);

				m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);

				if (itTarget != m_pStructData->regionEnd())
				{
					redundantSet.insert(itTarget.getID());
					redundantSet.insert(itr.getID());
				}
				lostCount = 0;
				
				setFrame(frame);
				CvRect &r = itr->rect;
				float scale = ui.imageView->adjustScale(r.width * 8, r.height * 8);
				ui.labelScale->setText(QString::number((int)(scale * 100)));
				ui.sliderScale->setValue((int)(scale * 100));
				
				ui.sliderScale->setEnabled(true);
				ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
				ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);
				qApp->processEvents();
			}
			else
			{
				setFrame(frame);
				//CvRect &r = itr->rect;
				float scale = ui.imageView->adjustScale(predictRect.width * 8, predictRect.height * 8);
				ui.labelScale->setText(QString::number((int)(scale * 100)));
				ui.sliderScale->setValue((int)(scale * 100));
				
				ui.sliderScale->setEnabled(true);
				ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
				ui.imageView->centerOn(predictPos);
				qApp->processEvents();
			}
			if (++lostCount > 3)
				break;

			if (m_pDlgFaceGrid->isEscape())
				break;
		}
	}

	// 後方トラッキング
	if (!m_pDlgFaceGrid->isEscape() && (mode & DlgFaceGrid::ModeAfter))
	{
		lostCount = 0;
		for (int frame = lastFrame + 1; frame < m_pAviManager->getFrameCount(); ++frame)
		{
			itr = m_pStructData->regionFitAt(m_curCamera, frame, object);
			if (itr != m_pStructData->regionEnd())
				continue;

			if (getPredictRect(object, frame, width, height, predictRect))
				searchRange = m_PredictSearchRange;
			else
				searchRange = m_InitialSearchRange;

			QPointF predictPos(predictRect.x + predictRect.width / 2, predictRect.y + predictRect.height / 2);

			// 自動トラッキング処理
			CvRect trackingRect;
			if (predictTrackingRectWithSVM(predictPos, frame, object, searchRange, trackingRect))
			{
				// 既存領域との重複をチェック
				std::vector<int> faces;
				int lastSelected;

				cv::Point point(trackingRect.x + trackingRect.width / 2, trackingRect.y + trackingRect.height / 2);
				doSelectFaces(frame, point, point, faces, lastSelected);
				StructData::iterator<SRegion> itTarget = m_pStructData->regionEnd();
				for (std::vector<int>::iterator itf = faces.begin(); itf != faces.end(); ++itf)
				{
					itr = m_pStructData->regionFitAt(*itf);
					if (rectsOverlap(itr->rect, trackingRect, 0.6f))
					{
						itTarget = itr;
						break;
					}
				}

				// 新規登録
				itr = m_pStructData->insertRegion(m_curCamera, frame, object, trackingRect, itTarget->type);

				m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);

				if (itTarget != m_pStructData->regionEnd())
				{
					redundantSet.insert(itTarget.getID());
					redundantSet.insert(itr.getID());
				}
				lostCount = 0;

				setFrame(frame);
				CvRect &r = itr->rect;
				float scale = ui.imageView->adjustScale(r.width * 8, r.height * 8);
				ui.labelScale->setText(QString::number((int)(scale * 100)));
				ui.sliderScale->setValue((int)(scale * 100));
				
				ui.sliderScale->setEnabled(true);
				ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
				ui.imageView->centerOn(r.x + r.width / 2, r.y + r.height / 2);
				qApp->processEvents();
			}
			else
			{
				setFrame(frame);
				//CvRect &r = itr->rect;
				float scale = ui.imageView->adjustScale(predictRect.width * 8, predictRect.height * 8);
				ui.labelScale->setText(QString::number((int)(scale * 100)));
				ui.sliderScale->setValue((int)(scale * 100));
				
				ui.sliderScale->setEnabled(true);
				ui.checkBoxAutoScale->setCheckState(Qt::Unchecked);
				ui.imageView->centerOn(predictPos);
				qApp->processEvents();
			}
			if (++lostCount > 3)
				break;

			if (m_pDlgFaceGrid->isEscape())
				break;
		}
	}

	if (!redundantSet.empty())
	{
		// マージダイアログを表示
		m_selectedItems.clear();
		std::set<int>::iterator its;
		for (its = redundantSet.begin(); its != redundantSet.end(); ++its)
		{
			m_selectedItems.insert(*its);
			m_lastSelected = *its;
		}
		emit sgSelectionChanged();

		//m_pDlgFaces->execMerge();
		m_pDlgFaceGrid->execMerge();
	}
	else
		emit sgSelectionChanged();

	m_lockonTarget = -1;
}

void QtHOG::on_actionExtract_MSER_triggered()
{
	cv::MSER mser;
	int w = m_pAviManager->getWidth();
	int h = m_pAviManager->getHeight();
	m_pStructData->mutex.lock();
	cv::Mat check = m_pAviManager->getImage(ui.sliderFrame->value());
	m_pStructData->mutex.unlock();

	cv::Mat gray(h, w, CV_8UC1), mask(h, w, CV_8UC1, 0);
	cv::cvtColor(check, gray, CV_BGR2GRAY);

	cv::Scalar mean, stddev;
	meanStdDev(gray, mean, stddev);
	//qDebug() << mean[0] << mean[1] << mean[2] << mean[3] << stddev[0] << stddev[1] << stddev[2];

	for (int row = 0; row < gray.rows; ++row)
	{
		for (int col = 0; col < gray.cols * 0.7; ++col)
		{
			int i = 255 * tanh((gray.at<uchar>(row, col) - mean[0]) * 0.8 / stddev[0]) + 127;
			gray.at<uchar>(row, col) = i > 255 ? 255 : (i < 0 ? 0 : i);
		}
	}

	cv::namedWindow("gray", CV_WINDOW_AUTOSIZE);
	cv::imshow("gray", gray);
	//cvWaitKey(0);

	cv::Mat tmp_img(check.size(), IPL_DEPTH_16S, 1);
	cv::Mat dst_img1(check.size(), IPL_DEPTH_8U, 1);

	// (2)Sobelフィルタによる微分画像の作成
	cv::Sobel (gray, tmp_img, tmp_img.depth(), 1, 0);
	cv::convertScaleAbs (tmp_img, dst_img1);

	cv::namedWindow("sobel");
	cv::imshow("sobel", dst_img1);

/*
	qDebug() << "pre mser";

	std::vector< std::vector<cv::Point> > msers;
	mser( gray, msers, mask);

	qDebug() << "post mser";

	for( int i = 0; i < msers.size(); i++){
		CvScalar rgb = CV_RGB(rand()%256, rand()%256, rand()%256);

		for( int j = 0; j < msers.at(i).size(); j++ ){
			//cv::circle(check, msers.at(i).at(j), 1, rgb, 1, 8, 0 );
			cv::line(check, msers.at(i).at(j), msers.at(i).at(j), rgb);
		}
	}

	cv::namedWindow("MSERS", CV_WINDOW_AUTOSIZE);
	cv::imshow("MSERS", check);
*/
}

int * map_to_flatmap(float * map, unsigned int size)
{
  /********** Flatmap **********/
  int *flatmap      = (int *) malloc(size*sizeof(int)) ;
  for (unsigned int p = 0; p < size; p++)
  {
    flatmap[p] = map[p];
  }

  bool changed = true;
  while (changed)
  {
    changed = false;
    for (unsigned int p = 0; p < size; p++)
    {
      changed = changed || (flatmap[p] != flatmap[flatmap[p]]);
      flatmap[p] = flatmap[flatmap[p]];
    }
  }

  /* Consistency check */
  for (unsigned int p = 0; p < size; p++)
    assert(flatmap[p] == flatmap[flatmap[p]]);

  return flatmap;
}

//const float RgbScale = 32.0f;

void QtHOG::on_actionSet_Blur_Intensity_For_Faces_triggered()
{
	bool bOk;
	int intensity = QInputDialog::getInt(this, "Set Blur Intensity For Faces", "Intensity for Face(1 - 10):", m_blurIntensityForFaces, 1, 10, 1, &bOk);
    if (!bOk)
		return;

	m_blurIntensityForFaces = intensity;
}

void QtHOG::on_actionSet_Blur_Intensity_For_Plates_triggered()
{
	bool bOk;
	int intensity = QInputDialog::getInt(this, "Set Blur Intensity For Plates", "Intensity for Plates(1 - 10):", m_blurIntensityForPlates, 1, 10, 1, &bOk);
    if (!bOk)
		return;

	m_blurIntensityForPlates = intensity;
}

// about masquerade
void QtHOG::on_actionAbout_Masquerade_triggered()
{
#pragma comment(lib, "version.lib")
	// リソースからバージョン番号を読み取り、ダイアログに反映
	wchar_t szModuleName[_MAX_PATH + 1];
	::GetModuleFileName( NULL, szModuleName, _MAX_PATH );
	DWORD dwSize = 0;
	DWORD dwReserved;
	dwSize = ::GetFileVersionInfoSize( szModuleName, &dwReserved );
	wchar_t *pBuf = new wchar_t[dwSize+1];
	::GetFileVersionInfo( szModuleName, 0, dwSize, pBuf );
	
	//言語とコードページを取得
	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;
	unsigned int dwLen;
	wchar_t nameLangCode[_MAX_PATH];
	if( ::VerQueryValue( pBuf, L"\\VarFileInfo\\Translation", (LPVOID *)&lpTranslate, &dwLen ) )
	{
		for(int i = 0; i <(dwLen/sizeof(struct LANGANDCODEPAGE)); i++)
		{
			wsprintf(nameLangCode, L"\\StringFileInfo\\%04x%04x\\", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
		}
	}

	//ローカル内関数
	struct
	{
		// QStringに直接wcharがセットできないので
		void setWchar2Qstr(QString &target, wchar_t* set)
		{
			size_t origsize = wcslen(set) + 1;
			size_t convertedChars = 0;
			char nstring[_MAX_PATH];
			wcstombs_s(&convertedChars, nstring, origsize, set, _TRUNCATE);
			target.append( nstring );
		}
		// StringFileInfoから情報を抜き出してセットする
		void setProductInfo2QStr(QString &target, wchar_t *pBuf, wchar_t* nameLangCode, wchar_t* infoSelectString, const char* defaultString)
		{
			wchar_t name[_MAX_PATH];
			LPVOID lpBuffer;
			UINT uLen;
			wsprintf(name, L"%s%s", nameLangCode, infoSelectString);
			if( ::VerQueryValue(pBuf, name, &lpBuffer, &uLen) )
			{
				setWchar2Qstr(target, (wchar_t*)lpBuffer);
			}
			else
			{
				target.append( defaultString );
			}
		}
	} localFunction;

	//バージョン情報を取得
	QString versionText;
	localFunction.setProductInfo2QStr(versionText, pBuf, nameLangCode, L"ProductVersion", "PROTO TYPE");
	versionText.append( "\n" );
	//リーガルコピーライト情報を取得
	localFunction.setProductInfo2QStr(versionText, pBuf, nameLangCode, L"LegalCopyright", "Copyright (C) 2012-2020 Iwane Laboratories, Ltd.");

	QMessageBox::about(this, "About Masquerade", versionText );
	delete [] pBuf;
}

void QtHOG::on_actionSelect_All_triggered()
{
	StructData::iterator<SRegion> it;
	m_selectedItems.clear();
	for (it = m_pStructData->regionBegin(m_curCamera, ui.sliderFrame->value()); it != m_pStructData->regionEnd(); ++it)
	{
		if (std::find(m_TargetType.begin(), m_TargetType.end(), it->type) == m_TargetType.end())
			continue;

		m_selectedItems.insert(it.getID());
	}
	m_lockonTarget = -1;
	setFrame(ui.sliderFrame->value(), true);
	emit sgSelectionChanged();
}

struct DirCluster
{
	MathUtil::Vec2d dir;
	int n;

	DirCluster() {PTM::clear(dir); n = 0;}
};

void QtHOG::on_editFrame_returnPressed()
{
	QString text = ui.editFrame->text();
	bool bOk;
	int frame = text.toInt(&bOk, 10);
	if (!bOk)
	{
		ui.editFrame->setText(QString::number(ui.sliderFrame->value()));
		return;
	}
	if (frame >= 0 && frame < m_pAviManager->getFrameCount())
		setFrame(frame);
}

// HBITMAPを作成するために必要なBITMAPINFOに各値をセット
BOOL setBitmapInfo(LPBITMAPINFO bmInfo, int width, int height, int BytesPerPixel, int origin) {
	// エラーチェック
	if(bmInfo==NULL) return FALSE;
	if(width<0) return FALSE;
	if(height<0) return FALSE;
	if(BytesPerPixel!=8 && BytesPerPixel!=24 && BytesPerPixel!=32) return FALSE;

	// HBITMAPを作成するために必要なBITMAPINFOに各値をセット
	BITMAPINFOHEADER *bmInfoHeader = &bmInfo->bmiHeader;

	memset(bmInfoHeader, 0, sizeof(*bmInfoHeader));
	bmInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmInfoHeader->biWidth = width;

	bmInfoHeader->biHeight = abs(height);

	//if(origin!=0) bmInfoHeader->biHeight = abs(height);
	//else bmInfoHeader->biHeight = -abs(height);

	bmInfoHeader->biPlanes = 1;
	bmInfoHeader->biBitCount = (unsigned short)BytesPerPixel;
	bmInfoHeader->biCompression = BI_RGB;

	if(BytesPerPixel==8) {
		RGBQUAD *BitmapColors = bmInfo->bmiColors;

		for(int i=0; i<256; i++) {
			BitmapColors[i].rgbRed = BitmapColors[i].rgbGreen = BitmapColors[i].rgbBlue = (BYTE)i;
			BitmapColors[i].rgbReserved = 0;
		}
	}

	return TRUE;
}

void QtHOG::on_actionExport_Masked_Images_triggered()
{
	// ファイルを読み込んでいなければ何もしない
	QString inputFile = this->m_pAviManager->getCurFileName();
	if(QFile::exists(inputFile) == false)
	{
		return;
	}

	// AVI, ZIC, IZICに対応
	QString filter = "";
	{
		QString inputFileExt = QFileInfo(inputFile).suffix();

		// 出力するファイルタイプを入力ファイルに合わせる
		if(inputFileExt == "zic")
		{
			filter = "ZIC file (*.zic)";
		}
		else if(inputFileExt == "izic")
		{
			filter = "IZIC file (*.izic)";
		}
		else
		{
			return;
		}
	}

	QString defaultName = m_pAviManager->getCurFileName();
	QFileInfo defaultFileInfo(m_pAviManager->getCurFileName());
	QDir defaultDir(defaultFileInfo.absolutePath());
	defaultDir.cd("masked");
	defaultName = defaultDir.absoluteFilePath(defaultFileInfo.fileName());

	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("Export masked images"),
		defaultName,
		filter, 
		NULL
	);

	if (fileName.isEmpty())
		return;

	QFileInfo fileInfo(fileName);
	if (!fileInfo.dir().exists())
		fileInfo.dir().mkpath(fileInfo.absolutePath());
	QString filePath = fileInfo.path();
	QString fileBase = fileInfo.baseName();
	QString fileExt = fileInfo.suffix();

	int startFrame = 0;
	int endFrame = m_pAviManager->getFrameCount() - 1;

	m_bCancel = false;

	if (fileExt == "zic")
	{
		exportMaskedZIC(fileName, startFrame, endFrame);
	}
	else if (fileExt == "izic")
	{
		exportMaskedIZIC(fileName, startFrame, endFrame);
	}

	return;
}

bool QtHOG::exportMaskedZIC(const QString &fileName, int startFrame, int endFrame, int resizeWidth, int resizeHeight)
{
	if (resizeWidth >= 0 || resizeHeight >= 0){
		// リサイズは許可しないとする
		return ( false );
	}

	if(this->m_pAviManager == NULL)
	{
		return false;
	}

	bool bUseTempFile = false;
	QString outFile = fileName;
	if(outFile == this->m_pAviManager->getCurFileName())
	{
		outFile = fileName+".temporary";
		bUseTempFile = true;
	}

	// ファイル出力が可能なようにディレクトリを生成する
	FileUtility::makeDirIfNotExist(outFile);

	// マスク対象ファイルオープン
	std::wstring outFileWStr = std::wstring((const wchar_t *)outFile.utf16());
	std::wstring inFileWStr = std::wstring((const wchar_t *)this->m_pAviManager->getCurFileName().utf16());

	// マスク処理DLLへのアクセス
	CEnmaskImageAccessor enmaskImage(outFileWStr.c_str());

	if (!enmaskImage.maskOpen(outFileWStr.c_str(), this->m_pAviManager->getCurrentContext()))
		return false;

	enmaskImage.maskSetBlurSize(m_blurIntensityForFaces);	// とりあえずFace用のサイズを使用する。マニュアルが増えたので１つに統合すべき 
	enmaskImage.maskSetFov(m_pAviManager->getFov());
	enmaskImage.maskSetJapegParam(m_JpegQuality);

	for(int cameraNo = 0; cameraNo < m_pAviManager->getCameraCount(); cameraNo++ )
	{
		m_pAviManager->changeCamera(cameraNo);
		for (int frame = startFrame; frame <= endFrame; frame++)
		{
			if (m_bCancel)
				break;
			m_executeStatus.setText(QString("Exporting (%1/%2)...").arg(frame).arg(endFrame));
			qApp->processEvents();
			// リージョンデータ作成 
			std::vector<RECT> rectList;
			StructData::iterator<SRegion> itr;
			for (itr = m_pStructData->regionBegin(cameraNo, frame); itr != m_pStructData->regionEnd(); ++itr)
			{
				RECT rect;
				rect.left = itr->rect.x;
				rect.top = itr->rect.y;
				rect.right = itr->rect.x+itr->rect.width;
				rect.bottom = itr->rect.y+itr->rect.height;
				rectList.push_back(rect);
			}
			// マスク処理
			if (rectList.empty())
				enmaskImage.maskEnmaskImage(frame, cameraNo, NULL, 0, m_pAviManager->getWidth(), m_pAviManager->getHeight());
			else
				enmaskImage.maskEnmaskImage(frame, cameraNo, &rectList[0], rectList.size(), m_pAviManager->getWidth(), m_pAviManager->getHeight());
		}
	}
	enmaskImage.maskClose();

	m_executeStatus.clear();
	qApp->processEvents();

	if (!m_bCancel && !bUseTempFile)
	{
		QFileInfo orgFileInfo(this->m_pAviManager->getCurFileName());
		QFileInfo outFileInfo(outFile);
		QString orgIcv = orgFileInfo.path() + "/" + orgFileInfo.baseName() + ".icv";
		QString outIcv = outFileInfo.path() + "/" + outFileInfo.baseName() + ".icv";
		QFile::copy(orgIcv, outIcv);
	}

	return !m_bCancel;
}

std::vector<cv::Rect_<double>> QtHOG::getEquirectangularRectList(cv::Rect_<double> rectOrg, double width, double height)
{
	std::vector<cv::Rect_<double>> result;
	std::vector<cv::Rect_<double>> temp;
	cv::Rect_<double> rectTmp = rectOrg;
	if (rectTmp.x < 0)
	{
		temp.push_back(cv::Rect_<double>(rectTmp.x + width, rectTmp.y, -rectTmp.x, rectTmp.height));
		rectTmp.x = 0;
		rectTmp.width += rectTmp.x;
	}
	if (rectTmp.x+rectTmp.width >= width)
	{
		if (rectTmp.x+rectTmp.width > width)
		{
			temp.push_back(cv::Rect_<double>(0, rectTmp.y, rectTmp.x+rectTmp.width - width, rectTmp.height));
			rectTmp.width = width - rectTmp.x;
		}
	}
	temp.push_back(rectTmp);
	for (std::vector<cv::Rect_<double>>::iterator itTemp = temp.begin();
		itTemp != temp.end();
		itTemp++)
	{
		if ((itTemp->y < 0) || (itTemp->y+itTemp->height >= height))
		{
			std::vector<int> x_candidate;
			x_candidate.push_back(itTemp->x);
			x_candidate.push_back(itTemp->x+itTemp->width);
			if (itTemp->x < width/2)
				x_candidate.push_back(itTemp->x+width/2);
			else
				x_candidate.push_back(itTemp->x-width/2);
			if (itTemp->x+itTemp->width < width/2)
				x_candidate.push_back(itTemp->x+width/2);
			else
				x_candidate.push_back(itTemp->x-width/2);
			if (itTemp->y < 0)
				result.push_back(cv::Rect_<double>(*std::min_element(x_candidate.begin(), x_candidate.end()), 0, 
					*std::max_element(x_candidate.begin(), x_candidate.end()) - *std::min_element(x_candidate.begin(), x_candidate.end()), itTemp->y + itTemp->height));
			else
				result.push_back(cv::Rect_<double>(*std::min_element(x_candidate.begin(), x_candidate.end()), itTemp->y, 
					*std::max_element(x_candidate.begin(), x_candidate.end()) - *std::min_element(x_candidate.begin(), x_candidate.end()), height - itTemp->y));
		}
		else
			result.push_back(*itTemp);
	}
	return result;
}

bool QtHOG::exportMaskedIZIC(const QString &fileName, int startFrame, int endFrame, int resizeWidth, int resizeHeight)
{
	if (resizeWidth >= 0 || resizeHeight >= 0)
	{
		// リサイズは許可しないとする
		return false;
	}
	
	if(this->m_pAviManager == NULL)
	{
		return false;
	}
	
	//// 同一ファイルへの書き込みは行わない(zicの処理に合わせた)
	//if(this->m_pAviManager->getCurFileName() == fileName)
	//{
	//	return false;
	//}

	// 書き込み先のファイル作成
	QString curFile = this->m_pAviManager->getCurFileName();
	QString outFile = fileName;
	std::wstring curFileWStr = std::wstring((const wchar_t *)curFile.utf16());
	std::wstring outFileWStr = std::wstring((const wchar_t *)outFile.utf16());
	if(curFile != outFile)
	{
		// ファイル出力が可能なようにディレクトリを生成する
		FileUtility::makeDirIfNotExist(fileName);

		QFileInfo fileInfo(fileName);
		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();
		
		// コピーに時間がかかるため
		m_executeStatus.setText(QString("Exporting ..."));
		qApp->processEvents();

		if(CopyFile(curFileWStr.c_str(), outFileWStr.c_str(), FALSE) == FALSE)
		{
			return false;
		}
	}

	// マスク処理DLLへのアクセス
	CEnmaskImageAccessor enmaskImage(outFileWStr.c_str());

	// マスク対象ファイルオープン
	if (!enmaskImage.maskOpen(outFileWStr.c_str(), NULL))
		return false;

	enmaskImage.maskSetBlurSize(m_blurIntensityForFaces);	// とりあえずFace用のサイズを使用する。マニュアルが増えたので１つに統合すべき 
	enmaskImage.maskSetFov(m_pAviManager->getFov());
	enmaskImage.maskSetJapegParam(m_JpegQuality);

	for(int cameraNo = 0; cameraNo < m_pAviManager->getCameraCount(); cameraNo++ )
	{
		m_pAviManager->changeCamera(cameraNo);
		for (int frame = startFrame; frame <= endFrame; frame++)
		{
			if (m_bCancel)
				break;
			m_executeStatus.setText(QString("Exporting (%1/%2)...").arg(frame).arg(endFrame));
			qApp->processEvents();
			// リージョンデータ作成 
			std::vector<RECT> rectList;
			StructData::iterator<SRegion> itr;
			for (itr = m_pStructData->regionBegin(cameraNo, frame); itr != m_pStructData->regionEnd(); ++itr)
			{
				RECT rect;
				rect.left = itr->rect.x;
				rect.top = itr->rect.y;
				rect.right = itr->rect.x+itr->rect.width;
				rect.bottom = itr->rect.y+itr->rect.height;
				rectList.push_back(rect);
			}
			if (rectList.empty())
				continue;
			// マスク処理
			if (!enmaskImage.maskEnmaskImage(frame, cameraNo, &rectList[0], rectList.size(), m_pAviManager->getWidth(), m_pAviManager->getHeight()))
			{
				continue;
			}
		}
	}
	enmaskImage.maskClose();

	m_executeStatus.clear();
	qApp->processEvents();

	return true;
}



// 未使用
void QtHOG::on_actionSet_Color_Threshold_triggered()
{
}

void QtHOG::on_actionSVM_Learning_triggered()
{
	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	QDir dir(dirName[0]);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

	QFileInfoList list = dir.entryInfoList();

	std::vector<float> data;
	std::vector<int> res;

	// CPU版 //
	cv::HOGDescriptor hog(cv::Size(96, 96), cv::Size(16, 16), cv::Size(8, 8), cv::Size(8, 8), 9, -1, 0.2, false, 1);
	cv::Mat img(HogUtils::RESIZE_Y * 2, HogUtils::RESIZE_X * 2, CV_8UC1);
	
	int pcount(0), ncount(0), descSize(-1);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);

		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (!(fileExt == "jpg" || fileExt == "png")) continue;

		bool bPositive = (fileBase.indexOf("positive") >= 0);
		if (!bPositive && fileBase.indexOf("negative") < 0)
			continue;

		cv::Mat sample = cv::imread(fileInfo.filePath().toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
		cv::Mat resizeImage;//, colorResizeImage;
		cv::resize(sample, resizeImage, cv::Size(HogUtils::RESIZE_Y * 2, HogUtils::RESIZE_X * 2));
		
		std::vector<float> d;
		hog.compute(resizeImage, d, cv::Size(8, 8), cv::Size(0, 0));
		cv::Mat descriptors(1, Constants::HogDim, CV_32FC1, &d[0]);

		if (descSize < 0)
			descSize = descriptors.cols * descriptors.rows;

		int curIndex = data.size();
		data.resize(data.size() + descSize);

		for (int i = 0; i < descriptors.rows; ++i)
			for (int j = 0; j < descriptors.cols; ++j)
				data[curIndex++] = descriptors.at<float>(i, j); 

		if (bPositive)
		{
			res.push_back(1);
			pcount++;
		}
		else
		{
			res.push_back(0);
			ncount++;
		}
	}


	CvMat data_mat, res_mat;
	CvTermCriteria criteria;
	CvSVM svm = CvSVM ();
	CvSVMParams param;

	// (3)SVM学習データとパラメータの初期化
	cvInitMatHeader (&data_mat, pcount + ncount, descSize, CV_32FC1, &data[0]);
	cvInitMatHeader (&res_mat, pcount + ncount, 1, CV_32SC1, &res[0]);
	criteria = cvTermCriteria (CV_TERMCRIT_EPS, 100000, FLT_EPSILON);
	param = CvSVMParams (CvSVM::C_SVC/*CvSVM::ONE_CLASS*/, CvSVM::LINEAR, 10.0, 0.09, 1.0, 10.0, 0.5, 1.0, NULL, criteria);
	
	svm.train (&data_mat, &res_mat, NULL, NULL, param);

	// SVMデータ書き出し
	QString selFilter = "";
	QString fileName = QFileDialog::getSaveFileName(this,
													tr("Write SVM data"), QDir::currentPath(),
													tr("SVM data (*.xml)"), &selFilter
													);
	if (fileName.isEmpty())
		return;

	svm.save(fileName.toLocal8Bit());
}

bool QtHOG::readPCAdata()
{
	if (m_pPCA)
	{
		delete m_pPCA;
		m_pPCA = 0;
	}

	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read PCA data"), QDir::currentPath(),
													tr("PCA data(*.yml);;All files (*.*)"), &selFilter);

	if (fileName.isEmpty())
		return false;

	m_pPCA = new cv::PCA();
	cv::FileStorage fs(fileName.toStdString(), cv::FileStorage::READ);
	
	cv::Mat eval, evec, mean;
	fs["eigenvalues"] >> eval;
	fs["eigenvectors"] >> evec;
	fs["mean"] >> mean;
	/*
	fs["eigen value"] >> m_pPCA->eigenvalues;
	fs["eigen vector"] >> m_pPCA->eigenvectors;
	fs["mean"] >> m_pPCA->mean;
	*/
	m_pPCA->eigenvalues = eval;
	m_pPCA->eigenvectors = evec;
	m_pPCA->mean = mean;

	return true;
}

void QtHOG::on_actionHead_Shoulder_SVM_Learning_triggered()
{
	return;

	if (!m_pPCA)
	{
		if (!readPCAdata())
			return;
	}

	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	QDir dir(dirName[0]);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

	QFileInfoList list = dir.entryInfoList();

	std::vector<float> data;
	std::vector<int> res;


	int pcount(0), ncount(0), descSize(-1);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);

		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (fileExt != "png")
			continue;

		bool bPositive = (fileBase.indexOf("positive") >= 0);
		if (!bPositive && fileBase.indexOf("negative") < 0)
			continue;

		cv::Mat sample = cv::imread(fileInfo.filePath().toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
	
		std::vector<float> feat, featLBP;
		Detector::getHoG(sample, feat);
		Detector::getLBP(sample, featLBP);

		for (int i = 0; i < featLBP.size(); ++i)
			feat.push_back(featLBP[i]);

		// PCA による次元圧縮
		cv::Mat src(1, feat.size(), CV_32FC1, &(feat[0]));
		cv::Mat result = m_pPCA->project(src);

		if (descSize < 0)
			//descSize = feat.size();
			descSize = result.cols;

		int curIndex = data.size();
		data.resize(data.size() + descSize);

		/*
		for (int i = 0; i < descriptors.rows; ++i)
			for (int j = 0; j < descriptors.cols; ++j)
				data[curIndex++] = descriptors.at<float>(i, j); 
		*/
		for (int i = 0; i < descSize; ++i)
			data[curIndex++] =result.at<float>(0, i);// feat[i];

		if (bPositive)
		{
			res.push_back(1);
			pcount++;
		}
		else
		{
			res.push_back(0);
			ncount++;
		}
	}
	CvMat data_mat, res_mat;
	CvTermCriteria criteria;
	CvSVM svm = CvSVM ();
	CvSVMParams param;

	// (3)SVM学習データとパラメータの初期化
	cvInitMatHeader (&data_mat, pcount + ncount, descSize, CV_32FC1, &data[0]);
	cvInitMatHeader (&res_mat, pcount + ncount, 1, CV_32SC1, &res[0]);
	criteria = cvTermCriteria (CV_TERMCRIT_EPS, 100000, FLT_EPSILON);
	param = CvSVMParams (CvSVM::C_SVC/*CvSVM::ONE_CLASS*/, CvSVM::LINEAR, 10.0, 0.09, 1.0, 10.0, 0.5, 1.0, NULL, criteria);

	// (4)SVMの学習とデータの保存
	svm.train (&data_mat, &res_mat, NULL, NULL, param);

	// SVMデータ書き出し
	QString selFilter = "";
	QString fileName = QFileDialog::getSaveFileName(this,
													tr("Write SVM data"), QDir::currentPath(),
													tr("SVM data (*.xml)"), &selFilter
													);
	if (fileName.isEmpty())
		return;

	svm.save(fileName.toLocal8Bit());
}

void QtHOG::on_actionPCA_with_samples_triggered()
{
	return;

	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	QDir dir(dirName[0]);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

	QFileInfoList list = dir.entryInfoList();

	std::vector<float> featHOG, featLBP;

	int pcount(0), ncount(0), descSize(-1);
	int numData = 0;
	for (int i = 0; i < list.size(); ++i)
	{
		//qDebug() << "i" << i;

		QFileInfo fileInfo = list.at(i);

		//QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (fileExt != "png")
			continue;

		bool bPositive = (fileBase.indexOf("positive") >= 0);
		if (!bPositive && fileBase.indexOf("negative") < 0)
			continue;

		++numData;
	}

	//std::vector<float> data(numSample * (3024 + 4956));
	cv::Mat dataMat(numData, 1620 + 2655, CV_32FC1);
	std::vector<int> res;
	cv::Mat sample;

	int curIndex = 0;
	for (int i = 0; i < list.size(); ++i)
	{
		qDebug() << "i" << i;

		QFileInfo fileInfo = list.at(i);

		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (fileExt != "png")
			continue;

		bool bPositive = (fileBase.indexOf("positive") >= 0);
		if (!bPositive && fileBase.indexOf("negative") < 0)
			continue;

		sample = cv::imread(fileInfo.filePath().toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

		Detector::getHoG(sample, featHOG);
		Detector::getLBP(sample, featLBP);

		//for (int i = 0; i < featLBP.size(); ++i)
		//	feat.push_back(featLBP[i]);

		//std::vector<float> feat;
		//Detector::getHoG(sample, feat);

		if (descSize < 0)
			descSize = featHOG.size() + featLBP.size();

		//int curIndex = data.size();
		//data.resize(data.size() + descSize);

		for (int j = 0; j < featHOG.size(); ++j)
			dataMat.at<float>(curIndex, j) = featHOG[j];
		
		for (int j = 0; j < featLBP.size(); ++j)
			dataMat.at<float>(curIndex, j + featHOG.size()) = featLBP[j];

		curIndex++;
	}

	//int numData = data.size() / descSize;
	//cv::Mat dataMat(numData, descSize, CV_32FC1);
/*
	for (int i = 0; i < numData; ++i)
	{
		for (int j = 0; j < descSize; ++j)
			dataMat.at<float>(i, j) = data[i * descSize + j];
	}
*/
	cv::PCA pca(dataMat, cv::Mat(), CV_PCA_DATA_AS_ROW, 200);

	qDebug() << numData << descSize;
	qDebug() << pca.eigenvalues.rows << pca.eigenvalues.cols;
	qDebug() << pca.eigenvectors.rows << pca.eigenvectors.cols;
	qDebug() << pca.mean.rows << pca.mean.cols;

	// SVMデータ書き出し
	QString selFilter = "";
	QString fileName = QFileDialog::getSaveFileName(this,
													tr("Write PCA data"), QDir::currentPath(),
													tr("PCA data (*.yml)"), &selFilter
													);
	if (fileName.isEmpty())
		return;

//	svm.save(fileName.toLocal8Bit());

	cv::FileStorage fs(fileName.toStdString(), cv::FileStorage::WRITE);
	fs << "eigenvalues" << pca.eigenvalues;
	fs << "eigenvectors" << pca.eigenvectors;
	fs << "mean" << pca.mean;
}

//bool QtHOG::detectFaceInRegion7(CvSVM *svm, cv::Mat image, const CvRect &rect, cv::Point &facePos)
//{
//	if(image.empty()) { return false; }
//	if(rect.width >= rect.height) return false;	// 横長画像ではresizeRect.heightがマイナスになる。正方形なら０ 
//	int faceSize = rect.width / 4;
//	int portraitSize = faceSize * 2;
//
//	float scale = (float)HogUtils::RESIZE_X * 2 / (float)portraitSize;
//	CvRect resizeRect;
//	resizeRect.width = scale * rect.width;
//	resizeRect.height = scale * rect.height;
//
//	cv::Mat rectImage = image(cv::Rect(rect));
//	cv::Mat resizeImage;
//	cv::resize(rectImage, resizeImage, cv::Size(resizeRect.width, resizeRect.height));
//
//	cv::Mat grayRect;
//	cv::cvtColor(resizeImage, grayRect, CV_BGR2GRAY);
//
//	//float data[HogUtils::TOTAL_DIM];
//	CvMat m;
//	cvInitMatHeader (&m, 1, Constants::HogDim, CV_32FC1, NULL);
//
//	float minScore = -1;
//
//	int xmargin = resizeRect.width / 8;
//	int ymargin = 0;
//
//	//cv::Mat scoreMat(resizeRect.width, resizeRect.height, CV_32F, cv::Scalar(0)); 
//	cv::Mat scoreMat(resizeRect.height, resizeRect.width, CV_32F, cv::Scalar(0)); 
//
//	cv::gpu::HOGDescriptor hog(cv::Size(96, 96), cv::Size(16, 16), cv::Size(8, 8), cv::Size(8, 8),
//								9, cv::gpu::HOGDescriptor::DEFAULT_WIN_SIGMA, 0.2, false, 1);
//
//	cv::gpu::GpuMat gpu_img, search_img;	
//	cv::gpu::GpuMat gpuDescriptors;
//
//	search_img.upload(grayRect);
//
//	int x;
//	for (int y = ymargin; y < resizeRect.height * 0.4 - HogUtils::RESIZE_Y * 2; y += 6)
//	{
//		for (x = xmargin; x < resizeRect.width - HogUtils::RESIZE_X * 2 - xmargin; x += 6)
//		{
//			gpu_img = search_img(cv::Rect(x, y, HogUtils::RESIZE_X * 2, HogUtils::RESIZE_Y * 2));
//			cv::Mat descriptors;
//
//			hog.getDescriptors(gpu_img, cv::Size(8, 8), gpuDescriptors);
//			gpuDescriptors.download(descriptors);
//
//			if (svm->predict(descriptors))
//			{
//				for (int yy = y + HogUtils::RESIZE_Y * 0.5; yy < y + 1.5 * HogUtils::RESIZE_Y; ++yy)
//				{
//					for (int xx = x + HogUtils::RESIZE_X * 0.5; xx < x + 1.5 * HogUtils::RESIZE_X; ++xx)
//					{
//						scoreMat.at<float>(yy, xx) += HogUtils::RESIZE_Y * 0.5 - abs(yy - y - HogUtils::RESIZE_Y);
//						scoreMat.at<float>(yy, xx) += HogUtils::RESIZE_X * 0.5 - abs(xx - x - HogUtils::RESIZE_X);
//					}
//				}
//				//cv::Point p(x + HogUtils::RESIZE_X, y + HogUtils::RESIZE_Y);
//				//cv::line(checkImage, p, p, CV_RGB(255, 0, 0));
//			}
//		}
//	}
//
//	double maxVal;
//	cv::Point maxLoc;
//	cv::minMaxLoc(scoreMat, 0, &maxVal, 0, &maxLoc);
//
//	facePos.x = (float)maxLoc.x / scale;
//	facePos.y = (float)maxLoc.y / scale;
//
//	qDebug() << "score" << maxVal;
//
//	gpu_img.release();
//	search_img.release();
//	gpuDescriptors.release();
//	
//	return (maxVal > 250.0f);
//}
//
bool QtHOG::detectFaceInRegion7NoGPU(CvSVM *svm, cv::Mat image, const CvRect &rect, cv::Point &facePos)
{
	if(image.empty()) { return false; }
	if(rect.width >= rect.height) return false;	// 横長画像ではresizeRect.heightがマイナスになる。正方形なら０ 
	
	int faceSize = rect.width / 4;
	int portraitSize = faceSize * 2;

	float scale = (float)HogUtils::RESIZE_X * 2 / (float)portraitSize;
	CvRect resizeRect;
	resizeRect.width = scale * rect.width;
	resizeRect.height = scale * rect.height;

	cv::Mat rectImage = image(cv::Rect(rect));
	cv::Mat resizeImage;
	cv::resize(rectImage, resizeImage, cv::Size(resizeRect.width, resizeRect.height));

	cv::Mat grayRect;
	cv::cvtColor(resizeImage, grayRect, CV_BGR2GRAY);

	float minScore = -1;

	int xmargin = resizeRect.width / 8;
	int ymargin = 0;

	//cv::Mat scoreMat(resizeRect.width, resizeRect.height, CV_32F, cv::Scalar(0)); 
	cv::Mat scoreMat(resizeRect.height, resizeRect.width, CV_32F, cv::Scalar(0)); 

	cv::HOGDescriptor hog(cv::Size(96, 96), cv::Size(16, 16), cv::Size(8, 8), cv::Size(8, 8), 9, -1, 0.2, false, 1);
	//cv::gpu::HOGDescriptor hog(cv::Size(96, 96), cv::Size(16, 16), cv::Size(8, 8), cv::Size(8, 8), 9, cv::gpu::HOGDescriptor::DEFAULT_WIN_SIGMA, 0.2, false, 1);

	cv::Mat search_img = grayRect;
	cv::Mat cpuDescriptors;

	for (int y = ymargin; y < resizeRect.height * 0.4 - HogUtils::RESIZE_Y * 2; y += 6)
	{
		for (int x = xmargin; x < resizeRect.width - HogUtils::RESIZE_X * 2 - xmargin; x += 6)
		{
			cv::Mat img = search_img(cv::Rect(x, y, HogUtils::RESIZE_X * 2, HogUtils::RESIZE_Y * 2));
			std::vector<float> d;
			hog.compute(img, d, cv::Size(8, 8), cv::Size(0, 0));				
			cv::Mat descriptors(1, Constants::HogDim, CV_32FC1, &d[0]);

			float score = svm->predict(descriptors, true);
			if (score <= m_FaceThreshold)
			{
				for (int yy = y + HogUtils::RESIZE_Y * 0.5; yy < y + 1.5 * HogUtils::RESIZE_Y; ++yy)
				{
					for (int xx = x + HogUtils::RESIZE_X * 0.5; xx < x + 1.5 * HogUtils::RESIZE_X; ++xx)
					{
						{
							scoreMat.at<float>(yy, xx) += HogUtils::RESIZE_Y * 0.5 - abs(yy - y - HogUtils::RESIZE_Y);
							scoreMat.at<float>(yy, xx) += HogUtils::RESIZE_X * 0.5 - abs(xx - x - HogUtils::RESIZE_X);
						}
					}
				}
			}
		}
	}

	/*
	for (int y = ymargin; y < resizeRect.height * 0.4 - HogUtils::RESIZE_Y * 2; y += 6)
	{
		for (int x = xmargin; x < resizeRect.width - HogUtils::RESIZE_X * 2 - xmargin; x += 6)
		{
			img = search_img(cv::Rect(x, y, HogUtils::RESIZE_X * 2, HogUtils::RESIZE_Y * 2));
			std::vector<float> d;
			hog.compute(img, d, cv::Size(8, 8), cv::Size(0, 0));				
			cv::Mat descriptors(1, Constants::HogDim, CV_32FC1, &d[0]);

			float score = svm->predict(descriptors, true);
			if (score <= FaceThreshold)
			{	
				for (int yy = y + HogUtils::RESIZE_Y * 0.5; yy < y + 1.5 * HogUtils::RESIZE_Y; ++yy)
				{
					for (int xx = x + HogUtils::RESIZE_X * 0.5; xx < x + 1.5 * HogUtils::RESIZE_X; ++xx)
					{
						{
							scoreMat.at<float>(yy, xx) += HogUtils::RESIZE_Y * 0.5 - abs(yy - y - HogUtils::RESIZE_Y);
							scoreMat.at<float>(yy, xx) += HogUtils::RESIZE_X * 0.5 - abs(xx - x - HogUtils::RESIZE_X);
						}
					}
				}
			}
		}
	}
	*/

	double maxVal;
	cv::Point maxLoc;
	cv::minMaxLoc(scoreMat, 0, &maxVal, 0, &maxLoc);

	facePos.x = (float)maxLoc.x / scale;
	facePos.y = (float)maxLoc.y / scale;

	qDebug() << "score" << maxVal;

	return (maxVal > 250.0f);
}

void QtHOG::onScrollBack()
{
	int frame = ui.sliderFrame->value();
	if (frame <= 0)
		return;

	setFrame(frame - 1, false, true);
}

void QtHOG::onScrollForward()
{
	int frame = ui.sliderFrame->value();
	if (frame >= m_pAviManager->getFrameCount() - 1)
		return;

	setFrame(frame + 1, false, true);
}

void QtHOG::onManualDetect()
{
//	m_mode = Constants::ModeManualDetect;
	ui.pushButtonManualDetect->setIcon(QIcon(":/Icon/AddIcon"));
	ui.pushButtonManualDetect->setChecked(true);
	m_targetObject = -1;

	//QCursor cursor;
	//cursor.setShape(Qt::WhatsThisCursor);
	//setCursor(cursor);
	//if (m_pDlgFaceGrid->isVisible())
	//		m_pDlgFaceGrid->setWaitRegion();
}

void QtHOG::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Comma || event->key() == Qt::Key_A)
		onScrollBack();
	else if (event->key() == Qt::Key_Period || event->key() == Qt::Key_G)
		onScrollForward();
	else if (event->key() == Qt::Key_Slash || event->key() == Qt::Key_T)
		on_pushButtonManualDetect_clicked( m_mode != Constants::ModeManualDetect ? true : false );//onManualDetect();
	else if (event->key() == Qt::Key_Escape)
		m_bCancel = true;
	else if (event->key() == Qt::Key_R)
		onAutoTrackingRequiredByR();
	else if (event->key() == Qt::Key_E)
		onAutoBackwardTrackingRequiredByE();
	else if (event->key() == Qt::Key_F)
		onAutoMultiTrackingRequiredByF();
	else if (event->key() == Qt::Key_Space )
		onCameraChange();
}

void QtHOG::onAutoTrackingRequiredByR()
{
	onIdle(true);
	if (m_selectedItems.size() != 1)
			return;
	if (*m_selectedItems.begin() < 0)
		return;

	StructData::iterator<SRegion> itr, itNeighbor;
	itr = m_pStructData->regionFitAt(*m_selectedItems.begin());
	int curFrame = ui.sliderFrame->value();

	if (itr->frame != curFrame)
		return;

	itNeighbor = m_pStructData->regionFitAt(m_curCamera, curFrame + 1, itr->object);
	
	// yamazaki
	// 最終フレームではトラッキングをしない
	int maxFrameNum = m_pStructData->getFrameCount(this->m_curCamera);
	if (maxFrameNum == curFrame + 1) return;

	if (itNeighbor == m_pStructData->regionEnd())
	{
		// 自動トラッキングを試みる
		onItemDoubleClicked(itr->object, curFrame + 1, false);
	}
	else
	{
		//kosshy wrote : 元からあった処理だけどこのままじゃ落ちるので一部修正
		//kosshy wrote : ここの条件は前方向に見つかった状態でマニュアルトラッキング
		if( (curFrame - 1) < 0 )
			return;

		itNeighbor = m_pStructData->regionFitAt(m_curCamera, curFrame - 1, itr->object);
		if (itNeighbor == m_pStructData->regionEnd())
		{
			// 自動トラッキングを試みる
			onItemDoubleClicked(itr->object, curFrame - 1, false);
		}
	}
}

//バックワードマニュアルトラッキング要求（前身のほぼコピペなので共通化推奨）
void QtHOG::onAutoBackwardTrackingRequiredByE()
{
	onIdle(true);
	if (m_selectedItems.size() != 1)
			return;
	if (*m_selectedItems.begin() < 0)
		return;

	StructData::iterator<SRegion> itr, itNeighbor;
	itr = m_pStructData->regionFitAt(*m_selectedItems.begin());
	int curFrame = ui.sliderFrame->value();

	if (itr->frame != curFrame)
		return;

	if( (curFrame - 1) < 0 )
		return;

	itNeighbor = m_pStructData->regionFitAt(m_curCamera, curFrame - 1, itr->object);
	if (itNeighbor == m_pStructData->regionEnd())
	{
		// 自動トラッキングを試みる
		onItemDoubleClicked(itr->object, curFrame - 1, false);
	}
}

//そのフレームのすべての矩形に対してトラッキング実行
void QtHOG::onAutoMultiTrackingRequiredByF()
{
	if (!m_svm)
	{
		if (!readSVMData())
			return;
	}

	onIdle(true);
	if (m_selectedItems.size() != 1)
			return;
	if (*m_selectedItems.begin() < 0)
		return;

	StructData::iterator<SRegion> itr, itNeighbor;
	int curFrame = ui.sliderFrame->value();
	
	std::vector<int> rectvec;
	for(itr = m_pStructData->regionBegin(m_curCamera, curFrame); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->frame != curFrame) continue;
		rectvec.push_back( itr->object );
	}
 
	for(std::vector<int>::iterator iterVec = rectvec.begin();
		iterVec != rectvec.end();
		++iterVec)
	{
		//forward
		itNeighbor = m_pStructData->regionFitAt(m_curCamera, curFrame + 1, *iterVec);
		if(itNeighbor == m_pStructData->regionEnd())
		{
			partTracking(*iterVec, curFrame + 1);
		}
		//backward
		//itNeighbor = m_pStructData->regionFitAt(0, curFrame - 1, *iterVec);
		//if(itNeighbor == m_pStructData->regionEnd())
		//{
		//	partTracking(*iterVec, curFrame - 1);
		//}
		//マニュアルなどの後方検索カット中
	}
	
	m_selectedItems.clear();
	m_selectedItems.insert(m_lastSelected);
	
	setFrame(curFrame+1, true);
}

void QtHOG::partTracking(int object, int frame)
{
	StructData::iterator<SRegion> itr;
	CvRect predictRect;

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	int searchRange;
	if (getPredictRect(object, frame, width, height, predictRect))
		searchRange = m_PredictSearchRange;//20;//30;
	else
		searchRange = m_InitialSearchRange;//80;//60;

	QPointF predictPos(predictRect.x + predictRect.width / 2, predictRect.y + predictRect.height / 2);
	
	CvRect trackingRect;
	if (predictTrackingRectWithSVM(predictPos, frame, object, searchRange, trackingRect))
	{
		std::vector<int> faces;
		int lastSelected;

		cv::Point point(trackingRect.x + trackingRect.width / 2, trackingRect.y + trackingRect.height / 2);
		doSelectFaces(frame, point, point, faces, lastSelected);
		StructData::iterator<SRegion> itTarget = m_pStructData->regionEnd();
		for (std::vector<int>::iterator itf = faces.begin(); itf != faces.end(); ++itf)
		{
			itr = m_pStructData->regionFitAt(*itf);
			if (rectsOverlap(itr->rect, trackingRect, 0.7f))
			{
				itTarget = itr;
				break;
			}
		}

		itr = m_pStructData->insertRegion(m_curCamera, frame, object, trackingRect, itTarget->type);

		m_pStructData->setRegionFlags(itr, SRegion::FlagTracked);

		m_lastSelected = itr.getID();

		if (itTarget != m_pStructData->regionEnd())
		{
			//m_selectedItems.insert(trackTarget);
			m_lastSelected = itTarget.getID();
			m_pDlgFaceGrid->execMerge();
		}
		//emit sgSelectionChanged();
	}
	else
	{
		//トラッキングできなかった場所を表示したいならここに新機能が必要
	}
	updateSelectNum();
}

void QtHOG::on_actionClear_Objects_triggered()
{
	int frame = ui.sliderFrame->value();
	m_pStructData->clearRegions();
	setFrame(frame, true);
}

void QtHOG::on_actionShowBatch_Dialog_triggered()
{
	qDebug() << "QtHOG::on_actionShowBatch_Dialog_triggered";

	if (m_pDlgBatch->exec() != QDialog::Accepted)
		return;

	switch (m_detectMethod)
	{
	case Constants::DetectMethod::HOG:
		if (!m_svm)
		{
			if (!readSVMData())
				return;
		}
		break;
	case Constants::DetectMethod::SECUWATCHER:
		if (!m_secuwatcherMask.setParameters(m_detectParam))
		{
			MessageBox(NULL, _T("secuwatcherMask initialize error"), _T("Masquerade"), MB_OK);
			return;
		}
		break;
	}

	const std::vector<DlgBatchProcess::BatchInfo> &batchInfoList = m_pDlgBatch->getBatchInfoList();

	ui.listWidgetBatch->clear();
	for (int i = 0; i < batchInfoList.size(); ++i)
	{
		ui.listWidgetBatch->addItem(QFileInfo(batchInfoList[i].aviFile).fileName());

		QListWidgetItem *item = ui.listWidgetBatch->item(ui.listWidgetBatch->count() - 1);
		item->setIcon(m_iconDisc);
	}

	ui.listWidgetBatch->setMaximumWidth(10000);
	ui.horizontalLayoutImage->setSpacing(6);

	//const int ResizeWidth = 2000;
	//const int ResizeHeight = 1000;
	//const int ResizeHalfHeight = ResizeHeight / 2;

	//cv::gpu::GpuMat gpu_img(ResizeHeight, ResizeWidth, CV_8UC1);
	//cv::Mat img(ResizeHeight, ResizeWidth, CV_8UC1);
	
	m_bCancel = false;
	for (int i = 0; i < batchInfoList.size(); ++i)
	{
		QListWidgetItem *item = ui.listWidgetBatch->item(i);
		item->setIcon(m_iconClock);

		m_pStructData->clearAll();

		m_TargetType.clear();
		for (int typeId = 0; typeId < (int)SRegion::TypeNum; ++typeId)
		{
			m_TargetType.push_back((SRegion::Type)typeId);
		}
		m_pDlgFaceGrid->setTargetType(&m_TargetType);
		ui.imageView->setTargetType(&m_TargetType);


		readAvi(batchInfoList[i].aviFile);

		if (batchInfoList[i].bReadRgn)
		{
			int width = m_pAviManager->getWidth();
			int height = m_pAviManager->getHeight();

			if (!batchInfoList[i].rgnFile.isEmpty())
				m_pStructData->readRegions(batchInfoList[i].rgnFile.toLocal8Bit(), width, height);
		}
		else
		{
			int startFrame(0), endFrame(m_pAviManager->getFrameCount() - 1);
			// 顔検出
			if (batchInfoList[i].bDetectRange)
			{
				startFrame = std::min<int>(std::max<int>(0, batchInfoList[i].detectFrom), endFrame);
				endFrame = std::max<int>(std::min<int>(batchInfoList[i].detectTo, endFrame), startFrame);
			}

			for(int cameraNo = 0; cameraNo < m_pAviManager->getCameraCount(); cameraNo++ )
			{
				setCameraNo(cameraNo);
				for (int frame = startFrame; frame <= endFrame; ++frame)
				{
					if (m_bCancel)
						break;

					////detectHOG(frame, gpu_img);
					//detect_Panoramic(frame);
					//detectFaceInFrame(frame);

					switch (m_imageType)
					{
					case Constants::ImageType::PANORAMIC:
						//計測対象１
						detect_Panoramic(frame);
						//計測対象２
						detectFaceInFrame(frame);
						break;
					case Constants::ImageType::PERSPECTIVE:
						//計測対象１
						detect_Perspective(frame);
						//計測対象２
						detectFaceInFrame(frame);
						break;
					default:
						continue;
					}
					setFrame(frame, false);
					m_curRegion = StructData::iterator<SRegion>();

					qApp->processEvents();
				}
			}
			if (m_bCancel)
				break;

			if (m_svm)		// SVMを使用しない場合は実施しないことにする。本当はUIを変更すべき...
			{
				// 顔トラッキング
				startFrame = 0;
				endFrame = m_pAviManager->getFrameCount() - 1;

				if (batchInfoList[i].bTrackRange)
				{
					startFrame = std::min<int>(std::max<int>(0, batchInfoList[i].trackFrom), endFrame);
					endFrame = std::max<int>(std::min<int>(batchInfoList[i].trackTo, endFrame), startFrame);
				}

				if (!objectTracking(startFrame, endFrame))
					break;
				if (!objectTracking(endFrame, startFrame))
					break;
			}


			if (batchInfoList[i].bDetect)
			{
				// パスが有効かチェック
				QString detectFile = m_pDlgBatch->getFilePath(batchInfoList[i].detectFile, i);
				FileUtility::makeDirIfNotExist(detectFile);
				QString path = QFileInfo(detectFile).path();
				QDir saveDir;
				if (saveDir.exists(path))
					m_pStructData->writeRegions(detectFile.toLocal8Bit(), m_pAviManager->getWidth(), m_pAviManager->getHeight());
			}

		}


		int startFrame = 0;
		int endFrame = m_pAviManager->getFrameCount() - 1;

		if (batchInfoList[i].bImage)
		{
			QString filePath;
			if (!batchInfoList[i].bOverwrite)
			{
				QString maskedImageFile = m_pDlgBatch->getFilePath(batchInfoList[i].MaskedImageFile, i);
				filePath = QFileInfo(maskedImageFile).filePath();
				// バッチ処理で設定されたフォルダへの対応
				FileUtility::makeDirIfNotExist(maskedImageFile);
			}
			else
			{
				filePath = this->m_pAviManager->getCurFileName();
			}
			
			// パスが有効かチェック
			QDir saveDir;
			if (saveDir.exists(QFileInfo(filePath).path()))
			{
				QString fileExt = QFileInfo(filePath).suffix();

				if (fileExt == "zic")
				{
					// testtest
					//if (!exportMaskedZIC(batchInfoList[i].MaskedImageFile, startFrame, endFrame, 2000, 1000))
					if (!exportMaskedZIC(filePath, startFrame, endFrame))
						break;
				}
				else if (fileExt == "izic")
				{
					if (!exportMaskedIZIC(filePath, startFrame, endFrame))
						break;
				}
			}
		}

		item->setIcon(m_iconDone);
		// データをクリア
		m_pStructData->clearRegions();
	}

	ui.listWidgetBatch->setMaximumWidth(0);
	ui.horizontalLayoutImage->setSpacing(0);
}

void QtHOG::on_actionForce_Auto_Save_triggered()
{
	onIdle(true);
}

void QtHOG::onIdle(bool bForce)
{
	if (ui.listWidgetBatch->count() > 0)
		return;	// バッチ処理中は何もしない
	if (m_pStructData->getObjects().empty())
		return;

	const QString &curFileName = m_pAviManager->getCurFileName();
	if (curFileName.isEmpty())
		return;

	if ((!bForce) && m_pTime->elapsed() < 1 * 60 * 1000)
		return;	// セーブ後、１分未満は何もしない

	QString autoSavePath = QFileInfo(curFileName).dir().absolutePath();
	QString autoSaveName = QDir::toNativeSeparators(autoSavePath + "\\" + QFileInfo(curFileName).baseName() + "___autosave___.rgn");

	QString msg = QString("Auto saved %1...").arg(autoSaveName); 
	m_executeStatus.setText(msg);
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	m_pStructData->mutex.lock();
	m_pStructData->writeRegions(autoSaveName.toLocal8Bit(), width, height);
	m_pStructData->mutex.unlock();

	m_executeStatus.clear();

	m_pTime->restart();
}

//void QtHOG::on_actionHaar_Detect_Test_triggered()
//{
//	return;
//
//	int frame = ui.sliderFrame->value();
//
//	cv::Mat frameImage;
//
//	m_pStructData->mutex.lock();
//	cv::resize(m_pAviManager->getImage(frame), frameImage, cvSize(1000, 500));
//	m_pStructData->mutex.unlock();
//
//	// 正面顔検出器のロード
//	CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*)cvLoad( "C:\\Develop\\OpenCV2.2\\opencv\\data\\haarcascades\\haarcascade_profileface.xml" );
//
//	CvMemStorage* storage = cvCreateMemStorage(0);
//	CvSeq* faces;
//	int i;
//
//	// 顔検出
//	faces = cvHaarDetectObjects( frameImage, cascade, storage, 1.2, 3, CV_HAAR_DO_CANNY_PRUNING , cvSize(10, 10), cvSize(100, 100) );
//
//	// 顔領域の描画
//	for( i = 0; i < faces->total; i++ )
//	{
//	// extract the rectanlges only
//		CvRect face_rect = *(CvRect*)cvGetSeqElem( faces, i );
//		cvRectangle( frameImage, cvPoint(face_rect.x,face_rect.y),
//		cvPoint((face_rect.x+face_rect.width),
//		(face_rect.y+face_rect.height)),
//		CV_RGB(255,0,0), 3 );
//	}
//
//	// 画像の表示
//	cvReleaseMemStorage( &storage );
//	cvNamedWindow( "face_detect", 0 );
//	cvShowImage( "face_detect", frameImage );
//	cvWaitKey(0);
//	cvReleaseHaarClassifierCascade( &cascade );
//
//	cvReleaseImage(&frameImage);
//
///*
//	int width = m_pAviManager->getWidth();
//	int height = m_pAviManager->getHeight();
//	
//	const int ResizeWidth = 2000;//2400;
//	const int ResizeHeight = 1000;//1200;
//	const int ResizeHalfHeight = ResizeHeight / 2;
//
//	
//	m_pStructData->mutex.lock();
//
//	cv::Mat frameMat = m_pAviManager->getImage(frame);
//	cv::Mat frameImage = frameMat.clone();
//
//	//cv::cvtColor(frameMat, frameImage, CV_BGR2GRAY);
//	m_pStructData->mutex.unlock();
//	//cv::Mat resizeImage;
//	//cv::resize(frameMat, resizeImage, cv::Size(ResizeWidth, ResizeHeight));
//	//cv::Mat halfImage = resizeImage(cv::Rect(0, ResizeHalfHeight, ResizeWidth, ResizeHalfHeight));
//
//	float scale = (float)ResizeWidth / (float)frameImage.size().width;
//
//	cv::gpu::GpuMat gpu_img;
//	gpu_img.upload(frameImage);
//	cv::gpu::CascadeClassifier_GPU classifier( "C:\\Develop\\OpenCV2.2\\opencv\\data\\haarcascades\\haarcascade_frontalface_default.xml");
//
//	if (classifier.empty())
//		qDebug() << "empty";
//
//	cv::gpu::GpuMat objbuf;
//	int detections_number = classifier.detectMultiScale(gpu_img, objbuf);
//	qDebug() << "detections_number" << detections_number;
//
//	cv::Mat obj_host;
//	// 矩形の検出数だけをダウンロードします．
//	objbuf.colRange(0, detections_number).download(obj_host);
//
//	cv::Rect* faces = obj_host.ptr<cv::Rect>();
//	for(int i = 0; i < detections_number; ++i)
//		cv::rectangle(frameImage, faces[i], CV_RGB(255, 0, 0));
//
//	imshow("Faces", frameImage);
//	*/
//}
//
void QtHOG::on_actionDetect_Head_Shoulder_triggered()
{
	return;

	if (!m_svm)
	{
		if (!readSVMData())
			return;
		//CvSVMParams p = m_svm->get_params();
		//qDebug() << p.svm_type;
	}

	if (!m_pPCA)
	{
		if (!readPCAdata())
			return;
	}

	int startFrame = ui.sliderFrame->value();
	int endFrame = /*m_pAviManager->getFrameCount() - 1;*/startFrame;
	//int startFrame = ui.sliderFrame->value();
	//int endFrame = startFrame;
	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	cv::Mat gradDir, gradMag, lbpMat;
	//for (unsigned int frame = 0; frame < m_pAviManager->getFrameCount(); ++frame)
	for (unsigned int frame = startFrame; frame <= endFrame; ++frame)
	{
		QString msg = QString("Detecting in (%1/%2)...").arg(frame).arg(m_pAviManager->getFrameCount()); 
		m_executeStatus.setText(msg);
		qApp->processEvents();

		cv::Mat frameImage = m_pAviManager->getImage(frame);
		cv::Mat grayImage, checkImage, cannyImage;
		cv::Rect halfRect(0, height / 2, width, height / 4);
		cv::cvtColor(frameImage(halfRect), grayImage, CV_BGR2GRAY);
		cv::cvtColor(grayImage, checkImage, CV_GRAY2RGB);

		Detector::calcGrad(grayImage, gradDir, gradMag);
		Detector::extractLBP(grayImage, lbpMat);

		cv::Canny(grayImage, cannyImage, 50, 150);

		int step = 6;
		std::vector<float> featLBP, feat;
		for (int y = Detector::TemplateHalfHeight; y < height / 4 - Detector::TemplateHalfHeight - 1; y += step)
		{
			for (int x = Detector::TemplateHalfWidth; x < width - Detector::TemplateHalfWidth - 1; x += step)
			{
				qDebug() << x << y;

				cv::Rect sampleRect(x - Detector::TemplateHalfWidth, y - Detector::TemplateHalfHeight, 
						Detector::TemplateWidth, Detector::TemplateHeight);

				int nonZero = cv::countNonZero(cannyImage(sampleRect));
				if (nonZero < 100)
					continue;

				Detector::getHoG_partial(gradDir, gradMag, x, y, feat);
				Detector::getLBP_partial(lbpMat, x, y, featLBP);

				int featSize = feat.size();
				feat.resize(featSize + featLBP.size());
				for (int i = 0; i < featLBP.size(); ++i)
					feat[featSize + i] = featLBP[i];

				// PCA による次元圧縮
				cv::Mat src(1, feat.size(), CV_32FC1, &(feat[0]));
				cv::Mat result = m_pPCA->project(src);

				CvMat m;
				cvInitMatHeader (&m, 1, result.cols, CV_32FC1, result.data);
				if (m_svm->predict(&m))
				{
					qDebug() << "Found" << x << y;
					cv::rectangle(checkImage, sampleRect, CV_RGB(255, 0, 0));
				}
			}
		}
		cv::namedWindow("Heads");
		cv::imshow("Heads", checkImage);
		cv::waitKey(10);
	}

	m_executeStatus.clear();
}

void QtHOG::on_actionShrink_Tracking_Data_triggered()
{
	return;

	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Tracking data"), QDir::currentPath(),
													tr("Tracking data file (*.trk);;All files (*.*)"), &selFilter
													);

	if (fileName.isEmpty())
		return;

	QFileInfo fileInfo(fileName);
	QString filePath = fileInfo.path();
	QString fileBase = fileInfo.baseName();
	QString fileExt = fileInfo.suffix();

	QString outFile = filePath + "\\" + fileBase + "_shrink.trk";

    FILE *fp;
	fp = ::fopen(fileName.toLocal8Bit(), "r");
    if (NULL == fp)
        return;

	FILE *out_fp;
	out_fp = ::fopen(outFile.toLocal8Bit(), "w");
	if (NULL == out_fp)
		return;

    int point;
    int frame;
	double px, py;
	unsigned int flags;
	//int lineCount;

	while (EOF != ::fscanf(fp, "%d %d %lf %lf %d", &point, &frame, &px, &py, &flags))
    {
		//iterator<STrack> ittrack = insert(cam, frame, point, px, py, flags);
		::fprintf(out_fp, "%d %d %18.18f %18.18f %d\n", point, frame / 2, px, py, flags);
    }
    ::fclose(fp);
	::fclose(out_fp);
}

void QtHOG::onCameraChange()
{
	setCameraNo(this->m_curCamera+1);
}

void QtHOG::setCameraNo(int cameraNo)
{
	this->m_curCamera = cameraNo;
	if(	(this->m_pAviManager->getCameraCount() <= this->m_curCamera) ||
		(0 > this->m_curCamera) )
	{
		this->m_curCamera = 0;
	}

	//IC並んでなかったりしたら失敗するのでキャッチ
	if( !( this->m_pAviManager->changeCamera( this->m_curCamera ) ) )
	{
		this->m_pAviManager->changeCamera( 0 );
		return;
	}

	//UIになにかのっけてるのでそちらも変更
	ui.imageView->changeCamera( this->m_curCamera );

	//ダイアログもカメラチェンジ
	m_pDlgFaceGrid->changeCamera( this->m_curCamera );

	//更新
	setFrame(m_curFrame, true, true);
}

void QtHOG::InitializeParameters()
{
	TCHAR exePath[MAX_PATH];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	::GetModuleFileName(NULL, exePath, MAX_PATH);
	_tsplitpath(exePath, drive, dir, fname, NULL);
	PWSTR pszPath;
	::SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pszPath);
	TCHAR	documentDir[MAX_PATH];
	_stprintf_s(documentDir, L"%s\\%s", pszPath, fname);
	_tmkdir(documentDir);

	TCHAR	filepath[MAX_PATH];
	_stprintf(filepath, L"%s\\%s.ini", documentDir, fname);
	TCHAR	localDir[MAX_PATH];
	::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath);
	_stprintf_s(localDir, L"%s\\%s", pszPath, fname);
	if (!PathFileExists(filepath))
	{
		TCHAR	IniFilepath[MAX_PATH];
		_stprintf(IniFilepath, L"%s\\%s.ini", localDir, fname);
		if (PathFileExists(IniFilepath))
			CopyFile(IniFilepath, filepath, TRUE);
	}


	TCHAR	buf[256];
	// QtHOGのパラメータ
	m_PredictSearchRange = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("PREDICT_SEARCH_RANGE"), 40, filepath);
	m_InitialSearchRange = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("INITIAL_SEARCH_RANGE"), 80, filepath);
	m_ResizeWidth = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("RESIZE_WIDTH"), 2000, filepath);
	m_ResizeHeight = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("RESIZE_HEIGHT"), 1000, filepath);
	GetPrivateProfileString(_T("MASKING_TOOL"), _T("DETECT_METHOD"), _T("HOG"), buf, sizeof(buf), filepath);
	if (_tcscmp(buf, _T("HOG"))==0)
		m_detectMethod = Constants::DetectMethod::HOG;
	else if (_tcscmp(buf, _T("SECUWATCHER"))==0)
		m_detectMethod = Constants::DetectMethod::SECUWATCHER;
	else
		m_detectMethod = Constants::DetectMethod::HOG;

	GetPrivateProfileString(_T("MASKING_TOOL"), _T("IMAGE_TYPE"), _T("PANORAMIC"), buf, sizeof(buf), filepath);
	if (_tcscmp(buf, _T("PANORAMIC"))==0)
		m_imageType = Constants::ImageType::PANORAMIC;
	else if (_tcscmp(buf, _T("PERSPECTIVE"))==0)
		m_imageType = Constants::ImageType::PERSPECTIVE;
	else
	{
		if (m_detectMethod == Constants::DetectMethod::SECUWATCHER)
			m_imageType = Constants::ImageType::PERSPECTIVE;
		else
			m_imageType = Constants::ImageType::PANORAMIC;
	}

	//GPU
	//m_useGPU = (GetPrivateProfileInt(_T("MASKING_TOOL"), _T("USE_GPU"), 0, filepath) != 0);

	//jpeg Q
	m_JpegQuality = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("JPEG_QUALITY"), 95, filepath);
	m_OutputMaskLog = (GetPrivateProfileInt(_T("MASKING_TOOL"), _T("OUTPUT_MASK_LOG"), 0, filepath) != 0);

	// SVM
	GetPrivateProfileString(_T("MASKING_TOOL"), _T("FACE_THRESHOLD"), _T("2.0"), buf, sizeof(buf), filepath);
	m_FaceThreshold = _tcstod(buf, NULL);

	// SecuwatcherMask
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("CHILD_PROCESS_NAME"), _T("SecuwatcherMaskDLL.exe"), m_childProcName, sizeof(m_childProcName), filepath);
	m_detectParam.model[secuwatcher_access::MODEL_FACE].valid = (GetPrivateProfileInt(_T("SECUWATCHER_MASK"), _T("USE_FACE_MODEL"), 0, filepath) != 0);
	m_detectParam.model[secuwatcher_access::MODEL_PLATE].valid = (GetPrivateProfileInt(_T("SECUWATCHER_MASK"), _T("USE_PLATE_MODEL"), 0, filepath) != 0);
	m_detectParam.model[secuwatcher_access::MODEL_PERSON].valid = (GetPrivateProfileInt(_T("SECUWATCHER_MASK"), _T("USE_PERSON_MODEL"), 0, filepath) != 0);

	TCHAR modelFilename[MAX_PATH];
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("FACE_CFG"), _T("face_network.wkit"), modelFilename, MAX_PATH, filepath);
	_stprintf_s(m_detectParam.model[secuwatcher_access::MODEL_FACE].cfgFilename, L"%s\\%s", localDir, modelFilename);
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("PLATE_CFG"), _T("plate_network.wkit"), modelFilename, MAX_PATH, filepath);
	_stprintf_s(m_detectParam.model[secuwatcher_access::MODEL_PLATE].cfgFilename, L"%s\\%s", localDir, modelFilename);
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("PERSON_CFG"), _T("person_network.wkit"), modelFilename, MAX_PATH, filepath);
	_stprintf_s(m_detectParam.model[secuwatcher_access::MODEL_PERSON].cfgFilename, L"%s\\%s", localDir, modelFilename);
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("FACE_WEIGHT"), _T("face_model.wkit"), modelFilename, MAX_PATH, filepath);
	_stprintf_s(m_detectParam.model[secuwatcher_access::MODEL_FACE].weightFilename, L"%s\\%s", localDir, modelFilename);
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("PLATE_WEIGHT"), _T("plate_model.wkit"), modelFilename, MAX_PATH, filepath);
	_stprintf_s(m_detectParam.model[secuwatcher_access::MODEL_PLATE].weightFilename, L"%s\\%s", localDir, modelFilename);
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("PERSON_WEIGHT"), _T("person_model.wkit"), modelFilename, MAX_PATH, filepath);
	_stprintf_s(m_detectParam.model[secuwatcher_access::MODEL_PERSON].weightFilename, L"%s\\%s", localDir, modelFilename);
	GetPrivateProfileString(_T("SECUWATCHER_MASK"), _T("DEEPLEARNING_THRESHOLD"), _T("0.2"), buf, sizeof(buf), filepath);
	m_detectParam.DL_thresh = _tcstod(buf, NULL);
}

void QtHOG::updateSelectNum()
{
	QString msg = QString("%1/%2 selected").arg(m_selectedItems.size()).arg(ui.imageView->getNumViewRegion());
	m_selectedStatus.setText(msg);
}

//アプリ本体
MyApplication::MyApplication(int& argc, char** argv)
	: QApplication(argc, argv)
#ifndef NOLICENSE
#endif
{
}

bool MyApplication::notify(QObject* receiver, QEvent* event)
{
	// 重いのでエラーがほとんどなくなったら外したい
    bool done = true;
    try {
        done = QApplication::notify(receiver, event);
    } catch (const std::exception& ex) {
		qDebug() << "notify catch exception :" << ex.what();
		qDebug() << event;
		QString exceptionMassage;
		exceptionMassage.append( "exception :" );
		if (ex.what())
			exceptionMassage.append( ex.what() );
		//exceptionMassage.append( "\nIf you find this report, please take it to A.I.C." );
		QMessageBox::critical(NULL, "Error Report", exceptionMassage);
    } catch (...) {
		qDebug() << "notify catch exception other break";
		QString exceptionMassage;
		exceptionMassage.append( "exception other break" );
		//exceptionMassage.append( "\nIf you find this report, please take it to A.I.C." );
		QMessageBox::critical(NULL, "Error Report", exceptionMassage);
    }
    return done;
}

#ifndef NOLICENSE
#endif	//NOLICENSE
