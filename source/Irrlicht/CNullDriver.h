// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_VIDEO_NULL_H_INCLUDED__
#define __C_VIDEO_NULL_H_INCLUDED__

#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IImagePresenter.h"
#include "IGPUProgrammingServices.h"
#include "irrArray.h"
#include "irrString.h"
#include "IMesh.h"
#include "IMeshBuffer.h"
#include "IMeshSceneNode.h"
#include "CFPSCounter.h"
#include "SVertexIndex.h"
#include "SExposedVideoData.h"

#if _MSC_VER && !__INTEL_COMPILER
	#define FW_AtomicCounter volatile long
#elif defined(__GNUC__)
	#define FW_AtomicCounter volatile int32_t
#endif

#ifdef _MSC_VER
#pragma warning( disable: 4996)
#endif

namespace irr
{
namespace io
{
	class IWriteFile;
	class IReadFile;
} // end namespace io
namespace video
{
	class IImageLoader;
	class IImageWriter;

	class CNullDriver : public IVideoDriver, public IGPUProgrammingServices
	{
    protected:
		//! destructor
		virtual ~CNullDriver();

	public:
        static FW_AtomicCounter ReallocationCounter;
        static FW_AtomicCounter incrementAndFetchReallocCounter();

		//! constructor
		CNullDriver(io::IFileSystem* io, const core::dimension2d<uint32_t>& screenSize);


        virtual bool initAuxContext() {return false;}
        virtual bool deinitAuxContext() {return false;}

        virtual IGPUBuffer* createGPUBuffer(const size_t &size, const void* data, const bool canModifySubData=false, const bool &inCPUMem=false, const E_GPU_BUFFER_ACCESS &usagePattern=EGBA_NONE) {return NULL;}

	    virtual IGPUMappedBuffer* createPersistentlyMappedBuffer(const size_t &size, const void* data, const E_GPU_BUFFER_ACCESS &usagePattern, const bool &assumedCoherent, const bool &inCPUMem=true) {return NULL;}

	    virtual void bufferCopy(IGPUBuffer* readBuffer, IGPUBuffer* writeBuffer, const size_t& readOffset, const size_t& writeOffset, const size_t& length);

	    virtual scene::IGPUMeshDataFormatDesc* createGPUMeshDataFormatDesc(core::LeakDebugger* dbgr=NULL) {return NULL;}

	    virtual scene::IGPUMesh* createGPUMeshFromCPU(scene::ICPUMesh* mesh, const E_MESH_DESC_CONVERT_BEHAVIOUR& bufferOptions=EMDCB_CLONE_AND_MIRROR_LAYOUT) {return NULL;}

		virtual bool beginScene(bool backBuffer=true, bool zBuffer=true,
				SColor color=SColor(255,0,0,0),
				const SExposedVideoData& videoData=SExposedVideoData(),
				core::rect<int32_t>* sourceRect=0);

		virtual bool endScene();

		//! Disable a feature of the driver.
		virtual void disableFeature(E_VIDEO_DRIVER_FEATURE feature, bool flag=true);

		//! queries the features of the driver, returns true if feature is available
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const;

		//! sets transformation
		virtual void setTransform(const E_4X3_TRANSFORMATION_STATE& state, const core::matrix4x3& mat);
		virtual void setTransform(const E_PROJECTION_TRANSFORMATION_STATE& state, const core::matrix4& mat);

		//! Retrieve the number of image loaders
		virtual uint32_t getImageLoaderCount() const;

		//! Retrieve the given image loader
		virtual IImageLoader* getImageLoader(uint32_t n);

		//! Retrieve the number of image writers
		virtual uint32_t getImageWriterCount() const;

		//! Retrieve the given image writer
		virtual IImageWriter* getImageWriter(uint32_t n);

		//! sets a material
		virtual void setMaterial(const SMaterial& material);

        //! needs to be "deleted" since its not refcounted
        virtual IDriverFence* placeFence(const bool& implicitFlushWaitSameThread=false) {return NULL;}

