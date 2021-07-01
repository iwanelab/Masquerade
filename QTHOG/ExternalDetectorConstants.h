#pragma once
namespace detector
{
	// WM_COPYDATA 種別
	const unsigned long datatype_Detect=1;
	const unsigned long datatype_Parameter=2;
	const unsigned long datatype_OutputMapFile=3;

	// detectメッセージ戻り値
	const int DETECT_SUCCESS = 0;
	const int DETECT_DETECT_ERROR = -1;
	const int DETECT_MAPFILE_ERROR = -2;
	const int DETECT_NOT_ENOUGH_MEMORY = 1;

	// イメージデータ構造
	struct inputImageHeader {
		__int32 rows;
		__int32 cols;
		__int32 type;
		__int32 step;
	};

	// 検出処理用メッセージ構造
	struct detectMessage {
		char mapfileDetectedData[32];
		__int64 mapfileSize;
		struct inputImageHeader imageHeader;
		unsigned char imageData[0];
	};
}
