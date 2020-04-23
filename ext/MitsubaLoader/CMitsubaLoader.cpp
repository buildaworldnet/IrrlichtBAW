#include "os.h"

#include <cwchar>

#include "../../ext/MitsubaLoader/CMitsubaLoader.h"
#include "../../ext/MitsubaLoader/ParserUtil.h"

namespace irr
{
using namespace asset;

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

bsdf::SBSDFUnion CMitsubaLoader::bsdfNode2bsdfStruct(SContext& _ctx, const CElementBSDF* _node, uint32_t _texHierLvl, float _mix2blend_weight, const CElementBSDF* _maskParent)
{
	auto textureData = [&, this](CElementTexture* _tex) {
		auto imgview = getTexture(_ctx, _texHierLvl, _tex).first;
		auto& img = imgview->getCreationParameters().image;
		return bsdf::getTextureData(img.get(), m_texPacker.get());
	};
	auto inheritOpacity = [_maskParent,&textureData](auto& _bsdfStruct) {
		if (_maskParent)
		{
			if (_maskParent->mask.opacity.value.type == SPropertyElementData::SPECTRUM)
				_bsdfStruct.opacity.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_maskParent->mask.opacity.value.vvalue.pointer));
			else if (_maskParent->mask.opacity.value.type == SPropertyElementData::INVALID)
				_bsdfStruct.opacity.texData = textureData(_maskParent->mask.opacity.texture);
		}
	};
	bsdf::SBSDFUnion retval;
	switch (_node->type)
	{
	case CElementBSDF::Type::DIFFUSE:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHDIFFUSE:
		if (_node->diffuse.reflectance.value.type==SPropertyElementData::SPECTRUM)
			retval.diffuse.reflectance.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->diffuse.reflectance.value.vvalue.pointer));
		else if (_node->diffuse.reflectance.value.type==SPropertyElementData::INVALID)
			retval.diffuse.reflectance.texData = textureData(_node->diffuse.reflectance.texture);

		if (_node->diffuse.alpha.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.diffuse.alpha.constant_f32) = _node->diffuse.alpha.value.fvalue;
		else if (_node->diffuse.alpha.value.type==SPropertyElementData::INVALID)
			retval.diffuse.alpha.texData = textureData(_node->diffuse.alpha.texture);

		inheritOpacity(retval.diffuse);
		break;
	case CElementBSDF::Type::DIELECTRIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::THINDIELECTRIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHDIELECTRIC:
		if (_node->dielectric.alpha.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.dielectric.alpha_u.constant_f32) = _node->dielectric.alpha.value.fvalue;
		else if (_node->dielectric.alpha.value.type==SPropertyElementData::INVALID)
			retval.dielectric.alpha_u.texData = textureData(_node->dielectric.alpha.texture);

		if (_node->dielectric.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
		{
			if (_node->dielectric.alphaV.value.type==SPropertyElementData::FLOAT)
				core::uintBitsToFloat(retval.dielectric.alpha_v.constant_f32) = _node->dielectric.alphaV.value.fvalue;
			else if (_node->dielectric.alphaV.value.type==SPropertyElementData::INVALID)
				retval.dielectric.alpha_v.texData = textureData(_node->dielectric.alphaV.texture);
		}
		
		retval.dielectric.eta = _node->dielectric.intIOR/_node->dielectric.extIOR;

		inheritOpacity(retval.dielectric);
		break;
	case CElementBSDF::Type::CONDUCTOR:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHCONDUCTOR:
		if (_node->conductor.alpha.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.conductor.alpha_u.constant_f32) = _node->conductor.alpha.value.fvalue;
		else if (_node->conductor.alpha.value.type == SPropertyElementData::INVALID)
			retval.conductor.alpha_u.texData = textureData(_node->conductor.alpha.texture);
		if (_node->conductor.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
		{
			if (_node->conductor.alphaV.value.type==SPropertyElementData::FLOAT)
				core::uintBitsToFloat(retval.conductor.alpha_v.constant_f32) = _node->conductor.alphaV.value.fvalue;
			else if (_node->conductor.alphaV.value.type==SPropertyElementData::INVALID)
				retval.conductor.alpha_v.texData = textureData(_node->conductor.alphaV.texture);
		}
		if (_node->conductor.eta.type==SPropertyElementData::SPECTRUM)
			retval.conductor.eta[0] = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>((_node->conductor.eta.vvalue/_node->conductor.extEta).pointer));
		if (_node->conductor.eta.type == SPropertyElementData::SPECTRUM)
			retval.conductor.eta[1] = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>((_node->conductor.k.vvalue/_node->conductor.extEta).pointer));

		inheritOpacity(retval.conductor);
		break;
	case CElementBSDF::Type::PLASTIC:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHPLASTIC:
		if (_node->plastic.alpha.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.plastic.alpha_u.constant_f32) = _node->plastic.alpha.value.fvalue;
		else if (_node->plastic.alpha.value.type==SPropertyElementData::INVALID)
			retval.plastic.alpha_u.texData = textureData(_node->plastic.alpha.texture);

		if (_node->plastic.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
		{
			if (_node->plastic.alphaV.value.type==SPropertyElementData::FLOAT)
				core::uintBitsToFloat(retval.plastic.alpha_v.constant_f32) = _node->plastic.alphaV.value.fvalue;
			else if (_node->plastic.alphaV.value.type==SPropertyElementData::INVALID)
				retval.plastic.alpha_v.texData = textureData(_node->plastic.alphaV.texture);
		}
		
		retval.plastic.eta = _node->plastic.intIOR/_node->plastic.extIOR;

		inheritOpacity(retval.plastic);
		break;
	case CElementBSDF::Type::COATING:
		_IRR_FALLTHROUGH;
	case CElementBSDF::Type::ROUGHCOATING:
		if (_node->coating.alpha.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.coating.alpha_u.constant_f32) = _node->coating.alpha.value.fvalue;
		else if (_node->coating.alpha.value.type==SPropertyElementData::INVALID)
			retval.coating.alpha_u.texData = textureData(_node->coating.alpha.texture);

		if (_node->coating.distribution==CElementBSDF::RoughSpecularBase::ASHIKHMIN_SHIRLEY)
		{
			if (_node->coating.alphaV.value.type==SPropertyElementData::FLOAT)
				core::uintBitsToFloat(retval.coating.alpha_v.constant_f32) = _node->coating.alphaV.value.fvalue;
			else if (_node->coating.alphaV.value.type==SPropertyElementData::INVALID)
				retval.coating.alpha_v.texData = textureData(_node->coating.alphaV.texture);
		}

		retval.coating.thickness_eta = core::Float16Compressor::compress(_node->coating.thickness);
		retval.coating.thickness_eta |= static_cast<uint32_t>(core::Float16Compressor::compress(_node->coating.intIOR/_node->coating.extIOR))<<16;

		if (_node->coating.sigmaA.value.type==SPropertyElementData::SPECTRUM)
			retval.coating.sigmaA.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->coating.sigmaA.value.vvalue.pointer));
		else if (_node->coating.sigmaA.value.type==SPropertyElementData::INVALID)
			retval.coating.sigmaA.texData = textureData(_node->coating.sigmaA.texture);

		inheritOpacity(retval.coating);
		break;
	case CElementBSDF::Type::BUMPMAP:
		break;
	case CElementBSDF::Type::PHONG:
		_IRR_DEBUG_BREAK_IF(1);//we dont care about PHONG
		break;
	case CElementBSDF::Type::WARD:
		if (_node->ward.alphaU.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.ward.alpha_u.constant_f32) = _node->ward.alphaU.value.fvalue;
		else if (_node->ward.alphaU.value.type==SPropertyElementData::INVALID)
			retval.ward.alpha_u.texData = textureData(_node->ward.alphaU.texture);

		if (_node->ward.alphaV.value.type==SPropertyElementData::FLOAT)
			core::uintBitsToFloat(retval.ward.alpha_v.constant_f32) = _node->ward.alphaV.value.fvalue;
		else if (_node->ward.alphaV.value.type==SPropertyElementData::INVALID)
			retval.ward.alpha_u.texData = textureData(_node->ward.alphaV.texture);

		inheritOpacity(retval.ward);
		break;
	case CElementBSDF::Type::MIXTURE_BSDF:
	{
		constexpr float vec3_one[3] {1.f,1.f,1.f};
		const core::vectorSIMDf w(_mix2blend_weight);
		retval.blend.weightL.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(vec3_one));
		retval.blend.weightR.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(w.pointer));
	}
		break;
	case CElementBSDF::Type::BLEND_BSDF:
		if (_node->blendbsdf.weight.value.type==SPropertyElementData::FLOAT)
		{
			const core::vectorSIMDf weight_r = core::vectorSIMDf(_node->blendbsdf.weight.value.fvalue);
			const core::vectorSIMDf weight_l = core::vectorSIMDf(1.f)-weight_r;
			retval.blend.weightL.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(weight_l.pointer));
			retval.blend.weightR.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(weight_r.pointer));
		}
		else if (_node->blendbsdf.weight.value.type==SPropertyElementData::INVALID)
		{
			retval.blend.weightL.texData = textureData(_node->blendbsdf.weight.texture);
		}
		break;
	case CElementBSDF::Type::MASK: _IRR_FALLTHROUGH;
	case CElementBSDF::Type::TWO_SIDED:
		assert(0);//TWO_SIDED and MASK shouldnt get to this function (they are translated into bitfields in instruction)
		break;
	case CElementBSDF::Type::DIFFUSE_TRANSMITTER:
		if (_node->difftrans.transmittance.value.type==SPropertyElementData::SPECTRUM)
			retval.diffuseTransmitter.transmittance.constant_rgb19e7 = rgb32f_to_rgb19e7(reinterpret_cast<const uint32_t*>(_node->difftrans.transmittance.value.vvalue.pointer));
		else if (_node->difftrans.transmittance.value.type==SPropertyElementData::INVALID)
			retval.diffuseTransmitter.transmittance.texData = textureData(_node->difftrans.transmittance.texture);

		inheritOpacity(retval.diffuseTransmitter);
		break;
	}

	return retval;
}