		//! loads a Texture
		virtual ITexture* getTexture(const io::path& filename, ECOLOR_FORMAT format = ECF_UNKNOWN);

		//! loads a Texture
		virtual ITexture* getTexture(io::IReadFile* file, ECOLOR_FORMAT format = ECF_UNKNOWN);

		//! Returns a texture by index
		virtual ITexture* getTextureByIndex(uint32_t index);

		//! Returns amount of textures currently loaded
		virtual uint32_t getTextureCount() const;

		//! Renames a texture
		virtual void renameTexture(ITexture* texture, const io::path& newName);

		//! creates a Texture
		virtual ITexture* addTexture(const ITexture::E_TEXTURE_TYPE& type, const uint32_t* size, uint32_t mipmapLevels, const io::path& name, ECOLOR_FORMAT format = ECF_A8R8G8B8);

        //!
        virtual ITexture* addTexture(const ITexture::E_TEXTURE_TYPE& type, const std::vector<CImageData*>& images, const io::path& name, ECOLOR_FORMAT format = ECF_UNKNOWN);

        //!
        virtual IMultisampleTexture* addMultisampleTexture(const IMultisampleTexture::E_MULTISAMPLE_TEXTURE_TYPE& type, const uint32_t& samples, const uint32_t* size, ECOLOR_FORMAT format = ECF_A8R8G8B8, const bool& fixedSampleLocations = false);

		//! A.
        virtual ITextureBufferObject* addTextureBufferObject(IGPUBuffer* buf, const ITextureBufferObject::E_TEXURE_BUFFER_OBJECT_FORMAT& format = ITextureBufferObject::ETBOF_RGBA8, const size_t& offset=0, const size_t& length=0);

		//! A.
        virtual E_MIP_CHAIN_ERROR validateMipChain(const ITexture* tex, const std::vector<CImageData*>& mipChain)
        {
            if (!tex)
                return EMCE_OTHER_ERR;

            if (mipChain.size()==0)
                return EMCE_NO_ERR;

            for (std::vector<CImageData*>::const_iterator it = mipChain.begin(); it!=mipChain.end(); it++)
            {
                CImageData* img = *it;
                if (!img)
                    return EMCE_INVALID_IMAGE;

                const uint32_t mipLevel = img->getSupposedMipLevel();
                if (mipLevel>=tex->getMipMapLevelCount())
                    return EMCE_MIP_LEVEL_OUT_OF_BOUND;

                core::vector3d<uint32_t> textureSizeAtThisMipLevel = *reinterpret_cast< const core::vector3d<uint32_t>* >(tex->getSize());
                textureSizeAtThisMipLevel /= core::vector3d<uint32_t>(0x1u<<mipLevel);
                if (textureSizeAtThisMipLevel.X==0)
                    textureSizeAtThisMipLevel.X = 1;
                if (textureSizeAtThisMipLevel.Y==0)
                    textureSizeAtThisMipLevel.Y = 1;
                if (textureSizeAtThisMipLevel.Z==0)
                    textureSizeAtThisMipLevel.Z = 1;

                core::aabbox3d<uint32_t> imgCube(core::vector3d<uint32_t>(img->getSliceMin()),core::vector3d<uint32_t>(img->getSliceMax()));
                if (!imgCube.isFullInside(core::aabbox3d<uint32_t>(core::vector3d<uint32_t>(0,0,0),textureSizeAtThisMipLevel)))
                    return EMCE_SUB_IMAGE_OUT_OF_BOUNDS;
            }

            return EMCE_NO_ERR;
        }

		//! Sets new multiple render targets.
		virtual bool setRenderTarget(IFrameBuffer* frameBuffer, bool setNewViewport=true);

		//! Clears the ZBuffer.
		/** Note that you usually need not to call this method, as it
		is automatically done in IVideoDriver::beginScene() or
		IVideoDriver::setRenderTarget() if you enable zBuffer. But if
		you have to render some special things, you can clear the
		zbuffer during the rendering process with this method any time.
		*/
		virtual void clearZBuffer(const float &depth=0.0);

