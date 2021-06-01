#pragma once

namespace Constants
{
	enum Mode {ModeNone = 0, ModeSkinLearning, ModeManualDetect};
	enum DetectType {TypeFace = 0, TypePlate};
	enum DetectMethod {HOG, SECUWATCHER};
	enum ImageType {PANORAMIC, PERSPECTIVE};

	const int HogDim = 4356;
}