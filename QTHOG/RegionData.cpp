#include "RegionData.h"
#include <QString>
//#include "CVDataFile.h"
#include <QDebug>
//#include "cvdata/CVDataFile.h"

StructData::StructData() :
	m_referenceCount(0)
{
	PTM::clear(zeroTrans);
	PTM::init_unitize(I);
}

bool StructData::isFrameValid(short cam, int frame) const
{
	if (frames.find(cam) == frames.end())
		return false;
	if (frame < 0 || const_cast<StructData*>(this)->frames[cam].size() <= frame)
		return false;

	return true;
}

bool StructData::isObjectValid(int object) const
{
	if (object < 0 || objects.size() <= object)
		return false;
	if (emptyObjects.find(object) != emptyObjects.end())
		return false;

	return true;
}

bool StructData::isRegionValid(int region) const
{
	if (region < 0 || regions.size() <= region)
		return false;
	if (emptyRegions.find(region) != emptyRegions.end())
		return false;

	return true;
}

StructData::iterator<SRegion> StructData::insertRegion(short cam, int frame, int object, CvRect rect, SRegion::Type type, int flags)
{
    // 既にデータがあるかチェック
	int newRegion;
    std::map<short, std::vector<SFrame> >::iterator itframe;
	std::map<std::pair<short, int>, int>::iterator itregion;

    itframe = frames.find(cam);
    if (object < 0)
    {
		newRegion = -1;
    }
    if (itframe == frames.end())
    {
		newRegion = -1;
    }
    else if (itframe->second.size() <= frame)
    {
		newRegion = -1;
    }
    else
    {
		itregion = itframe->second[frame].regions.find(std::pair<short, int>(0, object));
        if (itregion == itframe->second[frame].regions.end())
        {
			newRegion = -1;
        }
        else
        {
			newRegion = itregion->second;
        }
    }

	if (newRegion >= 0)
    {
		regions[newRegion].rect = rect;
		regions[newRegion].type = type;
		regions[newRegion].flags = flags;
    }
    else
    {
        // 新規登録
        if (object < 0)
        {
            if (!emptyObjects.empty())
            {
                object = *emptyObjects.begin();
				emptyObjects.erase(emptyObjects.begin());
            }
            else
                object = objects.size();
        }

//		int newTrackId;
        if (!emptyRegions.empty())
        {
            newRegion = *emptyRegions.begin();
			emptyRegions.erase(emptyRegions.begin());

            regions[newRegion].camera = cam;
            regions[newRegion].frame = frame;
            regions[newRegion].object = object;
            regions[newRegion].rect = rect;
            regions[newRegion].type = type;
            regions[newRegion].flags = flags;
        }
        else
        {
			regions.push_back(SRegion(cam, frame, object, type, flags, rect));
            newRegion = regions.size() - 1;
        }

        if (frames[cam].size() <= frame)
            frames[cam].resize(frame + 1);

		frames[cam][frame].regions[std::pair<short, int>(0, object)] = newRegion;

        if (object >= objects.size())
            objects.resize(object + 1);

		std::set<int>::iterator itp = emptyObjects.find(object);
		if (itp != emptyObjects.end())
			emptyObjects.erase(itp);
		objects[object].regions[std::pair<short, int>(cam, frame)] = newRegion;
    }
	return regionFitAt(newRegion);
}

StructData::iterator<SRegion> StructData::erase(StructData::iterator<SRegion> &it)
{
	iterator<SRegion> itNext = it;
	++itNext;

	int flag;
	if (itNext == regionEnd())
		flag = 0;	// 終端のイテレータ
	else if (itNext->object != it->object)
		flag = 1;	// フレーム内イテレータ
	else
		flag = 2;	// ポイント内イテレータ

	int nextCam;
	int nextFrame;
	int nextObject;

	if (flag)
	{
		nextCam = itNext->camera;
		nextFrame = itNext->frame;
		nextObject = itNext->object;
	}

	int regionID = it.getID();
	if (regionID < 0)
		return iterator<SRegion>();
	if (emptyRegions.find(regionID) != emptyRegions.end())
		return iterator<SRegion>();

	int frame = regions[regionID].frame;
	int object = regions[regionID].object;
	int camera = regions[regionID].camera;

	emptyRegions.insert(regionID);

	std::map<std::pair<short, int>, int>::iterator itRegion;
	itRegion = frames[camera][frame].regions.find(std::pair<short, int>(0, object));
	if (itRegion != frames[camera][frame].regions.end())
		frames[camera][frame].regions.erase(itRegion);

	itRegion = objects[object].regions.find(std::pair<short, int>(camera, frame));
	if (itRegion != objects[object].regions.end())
		objects[object].regions.erase(itRegion);

	if (objects[object].regions.empty())
		emptyObjects.insert(object);

	switch (flag)
	{
	case 0: return iterator<SRegion>();
	case 1: return regionFitAt(nextCam, nextFrame, nextObject);
	case 2: return regionOitAt(nextCam, nextFrame, nextObject);
	}
	return iterator<SRegion>();
}

