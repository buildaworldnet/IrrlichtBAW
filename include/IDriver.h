// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_I_DRIVER_H_INCLUDED__
#define __IRR_I_DRIVER_H_INCLUDED__

#include "IDriverMemoryAllocation.h"
#include "IGPUBuffer.h"
#include "ITexture.h"
#include "IMultisampleTexture.h"
#include "ITextureBufferObject.h"
#include "IFrameBuffer.h"
#include "IVideoCapabilityReporter.h"
#include "IQueryObject.h"
#include "IGPUTimestampQuery.h"

namespace irr
{
namespace video
{

	//! Interface to the functionality of the graphics API device which does not require the submission of GPU commands onto a queue.
	/** This interface only deals with OpenGL and Vulkan concepts which do not require a command to be recorded in a command buffer
	and then submitted to a command queue, i.e. functions which only require VkDevice or VkPhysicalDevice.
	Examples of such functionality are the creation of buffers, textures, etc.*/
	class IDriver : public virtual IReferenceCounted, public IVideoCapabilityReporter
	{
	public:
	    //! Best for Mesh data, UBOs, SSBOs, etc.
	    virtual IDriverMemoryAllocation* allocateDeviceLocalMemory(const IDriverMemoryBacked::SDriverMemoryRequirements& additionalReqs) {return nullptr;}

	    //! If cannot or don't want to use device local memory, then this memory can be used
	    /** If the above fails (only possible on vulkan) or we have perfomance hitches due to video memory oversubscription.*/
	    virtual IDriverMemoryAllocation* allocateSpilloverMemory(const IDriverMemoryBacked::SDriverMemoryRequirements& additionalReqs) {return nullptr;}

	    //! Best for staging uploads to the GPU, such as resource streaming, and data to update the above memory with
	    virtual IDriverMemoryAllocation* allocateUpStreamingMemory(const IDriverMemoryBacked::SDriverMemoryRequirements& additionalReqs) {return nullptr;}

	    //! Best for staging downloads from the GPU, such as query results, Z-Buffer, video frames for recording, etc.
	    virtual IDriverMemoryAllocation* allocateDownStreamingMemory(const IDriverMemoryBacked::SDriverMemoryRequirements& additionalReqs) {return nullptr;}

	    //! Should be just as fast to play around with on the CPU as regular malloc'ed memory, but slowest to access with GPU
	    virtual IDriverMemoryAllocation* allocateCPUSideGPUVisibleMemory(const IDriverMemoryBacked::SDriverMemoryRequirements& additionalReqs) {return nullptr;}

	    //! Low level function used to implement the above, use with caution
        virtual IGPUBuffer* createGPUBuffer(const IDriverMemoryBacked::SDriverMemoryRequirements& initialMreqs, const bool canModifySubData=false) {return nullptr;}

	    //! Creates the buffer, allocates memory dedicated memory and binds it at once.
	    inline IGPUBuffer* createDeviceLocalGPUBufferOnDedMem(const size_t& size)
	    {
	        IDriverMemoryBacked::SDriverMemoryRequirements reqs;
	        reqs.vulkanReqs.size = size;
	        reqs.vulkanReqs.alignment = 0;
	        reqs.vulkanReqs.memoryTypeBits = 0xffffffffu;
	        reqs.memoryHeapLocation = IDriverMemoryAllocation::ESMT_DEVICE_LOCAL;
	        reqs.mappingCapability = IDriverMemoryAllocation::EMCF_CANNOT_MAP;
	        reqs.prefersDedicatedAllocation = true;
	        reqs.requiresDedicatedAllocation = true;
            return this->createGPUBufferOnDedMem(reqs,false);
	    }

	    //! Creates the buffer, allocates memory dedicated memory and binds it at once.
	    virtual IGPUBuffer* createSpilloverGPUBufferOnDedMem(const size_t& size)
	    {
	        IDriverMemoryBacked::SDriverMemoryRequirements reqs;
	        reqs.vulkanReqs.size = size;
	        reqs.vulkanReqs.alignment = 0;
	        reqs.vulkanReqs.memoryTypeBits = 0xffffffffu;
	        reqs.memoryHeapLocation = IDriverMemoryAllocation::ESMT_NOT_DEVICE_LOCAL;
	        reqs.mappingCapability = IDriverMemoryAllocation::EMCF_CANNOT_MAP;
	        reqs.prefersDedicatedAllocation = true;
	        reqs.requiresDedicatedAllocation = true;
            return this->createGPUBufferOnDedMem(reqs,false);
	    }

