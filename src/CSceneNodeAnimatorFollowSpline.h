// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SCENE_NODE_ANIMATOR_FOLLOW_SPLINE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_FOLLOW_SPLINE_H_INCLUDED__

#include "ISceneNode.h"
#include "irrArray.h"
#include "ISceneNodeAnimatorFinishing.h"

namespace irr
{
namespace scene
{
	//! Scene node animator based free code Matthias Gall wrote and sent in. (Most of
	//! this code is written by him, I only modified bits.)
	class CSceneNodeAnimatorFollowSpline : public ISceneNodeAnimatorFinishing
	{
	public:

		//! constructor
		CSceneNodeAnimatorFollowSpline(u32 startTime,
			const core::array< core::vector3df >& points,
			f32 speed = 1.0f, f32 tightness = 0.5f, bool loop=true, bool pingpong=false);

		//! animates a scene node
		virtual void animateNode(IDummyTransformationSceneNode* node, u32 timeMs);

		//! Returns type of the scene node animator
		virtual ESCENE_NODE_ANIMATOR_TYPE getType() const { return ESNAT_FOLLOW_SPLINE; }

		//! Creates a clone of this animator.
		/** Please note that you will have to drop
		(IReferenceCounted::drop()) the returned pointer after calling
		this. */
		virtual ISceneNodeAnimator* createClone(IDummyTransformationSceneNode* node, ISceneManager* newManager=0);

	protected:

		//! clamps a the value idx to fit into range 0..size-1
		s32 clamp(s32 idx, s32 size);

		core::array< core::vector3df > Points;
		f32 Speed;
		f32 Tightness;
		u32 StartTime;
		bool Loop;
		bool PingPong;
	};


} // end namespace scene
} // end namespace irr

#endif

