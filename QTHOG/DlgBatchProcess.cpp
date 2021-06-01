#include "DlgBatchProcess.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <set>

DlgBatchProcess::DlgBatchProcess(QWidget *parent)
	: QDialog(parent)
	, m_baseDir(QString())
{
	ui.setupUi(this);

	QValidator *validator = new QIntValidator(0, 10000, this);
	
	ui.lineEditDetectFrom->setValidator(validator);
	ui.lineEditDetectTo->setValidator(validator);
	ui.lineEditTrackingFrom->setValidator(validator);
	ui.lineEditTrackingTo->setValidator(validator);

	//connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
	//m_timer.setInterval(100);
	//m_timer.start();

	m_iconClock = QIcon(":/Icon/Clock");
	m_iconWarning = QIcon(":/Icon/Warning");
	m_iconDone = QIcon(":/Icon/Done");
	m_iconDisc = QIcon(":/Icon/Disc");

	connect(ui.listWidgetInputFiles, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onItemClicked(QListWidgetItem*)));
}

DlgBatchProcess::~DlgBatchProcess()
{
}

//void DlgBatchProcess::onTimer()
void DlgBatchProcess::updateView()
{
	ui.lineEditBaseDir->setText(m_baseDir);
	int row = ui.listWidgetInputFiles->currentRow();

	if (row < 0)
	{
		ui.groupBoxDetection->setEnabled(false);
		ui.groupBoxTracking->setEnabled(false);
		ui.groupBoxMaskedImages->setEnabled(false);
		ui.groupBoxReadRgn->setEnabled(false);
		ui.buttonBoxBatch->button(QDialogButtonBox::Ok)->setEnabled(false);

		ui.lineEditDetected->clear();
		ui.lineEditDetectFrom->clear();
		ui.lineEditDetectTo->clear();
		ui.lineEditMaskedImages->clear();
		ui.lineEditTrackingFrom->clear();
		ui.lineEditTrackingTo->clear();
		ui.lineEditReadRgn->clear();
		return;
	}

	ui.groupBoxDetection->setEnabled(true);
	ui.groupBoxTracking->setEnabled(true);
	ui.groupBoxMaskedImages->setEnabled(true);
	ui.groupBoxReadRgn->setEnabled(true);

	// ファイルの設定をチェック
	bool bStartEnable = true;
	for (int i = 0; i < m_batchInfoList.size(); ++i)
	{
		if (i == row)
		{
			ui.groupBoxDetection->setChecked(m_batchInfoList[i].bDetect);

			ui.lineEditDetected->setText(getFilePath(m_batchInfoList[i].detectFile, row));
			ui.checkBoxDetectRange->setChecked(m_batchInfoList[i].bDetectRange);

			ui.lineEditDetectFrom->setEnabled(m_batchInfoList[i].bDetectRange && m_batchInfoList[i].bDetect);
			ui.lineEditDetectTo->setEnabled(m_batchInfoList[i].bDetectRange && m_batchInfoList[i].bDetect);
			ui.labelDetectArrow->setEnabled(m_batchInfoList[i].bDetectRange && m_batchInfoList[i].bDetect);
			if (m_batchInfoList[i].bDetectRange && m_batchInfoList[i].bDetect)
			{
				ui.lineEditDetectFrom->setText(QString::number(m_batchInfoList[i].detectFrom));
				ui.lineEditDetectTo->setText(QString::number(m_batchInfoList[i].detectTo));
			}
			else
			{
				ui.lineEditDetectFrom->clear();
				ui.lineEditDetectTo->clear();
			}


			ui.groupBoxTracking->setChecked(m_batchInfoList[i].bTrack);
			
			ui.checkBoxTrackingRange->setChecked(m_batchInfoList[i].bTrackRange);

			ui.lineEditTrackingFrom->setEnabled(m_batchInfoList[i].bTrackRange && m_batchInfoList[i].bTrack);
			ui.lineEditTrackingTo->setEnabled(m_batchInfoList[i].bTrackRange && m_batchInfoList[i].bTrack);
			ui.labelTrackArrow->setEnabled(m_batchInfoList[i].bTrackRange && m_batchInfoList[i].bTrack);
			if (m_batchInfoList[i].bTrackRange && m_batchInfoList[i].bTrack)
			{
				ui.lineEditTrackingFrom->setText(QString::number(m_batchInfoList[i].trackFrom));
				ui.lineEditTrackingTo->setText(QString::number(m_batchInfoList[i].trackTo));
			}
			else
			{
				ui.lineEditTrackingFrom->clear();
				ui.lineEditTrackingTo->clear();
			}

			ui.groupBoxReadRgn->setChecked(m_batchInfoList[i].bReadRgn);	
			ui.lineEditReadRgn->setText(getFilePath(m_batchInfoList[i].rgnFile, row));


			ui.groupBoxMaskedImages->setChecked(m_batchInfoList[i].bImage);

			ui.lineEditMaskedImages->setText(getFilePath(m_batchInfoList[i].MaskedImageFile, row));
			ui.checkBoxOverwrite->setChecked(m_batchInfoList[i].bOverwrite);

			ui.lineEditMaskedImages->setEnabled(!m_batchInfoList[i].bOverwrite);
		}

		QListWidgetItem *item = ui.listWidgetInputFiles->item(i);
		if (bStartEnable = (m_batchInfoList[i].bDetect || m_batchInfoList[i].bTrack || m_batchInfoList[i].bImage))
			item->setIcon(m_iconDisc);
		else
			item->setIcon(m_iconWarning);
	}

	// 処理をスタートできる状態かチェック
	ui.buttonBoxBatch->button(QDialogButtonBox::Ok)->setEnabled(bStartEnable);
}

