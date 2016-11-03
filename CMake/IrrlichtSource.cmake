# The ZLIB license
#
# Copyright (c) 2015 Andr� Netzeband
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgement in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#

INCLUDE_DIRECTORIES(
  "${CMAKE_SOURCE_DIR}/include/"
  "${CMAKE_SOURCE_DIR}/src/"
  )

SET (IRRLICHT_PUBLIC_HEADER_FILES
  include/IBuffer.h
  include/ICPUBuffer.h
  include/IDriverFence.h
  include/IFrameBuffer.h
  include/IGPUBuffer.h
  include/IGPUMappedBuffer.h
  include/IGPUTransientBuffer.h
  include/IMeshSceneNodeInstanced.h
  include/IMetaGranularBuffer.h
  include/IOcclusionQuery.h
  include/IRenderBuffer.h
  include/matrix4x3.h
  include/matrixSIMD4.h
  include/SAABoxCollider.h
  include/SCollisionEngine.h
  include/SCompoundCollider.h
  include/SEllipsoidCollider.h
  include/SIMDswizzle.h
  include/STriangleMeshCollider.h
  include/vectorSIMD.h
  )

SET (IRRMAIN_SOURCE_FILES
  src/COpenGLTransformFeedback.cpp
  src/COpenGLTransformFeedback.h
  src/COpenGLQuery.cpp
  src/COpenGLQuery.h
  src/C3DSMeshFileLoader.cpp
  src/CAnimatedMeshHalfLife.cpp
  src/CAnimatedMeshSceneNode.cpp
  src/CB3DMeshFileLoader.cpp
  src/CBillboardSceneNode.cpp
  src/CBoneSceneNode.cpp
  src/CBurningShader_Raster_Reference.cpp
  src/CCameraSceneNode.cpp
  src/CColladaFileLoader.cpp
  src/CColladaMeshWriter.cpp
  src/CColorConverter.cpp
  src/CCSMLoader.cpp
  src/CCubeSceneNode.cpp
  src/CDefaultSceneNodeAnimatorFactory.cpp
  src/CDefaultSceneNodeFactory.cpp
  src/CDepthBuffer.cpp
  src/CDMFLoader.cpp
  src/CEmptySceneNode.cpp
  src/CFileList.cpp
  src/CFileSystem.cpp
  src/CFPSCounter.cpp
  src/CGeometryCreator.cpp
  src/CGPUTransientBuffer.cpp
  src/CImage.cpp
  src/CImageLoaderBMP.cpp
  src/CImageLoaderDDS.cpp
  src/CImageLoaderJPG.cpp
  src/CImageLoaderPCX.cpp
  src/CImageLoaderPNG.cpp
  src/CImageLoaderPPM.cpp
  src/CImageLoaderPSD.cpp
  src/CImageLoaderRGB.cpp
  src/CImageLoaderTGA.cpp
  src/CImageLoaderWAL.cpp
  src/CImageWriterBMP.cpp
  src/CImageWriterJPG.cpp
  src/CImageWriterPCX.cpp
  src/CImageWriterPNG.cpp
  src/CImageWriterPPM.cpp
  src/CImageWriterPSD.cpp
  src/CImageWriterTGA.cpp
  src/CIrrDeviceConsole.cpp
  src/CIrrDeviceFB.cpp
  src/CIrrDeviceLinux.cpp
  src/CIrrDeviceSDL.cpp
  src/CIrrDeviceStub.cpp
  src/CIrrDeviceWin32.cpp
  src/CIrrDeviceWinCE.cpp
  src/CLightSceneNode.cpp
  src/CLimitReadFile.cpp
  src/CLMTSMeshFileLoader.cpp
  src/CLogger.cpp
  src/CLWOMeshFileLoader.cpp
  src/CMemoryFile.cpp
  src/CMeshCache.cpp
  src/CMeshManipulator.cpp
  src/CMeshSceneNode.cpp
  src/CMeshSceneNodeInstanced.cpp
  src/CMountPointReader.cpp
  src/CMS3DMeshFileLoader.cpp
  src/CMY3DMeshFileLoader.cpp
  src/CNPKReader.cpp
  src/CNullDriver.cpp
  src/COBJMeshFileLoader.cpp
  src/COBJMeshWriter.cpp
  src/COCTLoader.cpp
  src/COgreMeshFileLoader.cpp
  src/COpenGL3DTexture.cpp
  src/COpenGLDriver.cpp
  src/COpenGLExtensionHandler.cpp
  src/COpenGLFrameBuffer.cpp
  src/COpenGLOcclusionQuery.cpp
  src/COpenGLPersistentlyMappedBuffer.cpp
  src/COpenGLRenderBuffer.cpp
  src/COpenGLSLMaterialRenderer.cpp
  src/COpenGLTexture.cpp
  src/COpenGLVAO.cpp
  src/COSOperator.cpp
  src/CPakReader.cpp
  src/CPLYMeshFileLoader.cpp
  src/CPLYMeshWriter.cpp
  src/CReadFile.cpp
  src/CSceneManager.cpp
  src/CSceneNodeAnimatorCameraFPS.cpp
  src/CSceneNodeAnimatorCameraMaya.cpp
  src/CSceneNodeAnimatorDelete.cpp
  src/CSceneNodeAnimatorFlyCircle.cpp
  src/CSceneNodeAnimatorFlyStraight.cpp
  src/CSceneNodeAnimatorFollowSpline.cpp
  src/CSceneNodeAnimatorRotation.cpp
  src/CSceneNodeAnimatorTexture.cpp
  src/CSkinnedMesh.cpp
  src/CSkinnedMeshSceneNode.cpp
  src/CSkinnedMeshSceneNode.h
  src/CSkyBoxSceneNode.cpp
  src/CSkyDomeSceneNode.cpp
  src/CSMFMeshFileLoader.cpp
  src/CSoftwareDriver2.cpp
  src/CSoftwareDriver.cpp
  src/CSoftwareTexture2.cpp
  src/CSoftwareTexture.cpp
  src/CSphereSceneNode.cpp
  src/CSTLMeshFileLoader.cpp
  src/CSTLMeshWriter.cpp
  src/CTarReader.cpp
  src/CTRFlat.cpp
  src/CTRFlatWire.cpp
  src/CTRGouraud2.cpp
  src/CTRGouraudAlpha2.cpp
  src/CTRGouraudAlphaNoZ2.cpp
  src/CTRGouraud.cpp
  src/CTRGouraudWire.cpp
  src/CTRNormalMap.cpp
  src/CTRStencilShadow.cpp
  src/CTRTextureBlend.cpp
  src/CTRTextureDetailMap2.cpp
  src/CTRTextureFlat.cpp
  src/CTRTextureFlatWire.cpp
  src/CTRTextureGouraud2.cpp
  src/CTRTextureGouraudAdd2.cpp
  src/CTRTextureGouraudAdd.cpp
  src/CTRTextureGouraudAddNoZ2.cpp
  src/CTRTextureGouraudAlpha.cpp
  src/CTRTextureGouraudAlphaNoZ.cpp
  src/CTRTextureGouraud.cpp
  src/CTRTextureGouraudNoZ2.cpp
  src/CTRTextureGouraudNoZ.cpp
  src/CTRTextureGouraudVertexAlpha2.cpp
  src/CTRTextureGouraudWire.cpp
  src/CTRTextureLightMap2_Add.cpp
  src/CTRTextureLightMap2_M1.cpp
  src/CTRTextureLightMap2_M2.cpp
  src/CTRTextureLightMap2_M4.cpp
  src/CTRTextureLightMapGouraud2_M4.cpp
  src/CTRTextureWire2.cpp
  src/CVideoModeList.cpp
  src/CWADReader.cpp
  src/CWriteFile.cpp
  src/CXMeshFileLoader.cpp
  src/CXMLReader.cpp
  src/CXMLWriter.cpp
  src/CZBuffer.cpp
  src/CZipReader.cpp
  src/FW_Mutex.cpp
  src/IBurningShader.cpp
  src/Irrlicht.cpp
  src/irrXML.cpp
  src/os.cpp
  src/STextureSamplingParams.cpp
  )