		virtual void clearStencilBuffer(const int32_t &stencil);

		virtual void clearZStencilBuffers(const float &depth, const int32_t &stencil);

		virtual void clearColorBuffer(const E_FBO_ATTACHMENT_POINT &attachment, const int32_t* vals);
		virtual void clearColorBuffer(const E_FBO_ATTACHMENT_POINT &attachment, const uint32_t* vals);
		virtual void clearColorBuffer(const E_FBO_ATTACHMENT_POINT &attachment, const float* vals);

		virtual void clearScreen(const E_SCREEN_BUFFERS &buffer, const float* vals);
		virtual void clearScreen(const E_SCREEN_BUFFERS &buffer, const uint32_t* vals);


		virtual ITransformFeedback* createTransformFeedback() {return NULL;}

		//!
		virtual void bindTransformFeedback(ITransformFeedback* xformFeedback);

		virtual ITransformFeedback* getBoundTransformFeedback() {return NULL;}

        /** Only POINTS, LINES, and TRIANGLES are allowed as capture types.. no strips or fans!
        This issues an implicit call to bindTransformFeedback()
        **/
		virtual void beginTransformFeedback(ITransformFeedback* xformFeedback, const E_MATERIAL_TYPE& xformFeedbackShader, const scene::E_PRIMITIVE_TYPE& primType=scene::EPT_POINTS);

		//! A redundant wrapper call to ITransformFeedback::pauseTransformFeedback(), made just for clarity
		virtual void pauseTransformFeedback();

		//! A redundant wrapper call to ITransformFeedback::pauseTransformFeedback(), made just for clarity
		virtual void resumeTransformFeedback();

		virtual void endTransformFeedback();


		//! sets a viewport
		virtual void setViewPort(const core::rect<int32_t>& area);

		//! gets the area of the current viewport
		virtual const core::rect<int32_t>& getViewPort() const;

        virtual void drawMeshBuffer(scene::IGPUMeshBuffer* mb, IOcclusionQuery* query);

		//! Indirect Draw
		virtual void drawArraysIndirect(scene::IGPUMeshDataFormatDesc* vao, scene::E_PRIMITIVE_TYPE& mode, IGPUBuffer* indirectDrawBuff, const size_t& offset, const size_t& count, const size_t& stride, IOcclusionQuery* query = NULL);
		virtual void drawIndexedIndirect(scene::IGPUMeshDataFormatDesc* vao, scene::E_PRIMITIVE_TYPE& mode, const E_INDEX_TYPE& type, IGPUBuffer* indirectDrawBuff, const size_t& offset, const size_t& count, const size_t& stride, IOcclusionQuery* query = NULL);

		//! Draws a 3d line.
		virtual void draw3DLine(const core::vector3df& start,
			const core::vector3df& end, SColor color = SColor(255,255,255,255));

		//! Draws a 3d axis aligned box.
		virtual void draw3DBox(const core::aabbox3d<float>& box,
			SColor color = SColor(255,255,255,255));

		//! draws an 2d image
		virtual void draw2DImage(const video::ITexture* texture, const core::position2d<int32_t>& destPos);

		//! draws a set of 2d images, using a color and the alpha
		/** channel of the texture if desired. The images are drawn
		beginning at pos and concatenated in one line. All drawings
		are clipped against clipRect (if != 0).
		The subtextures are defined by the array of sourceRects
		and are chosen by the indices given.
		\param texture: Texture to be drawn.
		\param pos: Upper left 2d destination position where the image will be drawn.
		\param sourceRects: Source rectangles of the image.
		\param indices: List of indices which choose the actual rectangle used each time.
		\param kerningWidth: offset on position
		\param clipRect: Pointer to rectangle on the screen where the image is clipped to.
		This pointer can be 0. Then the image is not clipped.
		\param color: Color with which the image is colored.
		Note that the alpha component is used: If alpha is other than 255, the image will be transparent.
		\param useAlphaChannelOfTexture: If true, the alpha channel of the texture is
		used to draw the image. */
		virtual void draw2DImageBatch(const video::ITexture* texture,
				const core::position2d<int32_t>& pos,
				const core::array<core::rect<int32_t> >& sourceRects,
				const core::array<int32_t>& indices,
				int32_t kerningWidth = 0,
				const core::rect<int32_t>* clipRect = 0,
				SColor color=SColor(255,255,255,255),
				bool useAlphaChannelOfTexture=false);

