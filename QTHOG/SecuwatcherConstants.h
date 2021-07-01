namespace secuwatcher_mask_param
{
	// ���f���^�C�v
	enum eModelType { MODEL_PERSON, MODEL_PLATE, MODEL_FACE, NUM_OF_MODEL};		// secuwatcherMask MSDK_thread_ROI_detectLib�d�l�Ɋ�Â�����

	// ���o�����p�p�����[�^
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