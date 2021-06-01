#ifndef __UTILS_H__
#define __UTILS_H__

#include "PTM/TMatrixUtility.h"
//#ifndef M_PI
//#define M_PI 3.14159265
//#endif
#define _USE_MATH_DEFINES
//#include <math.h>
#include <opencv.hpp>

namespace MathUtil
{
    typedef PTM::TVector<2, int> Vec2i;
    typedef PTM::TVector<3, int> Vec3i;
    typedef PTM::TVector<4, int> Vec4i;

    typedef PTM::TVector<2, float> Vec2f;
    typedef PTM::TVector<3, float> Vec3f;
    typedef PTM::TVector<4, float> Vec4f;

    typedef PTM::TVector<2, double> Vec2d;
    typedef PTM::TVector<3, double> Vec3d;
    typedef PTM::TVector<4, double> Vec4d;
    typedef PTM::TVector<4, double> Vec4d;
    typedef PTM::TVector<5, double> Vec5d;
    typedef PTM::TVector<6, double> Vec6d;

    typedef PTM::TMatrixRow<2, 2, double> Mat2d;
    typedef PTM::TMatrixRow<3, 3, double> Mat3d;
    typedef PTM::TMatrixRow<4, 4, double> Mat4d;
    typedef PTM::TMatrixRow<6, 6, double> Mat6d;

	inline Vec2i makeVector2i(int v0, int v1)
	{
		Vec2i ret;
		ret[0] = v0;	ret[1] = v1;
		return ret;
	}

	inline Vec3d makeVector3i(int v0, int v1, int v2)
	{
		Vec3i ret;
		ret[0] = v0;	ret[1] = v1;	ret[2] = v2;
		return ret;
	}

	inline Vec2d makeVector2d(double v0, double v1)
	{
		Vec2d ret;
		ret[0] = v0;	ret[1] = v1;
		return ret;
	}

	inline Vec3d makeVector3d(double v0, double v1, double v2)
	{
		Vec3d ret;
		ret[0] = v0;	ret[1] = v1;	ret[2] = v2;
		return ret;
	}

	inline Vec4d makeVector4d(double v0, double v1, double v2, double v3)
	{
		Vec4d ret;
		ret[0] = v0;	ret[1] = v1;	ret[2] = v2;	ret[3] = v3;
		return ret;
	}

	inline Vec5d makeVector5d(double v0, double v1, double v2, double v3, double v4)
	{
		Vec5d ret;
		ret[0] = v0;	ret[1] = v1;	ret[2] = v2;	ret[3] = v3;	ret[4] = v4;
		return ret;
	}

	inline Vec6d makeVector6d(double v0, double v1, double v2, double v3, double v4, double v5)
	{
		Vec6d ret;
		ret[0] = v0;	ret[1] = v1;	ret[2] = v2;	ret[3] = v3;	ret[4] = v4;	ret[5] = v5;
		return ret;
	}

	inline Vec3d unproject(Vec2d vec)
	{
		Vec3d ret;
		ret[0] = vec[0];
		ret[1] = vec[1];
		ret[2] = vec[2];
        
		return ret;
	}

	inline Vec2d project(Vec3d vec)
	{
		Vec2d ret;
		ret[0] = vec[0] / vec[2];
		ret[1] = vec[1] / vec[2];

		return ret;
	}

	inline Vec3d project(Vec4d vec)
	{
		Vec3d ret;
		ret[0] = vec[0] / vec[3];
		ret[1] = vec[1] / vec[3];
		ret[2] = vec[2] / vec[3];

		return ret;
	}

	inline Mat3d vvt(Vec3d a, Vec3d b)
	{
		Mat3d ret;
		ret[0][0] = a[0] * b[0];
		ret[1][0] = ret[0][1] = a[0] * b[1];
		ret[2][0] = ret[0][2] = a[0] * b[2];
		ret[1][1] = a[1] * b[1];
		ret[2][1] = ret[1][2] = a[1] * b[2];
		ret[2][2] = a[2] * a[2];

		return  ret;
	}

	inline Mat3d init_cross(const Vec3d& v)
	{
		Mat3d m;
		m[0][0] = 0;		m[0][1] = -v[2];	m[0][2] =  v[1];
		m[1][0] = v[2];		m[1][1] = 0;		m[1][2] = -v[0];
		m[2][0] = -v[1];	m[2][1] =  v[0];	m[2][2] = 0;
		return m;
	}

	inline double norm3(const Vec3d& v)
	{
		return sqrt(PTM::dot(v, v));
		
	}

	inline double norm2(const Vec2d& v)
	{
		return sqrt(PTM::dot(v, v));
	}

	inline CvPoint rectCenter(const CvRect &rect)
	{
		CvPoint pt;
		pt.x = rect.x + (rect.width >> 1);
		pt.y = rect.y + (rect.height >> 1);

		return pt;
	}

	inline bool isInRect(CvRect rect, int x, int y)
	{
		return (x >= rect.x && x <= rect.x+rect.width) && (y >= rect.y && y <= rect.y+rect.height);
	}

	inline int mod(int a, int b)
	{
		return a - static_cast<int>(floor((double)a / (double)b) * (double)b);
	}

	inline bool convertTo(double totalMiliSec, int& hour, int& min, int& sec, int& milisec)
	{
		if(totalMiliSec < 0)
		{
			return false;
		}

		int time = totalMiliSec;
		hour = time / (60*60*1000);
		{
			time -= hour * (60*60*1000);
		}
		min = time / (60*1000);
		{
			time -= min * (60*1000);
		}
		sec = time / (1000);
		{
			time -= sec * (1000);
		}
		milisec = time;
		
		return true;
	}

    Vec3d convertPix2Dir(double px, double py, int w, int h);
    Vec3d convertPix2Dir(double px, double py, int w, int h, double fov);
	Vec2d convertDir2Angle(MathUtil::Vec3d dir);
	Vec2d convertDir2Pix(MathUtil::Vec3d dir, int w, int h);
	Vec2d convertDir2Pix(MathUtil::Vec3d dir, int w, int h, double fov);

	template <class AD, class BD, class CD, class DD>
	void svd(PTM::MatrixImp<AD> &Mat, PTM::VectorImp<BD>& W, PTM::MatrixImp<CD> &U, PTM::MatrixImp<DD> &VT)
	{
		CvMat Mat_, W_, U_, VT_;

		cvInitMatHeader(&Mat_, Mat.height(), Mat.width(), CV_64FC1, Mat);
		cvInitMatHeader(&W_, W.size(), 1, CV_64FC1, W);
		cvInitMatHeader(&U_, U.height(), U.width(), CV_64FC1, U);
		cvInitMatHeader(&VT_, VT.height(), VT.width(), CV_64FC1, VT);

		cvSVD(&Mat_, &W_, &U_, &VT_, CV_SVD_V_T);
	}

	template <class AD, class BD, class CD>
	bool choleskySolve(PTM::MatrixImp<AD> &A, PTM::VectorImp<BD> &B, PTM::VectorImp<CD> &X)
	{
		cv::Mat cv_A(A.height(), A.width(), CV_64FC1, A[0]);
		cv::Mat cv_B(B.size(), 1, CV_64FC1, &B[0]);
		cv::Mat cv_X(X.size(), 1, CV_64FC1, &X[0]);

		return cv::solve(cv_A, cv_B, cv_X, cv::DECOMP_CHOLESKY);
	}

	void invertRT(const Mat3d &R, const Vec3d &T, Mat3d &invR, Vec3d &invT);

} // namespace MathUtil

#endif // UTILS_H