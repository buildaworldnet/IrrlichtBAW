// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#ifndef __C_VIDEO_OPEN_GL_H_INCLUDED__
#define __C_VIDEO_OPEN_GL_H_INCLUDED__

#include "IrrCompileConfig.h"

#include "SIrrCreationParameters.h"

namespace irr
{
	class CIrrDeviceWin32;
	class CIrrDeviceLinux;
	class CIrrDeviceSDL;
	class CIrrDeviceMacOSX;
}

#ifdef _IRR_COMPILE_WITH_OPENGL_

#include "CNullDriver.h"
#include "IMaterialRendererServices.h"
// also includes the OpenGL stuff
#include "COpenGLExtensionHandler.h"
#include "COpenGLDriverFence.h"
#include "COpenGLTransformFeedback.h"
#include "COpenGLVAO.h"

#include <map>

namespace irr
{

namespace video
{
	class COpenGLTexture;
	class COpenGLFrameBuffer;

	class COpenGLDriver : public CNullDriver, public IMaterialRendererServices, public COpenGLExtensionHandler
	{
		friend class COpenGLTexture;
	public:

		#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceWin32* device);
		//! inits the windows specific parts of the open gl driver
		bool initDriver(CIrrDeviceWin32* device);
		bool changeRenderContext(const SExposedVideoData& videoData, CIrrDeviceWin32* device);
		#endif

		#ifdef _IRR_COMPILE_WITH_X11_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceLinux* device);
		//! inits the GLX specific parts of the open gl driver
		bool initDriver(CIrrDeviceLinux* device, GLXContext* auxCtxts);
		bool changeRenderContext(const SExposedVideoData& videoData, CIrrDeviceLinux* device);
		#endif

		#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceSDL* device);
		#endif

		#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceMacOSX *device);
		#endif

		//! generic version which overloads the unimplemented versions
		bool changeRenderContext(const SExposedVideoData& videoData, void* device) {return false;}

		//! destructor
		virtual ~COpenGLDriver();

        virtual bool initAuxContext(const size_t& ctxIx);
        virtual bool deinitAuxContext();


        virtual IGPUBuffer* createGPUBuffer(const size_t &size, const void* data, const bool canModifySubData=false, const bool &inCPUMem=false, const E_GPU_BUFFER_ACCESS &usagePattern=EGBA_NONE);

	    virtual IGPUMappedBuffer* createPersistentlyMappedBuffer(const size_t &size, const void* data, const E_GPU_BUFFER_ACCESS &usagePattern, const bool &assumedCoherent, const bool &inCPUMem=true);

        virtual void bufferCopy(IGPUBuffer* readBuffer, IGPUBuffer* writeBuffer, const size_t& readOffset, const size_t& writeOffset, const size_t& length);

	    virtual scene::IGPUMeshDataFormatDesc* createGPUMeshDataFormatDesc();

	    virtual scene::IGPUMesh* createGPUMeshFromCPU(scene::ICPUMesh* mesh, const E_MESH_DESC_CONVERT_BEHAVIOUR& bufferOptions=EMDCB_CLONE_AND_MIRROR_LAYOUT);

		//! clears the zbuffer
		virtual bool beginScene(bool backBuffer=true, bool zBuffer=true,
				SColor color=SColor(255,0,0,0),
				const SExposedVideoData& videoData=SExposedVideoData(),
				core::rect<s32>* sourceRect=0);

		//! presents the rendered scene on the screen, returns false if failed
		virtual bool endScene();


		virtual void beginQuery(IQueryObject* query);
		virtual void endQuery(IQueryObject* query);
		virtual void beginQuery(IQueryObject* query, const size_t& index);
		virtual void endQuery(IQueryObject* query, const size_t& index);

        virtual IOcclusionQuery* createOcclusionQuery(const E_OCCLUSION_QUERY_TYPE& heuristic);

        virtual IQueryObject* createPrimitivesGeneratedQuery();
        virtual IQueryObject* createXFormFeedbackPrimitiveQuery();
        virtual IQueryObject* createElapsedTimeQuery();
        virtual IGPUTimestampQuery* createTimestampQuery();


        virtual void drawMeshBuffer(scene::IGPUMeshBuffer* mb, IOcclusionQuery* query);


		//! queries the features of the driver, returns true if feature is available
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const
		{
			return FeatureEnabled[feature] && COpenGLExtensionHandler::queryFeature(feature);
		}

		virtual const video::SMaterial& getCurrentMaterial() const {return Material;}

		//! Sets a material. All 3d drawing functions draw geometry now
		//! using this material.
		//! \param material: Material to be used from now on.
		virtual void setMaterial(const SMaterial& material);

        //! needs to be "deleted" since its not refcounted
        virtual IDriverFence* placeFence()
        {
            return new COpenGLDriverFence();
        }

		//! \return Returns the name of the video driver. Example: In case of the Direct3D8
		//! driver, it would return "Direct3D8.1".
		virtual const wchar_t* getName() const;

		//! deletes all dynamic lights there are
		virtual void deleteAllDynamicLights();