void DlgBatchProcess::on_pushButtonAddInputFile_clicked()
{
	QString selFilter = "";
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
													tr("Open ZIC or IZIC file"), QDir::currentPath(),
													tr("ZIC or IZIC file (*.zic *.izic);;All files (*.*)"), &selFilter);
	bool bDuplicate = false;
	for (int i = 0; i < fileNames.size(); ++i)
	{
		bool bAlreadyExists = false;
		for (int j = 0; j < m_batchInfoList.size(); ++j)
		{
			if (m_batchInfoList[j].aviFile == fileNames[i])
			{
				bAlreadyExists = true;
				break;
			}
		}
		if (bAlreadyExists)
		{
			bDuplicate = true;
			continue;
		}


		// 拡張子に合わせて出力ファイル名を変更する
		{
			QString tempdir = getBaseDir(fileNames[i]);
			QString tempname = QFileInfo(fileNames[i]).baseName();
			QDir baseDir(tempdir);
			QString rgnFile = QDir::toNativeSeparators(baseDir.absoluteFilePath(tempname + ".rgn"));
			QString detectFile = QDir::toNativeSeparators(baseDir.absoluteFilePath(BatchProcessConst::c_MaskedDir + tempname + ".rgn"));
			
			QString maskedImageFile = "";
			{
				QString fileExt = QFileInfo(fileNames[i]).suffix();
				if(fileExt == "zic")
				{
					maskedImageFile = baseDir.absoluteFilePath(BatchProcessConst::c_MaskedDir + tempname + ".zic");
				}
				else if(fileExt == "izic")
				{
					maskedImageFile = baseDir.absoluteFilePath(BatchProcessConst::c_MaskedDir + tempname + ".izic");
				}
				else
				{
					continue;
				}
			}
			if (checkDuplicateFilePath(baseDir.absoluteFilePath(maskedImageFile), true) || checkDuplicateFilePath(baseDir.absoluteFilePath(detectFile), true))
			{
				bDuplicate = true;
				continue;
			}
			m_batchInfoList.resize(m_batchInfoList.size() + 1);
			m_batchInfoList.back().aviFile = fileNames[i];
			
			m_batchInfoList.back().bReadRgn = QFileInfo(rgnFile).exists();
			m_batchInfoList.back().bImage = true;

			m_batchInfoList.back().detectFile = getRelFilePath(detectFile, tempdir);
			m_batchInfoList.back().bDetect = true;

			m_batchInfoList.back().rgnFile = rgnFile;	// regionデータ読み込み用ファイル名は絶対パスで持つ
			m_batchInfoList.back().MaskedImageFile = getRelFilePath(maskedImageFile, tempdir);

			ui.listWidgetInputFiles->addItem(QDir::toNativeSeparators(fileNames[i]));
		}
	}
	if (bDuplicate)
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);


	ui.listWidgetInputFiles->setCurrentRow(ui.listWidgetInputFiles->count() - 1);

	updateView();
}

