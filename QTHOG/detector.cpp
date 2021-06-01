#include "detector.h"
#include <QDebug>

static uchar lbpPattern[] = {0x00,
								0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,	// 1
								0x03, 0x06, 0xc0, 0x18, 0x30, 0x60, 0xc0, 0x81,	// 2
								0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc1, 0x83,	// 3
								0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xe1, 0xc3, 0x87,	// 4
								0x1f, 0x3e, 0x7c, 0xf8, 0xf1, 0xe3, 0xc7, 0x8f,	// 5
								0x3f, 0x7e, 0xfc, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f,	// 6
								0x7f, 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf,	// 7
								0xff};

static std::map<uchar, uchar> mapLbpPattern;

void calcHog(int blockWidth, int blockHeight, int numBlockRows, int numBlockCols, int offset,
			 const cv::Mat &gradDir, const cv::Mat &gradMag, std::vector<float> &feat)
{
	int cellWidth = blockWidth / 2;
	int cellHeight = blockHeight / 2;

	for (int blockY = 0; blockY < numBlockRows; ++blockY)
	{
		for (int blockX = 0; blockX < numBlockCols; ++blockX)
		{
			int blockOffset = offset + 36 * (blockY * numBlockCols + blockX);

			//float blockNorm = 0;
			for (int cellY = 0; cellY < 2; ++cellY)
			{
				for (int cellX = 0; cellX < 2; ++cellX)
				{
					int cellOffset = blockOffset + 9 * (cellY * 2 + cellX);

					for (int y = 0; y < cellHeight; ++y)
					{
						int yy = blockY * blockHeight + cellY * cellHeight + y;
						for (int x = 0; x < cellWidth; ++x)
						{
							int xx = blockX * blockWidth + cellX * cellWidth + x;
							uchar bin = gradDir.at<uchar>(yy, xx);
							if (bin < 9)
							{
								float mag = gradMag.at<float>(yy, xx);
								feat[cellOffset + bin] += mag;
							}
						}
					}
				}
			}
			
			// ブロックごとの正規化
			float blockNorm = 0;
			for (int i = 0; i < 36; ++i)
				blockNorm += feat[blockOffset + i] * feat[blockOffset + i];
			
			double k = sqrt(blockNorm + 1.0);

			for (int i = 0; i < 36; ++i)
				feat[blockOffset + i] /= (float)k;
		}
	}
}

void calcHog2(int blockWidth, int blockHeight, int numBlockRows, int numBlockCols, int offset,
			 const cv::Mat &gradDir, const cv::Mat &gradMag, std::vector<float> &feat)
{
	int cellWidth = blockWidth / 2;
	int cellHeight = blockHeight / 2;

	for (int blockY = 0; blockY < numBlockRows; ++blockY)
	{
		for (int blockX = 0; blockX < numBlockCols; ++blockX)
		{
			int blockOffset = offset + 36 * (blockY * numBlockCols + blockX);

			//float blockNorm = 0;
			for (int cellY = 0; cellY < 2; ++cellY)
			{
				for (int cellX = 0; cellX < 2; ++cellX)
				{
					int cellOffset = blockOffset + 9 * (cellY * 2 + cellX);

					for (int y = 0; y < cellHeight; ++y)
					{
						int yy = blockY * blockHeight + cellY * cellHeight + y;
						for (int x = 0; x < cellWidth; ++x)
						{
							int xx = blockX * blockWidth + cellX * cellWidth + x;
							uchar bin = gradDir.at<uchar>(yy, xx);
							if (bin < 9)
							{
								float mag = gradMag.at<float>(yy, xx);

								uchar prevBin = (bin == 0) ? 8 : bin - 1;
								uchar nextBin = (bin == 8) ? 0 : bin + 1;

								feat[cellOffset + bin] += mag;

								feat[cellOffset + prevBin] += mag * 0.5f;
								feat[cellOffset + nextBin] += mag * 0.5f;
							}
						}
					}
				}
			}
			
			// ブロックごとの正規化
			float blockNorm = 0;
			for (int i = 0; i < 36; ++i)
				blockNorm += feat[blockOffset + i] * feat[blockOffset + i];
			
			double k = sqrt(blockNorm + 1.0);

			for (int i = 0; i < 36; ++i)
				feat[blockOffset + i] /= (float)k;
		}
	}
}

