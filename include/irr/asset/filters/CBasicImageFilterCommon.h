// Copyright (C) 2020- Mateusz 'DevSH' Kielan
// This file is part of the "IrrlichtBAW" engine.
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_C_BASIC_IMAGE_FILTER_COMMON_H_INCLUDED__
#define __IRR_C_BASIC_IMAGE_FILTER_COMMON_H_INCLUDED__

#include "irr/core/core.h"

#include "irr/asset/IImageFilter.h"

namespace irr
{
namespace asset
{

class CBasicImageFilterCommon
{
	public:
		template<typename F>
		static inline void executePerBlock(const ICPUImage* image, const IImage::SBufferCopy& region, F& f)
		{
			const auto& subresource = region.imageSubresource;
			const uint32_t layerLimit = subresource.baseArrayLayer+subresource.layerCount;

			const auto& params = image->getCreationParameters();
			IImage::SBufferCopy::TexelBlockInfo blockInfo(params.format);

			core::vector3du32_SIMD trueOffset;
			trueOffset.x = region.imageOffset.x;
			trueOffset.y = region.imageOffset.y;
			trueOffset.z = region.imageOffset.z;
			trueOffset = IImage::SBufferCopy::TexelsToBlocks(trueOffset,blockInfo);
			
			core::vector3du32_SIMD trueExtent = IImage::SBufferCopy::TexelsToBlocks(region.getTexelStrides(),blockInfo);

			const auto strides = region.getByteStrides(blockInfo,asset::getTexelOrBlockBytesize(params.format));

			core::vector3du32_SIMD blockCoord;
			for (auto& layer =(blockCoord[3]=subresource.baseArrayLayer); layer<layerLimit; layer++)
			for (auto& zBlock=(blockCoord[2]=trueOffset.z); zBlock<trueExtent.z; ++zBlock)
			for (auto& yBlock=(blockCoord[1]=trueOffset.y); yBlock<trueExtent.y; ++yBlock)
			for (auto& xBlock=(blockCoord[0]=trueOffset.x); xBlock<trueExtent.x; ++xBlock)
				f(region.getByteOffset(blockCoord,strides),blockCoord);
		}

		struct default_region_functor_t
		{
			inline bool operator()(IImage::SBufferCopy& newRegion, const IImage::SBufferCopy* referenceRegion) { return true; }
		};
		static default_region_functor_t default_region_functor;
		
		struct clip_region_functor_t
		{
			clip_region_functor_t(const ICPUImage::SSubresourceLayers& _subresrouce, const IImageFilter::IState::TexelRange& _range, E_FORMAT format) : 
				subresource(_subresrouce), range(_range), blockInfo(format), blockByteSize(getTexelOrBlockBytesize(format)) {}

			const ICPUImage::SSubresourceLayers&		subresource;
			const IImageFilter::IState::TexelRange&		range;
			const IImage::SBufferCopy::TexelBlockInfo	blockInfo;
			const uint32_t								blockByteSize;

			inline bool operator()(IImage::SBufferCopy& newRegion, const IImage::SBufferCopy* referenceRegion)
			{
				if (subresource.mipLevel!=referenceRegion->imageSubresource.mipLevel)
					return false;

				core::vector3du32_SIMD targetOffset(range.offset.x,range.offset.y,range.offset.z,subresource.baseArrayLayer);
				core::vector3du32_SIMD targetExtent(range.extent.width,range.extent.height,range.extent.depth,subresource.layerCount);
				auto targetLimit = targetOffset+targetExtent;

				const core::vector3du32_SIMD resultOffset(referenceRegion->imageOffset.x,referenceRegion->imageOffset.y,referenceRegion->imageOffset.z,referenceRegion->imageSubresource.baseArrayLayer);
				const core::vector3du32_SIMD resultExtent(referenceRegion->imageExtent.width,referenceRegion->imageExtent.height,referenceRegion->imageExtent.depth,referenceRegion->imageSubresource.layerCount);
				const auto resultLimit = resultOffset+resultExtent;

				auto offset = core::max(targetOffset,resultOffset);
				auto limit = core::min(targetLimit,resultLimit);
				if ((offset>=limit).any())
					return false;

				// compute new offset
				{
					const auto strides = referenceRegion->getByteStrides(blockInfo,blockByteSize);
					const core::vector3du32_SIMD offsetInOffset = offset-resultOffset;
					newRegion.bufferOffset += referenceRegion->getLocalOffset(offsetInOffset,strides);
				}

				newRegion.imageOffset.x = offset.x;
				newRegion.imageOffset.y = offset.y;
				newRegion.imageOffset.z = offset.z;
				newRegion.imageSubresource.baseArrayLayer = offset.w;
				auto extent = limit - offset;
				newRegion.imageExtent.width = extent.x;
				newRegion.imageExtent.height = extent.y;
				newRegion.imageExtent.depth = extent.z;
				newRegion.imageSubresource.layerCount = extent.w;
				return true;
			}
		};