_IRR_STATIC_INLINE_CONSTEXPR const char* FRAGMENT_SHADER =
R"(#version 430 core

layout (location = 0) in vec3 LocalPos;
layout (location = 1) in vec3 ViewPos;
layout (location = 2) in vec3 Normal;
layout (location = 3) in vec2 UV;
layout (location = 4) in vec3 Color;

layout (location = 0) out vec4 OutColor;

#define instr_t uvec2
//maybe will change to vec3
#define reg_t vec4
struct bsdf_data_t
{
	uvec4 data[3];
};

struct texture_data_t
{
	//2x unorm16
	uint pgTabUV;
	//2x unorm16 originalTexSz/maxAllocatableTexSz ratio
	uint size;
};
vec2 decodeRG16(in uint unorm16)
{
	vec2 uv = vec2( float(unorm16 & 0xffffu), float((unorm16>>16) & 0xffffu) );
	return uv * 1.52290219e-5;
}
vec2 unpackVirtualUV(in texture_data_t texData)
{
	return decodeRG16(texData.pgTabUV);
}
vec2 unpackSize(in texture_data_t texData)
{
	return decodeRG16(texData.size);
}

layout (set = 0, binding = 0) usampler2D pgTabTex;
layout (set = 0, binding = 1) sampler2DArray physPgTex;
layout (set = 0, binding = 2, std430) buffer INSTR_BUF //TODO maybe UBO instead of SSBO for instr buf? (only sequential access in this case, not so random)
{
	instr_t data[];
} instr_buf;
layout (set = 0, binding = 3, std430) buffer BSDF_BUF
{
	bsdf_data_t data[];
} bsdf_buf;

//in case of OBJ mesh albedo might be textured OR constant (in this case put albedo into push constants)
// (TODO) so OBJ meshes will need slightly different shader (doable with #ifdefs)
layout (set = 3, binding = 1) uniform sampler2D albedoMap;

layout (push_constant) uniform Block
{
	vec3 albedo;//for OBJ meshes
	uint instrOffset;
	uint instrCount;
} PC;

#define REG_COUNT 16
#define NORMAL_REG_COUNT 4

//i think ill have to create some c++ macro or something to create string with those
//becasue it's too fucked up to remember about every change in c++ and have to update everything here
#define INSTR_OPCODE_MASK			0x0fu
#define INSTR_REG_MASK				0xffu
#define INSTR_BSDF_BUF_OFFSET_SHIFT 13
#define INSTR_BSDF_BUF_OFFSET_MASK	0x7ffffu
#define INSTR_NDF_SHIFT 4
#define INSTR_NDF_MASK 0x3u
#define INSTR_NORMAL_REG_SHIFT		11
#define INSTR_NORMAL_REG_MASK		0x03u

//remember to keep it compliant with c++ enum!!
#define OP_DIFFUSE			0u
#define OP_ROUGHDIFFUSE		1u
#define OP_DIFFTRANS		2u
#define OP_DIELECTRIC		3u
#define OP_ROUGHDIELECTRIC	4u
#define OP_CONDUCTOR		5u
#define OP_ROUGHCONDUCTOR	6u
#define OP_PLASTIC			7u
#define OP_ROUGHPLASTIC		8u
#define OP_WARD				9u
#define OP_INVALID			10u
#define OP_COATING			11u
#define OP_ROUGHCOATING		12u
#define OP_BUMPMAP			13u
#define OP_BLEND			14u

#define NDF_BECKMANN	0u
#define NDF_GGX			1u
#define NDF_PHONG		2u
#define NDF_AS			3u

#include <irr/builtin/glsl/bsdf/common.glsl>
#include <irr/builtin/glsl/bsdf/brdf/specular/fresnel/fresnel.glsl>
#include <irr/builtin/glsl/bsdf/brdf/diffuse/fresnel_correction.glsl>
#include <irr/builtin/glsl/bsdf/brdf/diffuse/lambert.glsl>
#include <irr/builtin/glsl/bsdf/brdf/diffuse/oren_nayar.glsl>
#include <irr/builtin/glsl/bsdf/brdf/specular/ndf/ggx.glsl>
#include <irr/builtin/glsl/bsdf/brdf/specular/ashikhmin_shirley.glsl>
#include <irr/builtin/glsl/bsdf/brdf/specular/beckmann_smith.glsl>
#include <irr/builtin/glsl/bsdf/brdf/specular/ggx.glsl>
#include <irr/builtin/glsl/bump_mapping/height_mapping.glsl>

//first NORMAL_REG_COUNT registers are for normals
//register[0] always stores geometry normal (instructions not influenced by bumpmap in bsdf-tree specify normal register number as 0)
//all the other normal registers are used by OP_BUMPMAP instrctions to store perturbed geometry normal
//all register numbered >=NORMAL_REG_COUNT are general purpose and used by other instructions to store result
reg_t registers[REG_COUNT+NORMAL_REG_COUNT];