SET (IRRLICHT_PRIVATE_HEADER_FILES
  src/BuiltInFont.h
  src/C3DSMeshFileLoader.h
  src/CAnimatedMeshHalfLife.h
  src/CAnimatedMeshSceneNode.h
  src/CB3DMeshFileLoader.h
  src/CBillboardSceneNode.h
  src/CBlit.h
  src/CBoneSceneNode.h
  src/CCameraSceneNode.h
  src/CColladaFileLoader.h
  src/CColladaMeshWriter.h
  src/CColorConverter.h
  src/CCSMLoader.h
  src/CCubeSceneNode.h
  src/CDefaultSceneNodeAnimatorFactory.h
  src/CDefaultSceneNodeFactory.h
  src/CDepthBuffer.h
  src/CDMFLoader.h
  src/CEmptySceneNode.h
  src/CFileList.h
  src/CFileSystem.h
  src/CFPSCounter.h
  src/CGeometryCreator.h
  src/CImage.h
  src/CImageLoaderBMP.h
  src/CImageLoaderDDS.h
  src/CImageLoaderJPG.h
  src/CImageLoaderPCX.h
  src/CImageLoaderPNG.h
  src/CImageLoaderPPM.h
  src/CImageLoaderPSD.h
  src/CImageLoaderRGB.h
  src/CImageLoaderTGA.h
  src/CImageLoaderWAL.h
  src/CImageWriterBMP.h
  src/CImageWriterJPG.h
  src/CImageWriterPCX.h
  src/CImageWriterPNG.h
  src/CImageWriterPPM.h
  src/CImageWriterPSD.h
  src/CImageWriterTGA.h
  src/CIrrDeviceConsole.h
  src/CIrrDeviceFB.h
  src/CIrrDeviceLinux.h
  src/CIrrDeviceSDL.h
  src/CIrrDeviceStub.h
  src/CIrrDeviceWin32.h
  src/CIrrDeviceWinCE.h
  src/CLightSceneNode.h
  src/CLimitReadFile.h
  src/CLMTSMeshFileLoader.h
  src/CLogger.h
  src/CLWOMeshFileLoader.h
  src/CMemoryFile.h
  src/CMeshCache.h
  src/CMeshManipulator.h
  src/CMeshSceneNode.h
  src/CMeshSceneNodeInstanced.h
  src/CMountPointReader.h
  src/CMS3DMeshFileLoader.h
  src/CMY3DHelper.h
  src/CMY3DMeshFileLoader.h
  src/CNPKReader.h
  src/CNullDriver.h
  src/COBJMeshFileLoader.h
  src/COBJMeshWriter.h
  src/COCTLoader.h
  src/COgreMeshFileLoader.h
  src/COpenGL3DTexture.h
  src/COpenGLBuffer.h
  src/COpenGLDriverFence.h
  src/COpenGLDriver.h
  src/COpenGLExtensionHandler.h
  src/COpenGLFrameBuffer.h
  src/COpenGLMaterialRenderer.h
  src/COpenGLOcclusionQuery.h
  src/COpenGLPersistentlyMappedBuffer.h
  src/COpenGLRenderBuffer.h
  src/COpenGLSLMaterialRenderer.h
  src/COpenGLTexture.h
  src/COpenGLVAO.h
  src/COSOperator.h
  src/CPakReader.h
  src/CPLYMeshFileLoader.h
  src/CPLYMeshWriter.h
  src/CReadFile.h
  src/CSceneManager.h
  src/CSceneNodeAnimatorCameraFPS.h
  src/CSceneNodeAnimatorCameraMaya.h
  src/CSceneNodeAnimatorDelete.h
  src/CSceneNodeAnimatorFlyCircle.h
  src/CSceneNodeAnimatorFlyStraight.h
  src/CSceneNodeAnimatorFollowSpline.h
  src/CSceneNodeAnimatorRotation.h
  src/CSceneNodeAnimatorTexture.h
  src/CSkinnedMesh.h
  src/CSkyBoxSceneNode.h
  src/CSkyDomeSceneNode.h
  src/CSMFMeshFileLoader.h
  src/CSoftware2MaterialRenderer.h
  src/CSoftwareDriver2.h
  src/CSoftwareDriver.h
  src/CSoftwareTexture2.h
  src/CSoftwareTexture.h
  src/CSphereSceneNode.h
  src/CSTLMeshFileLoader.h
  src/CSTLMeshWriter.h
  src/CTarReader.h
  src/CTimer.h
  src/CTRTextureGouraud.h
  src/CVideoModeList.h
  src/CWADReader.h
  src/CWriteFile.h
  src/CXMeshFileLoader.h
  src/CXMLReader.h
  src/CXMLReaderImpl.h
  src/CXMLWriter.h
  src/CZBuffer.h
  src/CZipReader.h
  src/dmfsupport.h
  src/FW_Mutex.h
  src/glext.h
  src/glxext.h
  src/IBurningShader.h
  src/IDepthBuffer.h
  src/IImagePresenter.h
  src/ISceneNodeAnimatorFinishing.h
  src/ITriangleRenderer.h
  src/IZBuffer.h
  src/os.h
  src/resource.h
  src/S2DVertex.h
  src/S4DVertex.h
  src/SoftwareDriver2_compile_config.h
  src/SoftwareDriver2_helper.h
  src/stdafx.h
  src/wglext.h
  )

