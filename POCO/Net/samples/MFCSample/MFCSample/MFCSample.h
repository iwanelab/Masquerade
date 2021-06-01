
// MFCSample.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CMFCSampleApp:
// このクラスの実装については、MFCSample.cpp を参照してください。
//

class CMFCSampleApp : public CWinAppEx
{
public:
	CMFCSampleApp();

// オーバーライド
	public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CMFCSampleApp theApp;