void StructData::eraseObject(int object)
{
	StructData::iterator<SRegion> it;
	it = regionBegin(object);

	while (it != regionEnd())
		it = erase(it);

	emptyObjects.insert(object);
}

void StructData::clearAll()
{
    frames.clear();
	objects.clear();
    emptyTracks.clear();
    emptyPoints.clear();
	emptyObjects.clear();
}

int StructData::writeRegions(const char *filename, int width, int height)
{
	FILE *fp;
	fp = ::fopen(filename, "w");
	if (NULL == fp)
	{
		return -1;
	}

	int ret = 0;

	float fwidth = (float)width;
	float fheight = (float)height;

	for (unsigned int i = 0; i < objects.size(); ++i)
	{
		if (emptyObjects.find(i) != emptyObjects.end())
			continue;

		for (iterator<SRegion> it = regionBegin(i); it != regionEnd(); ++it)
		{
			CvRect &rect = it->rect;
			/*
			::fprintf(fp, "%d %d %d %d %d %d %d %f %f %f %f %f %f %d %d\n",
				i, it->camera, it->frame, rect.x, rect.y, rect.width, rect.height,
				//it->u[0], it->u[1], it->cov[0][0], it->cov[0][1], it->cov[1][0], it->cov[1][1],
				0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
				it->type, it->flags);
			*/
			float t = (float)it->u[0];
			float s = (float)it->u[1];

			::fprintf(fp, "%d %d %d %f %f %f %f %f %f %d %d\n",
				i, it->camera, it->frame, (float)rect.x / fwidth, (float)rect.y / fheight,
				(float)rect.width / fwidth, (float)rect.height / fheight, t, s,
				it->type, it->flags);

			ret++;
		}
	}
	::fclose(fp);

	return ret;
}

int StructData::readRegions(const char *filename, int width, int height)
{
    FILE *fp;
	fp = ::fopen(filename, "r");
    if (NULL == fp)
    {
        return -1;
    }

    int object, frame, cam;
	int type, flags;
	CvRect r;
    int ret = 0;
	//int lineCount;

	float fwidth = (float)width;
	float fheight = (float)height;

	float fx, fy, fw, fh, t, s;

	regions.clear();
	emptyRegions.clear();
	objects.clear();
	emptyObjects.clear();
	frames.clear();

	while (EOF != ::fscanf(fp, "%d %d %d %f %f %f %f %f %f %d %d",
		&object, &cam, &frame, &fx, &fy, &fw, &fh, &t, &s,
				&type, &flags))
    {
		r.x = fx * fwidth;
		r.y = fy * fheight;
		r.width = fw * fwidth;
		r.height = fh * fheight;

		//r.width = ((r.width + 3) / 4) * 4;

		if (r.x < 0)
			r.x = 0;
		else if (r.x + r.width >= width)
			r.x = width - r.width - 1;
		if (r.y < 0)
			r.y = 0;
		else if (r.y + r.height >= height)
			r.y = height - r.height - 1;

		iterator<SRegion> it = insertRegion(cam, frame, object, r, (SRegion::Type)type, flags);

		it->u[0] = t;
		it->u[1] = s;
        ++ret;
    }
    ::fclose(fp);

    return ret;
}


StructData::iterator<SRegion> StructData::regionBegin(short cam, int frame)
{
	if (!isFrameValid(cam, frame))
		return StructData::iterator<SRegion>();

	return StructData::iterator<SRegion>(&regions, &(const_cast<StructData*>(this)->frames[cam][frame].regions),
											const_cast<StructData*>(this)->frames[cam][frame].regions.begin());
}

