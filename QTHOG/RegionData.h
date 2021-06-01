#ifndef STRUCTDATA_H
#define STRUCTDATA_H

#include <set>
#include <vector>
#include <map>
//#include "Utils.h"
//#include "TooN/TooN.h"
//#include "PTM/TMatrixUtility.h"
#include "utilities.h"
#include <opencv.hpp>
#include <QMutex>


struct SRegion
{
	short camera;
	int frame;
	int object;

	enum Type {TypeHog = 0, TypeFace, TypePlate, TypeManual, TypeNum} type;
	int flags;
	enum RegionFlag {FlagHasFace = 0x00000001, FlagTracked = 0x00000002, FlagManual = 0x00000004};

	CvRect rect;
	MathUtil::Mat2d cov;
	MathUtil::Vec2d u;

	SRegion() : camera(0), frame(0), object(0), type(TypeHog), flags(0) {u.clear(); cov.clear();}
	SRegion(short camera_, int frame_, int object_, Type type_, int flags_, CvRect rect_) :
	camera(camera_), frame(frame_), object(object_), type(type_), flags(flags_), rect(rect_) {u.clear(); cov.clear();}
};

struct SFrame
{
    std::map<std::pair<short, int>, int> regions;

	PTM::TVector<3, double> trans;
	PTM::TMatrixRow<3, 3, double> rot;

	enum Flags {FRAME_CV = 0x00000001};

    int flags;

    SFrame() : flags(0) {}
    
	void clearRegions() {regions.clear();}
};

struct SObject
{
	std::map<std::pair<short, int>, int> regions;
    int flags;

	SObject() : flags(0) {}
    void clearRegions() {regions.clear();}
};


class StructData
{
public:
    // イテレータ
	template <class T>
    class iterator
    {
    private:
		std::vector<T>* m_pData;
		const std::map<std::pair<short, int>, int> *m_pTracks;
		std::map<std::pair<short, int>, int>::iterator m_it;
    public:
		// コンストラクタ
		iterator() : m_pData(0), m_pTracks(0) {}

		// コンストラクタ
		iterator(std::vector<T>* pData, const std::map<std::pair<short, int>, int> *pTracks, std::map<std::pair<short, int>, int>::iterator it)
                : m_pData(pData), m_pTracks(pTracks), m_it(it) {}

        // コピーコンストラクタ
        iterator(const iterator &it)
                : m_pData(it.m_pData), m_pTracks(it.m_pTracks), m_it(it.m_it) {}

		// trackID取得
		int getID() const
		{
			if (!m_pData)
				return -1;
			if (m_it == (*m_pTracks).end())
				return -1;
			return m_it->second;
		}

        ///実体参照
		T& operator *() const
		{
			if (m_it->second < 0 || m_it->second >= (int)(*m_pData).size())
				return T();
			return (*m_pData)[m_it->second];
		}

		T* operator ->() const
		{
			return &(*m_pData)[m_it->second];
		}

		// ++オペレータ
        iterator& operator ++()
        {
            ++m_it;
            return *this;
        }

		// --オペレータ
        iterator& operator --()
        {
            --m_it;
            return *this;
        }

        // 後置++オペレータ
        iterator operator ++(int)
        {
            // 現在の位置を保存しておく
			std::map<std::pair<short, int>, int>::iterator it = m_it;
            ++(*this);
            return iterator(m_pData, m_pTracks, it);
        }

        // 後置--オペレータ
        iterator operator --(int)
        {
            // 現在の位置を保存しておく
			std::map<std::pair<short, int>, int>::iterator it = m_it;
            --(*this);
            return iterator(m_pData, m_pTracks, it);
        }

        ///比較演算子
        const bool operator !=(const iterator& it) const
        {
            if (!m_pData)
            {
                if (!it.m_pData)
                    return false;
                return (it.m_it != (*it.m_pTracks).end());
            }
            else
            {
                if (it.m_pData)
                    return (m_it != it.m_it);
                return (m_it != (*m_pTracks).end());
            }
        }

        const bool operator ==(const iterator& it) const
        {
            if (!m_pData)
            {
                if (!it.m_pData)
                    return true;
                return (it.m_it == (*it.m_pTracks).end());
            }
            else
            {
                if (it.m_pData)
                    return (m_it == it.m_it);
                return (m_it == (*m_pTracks).end());
            }
        }
		const bool operator <(const iterator& it) const
		{
			if ((!m_pData) || (!it.m_pData))
				return false;
			else
			{
				if (m_it->first.first == it.m_it->first.first)
					return (m_it->first.second < it.m_it->first.second);
				else
					return (m_it->first.first < it.m_it->first.first);
			}
		}
		const bool operator >(const iterator& it) const
		{
			if ((!m_pData) || (!it.m_pData))
				return false;
			else
			{
				if (m_it->first.first == it.m_it->first.first)
					return (m_it->first.second > it.m_it->first.second);
				else
					return (m_it->first.first > it.m_it->first.first);
			}
		}
    };

	// イテレータ
	template <class T>
	class reverse_iterator
	{
	private:
		std::vector<T>* m_pData;
		const std::map<std::pair<short, int>, int> *m_pTracks;
		std::map<std::pair<short, int>, int>::reverse_iterator m_it;
	public:
		// コンストラクタ
		reverse_iterator() : m_pData(0), m_pTracks(0) {}

