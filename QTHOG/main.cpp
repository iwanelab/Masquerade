#include "qthog.h"
#include <QtGui/QApplication>
#include <QMessageBox>
#include <tchar.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shlwapi.h>

int main(int argc, char *argv[])
{
	MyApplication app(argc, argv);
	if (NULL == ::CreateMutex(NULL, FALSE, _T("Masquerade")) ||
		ERROR_ALREADY_EXISTS == ::GetLastError())
	{
		QMessageBox::critical(NULL, "Error Report", "Already running.");
		return 0;
	}

	QtHOG w;
	w.show();
	return app.exec();
}
