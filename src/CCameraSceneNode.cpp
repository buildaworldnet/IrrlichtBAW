// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CCameraSceneNode.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "os.h"

namespace irr
{
namespace scene
{


//! constructor
CCameraSceneNode::CCameraSceneNode(IDummyTransformationSceneNode* parent, ISceneManager* mgr, s32 id,
	const core::vector3df& position, const core::vector3df& lookat)
	: ICameraSceneNode(parent, mgr, id, position),
	Target(lookat), UpVector(0.0f, 1.0f, 0.0f), ZNear(1.0f), ZFar(3000.0f),
	InputReceiverEnabled(true), TargetAndRotationAreBound(false)
{
	#ifdef _DEBUG
	setDebugName("CCameraSceneNode");
	#endif

	// set default projection
	Fovy = core::PI / 2.5f;	// Field of view, in radians.

	const video::IVideoDriver* const d = mgr?mgr->getVideoDriver():0;
	if (d)
		Aspect = (f32)d->getCurrentRenderTargetSize().Width /
			(f32)d->getCurrentRenderTargetSize().Height;
	else
		Aspect = 4.0f / 3.0f;	// Aspect ratio.

	recalculateProjectionMatrix();
	recalculateViewArea();
}


//! Disables or enables the camera to get key or mouse inputs.
void CCameraSceneNode::setInputReceiverEnabled(bool enabled)
{
	InputReceiverEnabled = enabled;
}


//! Returns if the input receiver of the camera is currently enabled.
bool CCameraSceneNode::isInputReceiverEnabled() const
{
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return InputReceiverEnabled;
}


//! Sets the projection matrix of the camera.
/** The core::matrix4 class has some methods
to build a projection matrix. e.g: core::matrix4::buildProjectionMatrixPerspectiveFovLH
\param projection: The new projection matrix of the camera. */
void CCameraSceneNode::setProjectionMatrix(const core::matrix4& projection, bool isOrthogonal)
{
	IsOrthogonal = isOrthogonal;
	projMatrix = projection;
	concatMatrix = concatenateBFollowedByA(projMatrix,viewMatrix);
}


//! Gets the current projection matrix of the camera
//! \return Returns the current projection matrix of the camera.
const core::matrix4& CCameraSceneNode::getProjectionMatrix() const
{
	return projMatrix;
}


//! Gets the current view matrix of the camera
//! \return Returns the current view matrix of the camera.
const core::matrix4x3& CCameraSceneNode::getViewMatrix() const
{
	return viewMatrix;
}



//! It is possible to send mouse and key events to the camera. Most cameras
//! may ignore this input, but camera scene nodes which are created for
//! example with scene::ISceneManager::addMayaCameraSceneNode or
//! scene::ISceneManager::addFPSCameraSceneNode, may want to get this input
//! for changing their position, look at target or whatever.
bool CCameraSceneNode::OnEvent(const SEvent& event)
{
	if (!InputReceiverEnabled)
		return false;

	// send events to event receiving animators

	ISceneNodeAnimatorList::Iterator ait = Animators.begin();

	for (; ait != Animators.end(); ++ait)
		if ((*ait)->isEventReceiverEnabled() && (*ait)->OnEvent(event))
			return true;

	// if nobody processed the event, return false
	return false;
}


//! sets the look at target of the camera
//! \param pos: Look at target of the camera.
void CCameraSceneNode::setTarget(const core::vector3df& pos)
{
	Target = pos;

	if(TargetAndRotationAreBound)
	{
		const core::vector3df toTarget = Target - getAbsolutePosition();
		ISceneNode::setRotation(toTarget.getHorizontalAngle());
	}
}


//! Sets the rotation of the node.
/** This only modifies the relative rotation of the node.
If the camera's target and rotation are bound ( @see bindTargetAndRotation() )
then calling this will also change the camera's target to match the rotation.
\param rotation New rotation of the node in degrees. */
void CCameraSceneNode::setRotation(const core::vector3df& rotation)
{
	if(TargetAndRotationAreBound)
		Target = getAbsolutePosition() + rotation.rotationToDirection();

	ISceneNode::setRotation(rotation);
}


//! Gets the current look at target of the camera
//! \return Returns the current look at target of the camera
const core::vector3df& CCameraSceneNode::getTarget() const
{
	return Target;
}


//! sets the up vector of the camera
//! \param pos: New upvector of the camera.
void CCameraSceneNode::setUpVector(const core::vector3df& pos)
{
	UpVector = pos;
}


//! Gets the up vector of the camera.
//! \return Returns the up vector of the camera.
const core::vector3df& CCameraSceneNode::getUpVector() const
{
	return UpVector;
}


f32 CCameraSceneNode::getNearValue() const
{
	return ZNear;
}


f32 CCameraSceneNode::getFarValue() const
{
	return ZFar;
}


f32 CCameraSceneNode::getAspectRatio() const
{
	return Aspect;
}


f32 CCameraSceneNode::getFOV() const
{
	return Fovy;
}


void CCameraSceneNode::setNearValue(f32 f)
{
	ZNear = f;
	recalculateProjectionMatrix();
}


void CCameraSceneNode::setFarValue(f32 f)
{
	ZFar = f;
	recalculateProjectionMatrix();
}


void CCameraSceneNode::setAspectRatio(f32 f)
{
	Aspect = f;
	recalculateProjectionMatrix();
}


void CCameraSceneNode::setFOV(f32 f)
{
	Fovy = f;
	recalculateProjectionMatrix();
}


void CCameraSceneNode::recalculateProjectionMatrix()
{
	projMatrix.buildProjectionMatrixPerspectiveFovLH(Fovy, Aspect, ZNear, ZFar);
	concatMatrix = concatenateBFollowedByA(projMatrix,viewMatrix);
}


//! prerender
void CCameraSceneNode::OnRegisterSceneNode()
{
	if ( SceneManager->getActiveCamera () == this )
		SceneManager->registerNodeForRendering(this, ESNRP_CAMERA);

	ISceneNode::OnRegisterSceneNode();
}


//! render
void CCameraSceneNode::render()
{
	core::vector3df pos = getAbsolutePosition();
	core::vector3df tgtv = Target - pos;
	tgtv.normalize();

	// if upvector and vector to the target are the same, we have a
	// problem. so solve this problem:
	core::vector3df up = UpVector;
	up.normalize();

	f32 dp = tgtv.dotProduct(up);

	if ( core::equals(core::abs_<f32>(dp), 1.f) )
	{
		up.X += 0.5f;
	}

	viewMatrix.buildCameraLookAtMatrixLH(pos, Target, up);
	concatMatrix = concatenateBFollowedByA(projMatrix,viewMatrix);
	recalculateViewArea();

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if ( driver)
	{
		driver->setTransform(video::EPTS_PROJ,projMatrix);
		driver->setTransform(video::E4X3TS_VIEW, viewMatrix );
	}
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CCameraSceneNode::getBoundingBox()
{
	return ViewArea.getBoundingBox();
}


//! returns the view frustum. needed sometimes by bsp or lod render nodes.
const SViewFrustum* CCameraSceneNode::getViewFrustum() const
{
	return &ViewArea;
}


void CCameraSceneNode::recalculateViewArea()
{
	ViewArea.cameraPosition = getAbsolutePosition();

	ViewArea.setFrom(concatMatrix);
}



//! Set the binding between the camera's rotation adn target.
void CCameraSceneNode::bindTargetAndRotation(bool bound)
{
	TargetAndRotationAreBound = bound;
}


//! Gets the binding between the camera's rotation and target.
bool CCameraSceneNode::getTargetAndRotationBinding(void) const
{
	return TargetAndRotationAreBound;
}


//! Creates a clone of this scene node and its children.
ISceneNode* CCameraSceneNode::clone(IDummyTransformationSceneNode* newParent, ISceneManager* newManager)
{
	ICameraSceneNode::clone(newParent, newManager);

	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	CCameraSceneNode* nb = new CCameraSceneNode(newParent,
		newManager, ID, RelativeTranslation, Target);

	nb->ISceneNode::cloneMembers(this, newManager);
	nb->ICameraSceneNode::cloneMembers(this);

	nb->Target = Target;
	nb->UpVector = UpVector;
	nb->Fovy = Fovy;
	nb->Aspect = Aspect;
	nb->ZNear = ZNear;
	nb->ZFar = ZFar;
	nb->ViewArea = ViewArea;
	nb->InputReceiverEnabled = InputReceiverEnabled;
	nb->TargetAndRotationAreBound = TargetAndRotationAreBound;

	if ( newParent )
		nb->drop();
	return nb;
}


} // end namespace
} // end namespace