		//! Draws a set of 2d images, using a color and the alpha channel of the texture.
		/** All drawings are clipped against clipRect (if != 0).
		The subtextures are defined by the array of sourceRects and are
		positioned using the array of positions.
		\param texture Texture to be drawn.
		\param pos Array of upper left 2d destinations where the images
		will be drawn.
		\param sourceRects Source rectangles of the image.
		\param clipRect Pointer to rectangle on the screen where the
		images are clipped to.
		If this pointer is 0 then the image is not clipped.
		\param color Color with which the image is drawn.
		Note that the alpha component is used. If alpha is other than
		255, the image will be transparent.
		\param useAlphaChannelOfTexture: If true, the alpha channel of
		the texture is used to draw the image. */
		virtual void draw2DImageBatch(const video::ITexture* texture,
				const core::array<core::position2d<int32_t> >& positions,
				const core::array<core::rect<int32_t> >& sourceRects,
				const core::rect<int32_t>* clipRect=0,
				SColor color=SColor(255,255,255,255),
				bool useAlphaChannelOfTexture=false);

		//! Draws a 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
		virtual void draw2DImage(const video::ITexture* texture, const core::position2d<int32_t>& destPos,
			const core::rect<int32_t>& sourceRect, const core::rect<int32_t>* clipRect = 0,
			SColor color=SColor(255,255,255,255), bool useAlphaChannelOfTexture=false);

		//! Draws a part of the texture into the rectangle.
		virtual void draw2DImage(const video::ITexture* texture, const core::rect<int32_t>& destRect,
			const core::rect<int32_t>& sourceRect, const core::rect<int32_t>* clipRect = 0,
			const video::SColor* const colors=0, bool useAlphaChannelOfTexture=false);

		//! Draws a 2d rectangle
		virtual void draw2DRectangle(SColor color, const core::rect<int32_t>& pos, const core::rect<int32_t>* clip = 0);

		//! Draws a 2d rectangle with a gradient.
		virtual void draw2DRectangle(const core::rect<int32_t>& pos,
			SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
			const core::rect<int32_t>* clip = 0);

		//! Draws the outline of a 2d rectangle
		virtual void draw2DRectangleOutline(const core::recti& pos, SColor color=SColor(255,255,255,255));

		//! Draws a 2d line.
		virtual void draw2DLine(const core::position2d<int32_t>& start,
					const core::position2d<int32_t>& end,
					SColor color=SColor(255,255,255,255));

		//! get color format of the current color buffer
		virtual ECOLOR_FORMAT getColorFormat() const;

		//! get screen size
		virtual const core::dimension2d<uint32_t>& getScreenSize() const;

		//! get render target size
		virtual const core::dimension2d<uint32_t>& getCurrentRenderTargetSize() const;

		// get current frames per second value
		virtual int32_t getFPS() const;

		//! returns amount of primitives (mostly triangles) were drawn in the last frame.
		//! very useful method for statistics.
		virtual uint32_t getPrimitiveCountDrawn( uint32_t param = 0 ) const;

		//! \return Returns the name of the video driver. Example: In case of the DIRECT3D8
		//! driver, it would return "Direct3D8.1".
		virtual const wchar_t* getName() const;

		//! Adds an external image loader to the engine.
		virtual void addExternalImageLoader(IImageLoader* loader);

		//! Adds an external image writer to the engine.
		virtual void addExternalImageWriter(IImageWriter* writer);

