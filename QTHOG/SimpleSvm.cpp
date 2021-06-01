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
 * @return セットできない又は空のデータであればfalse
 */
bool SimpleSvm::setSvmData(CvSVM *svm)
{
	if( svm == NULL ) return false;
	if( bReadSvm_ )
	{
		// 外からセットされるので読んでいる場合はアンロード
	}

	pSvm_ = svm;

	//SVMデータの検証は後実装

	return true;
}


bool SimpleSvm::detectFromImage()
{
	return true;
}

}