void normalizeLBP(int blockWidth, int blockHeight, int numBlockRows, int numBlockCols, int offset,
			 const cv::Mat &lbpMat, std::vector<float> &feat)
{
	//int cellWidth = blockWidth / 2;
	//int cellHeight = blockHeight / 2;

	for (int blockY = 0; blockY < numBlockRows; ++blockY)
	{
		for (int blockX = 0; blockX < numBlockCols; ++blockX)
		{
			int blockOffset = offset + 59 * (blockY * numBlockCols + blockX);

			for (int y = 0; y < blockHeight; ++y)
			{
				int yy = blockY * blockHeight + y;
				for (int x = 0; x < blockWidth; ++x)
				{
					int xx = blockX * blockWidth + x;
					uchar bin = lbpMat.at<uchar>(yy, xx);
					if (bin < 59)
					{
						//float mag = gradMag.at<float>(yy, xx);
						feat[blockOffset + bin] += 1.0f;
					}
				}
			}
			
			// ブロックごとの正規化
			float blockNorm = 0;
			for (int i = 0; i < 59; ++i)
				blockNorm += feat[blockOffset + i] * feat[blockOffset + i];
			
			double k = sqrt(blockNorm + 1.0);

			for (int i = 0; i < 59; ++i)
				feat[blockOffset + i] /= (float)k;
		}
	}
}

void normalizeLBP2(int blockWidth, int blockHeight, int numBlockRows, int numBlockCols, int offset,
			 const cv::Mat &lbpMat, std::vector<float> &feat)
{
	//int cellWidth = blockWidth / 2;
	//int cellHeight = blockHeight / 2;

	for (int blockY = 0; blockY < numBlockRows; ++blockY)
	{
		for (int blockX = 0; blockX < numBlockCols; ++blockX)
		{
			int blockOffset = offset + 59 * (blockY * numBlockCols + blockX);

			for (int y = 0; y < blockHeight; ++y)
			{
				int yy = blockY * blockHeight + y;
				for (int x = 0; x < blockWidth; ++x)
				{
					int xx = blockX * blockWidth + x;
					uchar bin = lbpMat.at<uchar>(yy, xx);
					if (bin < 59)
					{
						//float mag = gradMag.at<float>(yy, xx);
						feat[blockOffset + bin] += 1.0f;

						if (bin != 0 && bin < 57)
						{
							//qDebug() << "bin" << bin;

							// Interpolated Voteのテスト
							uchar rot = (bin - 1) % 8;
							uchar prevBin = (rot == 0) ? bin + 7 : bin - 1;
							uchar nextBin = (rot == 7) ? bin - 7 : bin + 1;

							feat[blockOffset + prevBin] += 0.5f;
							feat[blockOffset + nextBin] += 0.5f;

							if (bin < 49)
								feat[blockOffset + bin + 8] += 0.3f;
							if (bin > 8)
								feat[blockOffset + bin - 8] += 0.3f;
						}
					}
				}
			}
			
			// ブロックごとの正規化
			float blockNorm = 0;
			for (int i = 0; i < 59; ++i)
				blockNorm += feat[blockOffset + i] * feat[blockOffset + i];
			
			double k = sqrt(blockNorm + 1.0);

			for (int i = 0; i < 59; ++i)
				feat[blockOffset + i] /= (float)k;
		}
	}
}