		//! Removes a texture from the texture cache and deletes it, freeing lot of
		//! memory.
		virtual void removeTexture(ITexture* texture);

		virtual void removeMultisampleTexture(IMultisampleTexture* tex);

		virtual void removeTextureBufferObject(ITextureBufferObject* tbo);

		virtual void removeRenderBuffer(IRenderBuffer* renderbuf);

		virtual void removeFrameBuffer(IFrameBuffer* framebuf);

		//! Removes all texture from the texture cache and deletes them, freeing lot of
		//! memory.
		virtual void removeAllTextures();

		virtual void removeAllMultisampleTextures();

		virtual void removeAllTextureBufferObjects();

		virtual void removeAllRenderBuffers();

		virtual void removeAllFrameBuffers();

		virtual IRenderBuffer* addRenderBuffer(const core::dimension2d<uint32_t>& size, ECOLOR_FORMAT format = ECF_A8R8G8B8);

		virtual IRenderBuffer* addMultisampleRenderBuffer(const uint32_t& samples, const core::dimension2d<uint32_t>& size, ECOLOR_FORMAT format = ECF_A8R8G8B8);

        virtual IFrameBuffer* addFrameBuffer();

		virtual void blitRenderTargets(IFrameBuffer* in, IFrameBuffer* out, bool copyDepth=true,
										core::recti srcRect=core::recti(0,0,0,0),
										core::recti dstRect=core::recti(0,0,0,0),
										bool bilinearFilter=false);

		//! Creates a normal map from a height map texture.
		//! \param amplitude: Constant value by which the height information is multiplied.
		virtual void makeNormalMapTexture(video::ITexture* texture, float amplitude=1.0f) const;

		//! Returns the maximum amount of primitives (mostly vertices) which
		//! the device is able to render with one drawIndexedTriangleList
		//! call.
		virtual uint32_t getMaximalIndicesCount() const;

		//! Enables or disables a texture creation flag.
		virtual void setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag, bool enabled);

		//! Returns if a texture creation flag is enabled or disabled.
		virtual bool getTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag) const;

		//! Creates a software image from a file.
		virtual std::vector<CImageData*> createImageDataFromFile(const io::path& filename);

		//! Creates a software image from a file.
		virtual std::vector<CImageData*> createImageDataFromFile(io::IReadFile* file);

		//! Creates a software image from a byte array.
		/** \param useForeignMemory: If true, the image will use the data pointer
		directly and own it from now on, which means it will also try to delete [] the
		data when the image will be destructed. If false, the memory will by copied. */
		virtual IImage* createImageFromData(CImageData* imageData, bool ownForeignMemory=true);

		//!
		virtual IImage* createImage(const ECOLOR_FORMAT& format, const core::dimension2d<uint32_t>& size);


	public:
		virtual void beginQuery(IQueryObject* query);
		virtual void endQuery(IQueryObject* query);
		virtual void beginQuery(IQueryObject* query, const size_t& index);
		virtual void endQuery(IQueryObject* query, const size_t& index);

        virtual IOcclusionQuery* createOcclusionQuery(const E_OCCLUSION_QUERY_TYPE& heuristic);

        virtual IQueryObject* createPrimitivesGeneratedQuery();
        virtual IQueryObject* createXFormFeedbackPrimitiveQuery();
        virtual IQueryObject* createElapsedTimeQuery();
        virtual IGPUTimestampQuery* createTimestampQuery();

		//! Only used by the engine internally.
		/** Used to notify the driver that the window was resized. */
		virtual void OnResize(const core::dimension2d<uint32_t>& size);

		//! Adds a new material renderer to the video device.
		virtual int32_t addMaterialRenderer(IMaterialRenderer* renderer,
				const char* name = 0);

		//! Returns driver and operating system specific data about the IVideoDriver.
		virtual const SExposedVideoData& getExposedVideoData();

		//! Returns type of video driver
		virtual E_DRIVER_TYPE getDriverType() const;

		//! Returns the transformation set by setTransform
		virtual const core::matrix4x3& getTransform(const E_4X3_TRANSFORMATION_STATE& state);