		template<typename F, typename G=default_region_functor_t>
		static inline void executePerRegion(const ICPUImage* image, F& f,
											const IImage::SBufferCopy* _begin=image->getRegions().begin(),
											const IImage::SBufferCopy* _end=image->getRegions().end(),
											G& g=default_region_functor)
		{
			for (auto it=_begin; it!=_end; it++)
			{
				IImage::SBufferCopy region = *it;
				if (g(region,it))
					executePerBlock<F>(image, region, f);
			}
		}

	protected:
		virtual ~CBasicImageFilterCommon() =0;

		static inline bool validateSubresourceAndRange(	const ICPUImage::SSubresourceLayers& subresource,
														const IImageFilter::IState::TexelRange& range,
														const ICPUImage* image)
		{
			if (!image)
				return false;
			const auto& params = image->getCreationParameters();

			if (!(range.extent.width&&range.extent.height&&range.extent.depth))
				return false;

			if (range.offset.x+range.extent.width>params.extent.width)
				return false;
			if (range.offset.y+range.extent.height>params.extent.height)
				return false;
			if (range.offset.z+range.extent.depth>params.extent.depth)
				return false;
			
			if (subresource.baseArrayLayer+subresource.layerCount>params.arrayLayers)
				return false;
			if (subresource.mipLevel>=params.mipLevels)
				return false;

			return true;
		}
};

class CBasicInImageFilterCommon : public CBasicImageFilterCommon
{
	public:
		class CState : public IImageFilter::IState
		{
			public:
				virtual ~CState() {}

				ICPUImage::SSubresourceLayers	subresource = {};
				TexelRange						inRange = {};
				const ICPUImage*				inImage = nullptr;
		};
		using state_type = CState;

		static inline bool validate(CState* state)
		{
			if (!state)
				return nullptr;

			if (!CBasicImageFilterCommon::validateSubresourceAndRange(state->subresource,state->inRange,state->inImage))
				return false;

			return true;
		}

	protected:
		virtual ~CBasicInImageFilterCommon() = 0;
};
class CBasicOutImageFilterCommon : public CBasicImageFilterCommon
{
	public:
		class CState : public IImageFilter::IState
		{
			public:
				virtual ~CState() {}

				ICPUImage::SSubresourceLayers	subresource = {};
				TexelRange						outRange = {};
				ICPUImage*						outImage = nullptr;
		};
		using state_type = CState;

		static inline bool validate(CState* state)
		{
			if (!state)
				return nullptr;

			if (!CBasicImageFilterCommon::validateSubresourceAndRange(state->subresource,state->outRange,state->outImage))
				return false;

			return true;
		}

	protected:
		virtual ~CBasicOutImageFilterCommon() = 0;
};
class CBasicInOutImageFilterCommon : public CBasicImageFilterCommon
{
	public:
		class CState : public IImageFilter::IState
		{
			public:
				virtual ~CState() {}

				ICPUImage::SSubresourceLayers	inSubresource = {};
				TexelRange						inRange = {};
				ICPUImage*						inImage = nullptr;
				ICPUImage::SSubresourceLayers	outSubresource = {};
				TexelRange						outRange = {};
				ICPUImage*						outImage = nullptr;
		};
		using state_type = CState;

		static inline bool validate(CState* state)
		{
			if (!state)
				return nullptr;

			if (!CBasicImageFilterCommon::validateSubresourceAndRange(state->inSubresource,state->inRange,state->inImage))
				return false;
			if (!CBasicImageFilterCommon::validateSubresourceAndRange(state->outSubresource,state->outRange,state->outImage))
				return false;

			return true;
		}

	protected:
		virtual ~CBasicInOutImageFilterCommon() = 0;
};
// will probably need some per-pixel helper class/functions (that can run a templated functor per-pixel to reduce code clutter)

} // end namespace asset
} // end namespace irr

#endif