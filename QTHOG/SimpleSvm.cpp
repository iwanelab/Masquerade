/**
 * @brief SimpleImpl SVM class
 *
 * ILAIC. kosshy.
 */

#include "SimpleSvm.h"

// use header
#include <opencv.hpp>

namespace SimpleMachineLearning
{

SimpleSvm::SimpleSvm()
	:pSvm_( NULL )
	,bReadSvm_( false )
{
}

SimpleSvm::~SimpleSvm()
{
}

/**
 * @brief set SVM data from 
 * @return �Z�b�g�ł��Ȃ����͋�̃f�[�^�ł����false
 */
bool SimpleSvm::setSvmData(CvSVM *svm)
{
	if( svm == NULL ) return false;
	if( bReadSvm_ )
	{
		// �O����Z�b�g�����̂œǂ�ł���ꍇ�̓A�����[�h
	}

	pSvm_ = svm;

	//SVM�f�[�^�̌��؂͌����

	return true;
}


bool SimpleSvm::detectFromImage()
{
	return true;
}

}