		virtual const core::matrix4& getTransform(const E_PROJECTION_TRANSFORMATION_STATE& state);

		//! Returns pointer to the IGPUProgrammingServices interface.
		virtual IGPUProgrammingServices* getGPUProgrammingServices();

		//! Returns pointer to material renderer or null
		virtual IMaterialRenderer* getMaterialRenderer(uint32_t idx);

		//! Returns amount of currently available material renderers.
		virtual uint32_t getMaterialRendererCount() const;

		//! Returns name of the material renderer
		virtual const char* getMaterialRendererName(uint32_t idx) const;

        virtual int32_t addHighLevelShaderMaterial(
            const char* vertexShaderProgram,
            const char* controlShaderProgram,
            const char* evaluationShaderProgram,
            const char* geometryShaderProgram,
            const char* pixelShaderProgram,
            uint32_t patchVertices=3,
            E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
            IShaderConstantSetCallBack* callback = 0,
            const char** xformFeedbackOutputs = NULL,
            const uint32_t& xformFeedbackOutputCount = 0,
            int32_t userData = 0,
            const char* vertexShaderEntryPointName="main",
            const char* controlShaderEntryPointName = "main",
            const char* evaluationShaderEntryPointName = "main",
            const char* geometryShaderEntryPointName = "main",
            const char* pixelShaderEntryPointName="main" );

        virtual int32_t addHighLevelShaderMaterialFromFiles(
            const io::path& vertexShaderProgramFileName,
            const io::path& controlShaderProgramFileName,
            const io::path& evaluationShaderProgramFileName,
            const io::path& geometryShaderProgramFileName,
            const io::path& pixelShaderProgramFileName,
            uint32_t patchVertices=3,
            E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
            IShaderConstantSetCallBack* callback = 0,
            const char** xformFeedbackOutputs = NULL,
            const uint32_t& xformFeedbackOutputCount = 0,
            int32_t userData = 0,
            const char* vertexShaderEntryPointName="main",
            const char* controlShaderEntryPointName = "main",
            const char* evaluationShaderEntryPointName = "main",
            const char* geometryShaderEntryPointName = "main",
            const char* pixelShaderEntryPointName="main");

        virtual int32_t addHighLevelShaderMaterialFromFiles(
            io::IReadFile* vertexShaderProgram,
            io::IReadFile* controlShaderProgram,
            io::IReadFile* evaluationShaderProgram,
            io::IReadFile* geometryShaderProgram,
            io::IReadFile* pixelShaderProgram,
            uint32_t patchVertices=3,
            E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
            IShaderConstantSetCallBack* callback = 0,
            const char** xformFeedbackOutputs = NULL,
            const uint32_t& xformFeedbackOutputCount = 0,
            int32_t userData = 0,
            const char* vertexShaderEntryPointName="main",
            const char* controlShaderEntryPointName = "main",
            const char* evaluationShaderEntryPointName = "main",
            const char* geometryShaderEntryPointName = "main",
            const char* pixelShaderEntryPointName="main");

        virtual bool replaceHighLevelShaderMaterial(const int32_t &materialIDToReplace,
            const char* vertexShaderProgram,
            const char* controlShaderProgram,
            const char* evaluationShaderProgram,
            const char* geometryShaderProgram,
            const char* pixelShaderProgram,
            uint32_t patchVertices=3,
            E_MATERIAL_TYPE baseMaterial=video::EMT_SOLID,
            IShaderConstantSetCallBack* callback=0,
            const char** xformFeedbackOutputs = NULL,
            const uint32_t& xformFeedbackOutputCount = 0,
            int32_t userData=0,
            const char* vertexShaderEntryPointName="main",
            const char* controlShaderEntryPointName="main",
            const char* evaluationShaderEntryPointName="main",
            const char* geometryShaderEntryPointName="main",
            const char* pixelShaderEntryPointName="main");


