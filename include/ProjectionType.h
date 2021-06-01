#pragma once
#include "Math/Matrix33.h"

namespace ILCore{
namespace IZIC{

	/**
	 * “Š‰e•û–@
	 */
	enum ProjectionType {
		CUBE_FRONT		= 0, //!< CUBE_FRONT
		CUBE_REAR		= 1,  //!< CUBE_REAR
		CUBE_LEFT		= 2,  //!< CUBE_LEFT
		CUBE_RIGHT		= 3, //!< CUBE_RIGHT
		CUBE_TOP		= 4,   //!< CUBE_TOP
		CUBE_BOTTOM		= 5,//!< CUBE_BOTTOM
		BILLBOARD		= 6,  //!< BILLBOARD
		SPHERE			= 7,    //!< SPHERE
	};

	/**
	 * “Š‰es—ñ
	 */
	const ILCore::Math::CMatrix33<double> projectionMatrix[] =
	{
		ILCore::Math::CMatrix33<double>(1., 0., 0., 0., 1., 0., 0., 0., 1.), //!< CUBE_FRONT ‚Ì“Š‰es—ñ
		ILCore::Math::CMatrix33<double>(-1., 0., 0., 0., 1., 0., 0., 0., -1.), //!< CUBE_REAR ‚Ì“Š‰es—ñ
		ILCore::Math::CMatrix33<double>(0., 0., -1., 0., 1., 0., 1., 0., 0.), //!< CUBE_LEFT ‚Ì“Š‰es—ñ
		ILCore::Math::CMatrix33<double>(0., 0., 1., 0., 1., 0., -1., 0., 0.), //!< CUBE_RIGHT ‚Ì“Š‰es—ñ
		ILCore::Math::CMatrix33<double>(1., 0., 0., 0., 0., 1., 0., -1., 0.), //!< CUBE_TOP ‚Ì“Š‰es—ñ
		ILCore::Math::CMatrix33<double>(1., 0., 0., 0., 0., -1., 0., 1., 0.), //!< CUBE_BOTTOM ‚Ì“Š‰es—ñ
	};

	const ILCore::Math::CVector3D<double> cubeFaceNormal[] =
	{
		ILCore::Math::CVector3D<double>( 0.,  0., -1.),	//!< CUBE_FRONT  Normal
		ILCore::Math::CVector3D<double>( 0.,  0., +1.),	//!< CUBE_REAR 	 Normal
		ILCore::Math::CVector3D<double>(-1.,  0.,  0.),	//!< CUBE_LEFT   Normal
		ILCore::Math::CVector3D<double>(+1.,  0.,  0.),	//!< CUBE_RIGHT  Normal
		ILCore::Math::CVector3D<double>( 0., +1.,  0.),	//!< CUBE_TOP    Normal
		ILCore::Math::CVector3D<double>( 0., -1.,  0.),	//!< CUBE_BOTTOM Normal
	};

}
}