/************************************
	HoG特徴量を計算
	img：グレースケール画像
	feat：計算された特徴量が入る
*************************************/
void Detector::getHoG(cv::Mat &src, std::vector<float> &feat)
{
	cv::Mat resizeSrc;

	// 画像サイズを変換
	if (src.cols != TemplateWidth || src.rows != TemplateHeight)
		cv::resize(src, resizeSrc, cv::Size(TemplateWidth, TemplateHeight));
	else
		resizeSrc = src;

	// 輝度勾配計算
	cv::Mat gradDir(TemplateWidth, TemplateHeight, CV_8UC1, cv::Scalar(100));
	cv::Mat gradMag(TemplateWidth, TemplateHeight, CV_32FC1, cv::Scalar(0));

	for (int y = 1; y < TemplateHeight - 1; ++y)
	{
		for (int x = 1; x < TemplateWidth - 1; ++x)
		{
			float dx = (float)(resizeSrc.at<uchar>(y, x + 1) - resizeSrc.at<uchar>(y, x - 1));
			float dy = (float)(resizeSrc.at<uchar>(y + 1, x) - resizeSrc.at<uchar>(y - 1, x));

			if (dx < 0)
			{
				dx = -dx;
				dy = -dy;
			}
			uchar bin = 0;
			if (dx > FLT_MIN)
			{
				float tanVal = dx / dy;
				
				if (tanVal > 0.1763f)	// tan10
				{
					if (tanVal > 1.1918f)	// tan50
					{
						if (tanVal > 2.7475f)	// tan70
							bin = 0;
						else
							bin = 1;
					}
					else
					{
						if (tanVal > 0.5774f)	// tan30
							bin = 2;
						else
							bin = 3;
					}
				}
				else
				{
					if (tanVal > -0.5774f)	// -tan30
					{
						if (tanVal > -0.1763)	// -tan10
							bin = 4;
						else
							bin = 5;
					}
					else
					{
						if (tanVal > -1.1918f)	// -tan50
							bin = 6;
						else if (tanVal > -2.7475f)	// -tan70
							bin = 7;
						else
							bin = 8;
					}
				}
			}	// if (dx > FLT_MIN)
			gradDir.at<uchar>(y, x) = bin;
			gradMag.at<float>(y, x) = sqrt(dx * dx + dy * dy);
		}	// for (int x = 1; x < TemplateWidth - 1; ++x)
	}	// for (int y = 1; y < TemplateHeight - 1; ++y)

	/*
	feat.resize(3024, 0);
	int offset[] = {0, 2304, 2880};
	// Level0ヒストグラム作成
	int blockHeight[] = {6, 12, 24};
	int blockWidth[] = {6, 12, 24};

	for (int level =0; level < 3; ++level)
	{
		int numBlockRows = TemplateHeight / blockHeight[level];
		int numBlockCols = TemplateWidth / blockWidth[level];

		calcHog(blockWidth[level], blockHeight[level], numBlockRows, numBlockCols, offset[level], gradDir, gradMag, feat);
	}
	*/
	feat.resize(1620, 0);
	int offset[] = {0, 1296};
	// Level0ヒストグラム作成
	int blockHeight[] = {8, 16};
	int blockWidth[] = {8, 16};

	for (int level = 0; level < 2; ++level)
	{
		int numBlockRows = TemplateHeight / blockHeight[level];
		int numBlockCols = TemplateWidth / blockWidth[level];

		calcHog2(blockWidth[level], blockHeight[level], numBlockRows, numBlockCols, offset[level], gradDir, gradMag, feat);
	}
}

void Detector::getLBP(cv::Mat &src, std::vector<float> &feat)
{
	cv::Mat resizeSrc;

	if (mapLbpPattern.empty())
	{
		// パターンのマップを初期化
		for (uchar i = 0; i < 58; ++i)
			mapLbpPattern[lbpPattern[i]] = i;
	}

	// 画像サイズを変換
	if (src.cols != TemplateWidth || src.rows != TemplateHeight)
		cv::resize(src, resizeSrc, cv::Size(TemplateWidth, TemplateHeight));
	else
		resizeSrc = src;

	// 輝度勾配計算
	cv::Mat lbpMat(TemplateWidth, TemplateHeight, CV_8UC1, cv::Scalar(100));

	uchar pixels[9];
	for (int y = 1; y < TemplateHeight - 1; ++y)
	{
		for (int x = 1; x < TemplateWidth - 1; ++x)
		{
			pixels[0] = src.at<uchar>(y - 1, x);
			pixels[1] = src.at<uchar>(y - 1, x + 1);
			pixels[2] = src.at<uchar>(y, x + 1);
			pixels[3] = src.at<uchar>(y + 1, x + 1);
			pixels[4] = src.at<uchar>(y + 1, x);
			pixels[5] = src.at<uchar>(y + 1, x - 1);
			pixels[6] = src.at<uchar>(y, x - 1);
			pixels[7] = src.at<uchar>(y - 1, x - 1);
			pixels[8] = src.at<uchar>(y, x);

			uchar pattern(0x00), orPattern(0x01);
			for (uchar j = 0; j < 8; ++j)
			{
				if (pixels[j] > pixels[8])
					pattern |= orPattern;
				orPattern <<= 1;
			}
			if (mapLbpPattern.find(pattern) != mapLbpPattern.end())
				lbpMat.at<uchar>(y, x) = mapLbpPattern[pattern];
			else
				lbpMat.at<uchar>(y, x) = 58;
		}	// for (int x = 1; x < TemplateWidth - 1; ++x)
	}	// for (int y = 1; y < TemplateHeight - 1; ++y)

	/*
	feat.resize(4956, 0);
	int offset[] = {0, 3776, 4720};
	// Level0ヒストグラム作成
	int blockHeight[] = {6, 12, 24};
	int blockWidth[] = {6, 12, 24};

	for (int level =0; level < 3; ++level)
	{
		int numBlockRows = TemplateHeight / blockHeight[level];
		int numBlockCols = TemplateWidth / blockWidth[level];

		normalizeLBP(blockWidth[level], blockHeight[level], numBlockRows, numBlockCols, offset[level], lbpMat, feat);
	}
	*/
	feat.resize(2655, 0);
	int offset[] = {0, 2124};
	// Level0ヒストグラム作成
	int blockHeight[] = {8, 16};
	int blockWidth[] = {8, 16};

	for (int level = 0; level < 2; ++level)
	{
		int numBlockRows = TemplateHeight / blockHeight[level];
		int numBlockCols = TemplateWidth / blockWidth[level];

		normalizeLBP2(blockWidth[level], blockHeight[level], numBlockRows, numBlockCols, offset[level], lbpMat, feat);
	}
}

