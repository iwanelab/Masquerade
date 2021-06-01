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
	~SimpleSvm();	// 何か継承したらvirtual忘れないように

	//自分でデータを読む場合はこのへんにLoad関数を追加

	/**
	 * @brief set SVM data from Outside
	 * @param value 現在の値
	 * @return セットできない又は空のデータであればfalse
	 */
	bool setSvmData(CvSVM *svm);

	/**
	 * @brief detect from Setting SVM data
	 * @return 一度でも検出したらtrue
	 */
	bool detectFromImage();

	//int	getDivideWidthStep() { return iDivideWidthStep_; }

private:
	CvSVM	*pSvm_;		//!< @brief SVM class pointer
	bool	bReadSvm_;	//!< @brief もしこのクラスがSVMを読んだなら最後にデリート
};

}
#endif //SIMPLESVM_H