INCLUDE_DIRECTORIES(
  src/zlib 
  )

SET (ZLIB_SOURCE_FILES
  src/zlib/adler32.c
  src/zlib/compress.c
  src/zlib/crc32.c
  src/zlib/deflate.c
  src/zlib/gzclose.c
  src/zlib/gzlib.c
  src/zlib/gzread.c
  src/zlib/gzwrite.c
  src/zlib/infback.c
  src/zlib/inffast.c
  src/zlib/inflate.c
  src/zlib/inftrees.c
  src/zlib/trees.c
  src/zlib/uncompr.c
  src/zlib/zutil.c
)

SET (ZLIB_HEADER_FILES
  src/zlib/crc32.h
  src/zlib/deflate.h
  src/zlib/gzguts.h
  src/zlib/inffast.h
  src/zlib/inffixed.h
  src/zlib/inflate.h
  src/zlib/inftrees.h
  src/zlib/trees.h
  src/zlib/zconf.h
  src/zlib/zlib.h
  src/zlib/zutil.h
  )

SET (JPEGLIB_SOURCE_FILES
  src/jpeglib/jaricom.c
  src/jpeglib/jcapimin.c
  src/jpeglib/jcapistd.c
  src/jpeglib/jcarith.c
  src/jpeglib/jccoefct.c
  src/jpeglib/jccolor.c
  src/jpeglib/jcdctmgr.c
  src/jpeglib/jchuff.c
  src/jpeglib/jcinit.c
  src/jpeglib/jcmainct.c
  src/jpeglib/jcmarker.c
  src/jpeglib/jcmaster.c
  src/jpeglib/jcomapi.c
  src/jpeglib/jcparam.c
  src/jpeglib/jcprepct.c
  src/jpeglib/jcsample.c
  src/jpeglib/jctrans.c
  src/jpeglib/jdapimin.c
  src/jpeglib/jdapistd.c
  src/jpeglib/jdarith.c
  src/jpeglib/jdatadst.c
  src/jpeglib/jdatasrc.c
  src/jpeglib/jdcoefct.c
  src/jpeglib/jdcolor.c
  src/jpeglib/jddctmgr.c
  src/jpeglib/jdhuff.c
  src/jpeglib/jdinput.c
  src/jpeglib/jdmainct.c
  src/jpeglib/jdmarker.c
  src/jpeglib/jdmaster.c
  src/jpeglib/jdmerge.c
  src/jpeglib/jdpostct.c
  src/jpeglib/jdsample.c
  src/jpeglib/jdtrans.c
  src/jpeglib/jerror.c
  src/jpeglib/jfdctflt.c
  src/jpeglib/jfdctfst.c
  src/jpeglib/jfdctint.c
  src/jpeglib/jidctflt.c
  src/jpeglib/jidctfst.c
  src/jpeglib/jidctint.c
  src/jpeglib/jmemmgr.c
  src/jpeglib/jmemnobs.c
  src/jpeglib/jquant1.c
  src/jpeglib/jquant2.c
  src/jpeglib/jutils.c
  )

