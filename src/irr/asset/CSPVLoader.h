// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef _C_SPIR_V_LOADER_H_INCLUDED__
#define _C_SPIR_V_LOADER_H_INCLUDED__

#include "irr/asset/IAssetLoader.h"

namespace irr
{
namespace asset
{

//!  Surface Loader for PNG files
class CSPVLoader final : public asset::IAssetLoader
{
		_IRR_STATIC_INLINE_CONSTEXPR uint32_t SPV_MAGIC_NUMBER = 0x07230203u;
	public:
		bool isALoadableFileFormat(io::IReadFile* _file) const override
		{
			uint32_t magicNumber = 0u;

			const size_t prevPos = _file->getPos();
			_file->seek(0u);
			_file->read(&magicNumber,sizeof(uint32_t));
			_file->seek(prevPos);

			return magicNumber==SPV_MAGIC_NUMBER;
		}

		const char** getAssociatedFileExtensions() const override
		{
			static const char* ext[]{ "spv", nullptr };
			return ext;
		}

		uint64_t getSupportedAssetTypesBitfield() const override { return asset::IAsset::ET_SHADER; }

		asset::SAssetBundle loadAsset(io::IReadFile* _file, const asset::IAssetLoader::SAssetLoadParams& _params, asset::IAssetLoader::IAssetLoaderOverride* _override = nullptr, uint32_t _hierarchyLevel = 0u) override;
};

} // namespace asset
} // namespace irr

#endif

