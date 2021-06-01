
// MFCSampleDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "MFCSample.h"
#include "MFCSampleDlg.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "poco/UnicodeConverter.h"
#include "Poco/Exception.h"
#include "poco/format.h"
#include "Poco/Util/WinRegistryKey.h"
#include <iostream>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// �A�v���P�[�V�����̃o�[�W�������Ɏg���� CAboutDlg �_�C�A���O

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

// ����
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMFCSampleDlg �_�C�A���O




CMFCSampleDlg::CMFCSampleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMFCSampleDlg::IDD, pParent)
	, m_URL(_T(""))
	, m_Response(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_URL, m_URL);
	DDX_Text(pDX, IDC_EDIT_RESPONSE, m_Response);
}

BEGIN_MESSAGE_MAP(CMFCSampleDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_SEND, &CMFCSampleDlg::OnBnClickedBtnSend)
END_MESSAGE_MAP()


// CMFCSampleDlg ���b�Z�[�W �n���h��

BOOL CMFCSampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "�o�[�W�������..." ���j���[���V�X�e�� ���j���[�ɒǉ����܂��B

	// IDM_ABOUTBOX �́A�V�X�e�� �R�}���h�͈͓̔��ɂȂ���΂Ȃ�܂���B
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���̃_�C�A���O�̃A�C�R����ݒ肵�܂��B�A�v���P�[�V�����̃��C�� �E�B���h�E���_�C�A���O�łȂ��ꍇ�A
	//  Framework �́A���̐ݒ�������I�ɍs���܂��B
	SetIcon(m_hIcon, TRUE);			// �傫���A�C�R���̐ݒ�
	SetIcon(m_hIcon, FALSE);		// �������A�C�R���̐ݒ�

	// TODO: �������������ɒǉ����܂��B
	Poco::Util::WinRegistryKey proxyEnable(HKEY_CURRENT_USER, 
		"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", true);
	if (proxyEnable.getInt("ProxyEnable"))
	{
		OutputDebugStringW(_T("Proxy Enable\r\n"));

		Poco::Util::WinRegistryKey proxyServer(HKEY_CURRENT_USER, 
			"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", true);
		if (proxyServer.exists())
		{
			std::string proxyServerName = proxyServer.getString("ProxyServer");
			Poco::URI proxyURI(proxyServerName);
			OutputDebugStringA("Proxy Host : ");
			OutputDebugStringA(proxyURI.getScheme().c_str());
			OutputDebugStringA("\r\n");
			std::wstring proxyHost;
			Poco::UnicodeConverter::toUTF16(proxyURI.getScheme(), proxyHost);
			GetDlgItem(IDC_EDIT_PROXY_SERVER)->SetWindowTextW(proxyHost.c_str());

			OutputDebugStringA("Proxy Port : ");
			OutputDebugStringA(proxyURI.getPath().c_str());
			OutputDebugStringA("\r\n");
			std::wstring proxyPort;
			Poco::UnicodeConverter::toUTF16(proxyURI.getPath(), proxyPort);
			GetDlgItem(IDC_EDIT_PROXY_PORT)->SetWindowTextW(proxyPort.c_str());

//			OutputDebugStringA(Poco::format("Proxy Port : %h\r\n", proxyURI.getPort()).c_str());
		}
	}
	else
	{
		OutputDebugStringW(_T("Proxy Disable\r\n"));
	}

	return TRUE;  // �t�H�[�J�X���R���g���[���ɐݒ肵���ꍇ�������ATRUE ��Ԃ��܂��B
}

void CMFCSampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �_�C�A���O�ɍŏ����{�^����ǉ�����ꍇ�A�A�C�R����`�悷�邽�߂�
//  ���̃R�[�h���K�v�ł��B�h�L�������g/�r���[ ���f�����g�� MFC �A�v���P�[�V�����̏ꍇ�A
//  ����́AFramework �ɂ���Ď����I�ɐݒ肳��܂��B

void CMFCSampleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �`��̃f�o�C�X �R���e�L�X�g

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// �N���C�A���g�̎l�p�`�̈���̒���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �A�C�R���̕`��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ���[�U�[���ŏ��������E�B���h�E���h���b�O���Ă���Ƃ��ɕ\������J�[�\�����擾���邽�߂ɁA
//  �V�X�e�������̊֐����Ăяo���܂��B
HCURSOR CMFCSampleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMFCSampleDlg::OnBnClickedBtnSend()
{
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	UpdateData(TRUE);
	m_Response.Empty();
	try
	{
		std::wstring wuri = static_cast<LPCTSTR>(m_URL);
		std::string suri;
		Poco::UnicodeConverter::toUTF8(wuri, suri);
		Poco::URI uri(suri);
		std::string path(uri.getPathAndQuery());
		if (path.empty()) 
			path = "/";

		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		if (static_cast<CButton *>(GetDlgItem(IDC_CHECK_USE_PROXY))->GetCheck())
		{
			CString cstr;
			GetDlgItem(IDC_EDIT_PROXY_SERVER)->GetWindowTextW(cstr);
			std::wstring wstr;
			wstr.assign(static_cast<LPCTSTR>(cstr));

			std::string proxyHost;
			Poco::UnicodeConverter::toUTF8(wstr, proxyHost);

			GetDlgItem(IDC_EDIT_PROXY_PORT)->GetWindowTextW(cstr);
			wstr.assign(static_cast<LPCTSTR>(cstr));

			session.setProxy(proxyHost, _ttoi(wstr.c_str()));
		}
		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path/*, Poco::Net::HTTPMessage::HTTP_1_1*/);
		session.sendRequest(req);
		Poco::Net::HTTPResponse res;
		std::istream& rs = session.receiveResponse(res);
		std::string sres;
//		sres = Poco::format("Http Status : %d ", (int)res.getStatus());
//		sres += res.getReason();
		std::wstring wres;
//		Poco::UnicodeConverter::toUTF16(sres, wres);
//		m_Response = wres.c_str();
		Poco::StreamCopier::copyToString(rs, sres);
		Poco::UnicodeConverter::toUTF16(sres, wres);
		m_Response += _T("\r\n");
		m_Response += wres.c_str();

//		std::cout << res.getStatus() << " " << res.getReason() << std::endl;
//		StreamCopier::copyStream(rs, std::cout);
	}
	catch (Poco::Exception& exc)
	{
		std::wstring wstr;
		Poco::UnicodeConverter::toUTF16(exc.displayText(), wstr);
		m_Response = wstr.c_str();
//		std::cerr << exc.displayText() << std::endl;
	}

	UpdateData(FALSE);
}
