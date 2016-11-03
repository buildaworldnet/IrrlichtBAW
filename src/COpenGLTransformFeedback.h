#ifndef __C_OPENGL_TRANSFORM_FEEDBACK_H_INCLUDED__
#define __C_OPENGL_TRANSFORM_FEEDBACK_H_INCLUDED__

#include "ITransformFeedback.h"

#ifdef _IRR_COMPILE_WITH_OPENGL_
#include "COpenGLExtensionHandler.h"
#include "COpenGLBuffer.h"
namespace irr
{
namespace video
{

class COpenGLBuffer;

class COpenGLTransformFeedback : public ITransformFeedback
{
    public:
        COpenGLTransformFeedback();

        virtual ~COpenGLTransformFeedback();


        virtual bool rebindRevalidate();

        virtual bool bindOutputBuffer(const size_t& index, IGPUBuffer* buffer, const size_t& offset=0, const size_t& length=0);

        virtual const IGPUBuffer* getOutputBuffer(const size_t &ix) const
        {
            if (ix>=_IRR_XFORM_FEEDBACK_MAX_BUFFERS_)
                return NULL;

            return xformFeedbackBuffers[ix];
        }

        virtual const size_t getOutputBufferOffset(const size_t &ix) const
        {
            if (ix>=_IRR_XFORM_FEEDBACK_MAX_BUFFERS_)
                return 0;

            return xformFeedbackBuffersOffset[ix];
        }

        inline const GLuint& getOpenGLHandle() const {return xformFeedbackHandle;}

        inline void setPrimitiveType(const GLenum& primType)
        {
            cachedPrimitiveType = primType;
        }
        inline void setMaterialType(const E_MATERIAL_TYPE& materialType)
        {
            cachedXFormFeedbackShader = materialType;
        }
        inline const int32_t& getMaterialType() const {return cachedXFormFeedbackShader;}

        //! Begin, Pause, Resume, End
		virtual void pauseTransformFeedback()
		{
		    pauseFeedback();
		}

		virtual void resumeTransformFeedback()
		{
		    beginResumeFeedback();
		}

        void beginResumeFeedback();

        void pauseFeedback();

        void endFeedback();


        bool isEnded() const {return !started;}


    protected:
        GLuint xformFeedbackHandle;
        size_t xformFeedbackBuffersOffset[_IRR_XFORM_FEEDBACK_MAX_BUFFERS_];
        size_t xformFeedbackBuffersSize[_IRR_XFORM_FEEDBACK_MAX_BUFFERS_];
        COpenGLBuffer* xformFeedbackBuffers[_IRR_XFORM_FEEDBACK_MAX_BUFFERS_];

        GLenum cachedPrimitiveType;
        int32_t cachedXFormFeedbackShader;

        bool started;
        uint32_t lastValidated;
};


} // end namespace video
} // end namespace irr
#endif // _IRR_COMPILE_WITH_OPENGL_

#endif