#define ADDR_LAYER_SHIFT 12
#define ADDR_Y_SHIFT 6
#define ADDR_X_MASK 0x3fu

#define TEXTURE_TILE_PER_DIMENSION 64
#define PAGE_SZ 128
#define TILE_PADDING 9

vec3 unpackPageID(in uint pageID)
{
	vec2 uv = vec2(float(pageID & ADDR_X_MASK), float((pageID>>ADDR_Y_SHIFT) & ADDR_X_MASK))*(PAGE_SZ+TILE_PADDING) + TILE_PADDING;
	uv /= vec2(textureSize(physPgTex.xy,0));
	return vec3(uv, float(pageID >> ADDR_LAYER_SHIFT));
}

#define PGTAB_SZ 128
#define MAX_LOD 7
vec4 vTextureGrad_helper(in vec2 virtualUV, int LoD, in mat2 gradients)
{
	int pgtabLoD = min(LoD,MAX_LOD); // assert(MAX_LOD<log2(PGTAB_SZ))
	int tilesInLodLevel = PGTAB_SZ>>LoD;
	ivec2 tileCoord = ivec2(virtualUV.xy*vec2(tilesInLodLevel));
	uvec2 pageID = texelFetch(pgTabTex,tileCoord,pgtabLoD).rg;
	vec3 physicalUV = unpackPageID(pgtabLoD<LoD ? pageID.y : pageID.x); // unpack to normalized coord offset + Layer in physical texture (just bit operations) and multiples
	//TODO if (pgtabLoD<LoD) then we're going into mip-tail page! (extra offsets needed)
	//TODO not sure about correctness of those 3 lines below, seems weird and i dont really understand
	vec2 tileFractionalCoordinate = virtualUV.xy-vec2(tileCoord)/tilesInLodLevel;
	tileFractionalCoordinate *= (PAGE_SZ/(PAGE_SZ+TILE_PADDING)); // take border into account
	physicalUV.xy += tileFractionalCoordinate;
	return textureGrad(physicalTileTexture,physicalUV,gradients);
}


//returns: x=dst, y=src1, z=src2
uvec3 instr_decodeRegisters(in instr_t instr)
{
	uvec3 regs(instr.y, (instr.y>>8), (instr.y>>16));
	return regs & uvec3(INSTR_REG_MASK); //this will do element-wise bit-wise AND, right?
}
#define REG_DST(r)	r.x
#define REG_SRC1(r)	r.y
#define REG_SRC2(r)	r.z

void instr_execute_ROUGHDIELECTRIC(in instr_t instr, in uvec3 regs, in irr_glsl_BSDFAnisotropicParams params)
{
	uint bsdf_i = (instr.x>>INSTR_BSDF_BUF_OFFSET_SHIFT) & INSTR_BSDF_BUF_OFFSET_MASK;
	uint ndf_i = (instr.x>>INSTR_NDF_SHIFT) & INSTR_NDF_MASK;
	bsdf_data_t bsdf_data = bsdf_buf.data[bsdf_i];

	float eta = uintBitsToFloat(bsdf_data.data[0].x);
	mat2x3 eta2 = mat2x3(vec3(eta*eta),vec3(0.0));
	vec2 a;
	a.x = uintBitsToFloat(bsdf_data.data[0].y);

	//TODO albedo (texture [this can go without VT] or vtx color attrib)
	//TODO figure out what to put as alpha into oren_nayar in case of anisotropic surface, a.x is wrong for sure (oren-nayar handles anisotropic surfaces for sure, i just need to define alpha)
	vec3 diff = irr_glsl_oren_nayar_cos_eval(params.isotropic, a.x) * (1.0-irr_glsl_fresnel_dielectric(eta,params.isotropic.NdotL)) * (1.0-irr_glsl_fresnel_dielectric(eta,params.isotropic.NdotV));
	diff *= irr_glsl_diffuseFresnelCorrectionFactor(vec3(eta), eta2[0]);
	vec3 spec;
	switch (ndf_i)
	{
	case NDF_BECKMANN:
		spec = irr_glsl_beckmann_smith_height_correlated_cos_eval(params.isotropic, eta2, sqrt(a.x), a.x);
		break;
	case NDF_GGX:
		spec = irr_glsl_ggx_height_correlated_cos_eval(params.isotropic, eta2, a.x);
		break;
	case NDF_PHONG:
		spec = vec3(0.0);
		break;
	case NDF_AS:
		a.y = uintBitsToFloat(bsdf_data.data[0].w);
		//TODO: sin_cos_phi
		//spec = irr_glsl_ashikhmin_shirley_cos_eval(params, vec2(1.0)/a, sin_cos_phi, a, eta2);
		break;
	}

	registers[NORMAL_REG_COUNT+REG_DST(regs)].xyz = diff+spec;
}
void instr_execute_BUMPMAP(in instr_t instr, in uvec3 regs, in irr_glsl_BSDFAnisotropicParams params)
{
	//TODO dHdScreen, need for sample from bumpmap and dFdx,dFdy but cannot do it here, need some trick for it (like keep VT tex data of bumpmaps in push consts)
	uint normal_reg = (instr.x >> INSTR_NORMAL_REG_SHIFT) & INSTR_NORMAL_REG_MASK;
	registers[normal_reg].xyz = irr_glsl_perturbNormal_heightMap(params.isotropic.interaction.N, params.isotropic.interaction.V.dPosdScreen, dHdScreen);
}
void instr_execute(in instr_t instr, in uvec3 regs, in irr_glsl_BSDFAnisotropicParams params, in mat2 dUV)
{
	switch (instr.x & INSTR_REG_MASK)
	{
	
	case OP_ROUGHDIELECTRIC: 
		instr_execute_ROUGHDIELECTRIC(instr, regs, params);
		break;
	//run func depending on opcode
	//the func will decide whether to (and which ones) fetch registers from `regs` array
	//and whether to fetch bsdf data from bsdf_buf (not all opcodes need it)
	//also stores the result into dst reg
	//....
	}
}

void main()
{
	registers[0].xyz = normalize(Normal);
	mat2 dUV = mat2(dFdx(UV),dFdy(UV));

	//all lighting computations are done in view space

	//TODO 
	//ignoring bump maps as for now, however i think we could keep array of irr_glsl_BSDFAnisotropicParams and call 
	//irr_glsl_calcBSDFIsotropicParams() just once per bumpmap (everytime an OP_BUMPMAP instr is executed)
	//although.. such array would statically consume A LOT of registers and we're already using quite much
	irr_glsl_ViewSurfaceInteraction interaction = irr_glsl_calcFragmentShaderSurfaceInteraction(vec3(0.0), ViewPos, registers[0].xyz);
	irr_glsl_BSDFIsotropicParams isoparams = irr_glsl_calcBSDFIsotropicParams(interaction, -ViewPos);
	//TODO: T,B tangents
	irr_glsl_BSDFAnisotropicParams params = irr_glsl_calcBSDFAnisotropicParams(isoparams, T, B);

	//TODO will conform to irrbaw shader standards later (irr_bsdf_cos_eval, irr_computeLighting...)
	for (uint i = 0u; i < PC.instrCount; ++i)
	{
		instr_t instr = instr_buf.data[PC.instrOffset+i];
		uvec3 regs = instr_decodeRegisters(instr);

		instr_execute(instr, regs, params);
	}

	OutColor = vec4(registers[NORMAL_REG_COUNT].xyz,1.0);//result is always in first general purpose reg
}
)";


