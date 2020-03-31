#include "os.h"

#include <cwchar>

#include "../../ext/MitsubaLoader/CMitsubaLoader.h"
#include "../../ext/MitsubaLoader/ParserUtil.h"


namespace irr
{
namespace ext
{
namespace MitsubaLoader
{

static uint64_t rgb32f_to_rgb19e7(const uint32_t _rgb[3])
{
	constexpr uint32_t mantissa_bitlen = 19u;
	constexpr uint32_t exp_bitlen = 7u;
	constexpr uint32_t exp_bias = 63u;

	constexpr uint32_t mantissa_bitlen_f32 = 23u;
	constexpr uint32_t exp_bitlen_f32 = 8u;
	constexpr uint32_t exp_bias_f32 = 127u;

	constexpr uint32_t mantissa_len_diff = mantissa_bitlen_f32 - mantissa_bitlen;
	constexpr uint32_t exp_bias_diff = exp_bias_f32 - exp_bias;

	uint64_t rgb19e7 = 0ull;
	for (uint32_t i = 0u; i < 3u; ++i)
	{
		uint64_t mantissa = _rgb[i] & 0x7fffffu;
		mantissa >>= mantissa_len_diff;
		mantissa <<= (mantissa_bitlen * i);
		rgb19e7 |= mantissa;
	}
	uint64_t exp = (_rgb[0] >> mantissa_bitlen_f32) & 0xffu;
	exp -= exp_bias_diff;
	rgb19e7 |= (exp << (3u * mantissa_bitlen));

	return rgb19e7;
}

static bsdf::SBSDFUnion bsdfNode2bsdfStruct(const CElementBSDF* _node)
{
	bsdf::SBSDFUnion retval;
	uint32_t bitfields = _node->type;
	switch (_node->type)
	{
	case CElementBSDF::Type::DIFFUSE:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHDIFFUSE:
		bitfields |= static_cast<uint32_t>(_node->diffuse.reflectance.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_REFL_TEX;
		bitfields |= static_cast<uint32_t>(_node->diffuse.alpha.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_MASK_ALPHA_U_TEX;
		if (_node->diffuse.reflectance.value.type==SPropertyElementData::SPECTRUM)
			retval.diffuse.reflectance = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->diffuse.reflectance.value.vvalue.pointer));
		if (_node->diffuse.alpha.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.diffuse.alpha[0]) = _node->diffuse.alpha.value.fvalue;
		break;
	case CElementBSDF::Type::DIELECTRIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::THINDIELECTRIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHDIELECTRIC:
		bitfields |= _node->dielectric.distribution << bsdf::BITFIELDS_SHIFT_NDF;
		bitfields |= static_cast<uint32_t>(_node->dielectric.alpha.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
		bitfields |= static_cast<uint32_t>(_node->dielectric.alphaV.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
		if (_node->dielectric.alpha.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.dielectric.alpha_u[0]) = _node->dielectric.alpha.value.fvalue;
		if (_node->dielectric.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY && _node->dielectric.alphaV.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.dielectric.alpha_v[0]) = _node->dielectric.alphaV.value.fvalue;
		//TODO int_ext_ior
		break;
	case CElementBSDF::Type::CONDUCTOR:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHCONDUCTOR:
		bitfields |= _node->conductor.distribution << bsdf::BITFIELDS_SHIFT_NDF;
		bitfields |= static_cast<uint32_t>(_node->conductor.alpha.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
		if (_node->conductor.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
			bitfields |= static_cast<uint32_t>(_node->conductor.alphaV.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
		if (_node->conductor.alpha.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.conductor.alpha_u[0]) = _node->conductor.alpha.value.fvalue;
		if (_node->conductor.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY && _node->conductor.alphaV.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.conductor.alpha_v[0]) = _node->conductor.alphaV.value.fvalue;
		if (_node->conductor.eta.type==SPropertyElementData::SPECTRUM)
			retval.conductor.ior[0] = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->conductor.eta.vvalue.pointer));
		if (_node->conductor.eta.type == SPropertyElementData::SPECTRUM)
			retval.conductor.ior[1] = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->conductor.k.vvalue.pointer));
		retval.conductor.ext_ior = _node->conductor.extEta;
		break;
	case CElementBSDF::Type::PLASTIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHPLASTIC:
		bitfields |= _node->plastic.distribution << bsdf::BITFIELDS_SHIFT_NDF;
		bitfields |= static_cast<uint32_t>(_node->plastic.alpha.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
		if (_node->plastic.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
			bitfields |= static_cast<uint32_t>(_node->plastic.alphaV.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
		bitfields |= static_cast<uint32_t>(_node->plastic.nonlinear)<<bsdf::BITFIELDS_SHIFT_NONLINEAR;
		if (_node->plastic.alpha.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.plastic.alpha_u[0]) = _node->plastic.alpha.value.fvalue;
		if (_node->plastic.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY && _node->plastic.alphaV.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.plastic.alpha_v[0]) = _node->plastic.alphaV.value.fvalue;
		//TODO int_ext_ior
		break;
	case CElementBSDF::Type::COATING:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHCOATING:
		bitfields |= _node->coating.distribution << bsdf::BITFIELDS_SHIFT_NDF;
		bitfields |= static_cast<uint32_t>(_node->coating.alpha.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
		if (_node->coating.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
			bitfields |= static_cast<uint32_t>(_node->coating.alphaV.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
		bitfields |= static_cast<uint32_t>(_node->coating.sigmaA.value.type==SPropertyElementData::INVALID)>>bsdf::BITFIELDS_SHIFT_SIGMA_A_TEX;
		if (_node->coating.alpha.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.coating.alpha_u[0]) = _node->coating.alpha.value.fvalue;
		if (_node->coating.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY && _node->coating.alphaV.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.coating.alpha_v[0]) = _node->coating.alphaV.value.fvalue;
		retval.coating.thickness = _node->coating.thickness;
		if (_node->coating.sigmaA.value.type==SPropertyElementData::SPECTRUM)
			retval.coating.sigmaA = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->coating.sigmaA.value.vvalue.pointer));
		break;
	case CElementBSDF::Type::BUMPMAP:
		break;
	case CElementBSDF::Type::PHONG:
		assert(0);
		break;
	case CElementBSDF::Type::WARD:
		bitfields |= _node->ward.variant << bsdf::BITFIELDS_SHIFT_WARD_VARIANT;
		bitfields |= static_cast<uint32_t>(_node->ward.alphaU.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
		bitfields |= static_cast<uint32_t>(_node->ward.alphaV.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
		if (_node->ward.alphaU.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.ward.alpha_u[0]) = _node->ward.alphaU.value.fvalue;
		if (_node->ward.alphaV.value.type==SPropertyElementData::FLOAT)
			reinterpret_cast<float&>(retval.ward.alpha_v[0]) = _node->ward.alphaV.value.fvalue;
		break;
	case CElementBSDF::Type::MIXTURE_BSDF:
		//TODO mixture will be translated into multiple blend BSDFs
		break;
	case CElementBSDF::Type::BLEND_BSDF:
		bitfields |= static_cast<uint32_t>(_node->blendbsdf.weight.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_WEIGHT_TEX;
		retval.blend.weight = rgb32f_to_rgb19e7(reinterpret_cast<uint32_t*>(core::vectorSIMDf(_node->blendbsdf.weight.value.fvalue).pointer));
		break;
	case CElementBSDF::Type::MASK:
		bitfields |= static_cast<uint32_t>(_node->mask.opacity.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_OPACITY_TEX;
		if (_node->mask.opacity.value.type==SPropertyElementData::SPECTRUM)
			retval.mask.opacity = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->mask.opacity.value.vvalue.pointer));
		break;
	case CElementBSDF::Type::TWO_SIDED:

		break;
	case CElementBSDF::Type::DIFFUSE_TRANSMITTER:
		bitfields |= static_cast<uint32_t>(_node->difftrans.transmittance.value.type==SPropertyElementData::INVALID)<<bsdf::BITFIELDS_SHIFT_SPEC_TRANS_TEX;
		if (_node->difftrans.transmittance.value.type==SPropertyElementData::SPECTRUM)
			retval.diffuseTransmitter.transmittance = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->difftrans.transmittance.value.vvalue.pointer));
		break;
	}
	//all union members has bitfields member at the same bytes
	retval.diffuse.bitfields = bitfields;

	return retval;
}

CMitsubaLoader::CMitsubaLoader(asset::IAssetManager* _manager) : asset::IAssetLoader(), manager(_manager)
{
#ifdef _IRR_DEBUG
	setDebugName("CMitsubaLoader");
#endif
}

bool CMitsubaLoader::isALoadableFileFormat(io::IReadFile* _file) const
{
	constexpr uint32_t stackSize = 16u*1024u;
	char tempBuff[stackSize+1];
	tempBuff[stackSize] = 0;

	static const char* stringsToFind[] = { "<?xml", "version", "scene"};
	static const wchar_t* stringsToFindW[] = { L"<?xml", L"version", L"scene"};
	constexpr uint32_t maxStringSize = 8u; // "version\0"
	static_assert(stackSize > 2u*maxStringSize, "WTF?");

	const size_t prevPos = _file->getPos();
	const auto fileSize = _file->getSize();
	if (fileSize < maxStringSize)
		return false;

	_file->seek(0);
	_file->read(tempBuff, 3u);
	bool utf16 = false;
	if (tempBuff[0]==0xEFu && tempBuff[1]==0xBBu && tempBuff[2]==0xBFu)
		utf16 = false;
	else if (reinterpret_cast<uint16_t*>(tempBuff)[0]==0xFEFFu)
	{
		utf16 = true;
		_file->seek(2);
	}
	else
		_file->seek(0);
	while (true)
	{
		auto pos = _file->getPos();
		if (pos >= fileSize)
			break;
		if (pos > maxStringSize)
			_file->seek(_file->getPos()-maxStringSize);
		_file->read(tempBuff,stackSize);
		for (auto i=0u; i<sizeof(stringsToFind)/sizeof(const char*); i++)
		if (utf16 ? (wcsstr(reinterpret_cast<wchar_t*>(tempBuff),stringsToFindW[i])!=nullptr):(strstr(tempBuff, stringsToFind[i])!=nullptr))
		{
			_file->seek(prevPos);
			return true;
		}
	}
	_file->seek(prevPos);
	return false;
}

const char** CMitsubaLoader::getAssociatedFileExtensions() const
{
	static const char* ext[]{ "xml", nullptr };
	return ext;
}


asset::SAssetBundle CMitsubaLoader::loadAsset(io::IReadFile* _file, const asset::IAssetLoader::SAssetLoadParams& _params, asset::IAssetLoader::IAssetLoaderOverride* _override, uint32_t _hierarchyLevel)
{
	ParserManager parserManager(manager->getFileSystem(),_override);
	if (!parserManager.parse(_file))
		return {};

	//
	auto currentDir = io::IFileSystem::getFileDir(_file->getFileName()) + "/";
	SContext ctx = {
		manager->getGeometryCreator(),
		manager->getMeshManipulator(),
		asset::IAssetLoader::SAssetLoadParams(_params.decryptionKeyLen,_params.decryptionKey,_params.cacheFlags,currentDir.c_str()),
		_override,
		parserManager.m_globalMetadata.get()
	};

	core::unordered_set<core::smart_refctd_ptr<asset::ICPUMesh>,core::smart_refctd_ptr<asset::ICPUMesh>::hash> meshes;

	for (auto& shapepair : parserManager.shapegroups)
	{
		auto* shapedef = shapepair.first;
		if (shapedef->type==CElementShape::Type::SHAPEGROUP)
			continue;

		core::smart_refctd_ptr<asset::ICPUMesh> mesh = getMesh(ctx,_hierarchyLevel,shapedef);
		if (!mesh)
			continue;

		IMeshMetadata* metadataptr = nullptr;
		auto found = meshes.find(mesh);
		if (found==meshes.end())
		{
			auto metadata = core::make_smart_refctd_ptr<IMeshMetadata>(
								core::smart_refctd_ptr(parserManager.m_globalMetadata),
								std::move(shapepair.second),
								shapedef
							);
			metadataptr = metadata.get();
			manager->setAssetMetadata(mesh.get(), std::move(metadata));
			meshes.insert(std::move(mesh));
		}
		else
		{
			assert(mesh->getMetadata() && strcmpi(mesh->getMetadata()->getLoaderName(),IMeshMetadata::LoaderName)==0);
			metadataptr = static_cast<IMeshMetadata*>(mesh->getMetadata());
		}

		metadataptr->instances.push_back({shapedef->getAbsoluteTransform(),shapedef->obtainEmitter()});
	}

	return {meshes};
}

CMitsubaLoader::SContext::shape_ass_type CMitsubaLoader::getMesh(SContext& ctx, uint32_t hierarchyLevel, CElementShape* shape)
{
	if (!shape)
		return nullptr;

	if (shape->type!=CElementShape::Type::INSTANCE)
		return loadBasicShape(ctx, hierarchyLevel, shape);
	else
	{
		// get group reference
		const CElementShape* parent = shape->instance.parent;
		if (!parent)
			return nullptr;
		assert(parent->type==CElementShape::Type::SHAPEGROUP);
		const CElementShape::ShapeGroup* shapegroup = &parent->shapegroup;
		
		return loadShapeGroup(ctx, hierarchyLevel, shapegroup);
	}
}

CMitsubaLoader::SContext::group_ass_type CMitsubaLoader::loadShapeGroup(SContext& ctx, uint32_t hierarchyLevel, const CElementShape::ShapeGroup* shapegroup)
{
	// find group
	auto found = ctx.groupCache.find(shapegroup);
	if (found != ctx.groupCache.end())
		return found->second;

	const auto children = shapegroup->children;

	auto mesh = core::make_smart_refctd_ptr<asset::CCPUMesh>();
	for (auto i=0u; i<shapegroup->childCount; i++)
	{
		auto child = children[i];
		if (!child)
			continue;

		core::smart_refctd_ptr<asset::ICPUMesh> lowermesh;
		assert(child->type!=CElementShape::Type::INSTANCE);
		if (child->type!=CElementShape::Type::SHAPEGROUP)
			lowermesh = loadBasicShape(ctx, hierarchyLevel, child);
		else
			lowermesh = loadShapeGroup(ctx, hierarchyLevel, &child->shapegroup);
		
		// skip if dead
		if (!lowermesh)
			continue;

		for (auto j=0u; j<lowermesh->getMeshBufferCount(); j++)
			mesh->addMeshBuffer(core::smart_refctd_ptr<asset::ICPUMeshBuffer>(lowermesh->getMeshBuffer(j)));
	}
	if (!mesh->getMeshBufferCount())
		return nullptr;

	mesh->recalculateBoundingBox();
	ctx.groupCache.insert({shapegroup,mesh});
	return mesh;
}

CMitsubaLoader::SContext::shape_ass_type CMitsubaLoader::loadBasicShape(SContext& ctx, uint32_t hierarchyLevel, CElementShape* shape)
{
	auto found = ctx.shapeCache.find(shape);
	if (found != ctx.shapeCache.end())
		return found->second;

	//! TODO: remove, after loader handedness fix
	static auto applyTransformToMB = [](asset::ICPUMeshBuffer* meshbuffer, core::matrix3x4SIMD tform) -> void
	{
		const auto index = meshbuffer->getPositionAttributeIx();
		core::vectorSIMDf vpos;
		for (uint32_t i = 0u; meshbuffer->getAttribute(vpos, index, i); i++)
		{
			tform.transformVect(vpos);
			meshbuffer->setAttribute(vpos, index, i);
		}
		meshbuffer->recalculateBoundingBox();
	};
	auto loadModel = [&](const ext::MitsubaLoader::SPropertyElementData& filename, int64_t index=-1) -> core::smart_refctd_ptr<asset::ICPUMesh>
	{
		assert(filename.type==ext::MitsubaLoader::SPropertyElementData::Type::STRING);
		auto retval = interm_getAssetInHierarchy(manager, filename.svalue, ctx.params, hierarchyLevel/*+ICPUSCene::MESH_HIERARCHY_LEVELS_BELOW*/, ctx.override);
		auto contentRange = retval.getContents();
		//
		uint32_t actualIndex = 0;
		if (index>=0ll)
		for (auto it=contentRange.first; it!=contentRange.second; it++)
		{
			auto meta = it->get()->getMetadata();
			if (!meta || core::strcmpi(meta->getLoaderName(),ext::MitsubaLoader::CSerializedMetadata::LoaderName))
				continue;
			auto serializedMeta = static_cast<CSerializedMetadata*>(meta);
			if (serializedMeta->id!=static_cast<uint32_t>(index))
				continue;
			actualIndex = it-contentRange.first;
			break;
		}
		//
		if (contentRange.first+actualIndex < contentRange.second)
		{
			auto asset = contentRange.first[actualIndex];
			if (asset && asset->getAssetType()==asset::IAsset::ET_MESH)
			{
				// make a (shallow) copy because the mesh will get mutilated and abused for metadata
				auto mesh = core::smart_refctd_ptr_static_cast<asset::ICPUMesh>(asset);
				auto copy = core::make_smart_refctd_ptr<asset::CCPUMesh>();
				for (auto j=0u; j<mesh->getMeshBufferCount(); j++)
					copy->addMeshBuffer(core::smart_refctd_ptr<asset::ICPUMeshBuffer>(mesh->getMeshBuffer(j)));
				copy->recalculateBoundingBox();
				manager->setAssetMetadata(copy.get(),core::smart_refctd_ptr<asset::IAssetMetadata>(mesh->getMetadata()));
				return copy;
			}
			else
				return nullptr;
		}
		else
			return nullptr;
	};

	core::smart_refctd_ptr<asset::ICPUMesh> mesh;
	bool flipNormals = false;
	bool faceNormals = false;
	float maxSmoothAngle = NAN;
	switch (shape->type)
	{
		case CElementShape::Type::CUBE:
		{
			auto cubeData = ctx.creator->createCubeMesh(core::vector3df(2.f));



			mesh = ctx.creator->createCubeMesh(core::vector3df(2.f));
			flipNormals = flipNormals!=shape->cube.flipNormals;
		}
			break;
		case CElementShape::Type::SPHERE:
			mesh = ctx.creator->createSphereMesh(1.f,64u,64u);
			flipNormals = flipNormals!=shape->sphere.flipNormals;
			{
				core::matrix3x4SIMD tform;
				tform.setScale(core::vectorSIMDf(shape->sphere.radius,shape->sphere.radius,shape->sphere.radius));
				tform.setTranslation(shape->sphere.center);
				shape->transform.matrix = core::concatenateBFollowedByA(shape->transform.matrix,core::matrix4SIMD(tform));
			}
			break;
		case CElementShape::Type::CYLINDER:
			{
				auto diff = shape->cylinder.p0-shape->cylinder.p1;
				mesh = ctx.creator->createCylinderMesh(1.f, 1.f, 64);
				core::vectorSIMDf up(0.f);
				float maxDot = diff[0];
				uint32_t index = 0u;
				for (auto i = 1u; i < 3u; i++)
					if (diff[i] < maxDot)
					{
						maxDot = diff[i];
						index = i;
					}
				up[index] = 1.f;
				core::matrix3x4SIMD tform;
				// mesh is left haded so transforming by LH matrix is fine (I hope but lets check later on)
				core::matrix3x4SIMD::buildCameraLookAtMatrixLH(shape->cylinder.p0,shape->cylinder.p1,up).getInverse(tform);
				core::matrix3x4SIMD scale;
				scale.setScale(core::vectorSIMDf(shape->cylinder.radius,shape->cylinder.radius,core::length(diff).x));
				shape->transform.matrix = core::concatenateBFollowedByA(shape->transform.matrix,core::matrix4SIMD(core::concatenateBFollowedByA(tform,scale)));
			}
			flipNormals = flipNormals!=shape->cylinder.flipNormals;
			break;
		case CElementShape::Type::RECTANGLE:
			mesh = ctx.creator->createRectangleMesh(core::vector2df_SIMD(1.f,1.f));
			flipNormals = flipNormals!=shape->rectangle.flipNormals;
			break;
		case CElementShape::Type::DISK:
			mesh = ctx.creator->createDiskMesh(1.f,64u);
			flipNormals = flipNormals!=shape->disk.flipNormals;
			break;
		case CElementShape::Type::OBJ:
			mesh = loadModel(shape->obj.filename);
			flipNormals = flipNormals==shape->obj.flipNormals;
			faceNormals = shape->obj.faceNormals;
			maxSmoothAngle = shape->obj.maxSmoothAngle;
			if (mesh) // awaiting the LEFT vs RIGHT HAND flag (just load as right handed in the future plz)
			{
				core::matrix3x4SIMD tform;
				tform.rows[0].x = -1.f; // restore handedness
				for (auto i = 0u; i < mesh->getMeshBufferCount(); i++)
					applyTransformToMB(mesh->getMeshBuffer(i), tform);
				mesh->recalculateBoundingBox();
			}
			if (mesh && shape->obj.flipTexCoords)
			{
				for (auto i = 0u; i < mesh->getMeshBufferCount(); i++)
				{
					auto meshbuffer = mesh->getMeshBuffer(i);
					core::vectorSIMDf uv;
					for (uint32_t i=0u; meshbuffer->getAttribute(uv, asset::EVAI_ATTR2, i); i++)
					{
						uv.y = -uv.y;
						meshbuffer->setAttribute(uv, asset::EVAI_ATTR2, i);
					}
				}
			}
			// collapse parameter gets ignored
			break;
		case CElementShape::Type::PLY:
			_IRR_DEBUG_BREAK_IF(true); // this code has never been tested
			mesh = loadModel(shape->ply.filename);
			flipNormals = flipNormals!=shape->ply.flipNormals;
			faceNormals = shape->ply.faceNormals;
			maxSmoothAngle = shape->ply.maxSmoothAngle;
			if (mesh && shape->ply.srgb)
			{
				uint32_t totalVertexCount = 0u;
				for (auto i = 0u; i < mesh->getMeshBufferCount(); i++)
					totalVertexCount += mesh->getMeshBuffer(i)->calcVertexCount();
				if (totalVertexCount)
				{
					constexpr uint32_t hidefRGBSize = 4u;
					auto newRGB = core::make_smart_refctd_ptr<asset::ICPUBuffer>(hidefRGBSize*totalVertexCount);
					uint32_t* it = reinterpret_cast<uint32_t*>(newRGB->getPointer());
					for (auto i = 0u; i < mesh->getMeshBufferCount(); i++)
					{
						auto meshbuffer = mesh->getMeshBuffer(i);
						uint32_t offset = reinterpret_cast<uint8_t*>(it)-reinterpret_cast<uint8_t*>(newRGB->getPointer());
						core::vectorSIMDf rgb;
						for (uint32_t i=0u; meshbuffer->getAttribute(rgb, asset::EVAI_ATTR1, i); i++,it++) // should be upstreamed into the PLY loader
						{
							for (auto i=0; i<3u; i++)
								rgb[i] = video::impl::srgb2lin(rgb[i]);
							meshbuffer->setAttribute(rgb,it,asset::EF_A2B10G10R10_UNORM_PACK32);
						}
						meshbuffer->getMeshDataAndFormat()->setVertexAttrBuffer(
								core::smart_refctd_ptr(newRGB),asset::EVAI_ATTR1,asset::EF_A2B10G10R10_UNORM_PACK32,hidefRGBSize,offset);
					}
				}
			}
			break;
		case CElementShape::Type::SERIALIZED:
			mesh = loadModel(shape->serialized.filename,shape->serialized.shapeIndex);
			flipNormals = flipNormals!=shape->serialized.flipNormals;
			faceNormals = shape->serialized.faceNormals;
			maxSmoothAngle = shape->serialized.maxSmoothAngle;
			break;
		case CElementShape::Type::SHAPEGROUP:
			_IRR_FALLTHROUGH;
		case CElementShape::Type::INSTANCE:
			assert(false);
			break;
		default:
			_IRR_DEBUG_BREAK_IF(true);
			break;
	}
	//
	if (!mesh)
		return nullptr;

	// flip normals if necessary
	if (flipNormals)
	for (auto i=0u; i<mesh->getMeshBufferCount(); i++)
		ctx.manipulator->flipSurfaces(mesh->getMeshBuffer(i));
	// flip normals if necessary
#define CRISS_FIX_THIS
#ifdef CRISS_FIX_THIS
	if (faceNormals || !std::isnan(maxSmoothAngle))
	{
		auto newMesh = core::make_smart_refctd_ptr<asset::CCPUMesh>();
		float smoothAngleCos = cos(core::radians(maxSmoothAngle));
		for (auto i=0u; i<mesh->getMeshBufferCount(); i++)
		{
			ctx.manipulator->filterInvalidTriangles(mesh->getMeshBuffer(i));
			auto newMeshBuffer = ctx.manipulator->createMeshBufferUniquePrimitives(mesh->getMeshBuffer(i));
			ctx.manipulator->calculateSmoothNormals(newMeshBuffer.get(), false, 0.f, asset::EVAI_ATTR3,
				[&](const asset::IMeshManipulator::SSNGVertexData& a, const asset::IMeshManipulator::SSNGVertexData& b, asset::ICPUMeshBuffer* buffer)
				{
					if (faceNormals)
						return a.indexOffset==b.indexOffset;
					else
						return core::dot(a.parentTriangleFaceNormal, b.parentTriangleFaceNormal).x >= smoothAngleCos;
				});

			asset::IMeshManipulator::SErrorMetric metrics[16];
			metrics[3].method = asset::IMeshManipulator::EEM_ANGLES;
			newMeshBuffer = ctx.manipulator->createOptimizedMeshBuffer(newMeshBuffer.get(),metrics);

			newMesh->addMeshBuffer(std::move(newMeshBuffer));
		}
		newMesh->recalculateBoundingBox();
		manager->setAssetMetadata(newMesh.get(), core::smart_refctd_ptr<asset::IAssetMetadata>(mesh->getMetadata()));
		mesh = std::move(newMesh);
	}
#endif

	//meshbuffer processing
	for (auto i = 0u; i < mesh->getMeshBufferCount(); i++)
	{
		auto* meshbuffer = mesh->getMeshBuffer(i);
		// add some metadata
		///auto meshbuffermeta = core::make_smart_refctd_ptr<IMeshBufferMetadata>(shapedef->type,shapedef->emitter ? shapedef->emitter.area:CElementEmitter::Area());
		///manager->setAssetMetadata(meshbuffer,std::move(meshbuffermeta));
		// TODO: change this with shader pipeline
		meshbuffer->getMaterial() = getBSDF(ctx, hierarchyLevel + asset::ICPUMesh::MESHBUFFER_HIERARCHYLEVELS_BELOW, shape->bsdf);
	}

	// cache and return
	ctx.shapeCache.insert({ shape,mesh });
	return mesh;
}


//! TODO: change to CPU graphics pipeline
CMitsubaLoader::SContext::bsdf_ass_type CMitsubaLoader::getBSDF(SContext& ctx, uint32_t hierarchyLevel, const CElementBSDF* bsdf)
{
	if (!bsdf)
		return nullptr; 

	auto found = ctx.pipelineCache.find(bsdf);
	if (found != ctx.pipelineCache.end())
		return found->second;

	// shader construction would take place here in the new pipeline
	SContext::bsdf_ass_type pipeline;
	NastyTemporaryBitfield nasty = { 0u };
	auto getColor = [](const SPropertyElementData& data) -> core::vectorSIMDf
	{
		switch (data.type)
		{
			case SPropertyElementData::Type::FLOAT:
				return core::vectorSIMDf(data.fvalue);
			case SPropertyElementData::Type::RGB:
				_IRR_FALLTHROUGH;
			case SPropertyElementData::Type::SRGB:
				return data.vvalue;
				break;
			case SPropertyElementData::Type::SPECTRUM:
				return data.vvalue;
				break;
			default:
				assert(false);
				break;
		}
		return core::vectorSIMDf();
	};
	constexpr uint32_t IMAGEVIEW_HIERARCHYLEVEL_BELOW = 1u; // below ICPUMesh, will move it there eventually with shader pipeline and become 2
	auto setTextureOrColorFrom = [&](const CElementTexture::SpectrumOrTexture& spctex) -> void
	{
		if (spctex.value.type!=SPropertyElementData::INVALID)
		{
			_mm_storeu_ps((float*)&pipeline.AmbientColor, getColor(spctex.value).getAsRegister());
		}
		else
		{
			pipeline.TextureLayer[0] = getTexture(ctx,hierarchyLevel+IMAGEVIEW_HIERARCHYLEVEL_BELOW,spctex.texture);
			nasty._bitfield |= MITS_USE_TEXTURE;
		}
	};
	// @criss you know that I'm doing absolutely nothing worth keeping around (not caring about BSDF actually)
	switch (bsdf->type)
	{
		case CElementBSDF::Type::DIFFUSE:
		case CElementBSDF::Type::ROUGHDIFFUSE:
			setTextureOrColorFrom(bsdf->diffuse.reflectance);
			break;
		case CElementBSDF::Type::DIELECTRIC:
		case CElementBSDF::Type::THINDIELECTRIC: // basically glass with no refraction
		case CElementBSDF::Type::ROUGHDIELECTRIC:
			{
				core::vectorSIMDf color(bsdf->dielectric.extIOR/bsdf->dielectric.intIOR);
				_mm_storeu_ps((float*)& pipeline.AmbientColor, color.getAsRegister());
			}
			break;
		case CElementBSDF::Type::CONDUCTOR:
		case CElementBSDF::Type::ROUGHCONDUCTOR:
			{
				auto color = core::vectorSIMDf(1.f)-getColor(bsdf->conductor.k);
				_mm_storeu_ps((float*)& pipeline.AmbientColor, color.getAsRegister());
			}
			break;
		case CElementBSDF::Type::PLASTIC:
		case CElementBSDF::Type::ROUGHPLASTIC:
			setTextureOrColorFrom(bsdf->plastic.diffuseReflectance);
			break;
		case CElementBSDF::Type::BUMPMAP:
			{
				pipeline = getBSDF(ctx,hierarchyLevel,bsdf->bumpmap.bsdf[0]);
				pipeline.TextureLayer[1] = getTexture(ctx,hierarchyLevel+IMAGEVIEW_HIERARCHYLEVEL_BELOW,bsdf->bumpmap.texture);
				nasty._bitfield |= MITS_BUMPMAP|reinterpret_cast<uint32_t&>(pipeline.MaterialTypeParam);				
			}
			break;
		case CElementBSDF::Type::TWO_SIDED:
			{
				pipeline = getBSDF(ctx,hierarchyLevel,bsdf->twosided.bsdf[0]);
				nasty._bitfield |= MITS_TWO_SIDED|reinterpret_cast<uint32_t&>(pipeline.MaterialTypeParam);				
			}
			break;
		case CElementBSDF::Type::MASK:
			{
				pipeline = getBSDF(ctx,hierarchyLevel,bsdf->mask.bsdf[0]);
				//bsdf->mask.opacity // ran out of space in SMaterial (can be texture or constant)
				nasty._bitfield |= /*MITS_MASK|*/reinterpret_cast<uint32_t&>(pipeline.MaterialTypeParam);				
			}
			break;
		case CElementBSDF::Type::DIFFUSE_TRANSMITTER:
			setTextureOrColorFrom(bsdf->difftrans.transmittance);
			break;
		default:
			_IRR_DEBUG_BREAK_IF(true); // TODO: more BSDF untangling!
			break;
	}
	reinterpret_cast<uint32_t&>(pipeline.MaterialTypeParam) = nasty._bitfield;
	pipeline.BackfaceCulling = false;

	ctx.pipelineCache.insert({bsdf,pipeline});
	return pipeline;
}

CMitsubaLoader::SContext::tex_ass_type CMitsubaLoader::getTexture(SContext& ctx, uint32_t hierarchyLevel, const CElementTexture* tex)
{
	if (!tex)
		return {};

	auto found = ctx.textureCache.find(tex);
	if (found != ctx.textureCache.end())
		return found->second;

	video::SMaterialLayer<asset::ICPUTexture> layer;
	switch (tex->type)
	{
		case CElementTexture::Type::BITMAP:
			{
				auto retval = interm_getAssetInHierarchy(manager,tex->bitmap.filename.svalue,ctx.params,hierarchyLevel,ctx.override);
				auto contentRange = retval.getContents();
				if (contentRange.first < contentRange.second)
				{
					auto asset = contentRange.first[0];
					if (asset && asset->getAssetType() == asset::IAsset::ET_IMAGE)
					{/*
						auto 
						{
							const void* src_container[4] = { data, nullptr, nullptr, nullptr };
							video::convertColor<EF_R8_UNORM, EF_R8_SRGB>(src_container, data, dim.X, dim); 
						}*/
						video::convertColor<asset::EF_R8G8B8A8_SRGB, asset::EF_B8G8R8A8_SRGB>(nullptr, nullptr, 0, core::vector3d<uint32_t>(0,0,0));
						auto texture = core::smart_refctd_ptr_static_cast<asset::ICPUTexture>(asset);
						auto getSingleChannelFormat = [](asset::E_FORMAT format, uint32_t index) -> asset::E_FORMAT
						{
							auto ratio = asset::getBytesPerPixel(format);
							ratio = decltype(ratio)(ratio.getNumerator(),ratio.getDenominator()*asset::getFormatChannelCount(format));
							switch (ratio.getRoundedUpInteger())
							{
								case 1:
									if (asset::isSRGBFormat(format))
										return index!=3 ? format:asset::EF_R8_UNORM;
									else
									{
										bool _signed = asset::isSignedFormat(format);
										if (asset::isIntegerFormat(format))
											return _signed ? asset::EF_R8_SINT : asset::EF_R8_UINT;
										else
											return _signed ? asset::EF_R8_SNORM:asset::EF_R8_UNORM;
									}
									break;
								case 2:
									if (asset::isFloatingPointFormat(format))
										return asset::EF_R16_SFLOAT;
									else
									{
										bool _signed = asset::isSignedFormat(format);
										if (asset::isIntegerFormat(format))
											return _signed ? asset::EF_R16_SINT : asset::EF_R16_UINT;
										else
											return _signed ? asset::EF_R16_SNORM:asset::EF_R16_UNORM;
									}
								case 3:
									_IRR_FALLTHROUGH;
								case 4:
									if (asset::isFloatingPointFormat(format))
										return asset::EF_R32_SFLOAT;
									else if (asset::isSignedFormat(format))
										return asset::EF_R32_SINT;
									else
										return asset::EF_R32_UINT;
									break;
								default:
									break;
							}
							return format;
						};
						// TODO: scrap this
						auto extractChannel = [&](uint32_t index) -> core::smart_refctd_ptr<asset::ICPUTexture>
						{
							core::vector<asset::CImageData*> subimages; // this will leak like crazy
							for (uint32_t level=0u; level<=texture->getHighestMip(); level)
							{
								auto rng = texture->getMipMap(level);
								for (auto it=rng.first; it!=rng.second; it++)
								{
									auto* olddata = *it;
									auto format = getSingleChannelFormat(olddata->getColorFormat(), index);
									auto* data = new asset::CImageData(nullptr,olddata->getSliceMin(),olddata->getSliceMax(),olddata->getSupposedMipLevel(),format);
									_IRR_DEBUG_BREAK_IF(true);
									subimages.push_back(data);
								}
							}
							return core::smart_refctd_ptr<asset::ICPUTexture>(asset::ICPUTexture::create(std::move(subimages), texture->getSourceFilename(), texture->getType()), core::dont_grab);
						};
						//  TODO: instead of making new texure with extracted channel just create a buffer view with appropriate rrrr,gggg,bbbb,aaaa swizzle
						switch (tex->bitmap.channel)
						{
							// no GL_R8_SRGB support yet
							case CElementTexture::Bitmap::CHANNEL::R:
								layer.Texture = extractChannel(0);
								break;
							case CElementTexture::Bitmap::CHANNEL::G:
								layer.Texture = extractChannel(1);
								break;
							case CElementTexture::Bitmap::CHANNEL::B:
								layer.Texture = extractChannel(2);
								break;
							case CElementTexture::Bitmap::CHANNEL::A:
								layer.Texture = extractChannel(3);
								break;/* special conversions needed to CIE space
							case CElementTexture::Bitmap::CHANNEL::X:
							case CElementTexture::Bitmap::CHANNEL::Y:
							case CElementTexture::Bitmap::CHANNEL::Z:*/
							default:
								layer.Texture = std::move(texture);
								break;
						}
						//! TODO: this stuff (custom shader sampling code?)
						_IRR_DEBUG_BREAK_IF(tex->bitmap.uoffset != 0.f);
						_IRR_DEBUG_BREAK_IF(tex->bitmap.voffset != 0.f);
						_IRR_DEBUG_BREAK_IF(tex->bitmap.uscale != 1.f);
						_IRR_DEBUG_BREAK_IF(tex->bitmap.vscale != 1.f);
					}
				}
				// adjust gamma on pixels (painful and long process)
				if (!std::isnan(tex->bitmap.gamma))
				{
					_IRR_DEBUG_BREAK_IF(true); // TODO
				}
				switch (tex->bitmap.filterType)
				{
					case CElementTexture::Bitmap::FILTER_TYPE::EWA:
						_IRR_FALLTHROUGH; // we dont support this fancy stuff
					case CElementTexture::Bitmap::FILTER_TYPE::TRILINEAR:
						layer.SamplingParams.MinFilter = video::ETFT_LINEAR_LINEARMIP;
						layer.SamplingParams.MaxFilter = video::ETFT_LINEAR_NO_MIP;
						break;
					default:
						layer.SamplingParams.MinFilter = video::ETFT_NEAREST_NEARESTMIP;
						layer.SamplingParams.MaxFilter = video::ETFT_NEAREST_NO_MIP;
						break;
				}
				layer.SamplingParams.AnisotropicFilter = core::max(core::findMSB(uint32_t(tex->bitmap.maxAnisotropy)),1);
				auto getWrapMode = [](CElementTexture::Bitmap::WRAP_MODE mode) -> video::E_TEXTURE_CLAMP
				{
					switch (mode)
					{
						case CElementTexture::Bitmap::WRAP_MODE::CLAMP:
							return video::ETC_CLAMP_TO_EDGE;
							break;
						case CElementTexture::Bitmap::WRAP_MODE::MIRROR:
							return video::ETC_MIRROR;
							break;
						case CElementTexture::Bitmap::WRAP_MODE::ONE:
							_IRR_DEBUG_BREAK_IF(true); // TODO : replace whole texture?
							break;
						case CElementTexture::Bitmap::WRAP_MODE::ZERO:
							_IRR_DEBUG_BREAK_IF(true); // TODO : replace whole texture?
							break;
						default:
							break;
					}
					return video::ETC_REPEAT;
				};
				layer.SamplingParams.TextureWrapU = getWrapMode(tex->bitmap.wrapModeU);
				layer.SamplingParams.TextureWrapV = getWrapMode(tex->bitmap.wrapModeV);
			}
			break;
		case CElementTexture::Type::SCALE:
			_IRR_DEBUG_BREAK_IF(true); // TODO
			return getTexture(ctx,hierarchyLevel,tex->scale.texture);
			break;
		default:
			_IRR_DEBUG_BREAK_IF(true);
			break;
	}
	ctx.textureCache.insert({tex,layer});
	return std::move(layer);
}

static bsdf::E_OPCODE BSDFtype2opcode(const CElementBSDF* bsdf)
{
	switch (bsdf->type)
	{
	case CElementBSDF::Type::DIFFUSE:
		return bsdf::OP_DIFFUSE;
	case CElementBSDF::Type::ROUGHDIFFUSE:
		return bsdf::OP_ROUGHDIFFUSE;
	case CElementBSDF::Type::DIFFUSE_TRANSMITTER:
		return bsdf::OP_DIFFTRANS;
	case CElementBSDF::Type::DIELECTRIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::THINDIELECTRIC:
		return bsdf::OP_DIELECTRIC;
	case CElementBSDF::Type::ROUGHDIELECTRIC:
		return bsdf::OP_ROUGHDIELECTRIC;
	case CElementBSDF::Type::CONDUCTOR:
		return bsdf::OP_CONDUCTOR;
	case CElementBSDF::Type::ROUGHCONDUCTOR:
		return bsdf::OP_ROUGHCONDUCTOR;
	case CElementBSDF::Type::PLASTIC:
		return bsdf::OP_PLASTIC;
	case CElementBSDF::Type::ROUGHPLASTIC:
		return bsdf::OP_ROUGHPLASTIC;
	case CElementBSDF::Type::COATING:
		return bsdf::OP_COATING;
	case CElementBSDF::Type::ROUGHCOATING:
		return bsdf::OP_ROUGHCOATING;
	case CElementBSDF::Type::BUMPMAP:
		return bsdf::OP_BUMPMAP;
	case CElementBSDF::Type::WARD:
		return bsdf::OP_WARD;
	case CElementBSDF::Type::BLEND_BSDF:
	case CElementBSDF::Type::MIXTURE_BSDF:
	case CElementBSDF::Type::MASK:
		return bsdf::OP_BLEND;
	case CElementBSDF::Type::TWO_SIDED:
		return BSDFtype2opcode(bsdf->twosided.bsdf[0]);
	default:
		return bsdf::OP_INVALID;
	}
}

void CMitsubaLoader::genBSDFtreeTraversal(SContext& ctx, const CElementBSDF* bsdf)
{
	struct stack_el {
		const CElementBSDF* bsdf;
		uint32_t instr_1st_dword;
		bool visited;
	};
	core::stack<stack_el> stack;
	uint32_t firstFreeNormalReg = 1u;//normal reg val 0 means geom normal without any perturbations
	auto push = [&](const CElementBSDF* _bsdf, uint32_t _parent1stDword) {
		auto writeInheritableFlags = [](uint32_t& dst, uint32_t parent) {
			dst |= (parent & (bsdf::BITFIELDS_MASK_TWOSIDED << bsdf::BITFIELDS_SHIFT_TWOSIDED));
			dst |= (parent & (bsdf::INSTR_NORMAL_REG_MASK << bsdf::INSTR_NORMAL_REG_SHIFT));
		};
		uint32_t _1stdword = BSDFtype2opcode(_bsdf);
		writeInheritableFlags(_1stdword, _parent1stDword);
		switch (_bsdf->type)
		{
		case CElementBSDF::Type::DIFFUSE:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::ROUGHDIFFUSE:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::DIFFUSE_TRANSMITTER:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::DIELECTRIC:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::THINDIELECTRIC:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::ROUGHDIELECTRIC:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::CONDUCTOR:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::ROUGHCONDUCTOR:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::PLASTIC:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::ROUGHPLASTIC:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::WARD:
			stack.push({_bsdf,_1stdword,false}); break;
		case CElementBSDF::Type::COATING:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::ROUGHCOATING:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::BUMPMAP:
			_1stdword &= (~(bsdf::INSTR_NORMAL_REG_MASK<<bsdf::INSTR_NORMAL_REG_SHIFT));//zero-out normal reg bitfield
			assert(firstFreeNormalReg<=bsdf::INSTR_NORMAL_REG_MASK);
			_1stdword |= (firstFreeNormalReg & bsdf::INSTR_NORMAL_REG_MASK) << bsdf::INSTR_NORMAL_REG_SHIFT;//write new val
			++firstFreeNormalReg;
			stack.push({_bsdf,_1stdword,false});

			_1stdword = BSDFtype2opcode(_bsdf->coating.bsdf[0]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({_bsdf->coating.bsdf[0],_1stdword,false});
			break;
		case CElementBSDF::Type::BLEND_BSDF:
			stack.push({ _bsdf,_1stdword,false });
			_1stdword = BSDFtype2opcode(_bsdf->blendbsdf.bsdf[1]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({ _bsdf->coating.bsdf[1],_1stdword,false });
			_1stdword = BSDFtype2opcode(_bsdf->blendbsdf.bsdf[0]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({ _bsdf->coating.bsdf[0],_1stdword,false });
			break;
		case CElementBSDF::Type::MIXTURE_BSDF:
		{
			//mixture is translated into tree of blends
			uint32_t blendbsdf = _1stdword;
			assert(_bsdf->mixturebsdf.childCount > 1u);
			for (uint32_t i = 0u; i < _bsdf->mixturebsdf.childCount-1ull; ++i)
			{
				stack.push({_bsdf,blendbsdf,false});
				auto* mixchild_bsdf = _bsdf->mixturebsdf.bsdf[_bsdf->mixturebsdf.childCount-i-1u];
				uint32_t mixchild = BSDFtype2opcode(mixchild_bsdf);
				writeInheritableFlags(mixchild, _parent1stDword);
				stack.push({mixchild_bsdf,mixchild,false});
			}
			uint32_t child0 = BSDFtype2opcode(_bsdf->mixturebsdf.bsdf[0]);
			writeInheritableFlags(child0, _parent1stDword);
			stack.push({_bsdf->mixturebsdf.bsdf[0],child0,false});
		}	
			break;
		case CElementBSDF::Type::MASK:
			stack.push({_bsdf,_1stdword,false});
			_1stdword = BSDFtype2opcode(_bsdf->mask.bsdf[0]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({_bsdf->mask.bsdf[0],_1stdword,false});
			_1stdword = bsdf::OP_TRANSPARENT;
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({nullptr,_1stdword,false});
			break;
		}
	};
	_IRR_STATIC_INLINE_CONSTEXPR uint32_t INSTR_REG_SHIFT_REL = 32u;
	auto emitInstr = [](uint32_t _1stdword, const CElementBSDF* _node, uint32_t _regs) -> bsdf::instr_t {
		uint32_t op = (_1stdword & bsdf::INSTR_OPCODE_MASK);
		switch (op)
		{
		case bsdf::OP_ROUGHDIFFUSE:
			_1stdword |= static_cast<uint32_t>(_node->diffuse.alpha.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_MASK_ALPHA_U_TEX;
			_IRR_FALLTHROUGH;
		case bsdf::OP_DIFFUSE:
			_1stdword |= static_cast<uint32_t>(_node->diffuse.reflectance.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_REFL_TEX;
			break;
		case bsdf::OP_ROUGHDIELECTRIC:
			_1stdword |= _node->dielectric.distribution << bsdf::BITFIELDS_SHIFT_NDF;
			_1stdword |= static_cast<uint32_t>(_node->dielectric.alpha.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
			if (_node->dielectric.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
				_1stdword |= static_cast<uint32_t>(_node->dielectric.alphaV.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
			break;
		case bsdf::OP_ROUGHCONDUCTOR:
			_1stdword |= _node->conductor.distribution << bsdf::BITFIELDS_SHIFT_NDF;
			_1stdword |= static_cast<uint32_t>(_node->conductor.alpha.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
			if (_node->conductor.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
				_1stdword |= static_cast<uint32_t>(_node->conductor.alphaV.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
			break;
		case bsdf::OP_ROUGHPLASTIC:
			_1stdword |= _node->plastic.distribution << bsdf::BITFIELDS_SHIFT_NDF;
			_1stdword |= static_cast<uint32_t>(_node->plastic.alpha.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
			if (_node->plastic.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
				_1stdword |= static_cast<uint32_t>(_node->plastic.alphaV.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
			_IRR_FALLTHROUGH;
		case bsdf::OP_PLASTIC:
			_1stdword |= static_cast<uint32_t>(_node->plastic.nonlinear) << bsdf::BITFIELDS_SHIFT_NONLINEAR;
			break;
		case bsdf::OP_ROUGHCOATING:
			_1stdword |= _node->coating.distribution << bsdf::BITFIELDS_SHIFT_NDF;
			_1stdword |= static_cast<uint32_t>(_node->coating.alpha.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
			if (_node->coating.distribution == CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
				_1stdword |= static_cast<uint32_t>(_node->coating.alphaV.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
			_IRR_FALLTHROUGH;
		case bsdf::OP_COATING:
			_1stdword |= static_cast<uint32_t>(_node->coating.sigmaA.value.type == SPropertyElementData::INVALID) >> bsdf::BITFIELDS_SHIFT_SIGMA_A_TEX;
			break;
		case bsdf::OP_WARD:
			_1stdword |= _node->ward.variant << bsdf::BITFIELDS_SHIFT_WARD_VARIANT;
			_1stdword |= static_cast<uint32_t>(_node->ward.alphaU.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_U_TEX;
			_1stdword |= static_cast<uint32_t>(_node->ward.alphaV.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_ALPHA_V_TEX;
			break;
		case bsdf::OP_BLEND:
			switch (_node->type)
			{
			case CElementBSDF::Type::BLEND_BSDF:
				_1stdword |= static_cast<uint32_t>(_node->blendbsdf.weight.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_WEIGHT_TEX;
				break;
			case CElementBSDF::Type::MASK:
				_1stdword |= static_cast<uint32_t>(_node->mask.opacity.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_WEIGHT_TEX;
				break;
			case CElementBSDF::Type::MIXTURE_BSDF:
				//always constant weights (not texture)
				break;
			default: break; //do not let warnings rise
			}
			break;
		case bsdf::OP_DIFFTRANS:
			_1stdword |= static_cast<uint32_t>(_node->difftrans.transmittance.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_SPEC_TRANS_TEX;
			break;
		default: break; //any other ones dont need any extra flags
		}

		bsdf::instr_t instr = _1stdword;
		instr |= static_cast<bsdf::instr_t>(_regs)<<INSTR_REG_SHIFT_REL;
		return instr;
	};
	uint32_t firstFreeReg = 0u;
	const uint32_t instrBufBase = ctx.instrBuffer.size();
	push(bsdf, 0u);
	while (!stack.empty())
	{
		auto& top = stack.top();
		uint32_t op = ((top.instr_1st_dword & bsdf::INSTR_OPCODE_MASK) >> bsdf::INSTR_OPCODE_SHIFT);
		if (op <= bsdf::OP_INVALID || top.visited)
		{
			uint32_t regs = 0u;
			if (op <= bsdf::OP_INVALID)
			{
				regs = (firstFreeReg & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_DST_SHIFT - INSTR_REG_SHIFT_REL);
				++firstFreeReg;
			}
			else if (op < bsdf::OP_BLEND)
			{
				regs = ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_DST_SHIFT - INSTR_REG_SHIFT_REL);
				regs |= ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_DST_SHIFT - INSTR_REG_SHIFT_REL);
			}
			else if (op == bsdf::OP_BLEND)
			{
				--firstFreeReg;
				regs = ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_DST_SHIFT - INSTR_REG_SHIFT_REL);
				regs |= ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_SRC1_SHIFT - INSTR_REG_SHIFT_REL);
				regs |= (firstFreeReg & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_SRC2_SHIFT - INSTR_REG_SHIFT_REL);
			}

			ctx.instrBuffer.push_back(emitInstr(top.instr_1st_dword, top.bsdf, regs));
			//TODO gen struct and insert into BSDF buf
			stack.pop();
		}
		else if (!top.visited)
		{
			top.visited = true;
			switch ((top.instr_1st_dword & bsdf::INSTR_OPCODE_MASK) >> bsdf::INSTR_OPCODE_SHIFT)
			{
			case bsdf::OP_BLEND:
				push(top.bsdf->blendbsdf.bsdf[1], top.instr_1st_dword);
				_IRR_FALLTHROUGH;
			default:
				push(top.bsdf->blendbsdf.bsdf[0], top.instr_1st_dword);
				break;
			}
		}
	}
	//reorder bumpmap instructions (they must be executed BEFORE instructions of tree nodes below it)
	core::vector<uint32_t> ixs;//indices of bumpmap instructions to reorder
	ixs.reserve(bsdf::INSTR_NORMAL_REG_MASK);
	for (uint32_t i = 0u; i < (ctx.instrBuffer.size()-instrBufBase); ++i)
	{
		const uint32_t ix = instrBufBase + i;
		const bsdf::instr_t& instr = ctx.instrBuffer[ix];
		if ((instr & bsdf::INSTR_OPCODE_MASK) == bsdf::OP_BUMPMAP)
			ixs.push_back(ix);
	}
	//reorder
	for (uint32_t ix : ixs)
	{
		const bsdf::instr_t instr = ctx.instrBuffer[ix];
		const uint32_t bmNormalReg = (instr >> bsdf::INSTR_NORMAL_REG_SHIFT)&bsdf::INSTR_NORMAL_REG_MASK;
		auto it = std::find_if(ctx.instrBuffer.crend()-ix-1u, ctx.instrBuffer.crend()-instrBufBase-1u, [bmNormalReg] (const bsdf::instr_t& _i) { return ((_i>>bsdf::INSTR_NORMAL_REG_SHIFT)&bsdf::INSTR_NORMAL_REG_MASK) != bmNormalReg; }).base()-1;
		ctx.instrBuffer.erase(ctx.instrBuffer.begin()+ix);
		ctx.instrBuffer.insert(it+1, instr);
	}
}


}
}
}