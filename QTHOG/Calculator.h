#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "utilities.h"
#include "RegionData.h"

#pragma warning(disable: 4819)

//class StructData;

class Calculator
{
public:
    Calculator();
	~Calculator();

private:
	StructData *m_pStructData;

public:
	void setStructData(StructData *pStructData) {pStructData->retain(); m_pStructData = pStructData;}
	MathUtil::Vec3d calcRegionPos(int object, int w, int h);
};

#endif // CALCULATOR_H