_IRR_STATIC_INLINE_CONSTEXPR const char* PIPELINE_LAYOUT_CACHE_KEY = "irr/builtin/pipeline_layout/loaders/mitsuba_xml/default";
_IRR_STATIC_INLINE_CONSTEXPR const char* PIPELINE_CACHE_KEY = "irr/builtin/graphics_pipeline/loaders/mitsuba_xml/default";

_IRR_STATIC_INLINE_CONSTEXPR uint32_t PAGE_TAB_TEX_BINDING = 0u;
_IRR_STATIC_INLINE_CONSTEXPR uint32_t PHYS_PAGE_TEX_BINDING = 1u;
_IRR_STATIC_INLINE_CONSTEXPR uint32_t INSTR_BUF_BINDING = 2u;
_IRR_STATIC_INLINE_CONSTEXPR uint32_t BSDF_BUF_BINDING = 3u;

template <typename AssetT>
static void insertAssetIntoCache(core::smart_refctd_ptr<AssetT>& asset, const char* path, IAssetManager* _assetMgr)
{
	asset::SAssetBundle bundle({ asset });
	_assetMgr->changeAssetKey(bundle, path);
	_assetMgr->insertAssetIntoCache(bundle);
}
template<typename AssetType, IAsset::E_TYPE assetType>
static core::smart_refctd_ptr<AssetType> getBuiltinAsset(const char* _key, IAssetManager* _assetMgr)
{
	size_t storageSz = 1ull;
	asset::SAssetBundle bundle;
	const IAsset::E_TYPE types[]{ assetType, static_cast<IAsset::E_TYPE>(0u) };

	_assetMgr->findAssets(storageSz, &bundle, _key, types);
	if (bundle.isEmpty())
		return nullptr;
	auto assets = bundle.getContents();
	//assert(assets.first != assets.second);

	return core::smart_refctd_ptr_static_cast<AssetType>(assets.first[0]);
}

CMitsubaLoader::CMitsubaLoader(asset::IAssetManager* _manager) : asset::IAssetLoader(), m_manager(_manager)
{
#ifdef _IRR_DEBUG
	setDebugName("CMitsubaLoader");
#endif

	core::smart_refctd_ptr<ICPUPipelineLayout> layout;
	{
		SPushConstantRange pcrng;
		pcrng.offset = 0u;
		pcrng.size = 2u*sizeof(uint32_t);//instr offset and count
		pcrng.stageFlags = ISpecializedShader::ESS_FRAGMENT;

		//those descriptors can go as ds0 since they're always the same for all geometry within scene loaded from mitsuba xml
		//(so ds itself will need to be returned through pipeline metadata same way as in MTL loader)
		//@devsh tell me if it's ok
		core::smart_refctd_ptr<ICPUDescriptorSetLayout> ds0layout;
		{
			std::array<ICPUDescriptorSetLayout::SBinding,4> bnds;
			//page table texture
			bnds[0].count = 1u;
			bnds[0].stageFlags = ICPUSpecializedShader::ESS_FRAGMENT;
			bnds[0].type = EDT_COMBINED_IMAGE_SAMPLER;
			bnds[0].binding = 0u;
			//physical pages texture
			bnds[1].count = 1u;
			bnds[1].stageFlags = ICPUSpecializedShader::ESS_FRAGMENT;
			bnds[1].type = EDT_COMBINED_IMAGE_SAMPLER;
			bnds[1].binding = 1u;
			//instruction buffer
			bnds[2].count = 1u;
			bnds[2].stageFlags = ICPUSpecializedShader::ESS_FRAGMENT;
			bnds[2].type = EDT_STORAGE_BUFFER;
			bnds[2].binding = 2u;
			//bsdf buffer
			bnds[3].count = 1u;
			bnds[3].stageFlags = ICPUSpecializedShader::ESS_FRAGMENT;
			bnds[3].type = EDT_STORAGE_BUFFER;
			bnds[3].binding = 3u;

			ds0layout = core::make_smart_refctd_ptr<ICPUDescriptorSetLayout>(bnds.data(),bnds.data()+bnds.size());
		}
		auto ds1layout = getBuiltinAsset<ICPUDescriptorSetLayout, IAsset::ET_DESCRIPTOR_SET_LAYOUT>("irr/builtin/descriptor_set_layout/basic_view_parameters", m_manager);

		layout = core::make_smart_refctd_ptr<ICPUPipelineLayout>(
			&pcrng, &pcrng+1,
			std::move(ds0layout), std::move(ds1layout), nullptr, nullptr
		);

		insertAssetIntoCache(layout, PIPELINE_LAYOUT_CACHE_KEY, m_manager);
	}
	{
		SRasterizationParams rasterParams;
		rasterParams.faceCullingMode = asset::EFCM_NONE;
		auto pipeline = core::make_smart_refctd_ptr<ICPURenderpassIndependentPipeline>(
			std::move(layout),
			nullptr, nullptr, //TODO shaders (will be same for all)
			//all the params will be overriden with those loaded with meshes
			SVertexInputParams(),
			SBlendParams(),
			SPrimitiveAssemblyParams(),
			rasterParams
		);

		insertAssetIntoCache(pipeline, PIPELINE_CACHE_KEY, m_manager);
	}
}

