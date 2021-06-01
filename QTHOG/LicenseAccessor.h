#pragma once
#include "MovieAccessor.h"

class CLicenseAccessor
{
private:
	typedef bool *(*typeCheckLicense)(LPCWSTR szLicenseFile);
	typeCheckLicense m_checkLicense;
	HMODULE m_MovieLoader;
public:
	CLicenseAccessor(void);
	~CLicenseAccessor(void);
	
	bool checkLicense(LPCWSTR szLicenseFile) { return m_checkLicense(szLicenseFile);};
	bool isEnable() {return m_checkLicense != NULL;};
};