void Detector::getHoG_partial(cv::Mat &gradDir, cv::Mat &gradMag, int x, int y, std::vector<float> &feat)
{
	//feat.resize(3024, 0);

	int xx = x - TemplateHalfWidth;
	int yy = y - TemplateHalfHeight;

	if (xx < 0 || xx + TemplateWidth >= gradDir.cols)
		return;
	if (yy < 0 || y + TemplateHeight >= gradDir.rows)
		return;

	cv::Mat partialDir = gradDir(cv::Rect(xx, yy, TemplateWidth, TemplateHeight)).clone();
	cv::Mat partialMag = gradMag(cv::Rect(xx, yy, TemplateWidth, TemplateHeight)).clone();

	feat.resize(1620, 0);
	int offset[] = {0, 1296};
	// Level0ヒストグラム作成
	int blockHeight[] = {8, 16};
	int blockWidth[] = {8, 16};

	for (int level = 0; level < 2; ++level)
	{
		int numBlockRows = TemplateHeight / blockHeight[level];
		int numBlockCols = TemplateWidth / blockWidth[level];

		calcHog2(blockWidth[level], blockHeight[level], numBlockRows, numBlockCols, offset[level], gradDir, gradMag, feat);
	}
}

void Detector::getLBP_partial(cv::Mat &lbpMat, int x, int y, std::vector<float> &feat)
{
	int xx = x - TemplateHalfWidth;
	int yy = y - TemplateHalfHeight;

	if (xx < 0 || xx + TemplateWidth >= lbpMat.cols)
		return;
	if (yy < 0 || y + TemplateHeight >= lbpMat.rows)
		return;

	cv::Mat partialLBP = lbpMat(cv::Rect(xx, yy, TemplateWidth, TemplateHeight)).clone();

	feat.resize(2655, 0);
	int offset[] = {0, 2124};
	// Level0ヒストグラム作成
	int blockHeight[] = {8, 16};
	int blockWidth[] = {8, 16};

	for (int level = 0; level < 2; ++level)
	{
		int numBlockRows = TemplateHeight / blockHeight[level];
		int numBlockCols = TemplateWidth / blockWidth[level];

		normalizeLBP2(blockWidth[level], blockHeight[level], numBlockRows, numBlockCols, offset[level], partialLBP, feat);
	}
}

