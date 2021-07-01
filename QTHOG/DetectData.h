#pragma once
namespace detector
{
	// ���f���^�C�v
	enum eModelType { MODEL_PERSON, MODEL_PLATE, MODEL_FACE, NUM_OF_MODEL};
	// ���o�f�[�^�\��
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