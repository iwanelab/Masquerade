#ifndef HogUtils_H
#define HogUtils_H

#include <opencv.hpp>
#include <omp.h>

static float angles[] = {0.0f, 0.8391f, 5.6713f, -1.7321f, -0.3640f, 0.3640f, 1.7321f, -5.6713f, -0.8391f};

namespace HogUtils
{
	/*****************************
		HoG�v�Z�̃p�����[�^
	******************************/
	const int CELL_X = 4;//5;	//1�Z�����̉���f��
	const int CELL_Y = 4;//5;	//1�Z�����̏c��f��
	const int CELL_BIN = 9;	//�P�x���z�����̕������i���ʂ͂X�j
	const int BLOCK_X = 3;	//1�u���b�N���̉��Z����
	const int BLOCK_Y = 3;	//1�u���b�N���̏c�Z����

	const int RESIZE_X = 48;//40;	//���T�C�Y��̉摜�̉���
	const int RESIZE_Y = 48;//40;	//���T�C�Y��̉摜�̏c��

	//�ȉ��̃p�����[�^�́A��̒l���玩���I�Ɍ��܂�܂�

	const int CELL_WIDTH = RESIZE_X / CELL_X;	//�Z���̐��i���j
	const int CELL_HEIGHT = RESIZE_Y / CELL_Y;	//�Z���̐��i�c�j
	const int BLOCK_WIDTH = CELL_WIDTH - BLOCK_X + 1;		//�u���b�N�̐��i���j
	const int BLOCK_HEIGHT = CELL_HEIGHT - BLOCK_Y + 1;	//�u���b�N�̐��i�c�j

	const int BLOCK_DIM	= BLOCK_X * BLOCK_Y * CELL_BIN;		//�P�u���b�N�̓����ʎ���
	const int TOTAL_DIM	= BLOCK_DIM * BLOCK_WIDTH * BLOCK_HEIGHT;	//HoG�S�̂̎���

	/************************************
		HoG�����ʂ��v�Z
		img�F�O���[�X�P�[���摜
		feat�F�v�Z���ꂽ�����ʂ�����
	*************************************/
	void GetHoG(cv::Mat src, float* feat)
	{
		//�͂��߂ɁA�摜�T�C�Y��ϊ��iCELL_X/Y�̔{���Ƃ���j
		cv::Mat img;
		//IplImage* img = cvCreateImage(cvSize(RESIZE_X,RESIZE_Y), 8, 1);
		//cvResize(src, img);
		if (src.size().width != RESIZE_X || src.size().height != RESIZE_Y)
			cv::resize(src, img, cv::Size(RESIZE_X, RESIZE_Y));
		else
			img = src;

		//�摜�T�C�Y
		const int width = RESIZE_X;
		const int height = RESIZE_Y;
		
		//�e�Z���̋P�x���z�q�X�g�O����
		float hist[CELL_WIDTH][CELL_HEIGHT][CELL_BIN];
		memset(hist, 0, CELL_WIDTH*CELL_HEIGHT*CELL_BIN * sizeof(float));

		//�e�s�N�Z���ɂ�����P�x���z���xm�ƌ��z����deg���Z�o���A�q�X�g�O������
		//���[�����̃s�N�Z���ł́A�v�Z���Ȃ�
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
						// ���ی�
						if (tanVal < angles[1]) {bin = 0;}//0;
						else if (tanVal < angles[2]) {bin = 1;}//1;
						else {bin = 2;}//4;
					}
					else
					{
						// ��l�ی�
						if (tanVal < angles[7]) {bin = 6;}//0;
						else if (tanVal < angles[8]) {bin = 7;}//14;
						else {bin = 8;}//11;
					}
				}
				else
				{
					if (dy >= 0)
					{
						// ���ی�
						if (tanVal < angles[3]) {bin = 2;}//7;
						else if (tanVal < angles[4]) {bin = 3;}//6;
						else {bin = 4;}//4;
					}
					else
					{
						// ��O�ی�
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

		//�u���b�N���ƂŐ��K��
		for(int y=0; y<BLOCK_HEIGHT; y++){
			for(int x=0; x<BLOCK_WIDTH; x++){
				
				//���̃u���b�N�̓����x�N�g���i����BLOCK_DIM=BLOCK_X*BLOCK_Y*CELL_BIN�j
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

				//�m�������v�Z���A���K��
				float norm = 0.0;
				for(int i=0; i<BLOCK_DIM; i++){
					norm += vec[i]*vec[i];
				}
				for(int i=0; i<BLOCK_DIM; i++){
					vec[i] /= sqrt(norm + 1.0);
				}

				//feat�ɑ��
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
		//�͂��߂ɁA�摜�T�C�Y��ϊ��iCELL_X/Y�̔{���Ƃ���j
		IplImage* img = cvCreateImage(cvSize(RESIZE_X,RESIZE_Y), 8, 1);
		cvResize(src, img);

		//�摜�T�C�Y
		const int width = RESIZE_X;
		const int height = RESIZE_Y;
		
		//�e�Z���̋P�x���z�q�X�g�O����
		float hist[CELL_WIDTH][CELL_HEIGHT][CELL_BIN];
		memset(hist, 0, CELL_WIDTH*CELL_HEIGHT*CELL_BIN * sizeof(float));

		//�e�s�N�Z���ɂ�����P�x���z���xm�ƌ��z����deg���Z�o���A�q�X�g�O������
		//���[�����̃s�N�Z���ł́A�v�Z���Ȃ�
		for(int y=0; y<height; y++){
			for(int x=0; x<width; x++){
				if(x==0 || y==0 || x==width-1 || y==height-1){
					continue;
				}
				float dx = img->imageData[y*img->widthStep+(x+1)] - img->imageData[y*img->widthStep+(x-1)];
				float dy = img->imageData[(y+1)*img->widthStep+x] - img->imageData[(y-1)*img->widthStep+x];
				float m = sqrt(dx*dx+dy*dy);
				
				float deg = (atan2(dy, dx)+CV_PI) * 180.0 / CV_PI;	//0.0�`360.0�͈̔͂ɂȂ�悤�ϊ�
				int bin = CELL_BIN * deg/360.0;
				
				if(bin < 0) bin=0;
				if(bin >= CELL_BIN) bin = CELL_BIN-1;
				hist[(int)(x/CELL_X)][(int)(y/CELL_Y)][bin] += m;
			}
		}

		//�u���b�N���ƂŐ��K��
		for(int y=0; y<BLOCK_HEIGHT; y++){
			for(int x=0; x<BLOCK_WIDTH; x++){
				
				//���̃u���b�N�̓����x�N�g���i����BLOCK_DIM=BLOCK_X*BLOCK_Y*CELL_BIN�j
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

				//�m�������v�Z���A���K��
				float norm = 0.0;
				for(int i=0; i<BLOCK_DIM; i++){
					norm += vec[i]*vec[i];
				}
				for(int i=0; i<BLOCK_DIM; i++){
					vec[i] /= sqrt(norm + 1.0);
				}

				//feat�ɑ��
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