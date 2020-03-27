// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef _C_GLSL_LOADER_H_INCLUDED__
#define _C_GLSL_LOADER_H_INCLUDED__

#include <algorithm>

#include "irr/asset/IAssetLoader.h"

namespace irr
{
namespace asset
{

//!  Surface Loader for PNG files
class CGLSLLoader final : public asset::IAssetLoader
{
	public:
		bool isALoadableFileFormat(io::IReadFile* _file) const override
		{
			const size_t prevPos = _file->getPos();
			_file->seek(0u);
			char tmp[10] = { 0 };
			char* end = tmp+sizeof(tmp);
			auto filesize = _file->getSize();
			while (_file->getPos()+sizeof(tmp)<filesize)
			{
				_file->read(tmp,sizeof(tmp));
				if (strncmp(tmp,"#version ",9u)==0)
					return true;

				auto found = std::find(tmp,end,'#');
				if (found==end || found==tmp)
					continue;

				_file->seek(_file->getPos()+found-end);
			}
			_file->seek(prevPos);

			return false;
		}

		const char** getAssociatedFileExtensions() const override
		{
			static const char* ext[]{ "vert","tesc","tese","geom","frag","comp", nullptr };
			return ext;
		}

		uint64_t getSupportedAssetTypesBitfield() const override { return asset::IAsset::ET_SPECIALIZED_SHADER; }

		asset::SAssetBundle loadAsset(io::IReadFile* _file, const asset::IAssetLoader::SAssetLoadParams& _params, asset::IAssetLoader::IAssetLoaderOverride* _override = nullptr, uint32_t _hierarchyLevel = 0u) override;
};

} // namespace asset
} // namespace irr

#endif