void DlgBatchProcess::on_pushButtonDeleteInputFile_clicked()
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;

	m_batchInfoList.erase(m_batchInfoList.begin() + row);

	//QListWidgetItem *item = ui.listWidgetInputFiles->currentItem();
	//ui.listWidgetInputFiles->removeItemWidget(ui.listWidgetInputFiles->currentItem());
	delete ui.listWidgetInputFiles->takeItem(row);
	ui.listWidgetInputFiles->setCurrentRow(ui.listWidgetInputFiles->count() - 1);

	updateView();
}

void DlgBatchProcess::on_groupBoxDetection_toggled(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	if (checkDuplicateFilePath())
	{
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
	}
	else
		m_batchInfoList[row].bDetect = bCheck;
	if (m_batchInfoList[row].bDetect && m_batchInfoList[row].detectFile.isEmpty())
	{
		QString detectFile = getFilePath(BatchProcessConst::c_MaskedDir + getBaseName() + ".rgn", row);
		if (checkDuplicateFilePath(getFilePath(detectFile, row), false))
		{
			QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
		}
		else
			m_batchInfoList[row].detectFile = getRelFilePath(detectFile);
		//QFileInfo fInfoMaskedImageFile(m_batchInfoList[row].MaskedImageFile);
		//m_batchInfoList[row].detectFile = fInfoMaskedImageFile.path() + "/" + fInfoMaskedImageFile.baseName() + ".rgn";
	}

	updateView();
}

void DlgBatchProcess::on_groupBoxTracking_toggled(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].bTrack = bCheck;

	updateView();
}

void DlgBatchProcess::on_groupBoxReadRgn_toggled(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].bReadRgn = bCheck;

	updateView();
}

void DlgBatchProcess::on_groupBoxMaskedImages_toggled(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	if (checkDuplicateFilePath())
	{
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
	}
	else
		m_batchInfoList[row].bImage = bCheck;

	updateView();
}

void DlgBatchProcess::on_groupBoxBaseDir_toggled(bool bCheck)
{
	if (checkDuplicateFilePath())
	{
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
		ui.groupBoxBaseDir->setChecked(!bCheck);
	}
	updateView();
}

void DlgBatchProcess::showEvent(QShowEvent *event)
{
	m_batchInfoList.clear();
	ui.listWidgetInputFiles->clear();

	updateView();
}

void DlgBatchProcess::on_lineEditDetected_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	if (checkDuplicateFilePath(text, false))
	{
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
	}
	else
		m_batchInfoList[row].detectFile = getRelFilePath(text);

	updateView();
}

void DlgBatchProcess::on_lineEditReadRgn_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].rgnFile = text;	// regionデータ読み込み用ファイル名は絶対パスで持つ

	updateView();
}

void DlgBatchProcess::on_lineEditMaskedImages_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	if (checkDuplicateFilePath(text, false))
	{
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
	}
	else
		m_batchInfoList[row].MaskedImageFile = getRelFilePath(text);

	updateView();
}

void DlgBatchProcess::on_lineEditBaseDir_textChanged(const QString &text)
{
	QString oldBaseDir = m_baseDir;
	m_baseDir = text;
	if (checkDuplicateFilePath())
	{
		QMessageBox::warning(this, BatchProcessConst::c_MessageTitle, BatchProcessConst::c_ErrorDuplicateFile);
		m_baseDir = oldBaseDir;
	}
	updateView();
}

void DlgBatchProcess::on_checkBoxDetectRange_clicked(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].bDetectRange = bCheck;

	updateView();
}

void DlgBatchProcess::on_checkBoxTrackingRange_clicked(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].bTrackRange = bCheck;

	updateView();
}

void DlgBatchProcess::on_checkBoxOverwrite_clicked(bool bCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].bOverwrite = bCheck;

	updateView();
}

void DlgBatchProcess::on_lineEditDetectFrom_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].detectFrom = text.toInt();

	updateView();
}

void DlgBatchProcess::on_lineEditDetectTo_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].detectTo = text.toInt();

	updateView();
}

void DlgBatchProcess::on_lineEditTrackingFrom_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].trackFrom = text.toInt();

	updateView();
}

void DlgBatchProcess::on_lineEditTrackingTo_textChanged(const QString &text)
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return;
	m_batchInfoList[row].trackTo = text.toInt();

	updateView();
}

void DlgBatchProcess::on_pushButtonDetected_clicked()
{
	QString selFilter = "";
	QString fileName = QFileDialog::getSaveFileName(this,
													tr("Write region data"), QDir::currentPath(),
													tr("Region file (*.rgn)"), &selFilter
													);
	if (!fileName.isEmpty())
	{
		ui.lineEditDetected->setText(fileName);

		updateView();
	}
}

