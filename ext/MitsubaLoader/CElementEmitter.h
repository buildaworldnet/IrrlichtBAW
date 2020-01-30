#ifndef __C_ELEMENT_EMITTER_H_INCLUDED__
#define __C_ELEMENT_EMITTER_H_INCLUDED__

#include <cmath>

#include "vectorSIMD.h"
#include "../../ext/MitsubaLoader/CElementTexture.h"

namespace irr
{
namespace ext
{
namespace MitsubaLoader
{
	

class CGlobalMitsubaMetadata;

class CElementEmitter : public IElement
{
	public:
		enum Type
		{
			INVALID,
			POINT,
			AREA,
			SPOT,
			DIRECTIONAL,
			COLLIMATED,
			//SKY,
			//SUN,
			//SUNSKY,
			ENVMAP,
			CONSTANT
		};
	struct SampledEmitter
	{
		SampledEmitter() : samplingWeight(1.f) {}

		float samplingWeight;
	};
		struct Point : SampledEmitter
		{
			core::vectorSIMDf intensity = core::vectorSIMDf(1.f); // Watts Steradian^-1
		};
		struct Area : SampledEmitter
		{
			core::vectorSIMDf radiance = core::vectorSIMDf(1.f); // Watts Meter^-2 Steradian^-1
		};
		struct Spot : SampledEmitter
		{
			core::vectorSIMDf intensity = core::vectorSIMDf(1.f); // Watts Steradian^-1
			float cutoffAngle = 20.f; // degrees, its the cone half-angle
			float beamWidth = NAN;
			CElementTexture* texture = nullptr;
		};
		struct Directional : SampledEmitter
		{
			core::vectorSIMDf irradiance = core::vectorSIMDf(1.f); // Watts Meter^-2
		};
		struct Collimated : SampledEmitter
		{
			core::vectorSIMDf power = core::vectorSIMDf(1.f); // Watts
		};/*
		struct Sky : SampledEmitter
		{
			float turbidity = 3.f;
			core::vectorSIMDf albedo = core::vectorSIMDf(0.15f);
			core::vectorSIMDf sunDirection = calculate default from tokyo japan at 15:00 on 10.07.2010;
			float stretch = 1.f; // must be [1,2]
			int32_t resolution = 512;
			float scale = 1.f;
		};
		struct Sun : SampledEmitter
		{
			float turbidity = 3.f;
			core::vectorSIMDf sunDirection = calculate default from tokyo japan at 15:00 on 10.07.2010;
			int32_t resolution = 512;
			float scale = 1.f;
			float sunRadiusScale = 1.f;

		};
		struct SunSky : Sky
		{
			float sunRadiusScale = 1.f;
		};*/
		struct EnvMap : SampledEmitter
		{
			SPropertyElementData filename;
			float scale = 1.f;
			float gamma = NAN;
			//bool cache = false;
		};
		struct Constant : SampledEmitter
		{
			core::vectorSIMDf radiance = core::vectorSIMDf(1.f); // Watts Meter^-2 Steradian^-1
		};


		CElementEmitter(const char* id) : IElement(id), type(Type::INVALID), /*toWorldType(IElement::Type::TRANSFORM),*/ transform()
		{
		}
		CElementEmitter(const CElementEmitter& other) : IElement(""), transform()
		{
			operator=(other);
		}
		virtual ~CElementEmitter()
		{
		}

		inline CElementEmitter& operator=(const CElementEmitter& other)
		{
			IElement::operator=(other);
			transform = other.transform;
			type = other.type;
			switch (type)
			{
				case Type::POINT:
					point = other.point;
					break;
				case Type::AREA:
					area = other.area;
					break;
				case Type::SPOT:
					spot = other.spot;
					break;
				case Type::DIRECTIONAL:
					directional = other.directional;
					break;
				case Type::COLLIMATED:
					collimated = other.collimated;
					break;/*
				case Type::SKY:
					sky = other.sky;
					break;
				case Type::SUN:
					sun = other.sun;
					break;
				case Type::SUNSKY:
					sunsky = other.sunsky;
					break;*/
				case Type::ENVMAP:
					envmap = other.envmap;
					break;
				case Type::CONSTANT:
					constant = other.constant;
					break;
				default:
					break;
			}
			return *this;
		}

		bool addProperty(SNamedPropertyElement&& _property) override;
		bool onEndTag(asset::IAssetLoader::IAssetLoaderOverride* _override, CGlobalMitsubaMetadata* globalMetadata) override;
		IElement::Type getType() const override { return IElement::Type::EMITTER; }
		std::string getLogName() const override { return "emitter"; }

		bool processChildData(IElement* _child, const std::string& name) override
		{
			if (!_child)
				return true;
			switch (_child->getType())
			{
				case IElement::Type::TRANSFORM:
					{
						auto tform = static_cast<CElementTransform*>(_child);
						if (name!="toWorld")
							return false;
						//toWorldType = IElement::Type::TRANSFORM;
						switch (type)
						{
							case Type::POINT:
								_IRR_FALLTHROUGH;
							case Type::SPOT:
								_IRR_FALLTHROUGH;
							case Type::DIRECTIONAL:
								_IRR_FALLTHROUGH;
							case Type::COLLIMATED:
								_IRR_FALLTHROUGH;/*
							case Type::SKY:
								_IRR_FALLTHROUGH;
							case Type::SUN:
								_IRR_FALLTHROUGH;
							case Type::SUNSKY:
								_IRR_FALLTHROUGH;*/
							case Type::ENVMAP:
								transform = *tform;
								return true;
							default:
								break;
						}
						return false;
					}
					break;/*
				case IElement::Type::ANIMATION:
					auto anim = static_cast<CElementAnimation>(_child);
					if (anim->name!="toWorld")
						return false;
					toWorlType = IElement::Type::ANIMATION;
					animation = anim;
					return true;
					break;*/
				case IElement::Type::TEXTURE:
					if (type!=SPOT || name!="texture")
						return false;
					spot.texture = static_cast<CElementTexture*>(_child);
					return true;
					break;
				default:
					break;
			}
			return false;
		}

		//
		Type type;
		CElementTransform transform;/*
		IElement::Type toWorldType;
		// nullptr means identity matrix
		union
		{
			CElementTransform* transform;
			CElementAnimation* animation;
		};*/
		union
		{
			Point		point;
			Area		area;
			Spot		spot;
			Directional	directional;
			Collimated	collimated;/*
			Sky			sky;
			Sun			sun;
			SunSky		sunsky;*/
			EnvMap		envmap;
			Constant	constant;
		};
};



}
}
}

#endif