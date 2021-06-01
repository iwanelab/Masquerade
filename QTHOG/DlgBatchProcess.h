#ifndef DLGBATCHPROCESS_H
#define DLGBATCHPROCESS_H

#include <QDialog>
#include "ui_DlgBatchProcess.h"
//#include <QTimer>

namespace BatchProcessConst
{
	const QString c_MaskedDir("Masked\\");
	const QString c_MessageTitle("Masquerade");
	const QString c_ErrorDuplicateFile("The change is invalid.\nThere is an output file with the same name.");
}

class DlgBatchProcess : public QDialog
{
	Q_OBJECT

public:
	struct BatchInfo
	{
		QString aviFile;
		QString detectFile;
		QString rgnFile;
		QString MaskedImageFile;

		bool bDetect;
		bool bTrack;
		bool bImage;
		bool bReadRgn;
		bool bDetectRange;
		bool bTrackRange;
		bool bOverwrite;
		//bool bValid;

		int detectFrom, detectTo;
		int trackFrom, trackTo;

		BatchInfo() : bDetect(false), bTrack(false), bImage(false), bDetectRange(false),
			bTrackRange(false), bOverwrite(false), /*bValid(false), */bReadRgn(false),
			detectFrom(0), detectTo(0), trackFrom(0), trackTo(0) {}
	};

public:
	DlgBatchProcess(QWidget *parent = 0);
	~DlgBatchProcess();

private:
	Ui::DlgBatchProcessClass ui;

private:
	//QTimer m_timer;
	std::vector<BatchInfo> m_batchInfoList;
	QString m_baseDir;
	QIcon m_iconClock;
	QIcon m_iconWarning;
	QIcon m_iconDone;
	QIcon m_iconDisc;

private slots:
	void on_pushButtonAddInputFile_clicked();
	void on_pushButtonDeleteInputFile_clicked();
	void on_groupBoxDetection_toggled(bool bCheck);
	void on_groupBoxTracking_toggled(bool bCheck);
	void on_groupBoxMaskedImages_toggled(bool bCheck);
	void on_groupBoxReadRgn_toggled(bool bCheck);
	void on_groupBoxBaseDir_toggled(bool bCheck);

	void on_lineEditDetected_textChanged(const QString &text);
	void on_lineEditMaskedImages_textChanged(const QString &text);
	void on_checkBoxDetectRange_clicked(bool bCheck);
	void on_checkBoxTrackingRange_clicked(bool bCheck);
	void on_checkBoxOverwrite_clicked(bool bCheck);

	void on_lineEditDetectFrom_textChanged(const QString &text);
	void on_lineEditDetectTo_textChanged(const QString &text);
	void on_lineEditTrackingFrom_textChanged(const QString &text);
	void on_lineEditTrackingTo_textChanged(const QString &text);
	void on_lineEditReadRgn_textChanged(const QString &text);
	void on_lineEditBaseDir_textChanged(const QString &text);

	void on_pushButtonDetected_clicked();
	void on_pushButtonMaskedImages_clicked();
	void on_pushButtonReadRgn_clicked();
	void on_pushButtonBaseDir_clicked();

	void onItemClicked(QListWidgetItem *item);

	//void onTimer();
	void updateView();

private:
	void showEvent(QShowEvent *event);
	QString getRelFilePath(const QString &filePath);
	QString getRelFilePath(const QString &filePath, const QString &dirPath);
	bool checkDuplicateFilePath(const QString &filePath, bool allCheck);
	bool checkDuplicateFilePath();
	bool checkSameFilePath(const QString &filePath1, const QString &filePath2);
	QString getBaseDir(const QString &filePath);
	QString getBaseDir();
	QString getBaseName();

public:
	const std::vector<DlgBatchProcess::BatchInfo> &getBatchInfoList() {return m_batchInfoList;}
	QString getFilePath(const QString &relFilePath, int row);
};

#endif // DLGBATCHPROCESS_H