void Detector::extractLBP(cv::Mat &src, cv::Mat &lbpMat)
{
	cv::Mat resizeSrc;

	if (mapLbpPattern.empty())
	{
		// パターンのマップを初期化
		for (uchar i = 0; i < 58; ++i)
			mapLbpPattern[lbpPattern[i]] = i;
	}

	// 画像サイズを変換
	if (src.cols != TemplateWidth || src.rows != TemplateHeight)
		cv::resize(src, resizeSrc, cv::Size(TemplateWidth, TemplateHeight));
	else
		resizeSrc = src;

	// 輝度勾配計算
	//cv::Mat lbpMat(TemplateWidth, TemplateHeight, CV_8UC1, cv::Scalar(100));

	uchar pixels[9];
	lbpMat = cv::Mat(src.rows, src.cols, CV_8UC1, cv::Scalar(100));
	//gradMag = cv::Mat(src.rows, src.cols, CV_32FC1, cv::Scalar(0));

	for (int y = 1; y < src.rows - 1; ++y)
	{
		for (int x = 1; x < src.cols - 1; ++x)
		{
			pixels[0] = src.at<uchar>(y - 1, x);
			pixels[1] = src.at<uchar>(y - 1, x + 1);
			pixels[2] = src.at<uchar>(y, x + 1);
			pixels[3] = src.at<uchar>(y + 1, x + 1);
			pixels[4] = src.at<uchar>(y + 1, x);
			pixels[5] = src.at<uchar>(y + 1, x - 1);
			pixels[6] = src.at<uchar>(y, x - 1);
			pixels[7] = src.at<uchar>(y - 1, x - 1);
			pixels[8] = src.at<uchar>(y, x);

			uchar pattern(0x00), orPattern(0x01);
			for (uchar j = 0; j < 8; ++j)
			{
				if (pixels[j] > pixels[8])
					pattern |= orPattern;
				orPattern <<= 1;
			}
			if (mapLbpPattern.find(pattern) != mapLbpPattern.end())
				lbpMat.at<uchar>(y, x) = mapLbpPattern[pattern];
			else
				lbpMat.at<uchar>(y, x) = 58;
		}	// for (int x = 1; x < TemplateWidth - 1; ++x)
	}	// for (int y = 1; y < TemplateHeight - 1; ++y)
}

void Detector::calcGrad(cv::Mat &src, cv::Mat &gradDir, cv::Mat &gradMag)
{
	// 輝度勾配計算
	gradDir = cv::Mat(src.rows, src.cols, CV_8UC1, cv::Scalar(100));
	gradMag = cv::Mat(src.rows, src.cols, CV_32FC1, cv::Scalar(0));

	for (int y = 1; y < src.rows - 1; ++y)
	{
		for (int x = 1; x < src.cols - 1; ++x)
		{
			float dx = (float)(src.at<uchar>(y, x + 1) - src.at<uchar>(y, x - 1));
			float dy = (float)(src.at<uchar>(y + 1, x) - src.at<uchar>(y - 1, x));

			if (dx < 0)
			{
				dx = -dx;
				dy = -dy;
			}
			uchar bin = 0;
			if (dx > FLT_MIN)
			{
				float tanVal = dx / dy;
				
				if (tanVal > 0.1763f)	// tan10
				{
					if (tanVal > 1.1918f)	// tan50
					{
						if (tanVal > 2.7475f)	// tan70
							bin = 0;
						else
							bin = 1;
					}
					else
					{
						if (tanVal > 0.5774f)	// tan30
							bin = 2;
						else
							bin = 3;
					}
				}
				else
				{
					if (tanVal > -0.5774f)	// -tan30
					{
						if (tanVal > -0.1763)	// -tan10
							bin = 4;
						else
							bin = 5;
					}
					else
					{
						if (tanVal > -1.1918f)	// -tan50
							bin = 6;
						else if (tanVal > -2.7475f)	// -tan70
							bin = 7;
						else
							bin = 8;
					}
				}
			}	// if (dx > FLT_MIN)
			gradDir.at<uchar>(y, x) = bin;
			gradMag.at<float>(y, x) = sqrt(dx * dx + dy * dy);
		}	// for (int x = 1; x < TemplateWidth - 1; ++x)
	}	// for (int y = 1; y < TemplateHeight - 1; ++y)
}

/******************************************
	２つの特徴ベクトルの距離を計算する
	feat1,feat2：HoG特徴ベクトル
*******************************************/

/*
double GetDistance(double* feat1, double* feat2)
{
	double dist = 0.0;
	for(int i=0; i<TOTAL_DIM; i++){
		dist += fabs(feat1[i] - feat2[i])*fabs(feat1[i] - feat2[i]);
	}
	return sqrt(dist);
}
*/