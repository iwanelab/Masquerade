#pragma once
namespace detector
{
	// モデルタイプ
	enum eModelType { MODEL_PERSON, MODEL_PLATE, MODEL_FACE, NUM_OF_MODEL};
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