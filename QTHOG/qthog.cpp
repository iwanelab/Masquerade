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
#include <highgui.h>
#include <vfw.h>
#include "IdleWatcher.h"
#include <QTime>

#include <QMessageBox>
#include <QUuid>

//testcode
#include <exception>
#include <QElapsedTimer>

#include "FileUtility.h"

#include "ExternalDetector.h"


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

	// 外部処理実行
	if (m_detectMethod == Constants::DetectMethod::EXTERNAL)
	{
		m_detector = dynamic_cast<IDetector *>(new CExternalDetector);

		TCHAR drive[_MAX_DRIVE];
		TCHAR dir[_MAX_DIR];
		TCHAR filename[_MAX_FNAME];
		TCHAR ext[_MAX_FNAME];
		// 起動中の外部処理を停止する。
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

		// 外部処理を実行する。
		TCHAR childProcPath[MAX_PATH];
		TCHAR exePath[MAX_PATH];
		::GetModuleFileName(NULL, exePath, MAX_PATH);
		_tsplitpath(exePath, drive, dir, NULL, NULL);
		_tmakepath_s(childProcPath, drive, dir, m_childProcName, NULL);
		if (!PathFileExists(childProcPath))
		{
			MessageBox(NULL, _T("not exist detector execute process file"), _T("Masquerade"), MB_OK);
			return;
		}
		m_detector->setWindowHandle(this->winId());
		if (!m_detector->initChildProcess(childProcPath))
		{
			MessageBox(NULL, _T("execute process error"), _T("Masquerade"), MB_OK);
			return;
		}
	}
}

QtHOG::~QtHOG()
{
	if (m_detectMethod == Constants::DetectMethod::EXTERNAL && m_detector != NULL)
		m_detector->closeChildProcess();
	delete m_pAviManager;
	delete m_pCalculator;
	//delete m_pDlgSkinColor;
	//delete m_pSkinClassifier;
	//delete m_pImageProcessor;

	m_pStructData->release();

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
void QtHOG::on_actionDetect_Faces_triggered()
{	
	//人検出、顔検出でGPUを利用する
//	bool useGPU = false;
	switch (m_detectMethod)
	{
	case Constants::DetectMethod::EXTERNAL:
		TCHAR	filepath[MAX_PATH];
		getInitialFileName(filepath);
		if (!m_detector->setParameters(filepath))
		{
			MessageBox(NULL, _T("detector process initialize error"), _T("Masquerade"), MB_OK);
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
			break;
		case Constants::ImageType::PERSPECTIVE:
			//計測対象１
			detect_Perspective(frame);
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
	case Constants::DetectMethod::EXTERNAL:
		detect(resizeImage, found, types);
		break;
	}

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
		case Constants::DetectMethod::EXTERNAL:
			itr = m_pStructData->insertRegion(m_curCamera, frame, -1, r, types[i]);
			break;
		}
	}
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
		case Constants::DetectMethod::EXTERNAL:
			detect(frameMat, found, types);
			break;
		}

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
			case Constants::DetectMethod::EXTERNAL:
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
void QtHOG::detect(cv::Mat image, std::vector<cv::Rect> &found, std::vector<SRegion::Type> &types)
{
	std::vector<detector::detectedData> detectDatas;
	if (!m_detector->detect(image, detectDatas))
	{
		MessageBox(NULL, _T("detect error"), _T("Masquerade"), MB_OK);
	}
	for (std::vector<detector::detectedData>::iterator detectData = detectDatas.begin(); detectData != detectDatas.end(); detectData++)
	{
		cv::Rect rectData = cvRect(detectData->rectData.x, detectData->rectData.y, detectData->rectData.width, detectData->rectData.height);

		//if (m_detectParam.model[detectData->modelType].valid)
		//{
			switch (detectData->modelType)
			{
			case detector::MODEL_PERSON:
				types.push_back(SRegion::TypeFace);
				found.push_back(rectData);
				break;
			case detector::MODEL_FACE:
				types.push_back(SRegion::TypeFace);
				found.push_back(rectData);
				break;
			case detector::MODEL_PLATE:
				types.push_back(SRegion::TypePlate);
				found.push_back(rectData);
				break;
			}
		//}
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

		CvRect trackingRect;
		if (predictTrackingRect(predictPos, frame, object, searchRange, trackingRect))
		{

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

bool QtHOG::predictTrackingRect(QPointF predictPos, int frame, int object, int searchRange, CvRect &newRect)
{	

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
		qDebug() << "ileagle : predictTrackingRect";
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

void QtHOG::onAutoTrackingRequired(int object, int mode)
{

	// とりあえずセーブ
	onIdle(true);

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
			if (predictTrackingRect(predictPos, frame, object, searchRange, trackingRect))
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
			if (predictTrackingRect(predictPos, frame, object, searchRange, trackingRect))
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
			if (predictTrackingRect(predictPos, frame, object, searchRange, trackingRect))
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
	if (predictTrackingRect(predictPos, frame, object, searchRange, trackingRect))
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
	case Constants::DetectMethod::EXTERNAL:
		TCHAR	filepath[MAX_PATH];
		getInitialFileName(filepath);
		if (!m_detector->setParameters(filepath))
		{
			MessageBox(NULL, _T("detector initialize error"), _T("Masquerade"), MB_OK);
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

					switch (m_imageType)
					{
					case Constants::ImageType::PANORAMIC:
						//計測対象１
						detect_Panoramic(frame);
						break;
					case Constants::ImageType::PERSPECTIVE:
						//計測対象１
						detect_Perspective(frame);
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
	TCHAR	filepath[MAX_PATH];
	getInitialFileName(filepath);
	TCHAR	buf[256];
	// QtHOGのパラメータ
	m_PredictSearchRange = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("PREDICT_SEARCH_RANGE"), 40, filepath);
	m_InitialSearchRange = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("INITIAL_SEARCH_RANGE"), 80, filepath);
	m_ResizeWidth = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("RESIZE_WIDTH"), 2000, filepath);
	m_ResizeHeight = GetPrivateProfileInt(_T("MASKING_TOOL"), _T("RESIZE_HEIGHT"), 1000, filepath);
	GetPrivateProfileString(_T("MASKING_TOOL"), _T("DETECT_METHOD"), _T("EXTERNAL"), buf, sizeof(buf), filepath);
	if (_tcscmp(buf, _T("SECUWATCHER"))==0)
		m_detectMethod = Constants::DetectMethod::EXTERNAL;
	else if (_tcscmp(buf, _T("EXTERNAL"))==0)
		m_detectMethod = Constants::DetectMethod::EXTERNAL;
	else
		m_detectMethod = Constants::DetectMethod::EXTERNAL;

	GetPrivateProfileString(_T("MASKING_TOOL"), _T("IMAGE_TYPE"), _T("PANORAMIC"), buf, sizeof(buf), filepath);
	if (_tcscmp(buf, _T("PANORAMIC"))==0)
		m_imageType = Constants::ImageType::PANORAMIC;
	else if (_tcscmp(buf, _T("PERSPECTIVE"))==0)
		m_imageType = Constants::ImageType::PERSPECTIVE;
	else
	{
		if (m_detectMethod == Constants::DetectMethod::EXTERNAL)
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

	// external process
	GetPrivateProfileString(_T("DETECTOR"), _T("CHILD_PROCESS_NAME"), _T("DNNDetector.exe"), m_childProcName, sizeof(m_childProcName), filepath);
}

void QtHOG::getInitialFileName(wchar_t *filepath)
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