INCLUDE_DIRECTORIES(
  src/jpeglib 
  )

SET (JPEGLIB_HEADER_FILES
  src/jpeglib/cderror.h 
  src/jpeglib/jmemsys.h 
  src/jpeglib/jmorecfg.h 
  src/jpeglib/jconfig.h 
  src/jpeglib/jdct.h 
  src/jpeglib/jerror.h 
  src/jpeglib/jinclude.h 
  src/jpeglib/jpegint.h 
  src/jpeglib/jpeglib.h 
  src/jpeglib/jversion.h 
  )

SET (LIBPNG_SOURCE_FILES
  src/libpng/example.c
  src/libpng/png.c
  src/libpng/pngerror.c
  src/libpng/pngget.c
  src/libpng/pngmem.c
  src/libpng/pngpread.c
  src/libpng/pngread.c
  src/libpng/pngrio.c
  src/libpng/pngrtran.c
  src/libpng/pngrutil.c
  src/libpng/pngset.c
  src/libpng/pngtest.c
  src/libpng/pngtrans.c
  src/libpng/pngwio.c
  src/libpng/pngwrite.c
  src/libpng/pngwtran.c
  src/libpng/pngwutil.c
)

INCLUDE_DIRECTORIES(
  src/libpng
  )

SET (LIBPNG_HEADER_FILES
  src/libpng/pngconf.h
  src/libpng/pngdebug.h
  src/libpng/png.h
  src/libpng/pnginfo.h
  src/libpng/pnglibconf.h
  src/libpng/pngpriv.h
  src/libpng/pngstruct.h
  )



