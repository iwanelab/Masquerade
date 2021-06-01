#include "utilities.h"

MathUtil::Vec3d MathUtil::convertPix2Dir(double px, double py, int w, int h)
{
	double theta, phi, c_th, s_th, c_ph, s_ph;
	Vec3d dir;

	theta = (2. * M_PI) * px / w;
	phi = M_PI * py / h;
	c_th = cos(theta);
	s_th = sin(theta);
	c_ph = cos(phi);
	s_ph = sin(phi);

	dir[0] = -s_ph * s_th;
	dir[1] = c_ph;
	dir[2] = s_ph * c_th;

	return dir;
}

MathUtil::Vec3d MathUtil::convertPix2Dir(double px, double py, int w, int h, double fov)
{
	double forcalLength = (double)w/2/tan(fov/2);
	Vec3d dir;
	dir[0] = px - (double)w/2;
	dir[1] = (double)h/2 - py;
	dir[2] = -forcalLength;

	return dir;
}

MathUtil::Vec2d MathUtil::convertDir2Angle(MathUtil::Vec3d dir)
{
	MathUtil::Vec2d angle;
	angle[0] = atan2(-dir[0], dir[2]);
	angle[1] = atan2(sqrt(dir[0] * dir[0] + dir[2] * dir[2]), dir[1]);

	return angle;
}

MathUtil::Vec2d MathUtil::convertDir2Pix(MathUtil::Vec3d dir, int w, int h)
{
	MathUtil::Vec2d pix;
	pix[0] = atan2(-dir[0], dir[2]) * w / (2. * M_PI);
	if (pix[0] < 0)	pix[0] += w;
	pix[1] = atan2(sqrt(dir[0] * dir[0] + dir[2] * dir[2]), dir[1]) * h / M_PI;

	return pix;
}

MathUtil::Vec2d MathUtil::convertDir2Pix(MathUtil::Vec3d dir, int w, int h, double fov)
{
	if (dir[2] >= 0)
		return MathUtil::makeVector2d(-1,-1);
	double forcalLength = (double)w/2/tan(fov/2);
	MathUtil::Vec2d pix;
	pix[0] = (dir[0]*forcalLength/dir[2]*-1) + (double)w/2;
	pix[1] = (double)h/2 - (dir[1]*forcalLength/dir[2]*-1);

	return pix;
}

void MathUtil::invertRT(const Mat3d &R, const Vec3d &T, Mat3d &invR, Vec3d &invT)
{
	invR = R.trans();
	invT = -invR * T;
}

