#ifndef HogUtils_H
#define HogUtils_H

#include <opencv.hpp>
#include <omp.h>

static float angles[] = {0.0f, 0.8391f, 5.6713f, -1.7321f, -0.3640f, 0.3640f, 1.7321f, -5.6713f, -0.8391f};

namespace HogUtils
{
	/*****************************
		HoG計算のパラメータ
	******************************/
	const int CELL_X = 4;//5;	//1セル内の横画素数
	const int CELL_Y = 4;//5;	//1セル内の縦画素数
	const int CELL_BIN = 9;	//輝度勾配方向の分割数（普通は９）
	const int BLOCK_X = 3;	//1ブロック内の横セル数
	const int BLOCK_Y = 3;	//1ブロック内の縦セル数

	const int RESIZE_X = 48;//40;	//リサイズ後の画像の横幅
	const int RESIZE_Y = 48;//40;	//リサイズ後の画像の縦幅

	//以下のパラメータは、上の値から自動的に決まります

	const int CELL_WIDTH = RESIZE_X / CELL_X;	//セルの数（横）
	const int CELL_HEIGHT = RESIZE_Y / CELL_Y;	//セルの数（縦）
	const int BLOCK_WIDTH = CELL_WIDTH - BLOCK_X + 1;		//ブロックの数（横）
	const int BLOCK_HEIGHT = CELL_HEIGHT - BLOCK_Y + 1;	//ブロックの数（縦）

	const int BLOCK_DIM	= BLOCK_X * BLOCK_Y * CELL_BIN;		//１ブロックの特徴量次元
	const int TOTAL_DIM	= BLOCK_DIM * BLOCK_WIDTH * BLOCK_HEIGHT;	//HoG全体の次元

