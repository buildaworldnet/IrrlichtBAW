// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GRAPHICS_PIPELINE_H_INCLUDED__
#define __I_GRAPHICS_PIPELINE_H_INCLUDED__

#include "stdint.h"
#include "IReferenceCounted.h"
#include "IMeshBuffer.h"
#include "IShaderProgram.h"
///#include "SRasterState.h"

namespace irr
{
namespace video
{

/**
class SFixedFuncLayout
{
    public:
        //some of this shit will be dynamic
        video::SRasterState rasterState; //vkPipelineRasterizationStateCreateInfo
        video::SDepthStencilState depthStencilState; //vkPipelineDepthStencilStateCreateInfo
        video::SMultisampleState multisampleState; //vkPipelineMultisampleStateCreateInfo
        video::SGlobalBlendState globalBlendState; //vkPipelineColorBlendStateCreateInfo
        video::SSeparateBlendState separateBlendState[OGL_STATE_MAX_DRAW_BUFFERS]; //vkPipelineColorBlendAttachmentState
        video::SPipelineLayout pipelineLayout; //VkPipelineLayoutCreateInfo

        //! Descriptor Sets
        video::SCombinedImageSamplers combinedImageSampler[MATERIAL_MAX_TEXTURES];
        video::SStorageImage storageImages[MATERIAL_MAX_IMAGES];
        video::SInputAttachment inputAttachments[OGL_STATE_MAX_DRAW_BUFFERS];
        video::SUniformBuffer uniformBuffers[MATERIAL_MAX_UNIFORM_BUFFER_OBJECTS];
        video::SStorageBuffer storageBuffers[MATERIAL_MAX_SSBOs];
        video::SUniformTexelBuffer uniformTexelBuffers[MATERIAL_MAX_TEXTURES];
};
*/

class IGraphicsPipeline : public virtual IReferenceCounted
{
    public:
        //
    protected:
        IGraphicsPipeline() : meshFormatDesc(NULL), shader(NULL)
        {
        }

        scene::IGPUMeshDataFormatDesc* meshFormatDesc; //vkPipelineVertexInputStateCreateInfo,vkPipelineInputAssemblyStateCreateInfo
        video::IShaderProgram* shader;
        ///video::SFixedFuncLayout ffLayout;
};


} // end namespace video
} // end namespace irr

#endif