StructData::iterator<SRegion> StructData::regionBegin(int object)
{
	if (!isObjectValid(object))
		return StructData::iterator<SRegion>();

	return StructData::iterator<SRegion>(&regions, &objects[object].regions, objects[object].regions.begin());
}

StructData::reverse_iterator<SRegion> StructData::regionRBegin(int object)
{
	if (!isObjectValid(object))
		return StructData::reverse_iterator<SRegion>();

	return StructData::reverse_iterator<SRegion>(&regions, &objects[object].regions, objects[object].regions.rbegin());
}

StructData::iterator<SRegion> StructData::regionFitAt(short cam, int frame, int object)
{
	if (!isFrameValid(cam, frame))
		return StructData::iterator<SRegion>();
	if (!isObjectValid(object))
		return StructData::iterator<SRegion>();

	std::map<std::pair<short, int>, int>::iterator it = frames[cam][frame].regions.find(std::pair<short, int>(0, object));
	if (it == frames[cam][frame].regions.end())
		return StructData::iterator<SRegion>();

	return StructData::iterator<SRegion>(&regions, &frames[cam][frame].regions, it);
}

StructData::iterator<SRegion> StructData::regionFitAt(int region)
{
	assert(region < regions.size());
	int cam = regions[region].camera;
	int frame = regions[region].frame;
	int object = regions[region].object;

	return regionFitAt(cam, frame, object);
}

StructData::iterator<SRegion> StructData::regionOitAt(short cam, int frame, int object)
{
	if (!isFrameValid(cam, frame))
		return StructData::iterator<SRegion>();
	if (!isObjectValid(object))
		return StructData::iterator<SRegion>();

	std::map<std::pair<short, int>, int>::iterator it = objects[object].regions.find(std::pair<short, int>(cam, frame));
	if (it == objects[object].regions.end())
		return StructData::iterator<SRegion>();

	return StructData::iterator<SRegion>(&regions, &objects[object].regions, it);
}

StructData::iterator<SRegion> StructData::regionEnd()
{
	return StructData::iterator<SRegion>();
}

void StructData::setTrans(short cam, int frame, const PTM::TVector<3, double> &t)
{
	if (frames[cam].size() <= frame)
		frames[cam].resize(frame + 1);

	frames[cam][frame].trans = t;
	frames[cam][frame].flags |= SFrame::FRAME_CV;
}

void StructData::setRot(short cam, int frame, const PTM::TMatrixRow<3, 3, double> &r)
{
	if (frames[cam].size() <= frame)
		frames[cam].resize(frame + 1);

	frames[cam][frame].rot = r;
	frames[cam][frame].flags |= SFrame::FRAME_CV;
}
/*
void StructData::getCV(int cam, int frame, Util::SE3 &se3) const
{
	if (!isFrameValid(cam, frame))
		return;

	se3 = const_cast<StructData*>(this)->frames[cam][frame].pose;
}
*/

const PTM::TVector<3, double> &StructData::getTrans(short cam, int frame) const
{
	if (frames.find(cam) == frames.end())
		return zeroTrans;
	if (const_cast<StructData*>(this)->frames[cam].size() <= frame)
		return zeroTrans;

	return const_cast<StructData*>(this)->frames[cam][frame].trans;
}

const PTM::TMatrixRow<3, 3, double> &StructData::getRot(short cam, int frame) const
{
	if (frames.find(cam) == frames.end())
		return I;
	if (const_cast<StructData*>(this)->frames[cam].size() <= frame)
		return I;

	return const_cast<StructData*>(this)->frames[cam][frame].rot;
}

// フラグアクセス
void StructData::setRegionFlags(const StructData::iterator<SRegion> &it, int flags)
{
	const int region = it.getID();
	if (!isRegionValid(region))
		return;
	regions[region].flags |= flags;
}

void StructData::setFrameFlags(short cam, int frame, int flags)
{
	if (!isFrameValid(cam, frame))
		return;
	frames[cam][frame].flags |= flags;
}

void StructData::clearFrameFlags(short cam, int frame, int flags)
{
	if (!isFrameValid(cam, frame))
		return;
	frames[cam][frame].flags &= ~flags;
}