		//! adds a dynamic light, returning an index to the light
		//! \param light: the light data to use to create the light
		//! \return An index to the light, or -1 if an error occurs
		virtual s32 addDynamicLight(const SLight& light);

		//! Turns a dynamic light on or off
		//! \param lightIndex: the index returned by addDynamicLight
		//! \param turnOn: true to turn the light on, false to turn it off
		virtual void turnLightOn(s32 lightIndex, bool turnOn);

		//! returns the maximal amount of dynamic lights the device can handle
		virtual u32 getMaximalDynamicLightAmount() const;

		//! sets a viewport
		virtual void setViewPort(const core::rect<s32>& area);

		//! Only used by the internal engine. Used to notify the driver that
		//! the window was resized.
		virtual void OnResize(const core::dimension2d<u32>& size);

		//! Returns type of video driver
		virtual E_DRIVER_TYPE getDriverType() const;

		//! get color format of the current color buffer
		virtual ECOLOR_FORMAT getColorFormat() const;

		//! Can be called by an IMaterialRenderer to make its work easier.
		virtual void setBasicRenderStates(const SMaterial& material, const SMaterial& lastmaterial,
			bool resetAllRenderstates);


        virtual void setShaderConstant(const void* data, s32 location, E_SHADER_CONSTANT_TYPE type, u32 number=1);
        virtual void setShaderTextures(const s32* textureIndices, s32 location, E_SHADER_CONSTANT_TYPE type, u32 number=1);

		//! sets the current Texture
		//! Returns whether setting was a success or not.
		bool setActiveTexture(u32 stage, video::ITexture* texture, const video::STextureSamplingParams &sampleParams);

		GLuint constructSamplerInCache(const uint64_t &hashVal);

        virtual s32 addHighLevelShaderMaterial(
            const c8* vertexShaderProgram,
            const c8* controlShaderProgram,
            const c8* evaluationShaderProgram,
            const c8* geometryShaderProgram,
            const c8* pixelShaderProgram,
            u32 patchVertices=3,
            E_MATERIAL_TYPE baseMaterial=video::EMT_SOLID,
            IShaderConstantSetCallBack* callback=0,
            const char** xformFeedbackOutputs = NULL,
            const uint32_t& xformFeedbackOutputCount = 0,
            const E_XFORM_FEEDBACK_ATTRIBUTE_MODE& attribLayout = EXFAM_COUNT_INVALID,
            s32 userData=0,
            const c8* vertexShaderEntryPointName="main",
            const c8* controlShaderEntryPointName="main",
            const c8* evaluationShaderEntryPointName="main",
            const c8* geometryShaderEntryPointName="main",
            const c8* pixelShaderEntryPointName="main");

		//! Returns a pointer to the IVideoDriver interface. (Implementation for
		//! IMaterialRendererServices)
		virtual IVideoDriver* getVideoDriver();

		//! Returns the maximum amount of primitives (mostly vertices) which
		//! the device is able to render with one drawIndexedTriangleList
		//! call.
		virtual u32 getMaximalIndicesCount() const;

		virtual IRenderBuffer* addRenderBuffer(const core::dimension2d<u32>& size, ECOLOR_FORMAT format = ECF_A8R8G8B8);

        virtual IFrameBuffer* addFrameBuffer();

		virtual bool setRenderTarget(IFrameBuffer* frameBuffer, bool setNewViewport=true);

		virtual void blitRenderTargets(IFrameBuffer* in, IFrameBuffer* out, bool copyDepth=true,
										core::recti srcRect=core::recti(0,0,0,0),
										core::recti dstRect=core::recti(0,0,0,0),
										bool bilinearFilter=false);


		//! Clears the ZBuffer.
		virtual void clearZBuffer(const float &depth=0.0);

		virtual void clearStencilBuffer(const int32_t &stencil);

		virtual void clearZStencilBuffers(const float &depth, const int32_t &stencil);

		virtual void clearColorBuffer(const E_FBO_ATTACHMENT_POINT &attachment, const int32_t* vals);
		virtual void clearColorBuffer(const E_FBO_ATTACHMENT_POINT &attachment, const uint32_t* vals);
		virtual void clearColorBuffer(const E_FBO_ATTACHMENT_POINT &attachment, const float* vals);

		virtual void clearScreen(const E_SCREEN_BUFFERS &buffer, const float* vals);
		virtual void clearScreen(const E_SCREEN_BUFFERS &buffer, const uint32_t* vals);


		virtual ITransformFeedback* createTransformFeedback();

		//!
		virtual void bindTransformFeedback(ITransformFeedback* xformFeedback);

		virtual ITransformFeedback* getBoundTransformFeedback() {return CurrentXFormFeedback;}

        /** Only POINTS, LINES, and TRIANGLES are allowed as capture types.. no strips or fans!
        This issues an implicit call to bindTransformFeedback()
        **/
		virtual void beginTransformFeedback(ITransformFeedback* xformFeedback, const E_MATERIAL_TYPE& xformFeedbackShader, const scene::E_PRIMITIVE_TYPE& primType=scene::EPT_POINTS);

		//! A redundant wrapper call to ITransformFeedback::pauseTransformFeedback(), made just for clarity
		virtual void pauseTransformFeedback();

