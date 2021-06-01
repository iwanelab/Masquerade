/**
 * @brief SimpleImpl SVM class
 *
 * ILAIC. kosshy.
 */

#ifndef SIMPLESVM_H
#define SIMPLESVM_H

#pragma once

namespace SimpleMachineLearning
{

//prototype
class CvSVM;

class SimpleSvm
{
public:
	SimpleSvm();
	~SimpleSvm();	// �����p��������virtual�Y��Ȃ��悤��

	//�����Ńf�[�^��ǂޏꍇ�͂��̂ւ��Load�֐���ǉ�

	/**
	 * @brief set SVM data from Outside
	 * @param value ���݂̒l
	 * @return �Z�b�g�ł��Ȃ����͋�̃f�[�^�ł����false
	 */
	bool setSvmData(CvSVM *svm);

	/**
	 * @brief detect from Setting SVM data
	 * @return ��x�ł����o������true
	 */
	bool detectFromImage();

	//int	getDivideWidthStep() { return iDivideWidthStep_; }

private:
	CvSVM	*pSvm_;		//!< @brief SVM class pointer
	bool	bReadSvm_;	//!< @brief �������̃N���X��SVM��ǂ񂾂Ȃ�Ō�Ƀf���[�g
};

}
#endif //SIMPLESVM_H