bool StructData::checkRegionFlags(const StructData::iterator<SRegion> &it, int flags) const
{
	int region = it.getID();
	if (!isRegionValid(region))
		return false;
	return (regions[region].flags & flags) == flags;
}

bool StructData::checkFrameFlags(short cam, int frame, int flags) const
{
	if (!isFrameValid(cam, frame))
		return false;
	return (const_cast<StructData*>(this)->frames[cam][frame].flags & flags) == flags;
}

// ファイル入出力
int StructData::importDat(const char *filename, bool bVerticalY)
{
	// フラグのクリア
	std::map<short, std::vector<SFrame> >::iterator itc;
	for (itc = frames.begin(); itc != frames.end(); ++itc)
	{
		for (size_t frame = 0; frame < itc->second.size(); ++frame)
			itc->second[frame].flags = 0;
	}

	FILE *fp;
	fp = ::fopen(filename, "r");
	if (NULL == fp)
		return -1;

	// ファイル終わりまで読み込み
	int ret = 0;
	int fr = 0;
	int cam;
	int dummy;
	unsigned int flags;
	//double dt[3], dr[9];
	PTM::TVector<3, double> T;
	PTM::TMatrixRow<3, 3, double> R, M;

	M[0][0] = 1.0;	M[0][1] = 0.0;	M[0][2] = 0.0;
	M[1][0] = 0.0;	M[1][1] = 0.0;	M[1][2] = -1.0;
	M[2][0] = 0.0;	M[2][1] = 1.0;	M[2][2] = 0.0;

	while (EOF != ::fscanf(fp, "%d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
							 &cam, &fr, &flags, &T[0], &T[1], &T[2], &R[0][0], &R[1][0], &R[2][0],
							 &R[0][1], &R[1][1], &R[2][1], &R[0][2], &R[1][2], &R[2][2]))
	{
		//Util::SO3 so3(R);

		if (!bVerticalY)
		{
			setTrans(cam, fr, T);
			setRot(cam, fr, R);
		}
		else
		{
			setTrans(cam, fr, M * T);
			setRot(cam, fr, M * R);
		}

		++ret;
	}
	::fclose(fp);

	return ret;
}

// ファイル入出力
int StructData::importCam(const char *filename)
{
	// フラグのクリア
	std::map<short, std::vector<SFrame> >::iterator itc;
	for (itc = frames.begin(); itc != frames.end(); ++itc)
	{
		for (size_t frame = 0; frame < itc->second.size(); ++frame)
			itc->second[frame].flags = 0;
	}

	FILE *fp;
	fp = ::fopen(filename, "r");
	if (NULL == fp)
		return -1;

	// ファイル終わりまで読み込み
	int ret = 0;
	int fr = 0;

	PTM::TMatrixRow<3, 3, double> C;
	C[0][0] = 0.0;	C[0][1] = -1.0;	C[0][2] = 0.0;
	C[1][0] = 0.0;	C[1][1] = 0.0;	C[1][2] = 1.0;
	C[2][0] = -1.0;	C[2][1] = 0.0;	C[2][2] = 0.0;

	PTM::TVector<3, double> T;
//	Mat3d R;
	while (EOF != ::fscanf(fp, "%d %lf %lf %lf", &fr, &T[0], &T[1], &T[2]))
	{
		//Util::SO3 so3;
		//T = C.inv() * T;
		//setCV(0, fr, Util::SE3(so3, T));
		setTrans(0, fr, T);
		setRot(0, fr, I);
		++ret;
	}
	::fclose(fp);

	return ret;
}

int StructData::exportDat(const char *filename) const
{
	int ret = 0;
	/*
	FILE *fp;
	fp = ::fopen(filename, "w");
	if (NULL == fp)
		return -1;


	//double trans[3], rot[9];
	Util::SE3 pose;
	Util::Vec3d T;
	Util::Mat3d R;

	std::map<int, std::vector<SFrame> >::const_iterator it;
	for (it = frames.begin(); it != frames.end(); ++it)
	{
		int frameCount = const_cast<StructData*>(this)->frames[it->first].size();
		for (int i = 0; i < frameCount; ++i)
		{
			if (!checkFrameFlags(it->first, i, FRAME_CV))
				continue;

			getCV(it->first, i, pose);
			T = pose.get_translation();
			R = pose.get_rotation().get_matrix();
			::fprintf(fp, "%d %d %d %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f\n",
						it->first, i, it->second[i].flags, T[0], T[1], T[2], R[0][0], R[1][0], R[2][0], R[0][1], R[1][1], R[2][1], R[0][2], R[1][2], R[2][2]);
			ret++;
		}
	}
	::fclose(fp);
*/
	return ret;
}

