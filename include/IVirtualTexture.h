// Copyright (C) 2017 Mateusz 'DevSH' Kielan
// This file is part of "IrrlichtBAw".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_VIRTUAL_TEXTURE_H_INCLUDED__
#define __I_VIRTUAL_TEXTURE_H_INCLUDED__

#include "EDriverTypes.h"
#include "CImageData.h"
#include "IFrameBuffer.h"

namespace irr
{
namespace video
{

class IVirtualTexture : public virtual IReferenceCounted
{
public:
    enum E_DIMENSION_COUNT
    {
        EDC_ZERO=0,
        EDC_ONE,
        EDC_TWO,
        EDC_THREE,
        EDC_COUNT,
        EDC_FORCE32BIT=0xffffffffu
    };
    enum E_VIRTUAL_TEXTURE_TYPE
    {
        EVTT_OPAQUE_FILTERABLE,
        EVTT_2D_MULTISAMPLE,
        EVTT_BUFFER_OBJECT,
        EVTT_VIEW,
        EVTT_COUNT
    };

	virtual E_DIMENSION_COUNT getDimensionality() const = 0;

	//! Get dimension (=size) of the texture.
	/** \return The size of the texture. */
	virtual const uint32_t* getSize() const = 0;

    //!
    virtual E_VIRTUAL_TEXTURE_TYPE getVirtualTextureType() const = 0;

	//! Get driver type of texture.
	/** This is the driver, which created the texture. This method is used
	internally by the video devices, to check, if they may use a texture
	because textures may be incompatible between different devices.
	\return Driver type of texture. */
	virtual E_DRIVER_TYPE getDriverType() const = 0;

	//! Get the color format of texture.
	/** \return The color format of texture. */
	virtual ECOLOR_FORMAT getColorFormat() const = 0;

	//! Returns if the texture has an alpha channel
	inline bool hasAlpha() const {
		return getColorFormat () == video::ECF_A8R8G8B8 || video::ECF_R8G8B8A8 || getColorFormat () == video::ECF_A1R5G5B5 || getColorFormat () == video::ECF_A16B16G16R16F || getColorFormat () == ECF_A32B32G32R32F
                                            || getColorFormat() == ECF_RGBA_BC1 || getColorFormat() == ECF_RGBA_BC2 || getColorFormat() == ECF_RGBA_BC3;
	}
};

class IRenderableVirtualTexture : public IRenderable, public IVirtualTexture
{
};

}
}

#endif // __I_VIRTUAL_TEXTURE_H_INCLUDED__
