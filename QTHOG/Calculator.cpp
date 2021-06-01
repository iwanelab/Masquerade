#include "Calculator.h"
#include "RegionData.h"

Calculator::Calculator() :
	m_pStructData(0)
{

}

Calculator::~Calculator()
{
	if (m_pStructData)
		m_pStructData->release();
}

static MathUtil::Vec3d calcPos(const std::vector<MathUtil::Vec3d> &transList, const std::vector<MathUtil::Mat3d> &rotList, const std::vector<MathUtil::Vec3d> &v)
{
    int numMeas = transList.size();

	PTM::TMatrixRow<3, 4, double> PDash;
	PTM::TMatrixRow<4, 4, double> VT;
	PTM::VMatrixRow<double> A, U;
    A.resize(numMeas * 2, 4);
    U.resize(numMeas * 2, numMeas * 2);

    for (size_t i = 0; i < numMeas; ++i)
    {
        PDash.vsub_matrix(0, 0, 3, 3) = rotList[i];
        PDash.col(3) = transList[i];

        A.row(2 * i)		= v[i][0] * PDash.row(2) - v[i][2] * PDash.row(0);
        A.row(2 * i + 1)	= v[i][1] * PDash.row(2) - v[i][2] * PDash.row(1);
    }

    MathUtil::Vec4d W;

    MathUtil::svd(A, W, U, VT);
    MathUtil::Vec4d v4Smallest = VT.row(3);
    if(v4Smallest[3] == 0.0)
        v4Smallest[3] = 0.00001;

    MathUtil::Vec3d pos;
    pos[0] = v4Smallest[0] / v4Smallest[3];
    pos[1] = v4Smallest[1] / v4Smallest[3];
    pos[2] = v4Smallest[2] / v4Smallest[3];

    return pos;
}

MathUtil::Vec3d Calculator::calcRegionPos(int object, int w, int h)
{
	StructData::iterator<SRegion> itr;

	std::vector<MathUtil::Mat3d> rList;
	std::vector<MathUtil::Vec3d> tList;
    std::vector<MathUtil::Vec3d> vList;

	MathUtil::Mat3d invR;
	MathUtil::Vec3d invT;

	for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
	{
		MathUtil::invertRT(m_pStructData->getRot(itr->camera, itr->frame), m_pStructData->getTrans(itr->camera, itr->frame), invR, invT);

		rList.push_back(invR);
		tList.push_back(invT);
	
		double cx = itr->rect.x + itr->rect.width / 2;
		double cy = itr->rect.y + itr->rect.height / 2;

		vList.push_back(MathUtil::convertPix2Dir(cx, cy, w, h));
    }

    return calcPos(tList, rList, vList);
}