int StructData::exportOldDat(const char *filename) const
{
	int ret = 0;
/*
	FILE *fp_trans, *fp_rot;

	QString fileNameTrans(filename), fileNameRot(filename);
	fileNameTrans.replace(".dat", "-trans.dat");
	fileNameRot.replace(".dat", "-rot.dat");

//	::fopen(&fp, filename, "w");
	fp_trans = ::fopen(fileNameTrans.toLocal8Bit(), "w");
	if (NULL == fp_trans)
		return -1;
	fp_rot = ::fopen(fileNameRot.toLocal8Bit(), "w");
	if (NULL == fp_rot)
		return -1;

	//double trans[3], rot[9];
	Util::SE3 pose;
	Util::Vec3d T;
	Util::Mat3d R, C;

	C[0][0] = 0.0;	C[0][1] = -1.0;	C[0][2] = 0.0;
	C[1][0] = 0.0;	C[1][1] = 0.0;	C[1][2] = 1.0;
	C[2][0] = -1.0;	C[2][1] = 0.0;	C[2][2] = 0.0;

	int frameCount = const_cast<StructData*>(this)->frames[0].size();
	for (int i = 0; i < frameCount; ++i)
	{
		if (i == 227)
			continue;

		int fr = (i > 227)? i - 1 : i;

		if (checkFrameFlags(0, i, FRAME_CV))
		{
			getCV(0, i, pose);
			T = pose.get_translation();
			R = pose.get_rotation().get_matrix();

			T = C * T;
			//R = C * R * C.inv();
		}
		::fprintf(fp_trans, "%d %18.18f %18.18f %18.18f\n", fr, T[0], T[1], T[2]);
		::fprintf(fp_rot, "%d %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f %18.18f\n",
					fr, R[0][0], R[1][0], R[2][0], R[0][1], R[1][1], R[2][1], R[0][2], R[1][2], R[2][2]);
		ret++;
	}
	::fclose(fp_trans);
	::fclose(fp_rot);
*/
	return ret;
}

/*
int StructData::importICV(const char *filename)
{
	CCVDataFile temp;
	if (!temp.readFromCVFile(filename)) return false; // 読み込みに失敗


	CCVData *pCVData = temp.getCVData(0);//contentIndex);
	if (pCVData == NULL) return false;	// content取得失敗

	std::set<int> registered;

	for (int i = 0; i < pCVData->getFrameNum(); i++) {
		const SCVFrameData &frameDataRef = pCVData->getCVData(i);
		//m_pFrameData->setTrans(FrameID(index, i), frameDataRef.trans.getPtr());
		//m_pFrameData->setRot(FrameID(index, i), frameDataRef.rot.getPtr());
		const double *trans, *rot;
		trans = frameDataRef.trans.getPtr();
		rot = frameDataRef.rot.getPtr();

		PTM::TMatrixRow<3, 3, double> R;
		R[0][0] = rot[0];	R[1][0] = rot[1];	R[2][0] = rot[2];
		R[0][1] = rot[3];	R[1][1] = rot[4];	R[2][1] = rot[5];
		R[0][2] = rot[6];	R[1][2] = rot[7];	R[2][2] = rot[8];
		
		PTM::TVector<3, double> T;
		T[0] = trans[0];	T[1] = trans[1];	T[2] = trans[2];
		setTrans(0, i, T);
		setRot(0, i, R);
	}
}
*/
int StructData::getFrameCount(int cam)
{
	if (frames.find(cam) == frames.end())
		return 0;
	return frames[cam].size();
}

int StructData::getRegionCount(int cam, int frame)
{
	if (cam < 0 || frame < 0)
		return 0;
	if (frames.find(cam) == frames.end())
		return 0;
	if (frames[cam].size() <= frame)
		return 0;
	return frames[cam][frame].regions.size();
}