static core::smart_refctd_ptr<ICPUDescriptorSet> makeDSForMeshbuffer()
{
	auto pplnlayout = getBuiltinAsset<ICPUPipelineLayout, IAsset::ET_PIPELINE_LAYOUT>(PIPELINE_LAYOUT_CACHE_KEY, nullptr/*TODO*/);
	auto ds3layout = pplnlayout->getDescriptorSetLayout(3u);

	auto ds3 = core::make_smart_refctd_ptr<ICPUDescriptorSet>(std::move(ds3layout));
	auto d = ds3->getDescriptors(0u).begin();
	d->desc = core::make_smart_refctd_ptr<ICPUBuffer>(sizeof(SBasicViewParameters));
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
	ParserManager parserManager(m_manager->getFileSystem(),_override);
	if (!parserManager.parse(_file))
		return {};

	//
	auto currentDir = io::IFileSystem::getFileDir(_file->getFileName()) + "/";
	SContext ctx = {
		m_manager->getGeometryCreator(),
		m_manager->getMeshManipulator(),
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
			m_manager->setAssetMetadata(mesh.get(), std::move(metadata));
			meshes.insert(std::move(mesh));
		}
		else
		{
			assert(mesh->getMetadata() && strcmpi(mesh->getMetadata()->getLoaderName(),IMeshMetadata::LoaderName)==0);
			metadataptr = static_cast<IMeshMetadata*>(mesh->getMetadata());
		}

		metadataptr->instances.push_back({shapedef->getAbsoluteTransform(),shapedef->obtainEmitter()});
	}

	auto metadata = createPipelineMetadata(createDS0(ctx), getBuiltinAsset<ICPUPipelineLayout, IAsset::ET_PIPELINE_LAYOUT>(PIPELINE_LAYOUT_CACHE_KEY, m_manager).get());
	for (auto& mesh : meshes)
	{
		for (uint32_t i = 0u; i < mesh->getMeshBufferCount(); ++i)
		{
			asset::ICPUMeshBuffer* mb = mesh->getMeshBuffer(i);
			asset::ICPURenderpassIndependentPipeline* pipeline = mb->getPipeline();
			if (!pipeline->getMetadata())
				m_manager->setAssetMetadata(pipeline, core::smart_refctd_ptr(metadata));
		}
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

//TODO : vtx input and assembly params are now ignored (mb is created without pipeline), later they need to be somehow forwarded and set on already created pipeline
static core::smart_refctd_ptr<ICPUMesh> createMeshFromGeomCreatorReturnType(IGeometryCreator::return_type&& _data, asset::IAssetManager* _manager)
{
	auto mb = core::make_smart_refctd_ptr<ICPUMeshBuffer>(
		nullptr, nullptr,
		_data.bindings, std::move(_data.indexBuffer)
	);
	mb->setIndexCount(_data.indexCount);
	mb->setIndexType(_data.indexType);
	mb->setBoundingBox(_data.bbox);

	auto mesh = core::make_smart_refctd_ptr<CCPUMesh>();
	mesh->addMeshBuffer(std::move(mb));

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
		auto retval = interm_getAssetInHierarchy(m_manager, filename.svalue, ctx.params, hierarchyLevel/*+ICPUSCene::MESH_HIERARCHY_LEVELS_BELOW*/, ctx.override);
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
				m_manager->setAssetMetadata(copy.get(),core::smart_refctd_ptr<asset::IAssetMetadata>(mesh->getMetadata()));
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

			mesh = createMeshFromGeomCreatorReturnType(ctx.creator->createCubeMesh(core::vector3df(2.f)), m_manager);
			flipNormals = flipNormals!=shape->cube.flipNormals;
		}
			break;
		case CElementShape::Type::SPHERE:
			mesh = createMeshFromGeomCreatorReturnType(ctx.creator->createSphereMesh(1.f,64u,64u), m_manager);
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
				mesh = createMeshFromGeomCreatorReturnType(ctx.creator->createCylinderMesh(1.f, 1.f, 64), m_manager);
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
			mesh = createMeshFromGeomCreatorReturnType(ctx.creator->createRectangleMesh(core::vector2df_SIMD(1.f,1.f)), m_manager);
			flipNormals = flipNormals!=shape->rectangle.flipNormals;
			break;
		case CElementShape::Type::DISK:
			mesh = createMeshFromGeomCreatorReturnType(ctx.creator->createDiskMesh(1.f,64u), m_manager);
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
					for (uint32_t i=0u; meshbuffer->getAttribute(uv, 2u, i); i++)
					{
						uv.y = -uv.y;
						meshbuffer->setAttribute(uv, 2u, i);
					}
				}
			}
			// collapse parameter gets ignored
			break;
		case CElementShape::Type::PLY:
			_IRR_DEBUG_BREAK_IF(true); // this code has never been tested
			mesh = loadModel(shape->ply.filename);
			mesh = mesh->clone(~0u);//clone everything
			flipNormals = flipNormals!=shape->ply.flipNormals;
			faceNormals = shape->ply.faceNormals;
			maxSmoothAngle = shape->ply.maxSmoothAngle;
			if (mesh && shape->ply.srgb)//TODO this probably shouldnt modify original mesh (the one cached in asset cache)
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
						for (uint32_t i=0u; meshbuffer->getAttribute(rgb, 1u, i); i++,it++)
						{
							for (auto i=0; i<3u; i++)
								rgb[i] = video::impl::srgb2lin(rgb[i]);
							meshbuffer->setAttribute(rgb,it,asset::EF_A2B10G10R10_UNORM_PACK32);
						}
						constexpr uint32_t COLOR_BUF_BINDING = 15u;
						auto& vtxParams = meshbuffer->getPipeline()->getVertexInputParams();
						vtxParams.attributes[1].format = EF_A2B10G10R10_UNORM_PACK32;
						vtxParams.attributes[1].relativeOffset = 0u;
						vtxParams.attributes[1].binding = COLOR_BUF_BINDING;
						vtxParams.bindings[COLOR_BUF_BINDING].inputRate = EVIR_PER_VERTEX;
						vtxParams.bindings[COLOR_BUF_BINDING].stride = hidefRGBSize;
						vtxParams.enabledBindingFlags |= (1u<<COLOR_BUF_BINDING);
						meshbuffer->setVertexBufferBinding({0ull,core::smart_refctd_ptr(newRGB)}, COLOR_BUF_BINDING);
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
			ctx.manipulator->calculateSmoothNormals(newMeshBuffer.get(), false, 0.f, newMeshBuffer->getNormalAttributeIx(),
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
		m_manager->setAssetMetadata(newMesh.get(), core::smart_refctd_ptr<asset::IAssetMetadata>(mesh->getMetadata()));
		mesh = std::move(newMesh);
	}
#endif

	std::pair<uint32_t, uint32_t> instrBufOffsetAndCount;
	{
		auto it = ctx.instrStreamCache.find(shape->bsdf);
		if (it != ctx.instrStreamCache.end())
			instrBufOffsetAndCount = it->second;
		else {
			instrBufOffsetAndCount = genBSDFtreeTraversal(ctx, shape->bsdf);
			ctx.instrStreamCache.insert({shape->bsdf,instrBufOffsetAndCount});
		}
	}

	//meshbuffer processing
	auto builtinPipeline = getBuiltinAsset<ICPURenderpassIndependentPipeline, IAsset::ET_RENDERPASS_INDEPENDENT_PIPELINE>(PIPELINE_CACHE_KEY, m_manager);
	for (auto i = 0u; i < mesh->getMeshBufferCount(); i++)
	{
		auto* meshbuffer = mesh->getMeshBuffer(i);
		//write starting instr buf offset (instr-wise) and instruction count
		uint32_t* pcData = reinterpret_cast<uint32_t*>(meshbuffer->getPushConstantsDataPtr());
		pcData[0] = instrBufOffsetAndCount.first;
		pcData[1] = instrBufOffsetAndCount.second;
		// add some metadata
		///auto meshbuffermeta = core::make_smart_refctd_ptr<IMeshBufferMetadata>(shapedef->type,shapedef->emitter ? shapedef->emitter.area:CElementEmitter::Area());
		///manager->setAssetMetadata(meshbuffer,std::move(meshbuffermeta));
		// TODO: change this with shader pipeline
		//meshbuffer->getMaterial() = getBSDF(ctx, hierarchyLevel + asset::ICPUMesh::MESHBUFFER_HIERARCHYLEVELS_BELOW, shape->bsdf);
		auto* prevPipeline = meshbuffer->getPipeline();
		//TODO do something to not always create new pipeline
		auto pipeline = core::smart_refctd_ptr_static_cast<ICPURenderpassIndependentPipeline>(//shallow copy because we're going to override parameter structs
			builtinPipeline->clone(0u)
		);
		pipeline->getVertexInputParams() = prevPipeline->getVertexInputParams();
		pipeline->getPrimitiveAssemblyParams() = prevPipeline->getPrimitiveAssemblyParams();
		//TODO blend and raster probably shouldnt be copied
		//pipeline->getBlendParams() = prevPipeline->getBlendParams();
		//pipeline->getRasterizationParams() = prevPipeline->getRasterizationParams();

		meshbuffer->setPipeline(std::move(pipeline));
	}

	// cache and return
	ctx.shapeCache.insert({ shape,mesh });
	return mesh;
}