void DlgBatchProcess::on_pushButtonReadRgn_clicked()
{
	QString selFilter = "";
	QString fileName = QFileDialog::getOpenFileName(this,
													tr("Read region data"), QDir::currentPath(),
													tr("Region file (*.rgn)"), &selFilter
													);
	if (!fileName.isEmpty())
	{
		ui.lineEditReadRgn->setText(fileName);

		updateView();
	}
}

void DlgBatchProcess::on_pushButtonMaskedImages_clicked()
{
	QString selFilter = "";
	QString fileName = QFileDialog::getSaveFileName(this,
													tr("Export masked images"), QDir::currentPath(),
													tr("ZIC file (*.zic);;IZIC file (*.izic)"), &selFilter
													);
	if (!fileName.isEmpty())
	{
		ui.lineEditMaskedImages->setText(fileName);

		updateView();
	}
}
void DlgBatchProcess::on_pushButtonBaseDir_clicked()
{
	QString selFilter = "";
	QString fileName = QFileDialog::getExistingDirectory(this,
														tr("Set base directory"),
														ui.lineEditBaseDir->text()
														);
	if (!fileName.isEmpty())
	{
		ui.lineEditBaseDir->setText(fileName);

		updateView();
	}
}

void DlgBatchProcess::onItemClicked(QListWidgetItem *item)
{
	updateView();
}

QString DlgBatchProcess::getFilePath(const QString &relFilePath, int row)
{
	if ((row < -1) || (row >= m_batchInfoList.size()))
		return QString();
	QDir baseDir(getBaseDir(m_batchInfoList[row].aviFile));
	return baseDir.absoluteFilePath(relFilePath);
}

QString DlgBatchProcess::getRelFilePath(const QString &filePath)
{
	return getRelFilePath(filePath, getBaseDir());
}

QString DlgBatchProcess::getRelFilePath(const QString &filePath, const QString &dirPath)
{
	QDir baseDir(dirPath);
	return baseDir.relativeFilePath(filePath);
}

bool DlgBatchProcess::checkDuplicateFilePath(const QString &filePath, bool allCheck)
{
	int row = ui.listWidgetInputFiles->currentRow();
	for (int i = 0; i < m_batchInfoList.size(); ++i)
	{
		if (!allCheck && (i == row))
			continue;
		if (checkSameFilePath(filePath, getFilePath(m_batchInfoList[i].MaskedImageFile, i)) || checkSameFilePath(filePath, getFilePath(m_batchInfoList[i].detectFile, i)))
			return true;
	}
	return false;
}

bool DlgBatchProcess::checkDuplicateFilePath()
{
	std::set<QString> filenames;
	QString checkFile;
	for (int i = 0; i < m_batchInfoList.size(); ++i)
	{
		checkFile = QDir::toNativeSeparators(getFilePath(m_batchInfoList[i].MaskedImageFile, i));
		if (filenames.find(checkFile) != filenames.end())
			return true;
		filenames.insert(checkFile);
		checkFile = QDir::toNativeSeparators(getFilePath(m_batchInfoList[i].detectFile, i));
		if (filenames.find(checkFile) != filenames.end())
			return true;
		filenames.insert(checkFile);
	}
	return false;
}

QString DlgBatchProcess::getBaseDir(const QString &filePath)
{
	if (ui.groupBoxBaseDir->isChecked())
	{
		if (!ui.lineEditBaseDir->text().isEmpty())
			return ui.lineEditBaseDir->text();
	}
	return QFileInfo(filePath).absolutePath();
}

QString DlgBatchProcess::getBaseDir()
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return QString();
	return getBaseDir(m_batchInfoList[row].aviFile);
}

QString DlgBatchProcess::getBaseName()
{
	int row = ui.listWidgetInputFiles->currentRow();
	if (row < 0)
		return QString();
	return QFileInfo(m_batchInfoList[row].aviFile).baseName();
}

bool DlgBatchProcess::checkSameFilePath(const QString &filePath1, const QString &filePath2)
{
	QString absFilePath1 = QDir::toNativeSeparators(filePath1);
	QString absFilePath2 = QDir::toNativeSeparators(filePath2);
	return absFilePath1.compare(absFilePath2, Qt::CaseInsensitive) == 0;
}

