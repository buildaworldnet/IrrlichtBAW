// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageLoaderPCX.h"

#ifdef _IRR_COMPILE_WITH_PCX_LOADER_

#include "IReadFile.h"
#include "SColor.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "os.h"
#include "string.h"


namespace irr
{
namespace video
{


//! constructor
CImageLoaderPCX::CImageLoaderPCX()
{
	#ifdef _DEBUG
	setDebugName("CImageLoaderPCX");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderPCX::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "pcx" );
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderPCX::isALoadableFileFormat(io::IReadFile* file) const
{
	uint8_t headerID;
	file->read(&headerID, sizeof(headerID));
	return headerID == 0x0a;
}


//! creates a image from the file
std::vector<CImageData*> CImageLoaderPCX::loadImage(io::IReadFile* file) const
{
	SPCXHeader header;
	int32_t* paletteData = 0;

	file->read(&header, sizeof(header));

    std::vector<CImageData*> retval;
	//! return if the header is wrong
	if (header.Manufacturer != 0x0a && header.Encoding != 0x01)
		return retval;

	// return if this isn't a supported type
	if ((header.BitsPerPixel != 8) && (header.BitsPerPixel != 4) && (header.BitsPerPixel != 1))
	{
		os::Printer::log("Unsupported bits per pixel in PCX file.",
			file->getFileName().c_str(), irr::ELL_WARNING);
		return retval;
	}

	// read palette
	if( (header.BitsPerPixel == 8) && (header.Planes == 1) )
	{
		// the palette indicator (usually a 0x0c is found infront of the actual palette data)
		// is ignored because some exporters seem to forget to write it. This would result in
		// no image loaded before, now only wrong colors will be set.
		const long pos = file->getPos();
		file->seek( file->getSize()-256*3, false );

		uint8_t *tempPalette = new uint8_t[768];
		paletteData = new int32_t[256];
		file->read( tempPalette, 768 );

		for( int32_t i=0; i<256; i++ )
		{
			paletteData[i] = (0xff000000 |
					(tempPalette[i*3+0] << 16) |
					(tempPalette[i*3+1] << 8) |
					(tempPalette[i*3+2]));
		}

		delete [] tempPalette;

		file->seek(pos);
	}
	else if( header.BitsPerPixel == 4 )
	{
		paletteData = new int32_t[16];
		for( int32_t i=0; i<16; i++ )
		{
			paletteData[i] = (0xff000000 |
					(header.Palette[i*3+0] << 16) |
					(header.Palette[i*3+1] << 8) |
					(header.Palette[i*3+2]));
		}
	}

	// read image data
	const int32_t width = header.XMax - header.XMin + 1;
	const int32_t height = header.YMax - header.YMin + 1;
	const int32_t imagebytes = header.BytesPerLine * header.Planes * header.BitsPerPixel * height / 8;
	uint8_t* PCXData = new uint8_t[imagebytes];

	uint8_t cnt, value;
	int32_t lineoffset=0, linestart=0, nextmode=1;
	for(int32_t offset = 0; offset < imagebytes; offset += cnt)
	{
		file->read(&cnt, 1);
		if( !((cnt & 0xc0) == 0xc0) )
		{
			value = cnt;
			cnt = 1;
		}
		else
		{
			cnt &= 0x3f;
			file->read(&value, 1);
		}
		if (header.Planes==1)
			memset(PCXData+offset, value, cnt);
		else
		{
			for (uint8_t i=0; i<cnt; ++i)
			{
				PCXData[linestart+lineoffset]=value;
				lineoffset += 3;
				if (lineoffset>=3*header.BytesPerLine)
				{
					lineoffset=nextmode;
					if (++nextmode==3)
						nextmode=0;
					if (lineoffset==0)
						linestart += 3*header.BytesPerLine;
				}
			}
		}
	}

	// create image
	video::CImageData* image = 0;
	int32_t pad = (header.BytesPerLine - width * header.BitsPerPixel / 8) * header.Planes;

	if (pad < 0)
		pad = -pad;

    uint32_t nullOffset[3] = {0,0,0};
    uint32_t imageSize[3] = {width,height,1};
	if (header.BitsPerPixel==8)
	{
		switch(header.Planes) // TODO: Other formats
		{
		case 1:
			image = new CImageData(NULL, nullOffset, imageSize, 0, ECF_A1R5G5B5);
			if (image)
				CColorConverter::convert8BitTo16Bit(PCXData, (int16_t*)image->getData(), width, height, paletteData, pad);
			break;
		case 3:
			image = new CImageData(NULL, nullOffset, imageSize, 0, ECF_R8G8B8);
			if (image)
				CColorConverter::convert24BitTo24Bit(PCXData, (uint8_t*)image->getData(), width, height, pad);
			break;
		}
	}
	else if (header.BitsPerPixel==4)
	{
		if (header.Planes==1)
		{
			image = new CImageData(NULL, nullOffset, imageSize, 0, ECF_A1R5G5B5);
			if (image)
				CColorConverter::convert4BitTo16Bit(PCXData, (int16_t*)image->getData(), width, height, paletteData, pad);
		}
	}
	else if (header.BitsPerPixel==1)
	{
		if (header.Planes==4)
		{
			image = new CImageData(NULL, nullOffset, imageSize, 0, ECF_A1R5G5B5);
			if (image)
				CColorConverter::convert4BitTo16Bit(PCXData, (int16_t*)image->getData(), width, height, paletteData, pad);
		}
		else if (header.Planes==1)
		{
			image = new CImageData(NULL, nullOffset, imageSize, 0, ECF_A1R5G5B5);
			if (image)
				CColorConverter::convert1BitTo16Bit(PCXData, (int16_t*)image->getData(), width, height, pad);
		}
	}

	// clean up

	delete [] paletteData;
	delete [] PCXData;

	retval.push_back(image);
	return retval;
}


//! creates a loader which is able to load pcx images
IImageLoader* createImageLoaderPCX()
{
	return new CImageLoaderPCX();
}



} // end namespace video
} // end namespace irr

#endif

