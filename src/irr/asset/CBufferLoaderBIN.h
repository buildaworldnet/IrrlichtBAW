// Copyright (C) 2009-2012 Gaz Davidson
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irr/asset/IAssetLoader.h"
#include "irr/asset/ICPUMeshBuffer.h"

namespace irr
{
	namespace asset
	{
		//! Binaryloader capable of loading source code in binary format
		class CBufferLoaderBIN : public asset::IAssetLoader
		{
			protected:
				~CBufferLoaderBIN();

			public:
				virtual bool isALoadableFileFormat(io::IReadFile* _file) const override;

				virtual const char** getAssociatedFileExtensions() const override
				{
					static const char* extensions[]{ "bin", nullptr };
					return extensions;
				}

				//virtual uint64_t getSupportedAssetTypesBitfield() const override { return asset::IAsset::TODO; } should I add new IAsset as binary?

				virtual asset::SAssetBundle loadAsset(io::IReadFile* _file, const asset::IAssetLoader::SAssetLoadParams& _params, asset::IAssetLoader::IAssetLoaderOverride* _override = nullptr, uint32_t _hierarchyLevel = 0u) override;

			private:
				struct SContext
				{
					io::IReadFile* files;
					core::vector<core::smart_refctd_ptr<char>> sourceCodeBuffersCache;
				};

				void loadEntireSourceCode(SContext& _ctx);
		};
	}
}