	/************************************
		HoG特徴量を計算
		img：グレースケール画像
		feat：計算された特徴量が入る
	*************************************/
	void GetHoG(cv::Mat src, float* feat)
	{
		//はじめに、画像サイズを変換（CELL_X/Yの倍数とする）
		cv::Mat img;
		//IplImage* img = cvCreateImage(cvSize(RESIZE_X,RESIZE_Y), 8, 1);
		//cvResize(src, img);
		if (src.size().width != RESIZE_X || src.size().height != RESIZE_Y)
			cv::resize(src, img, cv::Size(RESIZE_X, RESIZE_Y));
		else
			img = src;

		//画像サイズ
		const int width = RESIZE_X;
		const int height = RESIZE_Y;
		
		//各セルの輝度勾配ヒストグラム
		float hist[CELL_WIDTH][CELL_HEIGHT][CELL_BIN];
		memset(hist, 0, CELL_WIDTH*CELL_HEIGHT*CELL_BIN * sizeof(float));

		//各ピクセルにおける輝度勾配強度mと勾配方向degを算出し、ヒストグラムへ
		//※端っこのピクセルでは、計算しない
		int x, bin;
		float dx, dy, m, tanVal;

#undef for
#pragma omp parallel for private(m, x, dx, dy, bin, tanVal)
		for(int y=0; y<height; y++){
			for(x=0; x<width; x++){
				if(x==0 || y==0 || x==width-1 || y==height-1){
					continue;
				}
				//float dx = img->imageData[y*img->widthStep+(x+1)] - img->imageData[y*img->widthStep+(x-1)];
				dx = img.at<uchar>(y, x + 1) - img.at<uchar>(y, x - 1);
				dy = img.at<uchar>(y + 1, x) - img.at<uchar>(y - 1, x);
				m = sqrt(dx*dx+dy*dy);

				tanVal = dy / dx;
				if (dx >= 0)
				{
					if (dy >= 0)
					{
						// 第一象限
						if (tanVal < angles[1]) {bin = 0;}//0;
						else if (tanVal < angles[2]) {bin = 1;}//1;
						else {bin = 2;}//4;
					}
					else
					{
						// 第四象限
						if (tanVal < angles[7]) {bin = 6;}//0;
						else if (tanVal < angles[8]) {bin = 7;}//14;
						else {bin = 8;}//11;
					}
				}
				else
				{
					if (dy >= 0)
					{
						// 第二象限
						if (tanVal < angles[3]) {bin = 2;}//7;
						else if (tanVal < angles[4]) {bin = 3;}//6;
						else {bin = 4;}//4;
					}
					else
					{
						// 第三象限
						if (tanVal < angles[5]) {bin = 4;}//8;
						else if (tanVal < angles[6]) {bin = 5;}//9;
						else {bin = 6;}//11;
					}
				}
				
				//if(bin < 0) bin=0;
				//if(bin >= CELL_BIN) bin = CELL_BIN-1;
				hist[(int)(x/CELL_X)][(int)(y/CELL_Y)][bin] += m;
			}
		}

		//ブロックごとで正規化
		for(int y=0; y<BLOCK_HEIGHT; y++){
			for(int x=0; x<BLOCK_WIDTH; x++){
				
				//このブロックの特徴ベクトル（次元BLOCK_DIM=BLOCK_X*BLOCK_Y*CELL_BIN）
				float vec[BLOCK_DIM];
				memset(vec, 0, BLOCK_DIM*sizeof(float));
				for(int j=0; j<BLOCK_Y; j++){
					for(int i=0; i<BLOCK_X; i++){
						for(int d=0; d<CELL_BIN; d++){
							int index = j*(BLOCK_X*CELL_BIN) + i*CELL_BIN + d; 
							vec[index] = hist[x+i][y+j][d];
						}
					}
				}

				//ノルムを計算し、正規化
				float norm = 0.0;
				for(int i=0; i<BLOCK_DIM; i++){
					norm += vec[i]*vec[i];
				}
				for(int i=0; i<BLOCK_DIM; i++){
					vec[i] /= sqrt(norm + 1.0);
				}

				//featに代入
				for(int i=0; i<BLOCK_DIM; i++){
					int index = y*BLOCK_WIDTH*BLOCK_DIM + x*BLOCK_DIM + i;
					feat[index] = vec[i];
				}
			}
		}
		//cvReleaseImage(&img);
		return;
	}

/*
	void GetHoG(IplImage* src, float* feat)
	{
		//はじめに、画像サイズを変換（CELL_X/Yの倍数とする）
		IplImage* img = cvCreateImage(cvSize(RESIZE_X,RESIZE_Y), 8, 1);
		cvResize(src, img);

		//画像サイズ
		const int width = RESIZE_X;
		const int height = RESIZE_Y;
		
		//各セルの輝度勾配ヒストグラム
		float hist[CELL_WIDTH][CELL_HEIGHT][CELL_BIN];
		memset(hist, 0, CELL_WIDTH*CELL_HEIGHT*CELL_BIN * sizeof(float));

		//各ピクセルにおける輝度勾配強度mと勾配方向degを算出し、ヒストグラムへ
		//※端っこのピクセルでは、計算しない
		for(int y=0; y<height; y++){
			for(int x=0; x<width; x++){
				if(x==0 || y==0 || x==width-1 || y==height-1){
					continue;
				}
				float dx = img->imageData[y*img->widthStep+(x+1)] - img->imageData[y*img->widthStep+(x-1)];
				float dy = img->imageData[(y+1)*img->widthStep+x] - img->imageData[(y-1)*img->widthStep+x];
				float m = sqrt(dx*dx+dy*dy);
				
				float deg = (atan2(dy, dx)+CV_PI) * 180.0 / CV_PI;	//0.0～360.0の範囲になるよう変換
				int bin = CELL_BIN * deg/360.0;
				
				if(bin < 0) bin=0;
				if(bin >= CELL_BIN) bin = CELL_BIN-1;
				hist[(int)(x/CELL_X)][(int)(y/CELL_Y)][bin] += m;
			}
		}

		//ブロックごとで正規化
		for(int y=0; y<BLOCK_HEIGHT; y++){
			for(int x=0; x<BLOCK_WIDTH; x++){
				
				//このブロックの特徴ベクトル（次元BLOCK_DIM=BLOCK_X*BLOCK_Y*CELL_BIN）
				float vec[BLOCK_DIM];
				memset(vec, 0, BLOCK_DIM*sizeof(float));
				for(int j=0; j<BLOCK_Y; j++){
					for(int i=0; i<BLOCK_X; i++){
						for(int d=0; d<CELL_BIN; d++){
							int index = j*(BLOCK_X*CELL_BIN) + i*CELL_BIN + d; 
							vec[index] = hist[x+i][y+j][d];
						}
					}
				}

				//ノルムを計算し、正規化
				float norm = 0.0;
				for(int i=0; i<BLOCK_DIM; i++){
					norm += vec[i]*vec[i];
				}
				for(int i=0; i<BLOCK_DIM; i++){
					vec[i] /= sqrt(norm + 1.0);
				}

				//featに代入
				for(int i=0; i<BLOCK_DIM; i++){
					int index = y*BLOCK_WIDTH*BLOCK_DIM + x*BLOCK_DIM + i;
					feat[index] = vec[i];
				}
			}
		}
		cvReleaseImage(&img);
		return;
	}
*/
}

#endif