		//! Writes the provided image to disk file
		virtual bool writeImageToFile(IImage* image, const io::path& filename, uint32_t param = 0);

		//! Writes the provided image to a file.
		virtual bool writeImageToFile(IImage* image, io::IWriteFile * file, uint32_t param = 0);

		//! Sets the name of a material renderer.
		virtual void setMaterialRendererName(int32_t idx, const char* name);

		//! looks if the image is already loaded
		virtual video::ITexture* findTexture(const io::path& filename);

		//! Enable/disable a clipping plane.
		//! There are at least 6 clipping planes available for the user to set at will.
		//! \param index: The plane index. Must be between 0 and MaxUserClipPlanes.
		//! \param enable: If true, enable the clipping plane else disable it.
		virtual void enableClipPlane(uint32_t index, bool enable);

		//! Returns the graphics card vendor name.
		virtual std::string getVendorInfo() {return "Not available on this driver.";}

		//! Set the minimum number of vertices for which a hw buffer will be created
		/** \param count Number of vertices to set as minimum. */
		virtual void setMinHardwareBufferVertexCount(uint32_t count);

		//! Get the global Material, which might override local materials.
		/** Depending on the enable flags, values from this Material
		are used to override those of local materials of some
		meshbuffer being rendered. */
		virtual SOverrideMaterial& getOverrideMaterial();

		//! Get the 2d override material for altering its values
		virtual SMaterial& getMaterial2D();

		//! Enable the 2d override material
		virtual void enableMaterial2D(bool enable=true);

		//! Only used by the engine internally.
		virtual void setAllowZWriteOnTransparent(bool flag)
		{ AllowZWriteOnTransparent=flag; }

		//! Returns the maximum texture size supported.
		virtual const uint32_t* getMaxTextureSize(const ITexture::E_TEXTURE_TYPE& type) const;

		//! Color conversion convenience function
		/** Convert an image (as array of pixels) from source to destination
		array, thereby converting the color format. The pixel size is
		determined by the color formats.
		\param sP Pointer to source
		\param sF Color format of source
		\param sN Number of pixels to convert, both array must be large enough
		\param dP Pointer to destination
		\param dF Color format of destination
		*/
		virtual void convertColor(const void* sP, ECOLOR_FORMAT sF, int32_t sN,
				void* dP, ECOLOR_FORMAT dF) const;

		virtual bool checkDriverReset() {return false;}


		void addToTextureCache(video::ITexture* surface);

	protected:
        void addMultisampleTexture(IMultisampleTexture* tex);

        void addTextureBufferObject(ITextureBufferObject* tbo);

        void addRenderBuffer(IRenderBuffer* buffer);

        void addFrameBuffer(IFrameBuffer* framebuffer);

		//! deletes all textures
		void deleteAllTextures();

		//! opens the file and loads it into the surface
		video::ITexture* loadTextureFromFile(io::IReadFile* file, ECOLOR_FORMAT format=ECF_UNKNOWN, const io::path& hashName = "");

		//! returns a device dependent texture from a software surface (IImage)
		//! THIS METHOD HAS TO BE OVERRIDDEN BY DERIVED DRIVERS WITH OWN TEXTURES
		virtual video::ITexture* createDeviceDependentTexture(const ITexture::E_TEXTURE_TYPE& type, const uint32_t* size, uint32_t mipmapLevels, const io::path& name, ECOLOR_FORMAT format = ECF_A8R8G8B8);


		// adds a material renderer and drops it afterwards. To be used for internal creation
		int32_t addAndDropMaterialRenderer(IMaterialRenderer* m);

		//! deletes all material renderers
		void deleteMaterialRenders();

		// prints renderer version
		void printVersion();

		//! normal map lookup 32 bit version
		inline float nml32(int x, int y, int pitch, int height, int32_t *p) const
		{
			if (x < 0) x = pitch-1; if (x >= pitch) x = 0;
			if (y < 0) y = height-1; if (y >= height) y = 0;
			return (float)(((p[(y * pitch) + x])>>16) & 0xff);
		}

