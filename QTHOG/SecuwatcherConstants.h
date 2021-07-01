namespace secuwatcher_mask_param
{
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
}