namespace secuwatcher_access
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

	// モデルタイプ
	enum eModelType { MODEL_PERSON, MODEL_PLATE, MODEL_FACE, NUM_OF_MODEL};		// secuwatcherMask MSDK_thread_ROI_detectLib仕様に基づく順番

	// 検出処理用パラメータ
	struct detectParam {
		struct modelInfo {
			bool valid;
			char reserve[16 - sizeof(bool)];
			TCHAR cfgFilename[MAX_PATH];
			TCHAR weightFilename[MAX_PATH];
		} model[secuwatcher_access::NUM_OF_MODEL];
		float DL_thresh;
		bool useGPU;
		char reserve[16 - sizeof(float) - sizeof(bool)];
	};

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


	// 検出データ構造
	struct stRectData {
		__int32 x;
		__int32 y;
		__int32 width;
		__int32 height;
		float prob;
		__int32 obj_id;
	};
	struct detectedData {
		enum eModelType modelType;
		struct stRectData rectData;
	};

}