//! TODO: change to CPU graphics pipeline
//TODO this function will most likely be deleted, basically only instr buf offset/count pair is needed, pipelines wont change that much
CMitsubaLoader::SContext::bsdf_ass_type CMitsubaLoader::getBSDF(SContext& ctx, uint32_t hierarchyLevel, const CElementBSDF* bsdf)
{
	if (!bsdf)
		return nullptr; 

	auto found = ctx.pipelineCache.find(bsdf);
	if (found != ctx.pipelineCache.end())
		return found->second;

	const std::pair<uint32_t,uint32_t> instrBufOffsetAndCount = genBSDFtreeTraversal(ctx, bsdf);
	auto pipeline = core::make_smart_refctd_ptr<ICPURenderpassIndependentPipeline>(
		getBuiltinAsset<ICPUPipelineLayout,IAsset::ET_PIPELINE_LAYOUT>(PIPELINE_LAYOUT_CACHE_KEY, m_manager),//layout
		nullptr, nullptr,//TODO shaders
		SVertexInputParams(),
		SBlendParams(),
		SPrimitiveAssemblyParams(),
		SRasterizationParams()
	);

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

	ICPUImageView::SCreationParams viewParams;
	viewParams.flags = static_cast<ICPUImageView::E_CREATE_FLAGS>(0);
	viewParams.subresourceRange.aspectMask = static_cast<IImage::E_ASPECT_FLAGS>(0);
	viewParams.subresourceRange.baseArrayLayer = 0u;
	viewParams.subresourceRange.layerCount = 1u;
	viewParams.subresourceRange.baseMipLevel = 0u;
	viewParams.viewType = IImageView<ICPUImage>::ET_2D;
	ICPUSampler::SParams samplerParams;
	samplerParams.AnisotropicFilter = core::max(core::findMSB(uint32_t(tex->bitmap.maxAnisotropy)),1);
	samplerParams.LodBias = 0.f;
	samplerParams.TextureWrapW = ISampler::ETC_REPEAT;
	samplerParams.BorderColor = ISampler::ETBC_FLOAT_OPAQUE_BLACK;
	samplerParams.CompareEnable = false;
	samplerParams.CompareFunc = ISampler::ECO_NEVER;
	samplerParams.MaxLod = 10000.f;
	samplerParams.MinLod = 0.f;
	switch (tex->type)
	{
		case CElementTexture::Type::BITMAP:
			{
				auto retval = interm_getAssetInHierarchy(m_manager,tex->bitmap.filename.svalue,ctx.params,hierarchyLevel,ctx.override);
				auto contentRange = retval.getContents();
				if (contentRange.first < contentRange.second)
				{
					auto asset = contentRange.first[0];
					if (asset && asset->getAssetType() == asset::IAsset::ET_IMAGE)
					{
						auto texture = core::smart_refctd_ptr_static_cast<asset::ICPUImage>(asset);

						switch (tex->bitmap.channel)
						{
							// no GL_R8_SRGB support yet
							case CElementTexture::Bitmap::CHANNEL::R:
								{
								constexpr auto RED = ICPUImageView::SComponentMapping::ES_R;
								viewParams.components = {RED,RED,RED,RED};
								}
								break;
							case CElementTexture::Bitmap::CHANNEL::G:
								{
								constexpr auto GREEN = ICPUImageView::SComponentMapping::ES_G;
								viewParams.components = {GREEN,GREEN,GREEN,GREEN};
								}
								break;
							case CElementTexture::Bitmap::CHANNEL::B:
								{
								constexpr auto BLUE = ICPUImageView::SComponentMapping::ES_B;
								viewParams.components = {BLUE,BLUE,BLUE,BLUE};
								}
								break;
							case CElementTexture::Bitmap::CHANNEL::A:
								{
								constexpr auto ALPHA = ICPUImageView::SComponentMapping::ES_A;
								viewParams.components = {ALPHA,ALPHA,ALPHA,ALPHA};
								}
								break;/* special conversions needed to CIE space
							case CElementTexture::Bitmap::CHANNEL::X:
							case CElementTexture::Bitmap::CHANNEL::Y:
							case CElementTexture::Bitmap::CHANNEL::Z:*/
							default:
								break;
						}
						viewParams.subresourceRange.levelCount = texture->getCreationParameters().mipLevels;
						viewParams.format = texture->getCreationParameters().format;
						viewParams.image = std::move(texture);
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
						samplerParams.MinFilter = ISampler::ETF_LINEAR;
						samplerParams.MaxFilter = ISampler::ETF_LINEAR;
						samplerParams.MipmapMode = ISampler::ESMM_LINEAR;
						break;
					default:
						samplerParams.MinFilter = ISampler::ETF_NEAREST;
						samplerParams.MaxFilter = ISampler::ETF_NEAREST;
						samplerParams.MipmapMode = ISampler::ESMM_NEAREST;
						break;
				}
				auto getWrapMode = [](CElementTexture::Bitmap::WRAP_MODE mode)// -> video::E_TEXTURE_CLAMP
				{
					switch (mode)
					{
						case CElementTexture::Bitmap::WRAP_MODE::CLAMP:
							return ISampler::ETC_CLAMP_TO_EDGE;
							break;
						case CElementTexture::Bitmap::WRAP_MODE::MIRROR:
							return ISampler::ETC_MIRROR;
							break;
						case CElementTexture::Bitmap::WRAP_MODE::ONE:
							_IRR_DEBUG_BREAK_IF(true); // TODO : replace whole texture?
							break;
						case CElementTexture::Bitmap::WRAP_MODE::ZERO:
							_IRR_DEBUG_BREAK_IF(true); // TODO : replace whole texture?
							break;
						default:
							return ISampler::ETC_REPEAT;
							break;
					}
				};
				samplerParams.TextureWrapU = getWrapMode(tex->bitmap.wrapModeU);
				samplerParams.TextureWrapV = getWrapMode(tex->bitmap.wrapModeV);
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
	auto view = core::make_smart_refctd_ptr<ICPUImageView>(std::move(viewParams));
	auto sampler = core::make_smart_refctd_ptr<ICPUSampler>(samplerParams);
	SContext::tex_ass_type tex_ass{std::move(view),std::move(sampler)};
	ctx.textureCache.insert({tex,tex_ass});

	return tex_ass;
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
	case CElementBSDF::Type::DIELECTRIC: _IRR_FALLTHROUGH;
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
	case CElementBSDF::Type::BLEND_BSDF: _IRR_FALLTHROUGH;
	case CElementBSDF::Type::MIXTURE_BSDF:
		return bsdf::OP_BLEND;
	case CElementBSDF::Type::MASK:
		return BSDFtype2opcode(bsdf->mask.bsdf[0]);
	case CElementBSDF::Type::TWO_SIDED:
		return BSDFtype2opcode(bsdf->twosided.bsdf[0]);
	default:
		return bsdf::OP_INVALID;
	}
}

std::pair<uint32_t, uint32_t> CMitsubaLoader::genBSDFtreeTraversal(SContext& ctx, const CElementBSDF* bsdf)
{
	struct stack_el {
		const CElementBSDF* bsdf;
		uint32_t instr_1st_dword;
		bool visited;
		uint32_t weight_ix;
		const CElementBSDF* maskParent;
	};
	core::stack<stack_el> stack;
	uint32_t firstFreeNormalReg = 1u;//normal reg val 0 means geom normal without any perturbations
	auto push = [&](const CElementBSDF* _bsdf, uint32_t _parent1stDword, const CElementBSDF* _maskParent) {
		auto writeInheritableFlags = [](uint32_t& dst, uint32_t parent) {
			dst |= (parent & (bsdf::BITFIELDS_MASK_TWOSIDED << bsdf::BITFIELDS_SHIFT_TWOSIDED));
			dst |= (parent & (bsdf::BITFIELDS_MASK_MASKFLAG << bsdf::BITFIELDS_SHIFT_MASKFLAG));
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
			stack.push({_bsdf,_1stdword,false,0,_maskParent});
			break;
		case CElementBSDF::Type::COATING:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::ROUGHCOATING:
			_IRR_FALLTHROUGH;
		case CElementBSDF::Type::BUMPMAP:
			_1stdword &= (~(bsdf::INSTR_NORMAL_REG_MASK<<bsdf::INSTR_NORMAL_REG_SHIFT));//zero-out normal reg bitfield
			_IRR_DEBUG_BREAK_IF(firstFreeNormalReg>bsdf::INSTR_NORMAL_REG_MASK);
			_1stdword |= (firstFreeNormalReg & bsdf::INSTR_NORMAL_REG_MASK) << bsdf::INSTR_NORMAL_REG_SHIFT;//write new val
			++firstFreeNormalReg;
			stack.push({_bsdf,_1stdword,false,0,_maskParent});

			_1stdword = BSDFtype2opcode(_bsdf->meta_common.bsdf[0]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({_bsdf->meta_common.bsdf[0],_1stdword,false,0,_maskParent});
			break;
		case CElementBSDF::Type::BLEND_BSDF:
			stack.push({ _bsdf,_1stdword,false,0,_maskParent });
			_1stdword = BSDFtype2opcode(_bsdf->blendbsdf.bsdf[1]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({ _bsdf->meta_common.bsdf[1],_1stdword,false,0,_maskParent });
			_1stdword = BSDFtype2opcode(_bsdf->blendbsdf.bsdf[0]);
			writeInheritableFlags(_1stdword, _parent1stDword);
			stack.push({ _bsdf->meta_common.bsdf[0],_1stdword,false,0,_maskParent });
			break;
		case CElementBSDF::Type::MIXTURE_BSDF:
		{
			//mixture is translated into tree of blends
			uint32_t blendbsdf = _1stdword;
			assert(_bsdf->mixturebsdf.childCount > 1u);
			for (uint32_t i = 0u; i < _bsdf->mixturebsdf.childCount-1ull; ++i)
			{
				uint32_t weight_ix = _bsdf->mixturebsdf.childCount-i-1u;
				stack.push({_bsdf,blendbsdf,false,weight_ix,_maskParent});
				auto* mixchild_bsdf = _bsdf->mixturebsdf.bsdf[weight_ix];
				uint32_t mixchild = BSDFtype2opcode(mixchild_bsdf);
				writeInheritableFlags(mixchild, _parent1stDword);
				stack.push({mixchild_bsdf,mixchild,false,0,_maskParent});
			}
			uint32_t child0 = BSDFtype2opcode(_bsdf->mixturebsdf.bsdf[0]);
			writeInheritableFlags(child0, _parent1stDword);
			stack.push({_bsdf->mixturebsdf.bsdf[0],child0,false,0});
		}	
			break;
		case CElementBSDF::Type::MASK:
			_1stdword |= 1u<<bsdf::BITFIELDS_SHIFT_MASKFLAG;
			stack.push({_bsdf->mask.bsdf[0],_1stdword,false,0,_bsdf});
			break;
		case CElementBSDF::Type::TWO_SIDED:
			_1stdword |= 1u<<bsdf::BITFIELDS_SHIFT_TWOSIDED;
			stack.push({_bsdf->twosided.bsdf[0],_1stdword,false,0,_maskParent});
			break;
		case CElementBSDF::Type::PHONG:
			_IRR_DEBUG_BREAK_IF(1);
			break;
		}
	};
	_IRR_STATIC_INLINE_CONSTEXPR uint32_t INSTR_REG_SHIFT_REL = 32u;
	auto emitInstr = [](uint32_t _1stdword, const CElementBSDF* _node, uint32_t _regs, uint32_t _bsdfBufOffset) -> bsdf::instr_t {
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
				//always constant weights (not texture) -- leaving weight tex flag as 0
				break;
			default: break; //do not let warnings rise
			}
			break;
		case bsdf::OP_DIFFTRANS:
			_1stdword |= static_cast<uint32_t>(_node->difftrans.transmittance.value.type == SPropertyElementData::INVALID) << bsdf::BITFIELDS_SHIFT_SPEC_TRANS_TEX;
			break;
		default: break; //any other ones dont need any extra flags
		}
		//write index into bsdf buffer
		_1stdword &= (~(bsdf::INSTR_BSDF_BUF_OFFSET_MASK<<bsdf::INSTR_BSDF_BUF_OFFSET_SHIFT));
		_1stdword |= ((_bsdfBufOffset & bsdf::INSTR_BSDF_BUF_OFFSET_MASK) << bsdf::INSTR_BSDF_BUF_OFFSET_SHIFT);

		bsdf::instr_t instr = _1stdword;
		instr |= static_cast<bsdf::instr_t>(_regs)<<INSTR_REG_SHIFT_REL;
		return instr;
	};
	uint32_t firstFreeReg = 0u;
	const uint32_t instrBufBase = ctx.instrBuffer.size();
	push(bsdf, 0u, nullptr);
	while (!stack.empty())
	{
		auto& top = stack.top();
		uint32_t op = ((top.instr_1st_dword >> bsdf::INSTR_OPCODE_SHIFT) & bsdf::INSTR_OPCODE_MASK);
		_IRR_DEBUG_BREAK_IF(op==bsdf::OP_INVALID);
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
				regs |= ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_SRC1_SHIFT - INSTR_REG_SHIFT_REL);
			}
			else if (op == bsdf::OP_BLEND)
			{
				--firstFreeReg;
				regs = ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_DST_SHIFT - INSTR_REG_SHIFT_REL);
				regs |= ((firstFreeReg-1u) & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_SRC1_SHIFT - INSTR_REG_SHIFT_REL);
				regs |= (firstFreeReg & bsdf::INSTR_REG_MASK) << (bsdf::INSTR_REG_SRC2_SHIFT - INSTR_REG_SHIFT_REL);
			}

			const uint32_t bsdfBufIx = ctx.bsdfBuffer.size();
			if (top.bsdf)//if top.bsdf is nullptr, then bsdf buf offset will be irrelevant for this instruction (may be any value and won't ever be fetched anyway)
			{
				ctx.bsdfBuffer.push_back(
					bsdfNode2bsdfStruct(ctx, top.bsdf, 0u, top.bsdf->type==CElementBSDF::Type::MIXTURE_BSDF ? top.bsdf->mixturebsdf.weights[top.weight_ix] : 0.f, top.maskParent)
				);
				assert(bsdfBufIx < bsdf::INSTR_BSDF_BUF_OFFSET_MASK);
			}
			ctx.instrBuffer.push_back(emitInstr(top.instr_1st_dword, top.bsdf, regs, bsdfBufIx));
			stack.pop();
		}
		else if (!top.visited)
		{
			top.visited = true;
			switch ((top.instr_1st_dword & bsdf::INSTR_OPCODE_MASK))
			{
			case bsdf::OP_BLEND:
				push(top.bsdf->blendbsdf.bsdf[1], top.instr_1st_dword, top.maskParent);
				_IRR_FALLTHROUGH;
			default:
				push(top.bsdf->blendbsdf.bsdf[0], top.instr_1st_dword, top.maskParent);
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
		_IRR_DEBUG_BREAK_IF(ixs.size()>bsdf::INSTR_NORMAL_REG_MASK);
	}
	//reorder
	for (uint32_t ix : ixs)
	{
		const bsdf::instr_t instr = ctx.instrBuffer[ix];
		const uint32_t bmNormalReg = (instr >> bsdf::INSTR_NORMAL_REG_SHIFT)&bsdf::INSTR_NORMAL_REG_MASK;
		auto it = std::find_if(ctx.instrBuffer.begin()+instrBufBase, ctx.instrBuffer.begin()+ix, [bmNormalReg] (const bsdf::instr_t& _i) { return ((_i>>bsdf::INSTR_NORMAL_REG_SHIFT)&bsdf::INSTR_NORMAL_REG_MASK) != bmNormalReg; });
		//move bumpmap instruction to before first instruction which is using normal register of interest
		ctx.instrBuffer.erase(ctx.instrBuffer.begin()+ix);
		ctx.instrBuffer.insert(it, instr);
	}

	return {instrBufBase, ctx.instrBuffer.size()-instrBufBase};
}

