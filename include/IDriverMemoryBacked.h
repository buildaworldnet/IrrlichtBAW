// Copyright (C) 2016 Mateusz "DeVsh" Kielan
// This file is part of the "Irrlicht Engine" and "Build A World".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// and on http://irrlicht.sourceforge.net/forum/viewtopic.php?f=2&t=49672

#ifndef __I_DRIVER_MEMORY_BACKED_H_INCLUDED__
#define __I_DRIVER_MEMORY_BACKED_H_INCLUDED__

#include <algorithm>
#include "IDriverMemoryAllocation.h"

namespace irr
{
namespace video
{

typedef uint64_t VkDeviceSize;
//placeholder until we configure Vulkan SDK
typedef struct VkMemoryRequirements {
    VkDeviceSize    size;
    VkDeviceSize    alignment;
    uint32_t        memoryTypeBits;
} VkMemoryRequirements; //depr

//! Interface from which resources backed by IDriverMemoryAllocation, such as ITexture and IGPUBuffer, inherit from
class IDriverMemoryBacked : public virtual IReferenceCounted
{
    public:
        struct SDriverMemoryRequirements
        {
            VkMemoryRequirements vulkanReqs; ///< Used and valid only in Vulkan
            uint32_t memoryHeapLocation             : 2; //IDriverMemoryAllocation::E_SOURCE_MEMORY_TYPE
            uint32_t mappingCapability              : 4; //IDriverMemoryAllocation::E_MAPPING_CAPABILITY_FLAGS
            uint32_t prefersDedicatedAllocation     : 1;
            uint32_t requiresDedicatedAllocation    : 1;
        };
        //! Combine requirements
        /** \return true on success, some requirements are mutually exclusive, so it may be impossible to combine them. */
        static inline bool combineRequirements(SDriverMemoryRequirements& out, const SDriverMemoryRequirements& a, const SDriverMemoryRequirements& b)
        {
            if (!IDriverMemoryAllocation::validFlags(static_cast<IDriverMemoryAllocation::E_MAPPING_CAPABILITY_FLAGS>(a.mappingCapability)) ||
                !IDriverMemoryAllocation::validFlags(static_cast<IDriverMemoryAllocation::E_MAPPING_CAPABILITY_FLAGS>(b.mappingCapability)))
                return false;

            switch (a.memoryHeapLocation)
            {
                case IDriverMemoryAllocation::ESMT_DEVICE_LOCAL:
                case IDriverMemoryAllocation::ESMT_NOT_DEVICE_LOCAL:
                    if (b.memoryHeapLocation!=IDriverMemoryAllocation::ESMT_DONT_KNOW)
                        return false;
                    out.memoryHeapLocation = a.memoryHeapLocation;
                    break;
                default:
                    out.memoryHeapLocation = b.memoryHeapLocation;
                    break;
            }
            out.mappingCapability = a.mappingCapability|b.mappingCapability;
            out.prefersDedicatedAllocation = a.prefersDedicatedAllocation|b.prefersDedicatedAllocation;
            out.requiresDedicatedAllocation = a.requiresDedicatedAllocation|b.requiresDedicatedAllocation;

            //! Not on Vulkan, then OpenGL doesn't need more checks
            if (a.vulkanReqs.size==0u&&b.vulkanReqs.size==0u)
                return true;

            //! On Vulkan and don't know is not an option [can be removed later]
            if (out.memoryHeapLocation==IDriverMemoryAllocation::ESMT_DONT_KNOW)
                return false;

            auto isPowerOfTwo = [] (const uint64_t& N) -> bool {return N && !(N & (N - 1ll));};
            if (!isPowerOfTwo(a.vulkanReqs.alignment) || !isPowerOfTwo(b.vulkanReqs.alignment))
                return false;

            out.vulkanReqs.size = std::max(a.vulkanReqs.size,b.vulkanReqs.size);
            out.vulkanReqs.alignment = std::max(a.vulkanReqs.alignment,b.vulkanReqs.alignment);
            out.vulkanReqs.memoryTypeBits = a.vulkanReqs.memoryTypeBits&b.vulkanReqs.memoryTypeBits;
            if (out.vulkanReqs.memoryTypeBits!=0u)
                return false;

            return true;
        }

        //! Before allocating memory from the driver or trying to bind a range of an existing allocation
        virtual const SDriverMemoryRequirements& getMemoryReqs() const = 0;

        //! Returns the allocation which is bound to the resource
        virtual IDriverMemoryAllocation* getBoundMemory() = 0;

        //! Constant version
        virtual const IDriverMemoryAllocation* getBoundMemory() const = 0;

        //! Binds memory allocation to provide the backing for the resource.
        /** Available only on Vulkan, in OpenGL all resources create their own memory implicitly,
        so pooling or aliasing memory for different resources is not possible.
        There is no unbind, so once memory is bound it remains bound until you destroy the resource object.
        Actually all resource classes in OpenGL implement both IDriverMemoryBacked and IDriverMemoryAllocation,
        so effectively the memory is pre-bound at the time of creation.
        \return true on success, always false under OpenGL.*/
        virtual bool bindMemory(IDriverMemoryAllocation* allocation, const size_t& offset, const size_t& size) {return false;}
};

} // end namespace scene
} // end namespace irr

#endif