	    //! Creates the buffer, allocates memory dedicated memory and binds it at once.
	    virtual IGPUBuffer* createUpStreamingGPUBufferOnDedMem(const size_t& size)
	    {
	        IDriverMemoryBacked::SDriverMemoryRequirements reqs;
	        reqs.vulkanReqs.size = size;
	        reqs.vulkanReqs.alignment = 0;
	        reqs.vulkanReqs.memoryTypeBits = 0xffffffffu;
	        reqs.memoryHeapLocation = IDriverMemoryAllocation::ESMT_DEVICE_LOCAL;
	        reqs.mappingCapability = IDriverMemoryAllocation::EMCF_CAN_MAP_FOR_WRITE;
	        reqs.prefersDedicatedAllocation = true;
	        reqs.requiresDedicatedAllocation = true;
            return this->createGPUBufferOnDedMem(reqs,false);
	    }

	    //! Creates the buffer, allocates memory dedicated memory and binds it at once.
	    virtual IGPUBuffer* createDownStreamingGPUBufferOnDedMem(const size_t& size)
	    {
	        IDriverMemoryBacked::SDriverMemoryRequirements reqs;
	        reqs.vulkanReqs.size = size;
	        reqs.vulkanReqs.alignment = 0;
	        reqs.vulkanReqs.memoryTypeBits = 0xffffffffu;
	        reqs.memoryHeapLocation = IDriverMemoryAllocation::ESMT_NOT_DEVICE_LOCAL;
	        reqs.mappingCapability = IDriverMemoryAllocation::EMCF_CAN_MAP_FOR_READ|IDriverMemoryAllocation::EMCF_COHERENT|IDriverMemoryAllocation::EMCF_CACHED;
	        reqs.prefersDedicatedAllocation = true;
	        reqs.requiresDedicatedAllocation = true;
            return this->createGPUBufferOnDedMem(reqs,false);
	    }

	    //! Creates the buffer, allocates memory dedicated memory and binds it at once.
	    virtual IGPUBuffer* createCPUSideGPUVisibleGPUBufferOnDedMem(const size_t& size)
	    {
	        IDriverMemoryBacked::SDriverMemoryRequirements reqs;
	        reqs.vulkanReqs.size = size;
	        reqs.vulkanReqs.alignment = 0;
	        reqs.vulkanReqs.memoryTypeBits = 0xffffffffu;
	        reqs.memoryHeapLocation = IDriverMemoryAllocation::ESMT_NOT_DEVICE_LOCAL;
	        reqs.mappingCapability = IDriverMemoryAllocation::EMCF_CAN_MAP_FOR_READ|IDriverMemoryAllocation::EMCF_CAN_MAP_FOR_WRITE|IDriverMemoryAllocation::EMCF_COHERENT|IDriverMemoryAllocation::EMCF_CACHED;
	        reqs.prefersDedicatedAllocation = true;
	        reqs.requiresDedicatedAllocation = true;
            return this->createGPUBufferOnDedMem(reqs,false);
	    }

	    //! Low level function used to implement the above, use with caution
        virtual IGPUBuffer* createGPUBufferOnDedMem(const IDriverMemoryBacked::SDriverMemoryRequirements& initialMreqs, const bool canModifySubData=false) = 0;


        //! Creates a VAO or InputAssembly for OpenGL and Vulkan respectively
	    virtual scene::IGPUMeshDataFormatDesc* createGPUMeshDataFormatDesc(core::LeakDebugger* dbgr=NULL) {return nullptr;}


        //! Creates a framebuffer object with no attachments
        virtual IFrameBuffer* addFrameBuffer() {return nullptr;}


        //these will have to be created by a query pool anyway
        virtual IQueryObject* createPrimitivesGeneratedQuery() {return nullptr;}
        virtual IQueryObject* createXFormFeedbackPrimitiveQuery() {return nullptr;} //depr
        virtual IQueryObject* createElapsedTimeQuery() {return nullptr;}
        virtual IGPUTimestampQuery* createTimestampQuery() {return nullptr;}


		//! Convenience function for releasing all images in a mip chain.
		/**
		\param List of .
		\return .
		Bla bla. */
		static inline void dropWholeMipChain(const std::vector<CImageData*>& mipImages)
		{
		    for (std::vector<CImageData*>::const_iterator it=mipImages.begin(); it!=mipImages.end(); it++)
                (*it)->drop();
		}
		//!
		template< class Iter >
		static inline void dropWholeMipChain(Iter it, Iter limit)
		{
		    for (; it!=limit; it++)
                (*it)->drop();
		}
	};

} // end namespace video
} // end namespace irr


#endif