core::smart_refctd_ptr<ICPUDescriptorSet> CMitsubaLoader::createDS0(const SContext& _ctx)
{
	auto pplnLayout = getBuiltinAsset<ICPUPipelineLayout,IAsset::ET_PIPELINE_LAYOUT>(PIPELINE_LAYOUT_CACHE_KEY, m_manager);
	auto ds0layout = pplnLayout->getDescriptorSetLayout(0u);

	auto ds0 = core::make_smart_refctd_ptr<ICPUDescriptorSet>(std::move(ds0layout));
	auto d = ds0->getDescriptors(PAGE_TAB_TEX_BINDING).begin();
	{
		auto* pgtab = m_texPacker->getPageTable();

		ICPUImageView::SCreationParams params;
		params.flags = static_cast<IImageView<ICPUImage>::E_CREATE_FLAGS>(0);
		params.format = pgtab->getCreationParameters().format;
		params.image = core::smart_refctd_ptr<ICPUImage>(pgtab);
		params.viewType = IImageView<ICPUImage>::ET_2D;
		params.subresourceRange.aspectMask = static_cast<IImage::E_ASPECT_FLAGS>(0);
		params.subresourceRange.baseArrayLayer = 0u;
		params.subresourceRange.layerCount = 1u;
		params.subresourceRange.baseMipLevel = 0u;
		params.subresourceRange.levelCount = pgtab->getCreationParameters().mipLevels;
		auto pgtabView = ICPUImageView::create(std::move(params));

		d->desc = std::move(pgtabView);
		d->image.imageLayout = EIL_UNDEFINED;
		d->image.sampler = nullptr;//TODO
	}
	d = ds0->getDescriptors(PHYS_PAGE_TEX_BINDING).begin();
	{
		auto* physpgTex = m_texPacker->getPhysicalAddressTexture();

		ICPUImageView::SCreationParams params;
		params.flags = static_cast<IImageView<ICPUImage>::E_CREATE_FLAGS>(0);
		params.format = physpgTex->getCreationParameters().format;
		params.image = core::smart_refctd_ptr<ICPUImage>(physpgTex);
		params.viewType = IImageView<ICPUImage>::ET_2D_ARRAY;
		params.subresourceRange.aspectMask = static_cast<IImage::E_ASPECT_FLAGS>(0);
		params.subresourceRange.baseArrayLayer = 0u;
		params.subresourceRange.layerCount = physpgTex->getCreationParameters().arrayLayers;
		params.subresourceRange.baseMipLevel = 0u;
		params.subresourceRange.levelCount = 1u;
		auto physPgTexView = ICPUImageView::create(std::move(params));

		d->desc = std::move(physPgTexView);
		d->image.imageLayout = EIL_UNDEFINED;
		d->image.sampler = nullptr;//TODO
	}
	d = ds0->getDescriptors(INSTR_BUF_BINDING).begin();
	{
		auto instrbuf = core::make_smart_refctd_ptr<ICPUBuffer>(_ctx.instrBuffer.size()*sizeof(decltype(_ctx.instrBuffer)::value_type));
		memcpy(instrbuf->getPointer(), _ctx.instrBuffer.data(), instrbuf->getSize());

		d->buffer.offset = 0u;
		d->buffer.size = instrbuf->getSize();
		d->desc = std::move(instrbuf);
	}
	d = ds0->getDescriptors(BSDF_BUF_BINDING).begin();
	{
		auto bsdfbuf = core::make_smart_refctd_ptr<ICPUBuffer>(_ctx.bsdfBuffer.size()*sizeof(decltype(_ctx.bsdfBuffer)::value_type));
		memcpy(bsdfbuf->getPointer(), _ctx.bsdfBuffer.data(), bsdfbuf->getSize());

		d->buffer.offset = 0u;
		d->buffer.size = bsdfbuf->getSize();
		d->desc = std::move(bsdfbuf);
	}

	return ds0;
}

