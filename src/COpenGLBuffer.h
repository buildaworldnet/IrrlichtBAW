#ifndef __C_OPEN_GL_BUFFER_H_INCLUDED__
#define __C_OPEN_GL_BUFFER_H_INCLUDED__

#include "IGPUBuffer.h"
#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_OPENGL_
#include "CNullDriver.h"
#include "COpenGLExtensionHandler.h"

namespace irr
{
namespace video
{



//! get the amount of Bits per Pixel of the given color format
inline u32 getBitsPerPixelFromGLenum(const GLenum& format)
{
    switch(format)
    {
        case GL_R8:
        case GL_R8I:
        case GL_R8UI:
            return 8;
        case GL_R16:
        case GL_R16F:
        case GL_R16I:
        case GL_R16UI:
            return 16;
        case GL_R32F:
        case GL_R32I:
        case GL_R32UI:
            return 32;
        case GL_RG8:
        case GL_RG8I:
        case GL_RG8UI:
            return 16;
        case GL_RG16:
        case GL_RG16F:
        case GL_RG16I:
        case GL_RG16UI:
            return 32;
        case GL_RG32F:
        case GL_RG32I:
        case GL_RG32UI:
            return 64;
        case GL_RGB32F:
        case GL_RGB32I:
        case GL_RGB32UI:
            return 96;
        case GL_RGBA8:
        case GL_RGBA8I:
        case GL_RGBA8UI:
            return 32;
        case GL_RGBA16:
        case GL_RGBA16F:
        case GL_RGBA16I:
        case GL_RGBA16UI:
            return 64;
        case GL_RGBA32F:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return 128;
        default:
            return 0;
    }
}


class COpenGLBuffer : public virtual IGPUBuffer
{
    public:
        COpenGLBuffer(const size_t &size, const void* data, const GLbitfield &flags) : BufferName(0), BufferSize(0), cachedFlags(0)
        {
			lastTimeReallocated = 0;
            COpenGLExtensionHandler::extGlCreateBuffers(1,&BufferName);
            if (BufferName==0)
                return;

            COpenGLExtensionHandler::extGlNamedBufferStorage(BufferName,size,data,flags);
            cachedFlags = flags;
            BufferSize = size;
        }

        virtual ~COpenGLBuffer()
        {
            if (BufferName)
                COpenGLExtensionHandler::extGlDeleteBuffers(1,&BufferName);
        }


        virtual core::E_BUFFER_TYPE getBufferType() const {return core::EBT_UNSPECIFIED_BUFFER;}

        inline const GLuint& getOpenGLName() const {return BufferName;}

        virtual const uint64_t &getSize() const {return BufferSize;}

        virtual void updateSubRange(const size_t& offset, const size_t& size, const void* data)
        {
            if (cachedFlags&GL_DYNAMIC_STORAGE_BIT)
                COpenGLExtensionHandler::extGlNamedBufferSubData(BufferName,offset,size,data);
        }

        virtual void clandestineRecreate(const size_t& size, const void* data)
        {
            COpenGLExtensionHandler::extGlDeleteBuffers(1,&BufferName);
            COpenGLExtensionHandler::extGlCreateBuffers(1,&BufferName);
            if (BufferName==0)
                return;

            COpenGLExtensionHandler::extGlNamedBufferStorage(BufferName,size,data,cachedFlags);
            BufferSize = size;

            lastTimeReallocated = CNullDriver::incrementAndFetchReallocCounter();
        }

        virtual bool canUpdateSubRange() const {return cachedFlags&GL_DYNAMIC_STORAGE_BIT;}

        virtual bool reallocate(const size_t &newSize, const bool& forceRetentionOfData=false, const bool &reallocateIfShrink=false)
        {
            return reallocate(newSize,forceRetentionOfData,reallocateIfShrink,0);
        }

    protected:
        GLbitfield cachedFlags;
        size_t BufferSize;
        GLuint BufferName;

        virtual bool reallocate(const size_t &newSize, const bool& forceRetentionOfData, const bool &reallocateIfShrink, const size_t& wraparoundStart)
        {
            if (newSize==BufferSize)
                return true;

            if (newSize<BufferSize&&(!reallocateIfShrink))
                return true;

            if (forceRetentionOfData)
            {
                GLuint newBufferHandle = 0;
                COpenGLExtensionHandler::extGlCreateBuffers(1,&newBufferHandle);
                if (newBufferHandle==0)
                    return false;

                COpenGLExtensionHandler::extGlNamedBufferStorage(newBufferHandle,newSize,NULL,cachedFlags);
                if (wraparoundStart&&newSize>BufferSize)
                {
                    size_t wrap = wraparoundStart%BufferSize;
                    COpenGLExtensionHandler::extGlCopyNamedBufferSubData(BufferName,newBufferHandle,wrap,wrap,BufferSize-wrap);
                    COpenGLExtensionHandler::extGlCopyNamedBufferSubData(BufferName,newBufferHandle,0,BufferSize,wrap);
                }
                else
                    COpenGLExtensionHandler::extGlCopyNamedBufferSubData(BufferName,newBufferHandle,0,0,core::min_(newSize,BufferSize));
                BufferSize = newSize;

                COpenGLExtensionHandler::extGlDeleteBuffers(1,&BufferName);
                BufferName = newBufferHandle;
            }
            else
            {
                COpenGLExtensionHandler::extGlDeleteBuffers(1,&BufferName);
                COpenGLExtensionHandler::extGlCreateBuffers(1,&BufferName);
                if (BufferName==0)
                    return false;

                COpenGLExtensionHandler::extGlNamedBufferStorage(BufferName,newSize,NULL,cachedFlags);
                BufferSize = newSize;
            }
            lastTimeReallocated = CNullDriver::incrementAndFetchReallocCounter();

            return true;
        }
};

} // end namespace video
} // end namespace irr

#endif
#endif
