#pragma once
#include "RegionData.h"
#include <QGraphicsItem>

class CObjectColor
{
public:
	CObjectColor()
	{
		m_RegionColor[true][SRegion::TypeHog] = QColor(0,255,0);
		m_RegionColor[true][SRegion::TypeFace] = QColor(255,0,255);
		m_RegionColor[true][SRegion::TypePlate] = QColor(255,0,255);
		m_RegionColor[true][SRegion::TypeManual] = QColor(255,0,255);
		m_RegionColor[false][SRegion::TypeHog] = QColor(128,0,255);
		m_RegionColor[false][SRegion::TypeFace] = QColor(255,64,0);
		m_RegionColor[false][SRegion::TypePlate] = QColor(255,255,0);
		m_RegionColor[false][SRegion::TypeManual] = QColor(255,128,255);
	};
	~CObjectColor() {};
	QColor getQColor(bool isChecked, SRegion::Type type) { return m_RegionColor[isChecked][type];};
	
private:
	std::map<bool, std::map<SRegion::Type, QColor>> m_RegionColor;
};
