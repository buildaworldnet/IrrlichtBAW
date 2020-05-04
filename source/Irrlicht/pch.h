#ifndef __IRR_PCH_H_INCLUDED__
#define __IRR_PCH_H_INCLUDED__

#include "irrlicht.h"

#include "irr/core/pch_core.h"
#include "irr/asset/pch_asset.h"
#include "irr/video/pch_video.h"

#include "irrString.h"
#include "aabbox3d.h"
#include "CConcurrentObjectCache.h"
#include "CObjectCache.h"
#include "COpenGLStateManager.h"
#include "dimension2d.h"
#include "EDeviceTypes.h"
#include "EDriverTypes.h"
#include "ESceneNodeAnimatorTypes.h"
#include "ESceneNodeTypes.h"
#include "IAnimatedMesh.h"
#include "IAnimatedMeshSceneNode.h"
#include "ICameraSceneNode.h"
#include "ICursorControl.h"
#include "IDriver.h"
#include "IDriverFence.h"
#include "IDriverMemoryAllocation.h"
#include "IDriverMemoryBacked.h"
#include "IDummyTransformationSceneNode.h"
#include "IEventReceiver.h"
#include "IFileArchive.h"
#include "IFileList.h"
#include "IFileSystem.h"
#include "IFrameBuffer.h"
#include "IGPUBuffer.h"
#include "IGPUTimestampQuery.h"
#include "ILogger.h"
#include "IOSOperator.h"
#include "IQueryObject.h"
#include "IReadFile.h"
#include "IrrlichtDevice.h"
#include "ISceneManager.h"
#include "ISceneNode.h"
#include "ISceneNodeAnimator.h"
#include "ISceneNodeAnimatorCameraFPS.h"
#include "ISceneNodeAnimatorCameraMaya.h"
#include "ISkinnedMeshSceneNode.h"
#include "ISkinningStateManager.h"
#include "ITimer.h"
#include "IVideoCapabilityReporter.h"
#include "IVideoDriver.h"
#include "IWriteFile.h"
#include "Keycodes.h"
#include "line3d.h"
#include "matrix3x4SIMD.h"
#include "matrix4SIMD.h"
#include "matrix4x3.h"
#include "matrixutil.h"
#include "path.h"
#include "position2d.h"
#include "quaternion.h"
#include "rect.h"
#include "SAABoxCollider.h"
#include "SCollisionEngine.h"
#include "SColor.h"
#include "SCompoundCollider.h"
#include "SEllipsoidCollider.h"
#include "SExposedVideoData.h"
#include "SIrrCreationParameters.h"
#include "SKeyMap.h"
#include "splines.h"
#include "STriangleMeshCollider.h"
#include "SViewFrustum.h"
#include "vector2d.h"
#include "vector3d.h"
#include "vectorSIMD.h"
#include "CCameraSceneNode.h"
#include "CFileList.h"
#include "CFileSystem.h"
#include "CFPSCounter.h"
#include "CIrrDeviceConsole.h"
#include "CIrrDeviceLinux.h"
#include "CIrrDeviceStub.h"
#include "CIrrDeviceWin32.h"
#include "CLimitReadFile.h"
#include "CLogger.h"
#include "CMemoryFile.h"
#include "CMountPointReader.h"
#include "CNullDriver.h"
#include "COpenCLHandler.h"
#include "COpenGLBuffer.h"
#include "COpenGLDriver.h"
#include "COpenGLDriverFence.h" 
#include "COpenGLExtensionHandler.h"
#include "COpenGLFrameBuffer.h"
#include "COpenGLQuery.h"
#include "COpenGLTimestampQuery.h"
#include "COSOperator.h"
#include "CPakReader.h"
#include "CReadFile.h"
#include "CSceneManager.h"
#include "CSceneNodeAnimatorCameraFPS.h"
#include "CSceneNodeAnimatorCameraMaya.h"
#include "CSceneNodeAnimatorDelete.h"
#include "CSceneNodeAnimatorFlyCircle.h"
#include "CSceneNodeAnimatorFlyStraight.h"
#include "CSceneNodeAnimatorFollowSpline.h"
#include "CSceneNodeAnimatorRotation.h"
#include "CSkinnedMeshSceneNode.h"
#include "CSkinningStateManager.h"
#include "CTarReader.h"
#include "CWriteFile.h"
#include "CZipReader.h"
#include "FW_Mutex.h" // refactor
#include "ISceneNodeAnimatorFinishing.h"
#include "os.h"

#endif //__IRR_PCH_H_INCLUDED__