core::smart_refctd_ptr<CMitsubaPipelineMetadata> CMitsubaLoader::createPipelineMetadata(core::smart_refctd_ptr<ICPUDescriptorSet>&& _ds0, const ICPUPipelineLayout* _layout)
{
	constexpr size_t DS1_METADATA_ENTRY_CNT = 3ull;
	core::smart_refctd_dynamic_array<IPipelineMetadata::ShaderInputSemantic> inputs = core::make_refctd_dynamic_array<decltype(inputs)>(DS1_METADATA_ENTRY_CNT);
	{
		const ICPUDescriptorSetLayout* ds1layout = _layout->getDescriptorSetLayout(1u);

		constexpr IPipelineMetadata::E_COMMON_SHADER_INPUT types[DS1_METADATA_ENTRY_CNT]{ IPipelineMetadata::ECSI_WORLD_VIEW_PROJ, IPipelineMetadata::ECSI_WORLD_VIEW, IPipelineMetadata::ECSI_WORLD_VIEW_INVERSE_TRANSPOSE };
		constexpr uint32_t sizes[DS1_METADATA_ENTRY_CNT]{ sizeof(SBasicViewParameters::MVP), sizeof(SBasicViewParameters::MV), sizeof(SBasicViewParameters::NormalMat) };
		constexpr uint32_t relOffsets[DS1_METADATA_ENTRY_CNT]{ offsetof(SBasicViewParameters,MVP), offsetof(SBasicViewParameters,MV), offsetof(SBasicViewParameters,NormalMat) };
		for (uint32_t i = 0u; i < DS1_METADATA_ENTRY_CNT; ++i)
		{
			auto& semantic = (*inputs)[i];
			semantic.type = types[i];
			semantic.descriptorSection.type = IPipelineMetadata::ShaderInput::ET_UNIFORM_BUFFER;
			semantic.descriptorSection.uniformBufferObject.binding = ds1layout->getBindings().begin()[0].binding;
			semantic.descriptorSection.uniformBufferObject.set = 1u;
			semantic.descriptorSection.uniformBufferObject.relByteoffset = relOffsets[i];
			semantic.descriptorSection.uniformBufferObject.bytesize = sizes[i];
			semantic.descriptorSection.shaderAccessFlags = ICPUSpecializedShader::ESS_VERTEX;
		}
	}

	return core::make_smart_refctd_ptr<CMitsubaPipelineMetadata>(std::move(_ds0), std::move(inputs));
}


}
}
}