		// コンストラクタ
		reverse_iterator(std::vector<T>* pData, const std::map<std::pair<short, int>, int> *pTracks, std::map<std::pair<short, int>, int>::reverse_iterator it)
				: m_pData(pData), m_pTracks(pTracks), m_it(it) {}

		// コピーコンストラクタ
		reverse_iterator(const reverse_iterator &it)
				: m_pData(it.m_pData), m_pTracks(it.m_pTracks), m_it(it.m_it) {}

		// trackID取得
		int getID() const
		{
			if (!m_pData)
				return -1;
			if (m_it == (*m_pTracks).rend())
				return -1;
			return m_it->second;
		}

		///実体参照
		T& operator *() const
		{
			if (m_it->second < 0 || m_it->second >= (int)(*m_pData).size())
				return T();
			return (*m_pData)[m_it->second];
		}

		T* operator ->() const
		{
			return &(*m_pData)[m_it->second];
		}

		// ++オペレータ
		reverse_iterator& operator ++()
		{
			++m_it;
			return *this;
		}

		// --オペレータ
		reverse_iterator& operator --()
		{
			--m_it;
			return *this;
		}

		// 後置++オペレータ
		reverse_iterator operator ++(int)
		{
			// 現在の位置を保存しておく
			std::map<std::pair<short, int>, int>::reverse_iterator it = m_it;
			++(*this);
			return reverse_iterator(m_pData, m_pTracks, it);
		}

		// 後置--オペレータ
		reverse_iterator operator --(int)
		{
			// 現在の位置を保存しておく
			std::map<std::pair<short, int>, int>::reverse_iterator it = m_it;
			--(*this);
			return reverse_iterator(m_pData, m_pTracks, it);
		}

		///比較演算子
		const bool operator !=(const reverse_iterator& it) const
		{
			if (!m_pData)
			{
				if (!it.m_pData)
					return false;
				return (it.m_it != (*it.m_pTracks).rend());
			}
			else
			{
				if (it.m_pData)
					return (m_it != it.m_it);
				return (m_it != (*m_pTracks).rend());
			}
		}

		const bool operator ==(const reverse_iterator& it) const
		{
			if (!m_pData)
			{
				if (!it.m_pData)
					return true;
				return (it.m_it == (*it.m_pTracks).rend());
			}
			else
			{
				if (it.m_pData)
					return (m_it == it.m_it);
				return (m_it == (*m_pTracks).rend());
			}
		}
	};

private:
	inline bool isFrameValid(short cam, int frame) const;
	inline bool isObjectValid(int object) const;
	inline bool isRegionValid(int region) const;

	std::set<int> emptyTracks;
	std::vector<SRegion> regions;
	std::set<int> emptyRegions;

	PTM::TVector<3, double> zeroTrans;
	PTM::TMatrixRow<3, 3, double> I;

	int m_referenceCount;

public: 
	QMutex mutex;

    StructData();
	StructData::iterator<SRegion> insertRegion(short cam, int frame, int object, CvRect rect, SRegion::Type type, int flags = 0);

	void clearRegions()
	{
		regions.clear();
		emptyRegions.clear();
		objects.clear();
		emptyObjects.clear();
		frames.clear();
	}

	iterator<SRegion> erase(StructData::iterator<SRegion> &it);
	void eraseObject(int object);

    void clearAll();

    iterator<SRegion> regionBegin(short cam, int frame);
    iterator<SRegion> regionBegin(int object);
	reverse_iterator<SRegion> regionRBegin(int object);
    iterator<SRegion> regionEnd();
    iterator<SRegion> regionFitAt(short cam, int frame, int object);
    iterator<SRegion> regionFitAt(int object);
    iterator<SRegion> regionOitAt(short cam, int frame, int object);

    void setTrans(short cam, int frame, const PTM::TVector<3, double> &t);
	void setRot(short cam, int frame, const PTM::TMatrixRow<3, 3, double> &r);
	void setPos(int point, const PTM::TVector<3, double> &pos);

    //void getCV(int cam, int frame, Util::SE3 &se3) const;
	const PTM::TVector<3, double> &getTrans(short cam, int frame) const;
	const PTM::TMatrixRow<3, 3, double> &getRot(short cam, int frame) const;

	// フラグアクセス
	void setFrameFlags(short cam, int frame, int flags);
	void setRegionFlags(const StructData::iterator<SRegion> &it, int flags);

	void clearFrameFlags(short cam, int frame, int flags);

	bool checkFrameFlags(short cam, int frame, int flags) const;
	bool checkRegionFlags(const StructData::iterator<SRegion> &it, int flags) const;

	// ファイル入出力
	int importDat(const char *filename, bool bVerticalY = false);
	int exportDat(const char *filename) const;
	int exportOldDat(const char *filename) const;

	int readRegions(const char *filename, int width, int height);
	int writeRegions(const char *filename, int width, int height);

	// フォーマット互換
	int importCam(const char *filename);
	//int importICV(const char *filename);

private:
    std::map<short, std::vector<SFrame> > frames;
	std::set<int> emptyPoints;
	std::vector<SObject> objects;
	std::set<int> emptyObjects;

public:
	const std::map<short, std::vector<SFrame> > getFrames() {return frames;}
	const std::vector<SObject>& getObjects() {return objects;}
	const std::set<int> getEmptyObjects() {return emptyObjects;}
	int getFrameCount(int cam);
	int getRegionCount(int cam, int frame);

	void retain() {m_referenceCount++;}
	void release() {if (--m_referenceCount <= 0) this->~StructData();}
};

#endif // STRUCTDATA_H