		//! A redundant wrapper call to ITransformFeedback::pauseTransformFeedback(), made just for clarity
		virtual void resumeTransformFeedback();

		virtual void endTransformFeedback();


		//! checks if an OpenGL error has happend and prints it
		//! for performance reasons only available in debug mode
		bool testGLError();

		//! Enable/disable a clipping plane.
		//! There are at least 6 clipping planes available for the user to set at will.
		//! \param index: The plane index. Must be between 0 and MaxUserClipPlanes.
		//! \param enable: If true, enable the clipping plane else disable it.
		virtual void enableClipPlane(u32 index, bool enable);

		//! Enable the 2d override material
		virtual void enableMaterial2D(bool enable=true);

		//! Returns the graphics card vendor name.
		virtual core::stringc getVendorInfo() {return VendorName;}

		//! Removes a texture from the texture cache and deletes it, freeing lot of memory.
		void removeTexture(ITexture* texture);

		//! Convert E_PRIMITIVE_TYPE to OpenGL equivalent
		GLenum primitiveTypeToGL(scene::E_PRIMITIVE_TYPE type) const;

		//! Get ZBuffer bits.
		GLenum getZBufferBits() const;

		//! sets the needed renderstates
		void setRenderStates3DMode();

	private:
	    COpenGLVAO* CurrentVAO;

        bool XFormFeedbackRunning;
	    COpenGLTransformFeedback* CurrentXFormFeedback;

		//! inits the parts of the open gl driver used on all platforms
		bool genericDriverInit();
		//! returns a device dependent texture from a software surface (IImage)
		virtual video::ITexture* createDeviceDependentTexture(IImage* surface, const io::path& name, void* mipmapData=NULL);
		virtual video::ITexture* createDeviceDependentTexture(const ITexture::E_TEXTURE_TYPE& type, const uint32_t* size, uint32_t mipmapLevels, const io::path& name, ECOLOR_FORMAT format = ECF_A8R8G8B8);

		// returns the current size of the screen or rendertarget
		virtual const core::dimension2d<u32>& getCurrentRenderTargetSize() const;

		void createMaterialRenderers();


		core::stringw Name;

		//! enumeration for rendering modes such as 2d and 3d for minizing the switching of renderStates.
		enum E_RENDER_MODE
		{
			ERM_NONE = 0,	// no render state has been set yet.
			ERM_2D,		// 2d drawing rendermode
			ERM_3D		// 3d rendering mode
		};

		E_RENDER_MODE CurrentRenderMode;
		//! bool to make all renderstates reset if set to true.
		bool ResetRenderStates;
		u8 AntiAlias;

		SMaterial Material, LastMaterial;
		COpenGLFrameBuffer* CurrentFBO;
		class STextureStageCache
		{
			const ITexture* CurrentTexture[MATERIAL_MAX_TEXTURES];
		public:
			STextureStageCache()
			{
				for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
				{
					CurrentTexture[i] = 0;
				}
			}

			~STextureStageCache()
			{
				clear();
			}

			void set(u32 stage, const ITexture* tex)
			{
				if (stage<MATERIAL_MAX_TEXTURES)
				{
					const ITexture* oldTexture=CurrentTexture[stage];
					if (tex)
						tex->grab();
					CurrentTexture[stage]=tex;
					if (oldTexture)
						oldTexture->drop();
				}
			}

			const ITexture* operator[](int stage) const
			{
				if ((u32)stage<MATERIAL_MAX_TEXTURES)
					return CurrentTexture[stage];
				else
					return 0;
			}

			void remove(const ITexture* tex);

			void clear();
		};
		STextureStageCache CurrentTexture;
        uint64_t CurrentSamplerHash[MATERIAL_MAX_TEXTURES];
        std::map<uint64_t,GLuint> SamplerMap;


		core::dimension2d<u32> CurrentRendertargetSize;

		core::stringc VendorName;

		//! Color buffer format
		ECOLOR_FORMAT ColorFormat;

		SIrrlichtCreationParameters Params;

		//! All the lights that have been requested; a hardware limited
		//! number of them will be used at once.
		struct RequestedLight
		{
			RequestedLight(SLight const & lightData)
				: LightData(lightData), DesireToBeOn(true) { }

			SLight	LightData;
			bool	DesireToBeOn;
		};
		core::array<RequestedLight> RequestedLights;

		#ifdef _IRR_WINDOWS_API_
			HDC HDc; // Private GDI Device Context
			HWND Window;
		#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
			CIrrDeviceWin32 *Win32Device;
		#endif
		#endif
		#ifdef _IRR_COMPILE_WITH_X11_DEVICE_
			GLXDrawable Drawable;
			Display* X11Display;
			CIrrDeviceLinux *X11Device;
		#endif
		#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_
			CIrrDeviceMacOSX *OSXDevice;
		#endif
		#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
			CIrrDeviceSDL *SDLDevice;
		#endif

		void* AuxContext;

		E_DEVICE_TYPE DeviceType;
	};

} // end namespace video
} // end namespace irr


#endif // _IRR_COMPILE_WITH_OPENGL_
#endif