		//! normal map lookup 16 bit version
		inline float nml16(int x, int y, int pitch, int height, int16_t *p) const
		{
			if (x < 0) x = pitch-1; if (x >= pitch) x = 0;
			if (y < 0) y = height-1; if (y >= height) y = 0;

			return (float) getAverage ( p[(y * pitch) + x] );
		}

		struct SSurface
		{
			video::ITexture* Surface;

			bool operator < (const SSurface& other) const
			{
			    return Surface->getName()<other.Surface->getName();
			    /*
			    int res = strcmp(Surface->getName().getInternalName().c_str(),other.Surface->getName().getInternalName().c_str());
			    if (res<0)
                    return true;
                else if (res>0)
                    return false;
                else
                    return Surface < other.Surface;
                */
			}
		};

		struct SMaterialRenderer
		{
			core::stringc Name;
			IMaterialRenderer* Renderer;
		};

		class SDummyTexture : public ITexture
		{
                core::dimension2d<uint32_t> size;
		    public:
                SDummyTexture(const io::path& name) : ITexture(name), size(0,0) {}

                virtual const E_DIMENSION_COUNT getDimensionality() const {return EDC_TWO;}
                virtual const E_TEXTURE_TYPE getTextureType() const {return ETT_2D;}
                virtual const E_VIRTUAL_TEXTURE_TYPE getVirtualTextureType() const {return EVTT_OPAQUE_FILTERABLE;}
                virtual const uint32_t* getSize() const { return &size.Width; }
                virtual uint32_t getMipMapLevelCount() const {return 1;}
                virtual core::dimension2du getRenderableSize() const { return size; }
                virtual E_DRIVER_TYPE getDriverType() const { return video::EDT_NULL; }
                virtual ECOLOR_FORMAT getColorFormat() const { return video::ECF_A1R5G5B5; }
                virtual uint32_t getPitch() const { return 0; }
                virtual void regenerateMipMapLevels() {}
                virtual bool updateSubRegion(const ECOLOR_FORMAT &inDataColorFormat, const void* data, const uint32_t* minimum, const uint32_t* maximum, int32_t mipmap=0, const uint32_t& unpackRowByteAlignment=0) {return false;}
                virtual bool resize(const uint32_t* size, const uint32_t& mipLevels=0) {return false;}
		};
		std::vector<SSurface> Textures;

		core::array<IMultisampleTexture*> MultisampleTextures;
		core::array<ITextureBufferObject*> TextureBufferObjects;
		core::array<IRenderBuffer*> RenderBuffers;
		core::array<IFrameBuffer*> FrameBuffers;

		core::array<video::IImageLoader*> SurfaceLoader;
		core::array<video::IImageWriter*> SurfaceWriter;
		core::array<SMaterialRenderer> MaterialRenderers;


        IQueryObject* currentQuery[EQOT_COUNT][_IRR_XFORM_FEEDBACK_MAX_STREAMS_];

		io::IFileSystem* FileSystem;

		core::rect<int32_t> ViewPort;
		core::dimension2d<uint32_t> ScreenSize;

		uint32_t matrixModifiedBits;
		core::matrix4 ProjectionMatrices[EPTS_COUNT];
		core::matrix4x3 TransformationMatrices[E4X3TS_COUNT];

		CFPSCounter FPSCounter;

        scene::IGPUMeshBuffer* boxLineMesh;

		uint32_t PrimitivesDrawn;
		uint32_t MinVertexCountForVBO;

		uint32_t TextureCreationFlags;

		SExposedVideoData ExposedData;

		SOverrideMaterial OverrideMaterial;
		SMaterial OverrideMaterial2D;
		SMaterial InitMaterial2D;
		bool OverrideMaterial2DEnabled;

		bool AllowZWriteOnTransparent;

		bool FeatureEnabled[video::EVDF_COUNT];

		uint32_t MaxTextureSizes[ITexture::ETT_COUNT][3];
	};

} // end namespace video
} // end namespace irr


#endif