SET (LIBAESGM_SOURCE_FILES
  src/aesGladman/aescrypt.cpp
  src/aesGladman/aeskey.cpp
  src/aesGladman/aestab.cpp
  src/aesGladman/fileenc.cpp
  src/aesGladman/hmac.cpp
  src/aesGladman/prng.cpp
  src/aesGladman/pwd2key.cpp
  src/aesGladman/sha1.cpp
  src/aesGladman/sha2.cpp

  )

SET (LIBAESGM_HEADER_FILES
  src/aesGladman/pwd2key.h
  src/aesGladman/sha1.h
  src/aesGladman/sha2.h
  src/aesGladman/aes.h
  src/aesGladman/aesopt.h
  src/aesGladman/fileenc.h
  src/aesGladman/hmac.h
  src/aesGladman/prng.h
  )

SET (BZIP2_SOURCE_FILES
  src/bzip2/blocksort.c
  src/bzip2/bzcompress.c
  src/bzip2/bzlib.c
  src/bzip2/bzlib.h
  src/bzip2/bzlib_private.h
  src/bzip2/crctable.c
  src/bzip2/decompress.c
  src/bzip2/huffman.c
  src/bzip2/randtable.c
  )

SET (BZIP2_HEADER_FILES
  src/bzip2/bzlib.h
  src/bzip2/bzlib_private.h
)

SET (LZMA_HEADER_FILES
  src/lzma/LzmaDec.c
  )

SET (LZMA_SOURCE_FILES
  src/lzma/LzmaDec.h
  src/lzma/Types.h
  )

SET (ALL_SOURCE_FILES
	${IRRMAIN_SOURCE_FILES}
        ${JPEGLIB_SOURCE_FILES}
	${ZLIB_SOURCE_FILES}
	${LIBPNG_SOURCE_FILES}
	${LIBAESGM_SOURCE_FILES}
        ${BZIP2_SOURCE_FILES}
        ${LZMA_SOURCE_FILES}
)

SET (ALL_HEADER_FILES
        ${JPEGLIB_HEADER_FILES}
	${IRRLICHT_PUBLIC_HEADER_FILES}
	${IRRLICHT_PRIVATE_HEADER_FILES}
	${ZLIB_HEADER_FILES}
	${LIBPNG_HEADER_FILES}
	${LIBAESGM_HEADER_FILES}
        ${BZIP2_HEADER_FILES}
        ${LZMA_HEADER_FILES}
)
