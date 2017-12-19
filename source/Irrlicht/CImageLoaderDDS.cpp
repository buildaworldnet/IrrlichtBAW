// Copyright (C) 2002-2012 Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

/*
	Based on Code from Copyright (c) 2003 Randy Reddig
	Based on code from Nvidia's DDS example:
	http://www.nvidia.com/object/dxtc_decompression_code.html

	mainly c to cpp
*/


#include "CImageLoaderDDS.h"

#ifdef _IRR_COMPILE_WITH_DDS_LOADER_

#include "IReadFile.h"
#include "os.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "irrString.h"


namespace irr
{

namespace video
{


#ifndef __BIG_ENDIAN__
#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif
#endif


#ifdef __BIG_ENDIAN__

	int32_t   DDSBigLong( int32_t src ) { return src; }
	int16_t DDSBigShort( int16_t src ) { return src; }
	float DDSBigFloat( float src ) { return src; }

	int32_t DDSLittleLong( int32_t src )
	{
		return ((src & 0xFF000000) >> 24) |
			((src & 0x00FF0000) >> 8) |
			((src & 0x0000FF00) << 8) |
			((src & 0x000000FF) << 24);
	}

	int16_t DDSLittleShort( int16_t src )
	{
		return ((src & 0xFF00) >> 8) |
			((src & 0x00FF) << 8);
	}

	float DDSLittleFloat( float src )
	{
		floatSwapUnion in,out;
		in.f = src;
		out.c[ 0 ] = in.c[ 3 ];
		out.c[ 1 ] = in.c[ 2 ];
		out.c[ 2 ] = in.c[ 1 ];
		out.c[ 3 ] = in.c[ 0 ];
		return out.f;
	}

#else /*__BIG_ENDIAN__*/

	int32_t   DDSLittleLong( int32_t src ) { return src; }
	int16_t DDSLittleShort( int16_t src ) { return src; }
	float DDSLittleFloat( float src ) { return src; }

	int32_t DDSBigLong( int32_t src )
	{
		return ((src & 0xFF000000) >> 24) |
			((src & 0x00FF0000) >> 8) |
			((src & 0x0000FF00) << 8) |
			((src & 0x000000FF) << 24);
	}

	int16_t DDSBigShort( int16_t src )
	{
		return ((src & 0xFF00) >> 8) |
			((src & 0x00FF) << 8);
	}

	float DDSBigFloat( float src )
	{
		floatSwapUnion in,out;
		in.f = src;
		out.c[ 0 ] = in.c[ 3 ];
		out.c[ 1 ] = in.c[ 2 ];
		out.c[ 2 ] = in.c[ 1 ];
		out.c[ 3 ] = in.c[ 0 ];
		return out.f;
	}

#endif /*__BIG_ENDIAN__*/

namespace
{

/*!
	DDSDecodePixelFormat()
	determines which pixel format the dds texture is in
*/
void DDSDecodePixelFormat( ddsBuffer *dds, eDDSPixelFormat *pf )
{
	/* dummy check */
	if(	dds == NULL || pf == NULL )
		return;

	/* extract fourCC */
	const uint32_t fourCC = dds->pixelFormat.fourCC;

	/* test it */
	if( fourCC == 0 )
	{
	    bool hasAlpha = false;
	    bool hasRGB = false;
	    bool hasLuma = false;
	    uint32_t bitDepth = dds->pixelFormat.privateFormatBitCount;

	    if (dds->pixelFormat.flags&0x3)
	    {
            hasAlpha = true;
	    }
	    if (dds->pixelFormat.flags&0x40)
	    {
            hasRGB = true;
	    }
	    if (dds->pixelFormat.flags&0x20000)
	    {
            hasLuma = true;
	    }

        if (bitDepth==32&&(dds->pixelFormat.rBitMask&0x00ff0000)&&hasRGB&&hasAlpha)
            *pf = DDS_PF_ARGB8888;
        else if (bitDepth==32&&(dds->pixelFormat.rBitMask&0xff)&&hasRGB&&hasAlpha)
            *pf = DDS_PF_ABGR8888;
        else if (bitDepth==24&&(dds->pixelFormat.rBitMask&0x00ff0000)&&hasRGB)
            *pf = DDS_PF_RGB888;
        else if (bitDepth==16&&(dds->pixelFormat.rBitMask&0x7c00)&&hasRGB&&hasAlpha)
            *pf = DDS_PF_ARGB1555;
        else if (bitDepth==16&&(dds->pixelFormat.rBitMask&0xf800)&&hasRGB)
            *pf = DDS_PF_RGB565;
        else if (bitDepth==16&&(dds->pixelFormat.rBitMask&0xff)&&hasLuma&&hasAlpha)
            *pf = DDS_PF_LA88;
        else if (bitDepth==8&&(dds->pixelFormat.rBitMask&0xff)&&hasLuma)
            *pf = DDS_PF_L8;
        else if (bitDepth==8&&(dds->pixelFormat.rBitMask==0)&&hasAlpha)
            *pf = DDS_PF_A8;
        else
            *pf = DDS_PF_UNKNOWN;
	}
	else if( fourCC == *((uint32_t*) "DXT1") )
	{ // sodan was here
//	    if (dds->pixelFormat.privateFormatBitCount==24)
            *pf = DDS_PF_DXT1;
/*        else if (dds->pixelFormat.privateFormatBitCount==32)
            *pf = DDS_PF_DXT1_ALPHA;
        else
            printf("IRRLICHT BUUUUUUUUGGGGG!!!!!!!!!!\n SHOOT SOMEONE!\n");*/
	}
	else if( fourCC == *((uint32_t*) "DXT2") )
		*pf = DDS_PF_DXT2;
	else if( fourCC == *((uint32_t*) "DXT3") )
		*pf = DDS_PF_DXT3;
	else if( fourCC == *((uint32_t*) "DXT4") )
		*pf = DDS_PF_DXT4;
	else if( fourCC == *((uint32_t*) "DXT5") )
		*pf = DDS_PF_DXT5;
	else
		*pf = DDS_PF_UNKNOWN;
}


/*!
DDSGetInfo()
extracts relevant info from a dds texture, returns 0 on success
*/
int32_t DDSGetInfo( ddsBuffer *dds, int32_t *width, int32_t *height, int32_t *depth, eDDSPixelFormat *pf )
{
	/* dummy test */
	if( dds == NULL )
		return -1;

	/* test dds header */
	if( *((int32_t*) dds->magic) != *((int32_t*) "DDS ") )
		return -1;
	if( DDSLittleLong( dds->size ) != 124 )
		return -1;

	/* extract width and height */
	if ( width != NULL )
		*width = DDSLittleLong( dds->width );
	if ( height != NULL )
		*height = DDSLittleLong( dds->height );
    if ( depth != NULL && (dds->flags & 0x800000u) )//DDSD_DEPTH)
        *depth = DDSLittleLong( dds->depth );
    else
        *depth = 1;

	/* get pixel format */
	DDSDecodePixelFormat( dds, pf );

	/* return ok */
	return 0;
}


} // end anonymous namespace


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderDDS::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "dds" );
}


//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderDDS::isALoadableFileFormat(io::IReadFile* file) const
{
	if (!file)
		return false;

	ddsBuffer header;
	file->read(&header, sizeof(header));

	int32_t width, height, depth;
	eDDSPixelFormat pixelFormat;

	return (0 == DDSGetInfo( &header, &width, &height, &depth, &pixelFormat));
}


//! creates a surface from the file
std::vector<CImageData*> CImageLoaderDDS::loadImage(io::IReadFile* file) const
{
	std::vector<CImageData*> images;

    video::eDDSPixelFormat pixelFormat;
    int32_t width, height, depth, mipmapCnt;

	ddsBuffer header;
	file->read(&header, sizeof(header)-4);

	if ( 0 == DDSGetInfo( &header, &width, &height, &depth, &pixelFormat) )
	{
	    if (header.flags & 0x20000)//DDSD_MIPMAPCOUNT)
            mipmapCnt = header.mipMapCount;
	    else
            mipmapCnt = 1;


        for (int32_t i=0; i<mipmapCnt; i++)
        {
            uint32_t zeroDummy[3] = {0,0,0};
            uint32_t mipSize[3] = {0,height,depth};
            uint32_t& tmpWidth = mipSize[0];
            switch( pixelFormat )
            {
                case DDS_PF_DXT1:
                case DDS_PF_DXT2:
                case DDS_PF_DXT3:
                case DDS_PF_DXT4:
                case DDS_PF_DXT5:
                    tmpWidth = width;
                    break;
                default:
                    tmpWidth = header.pitch;
                    break;
            }
            uint32_t& tmpHeight = mipSize[1];
            uint32_t& tmpDepth = mipSize[2];
            tmpWidth += (uint32_t(1)<<i)-1;
            tmpHeight += (uint32_t(1)<<i)-1;
            if (false)
                tmpDepth += (uint32_t(1)<<i)-1; //! CHANGE AGAIN FOR 2D ARRAY AND CUBEMAP TEXTURES
            tmpWidth /= uint32_t(1)<<i;
            tmpHeight /= uint32_t(1)<<i;
            if (false)
                tmpDepth /= uint32_t(1)<<i; //! CHANGE AGAIN FOR 2D ARRAY AND CUBEMAP TEXTURES

            /* decompress */
            ECOLOR_FORMAT colorFormat = ECF_UNKNOWN;
            switch( pixelFormat )
            {
                case DDS_PF_ARGB8888:
                case DDS_PF_ABGR8888:
                    /* fixme: support other [a]rgb formats */
                    {
                        colorFormat = pixelFormat==DDS_PF_ABGR8888 ? ECF_R8G8B8A8:ECF_A8R8G8B8;
                        CImageData* data = new CImageData(NULL,zeroDummy,mipSize,i,colorFormat,4);
                        file->read(data->getData(),data->getImageDataSizeInBytes());
                        images.push_back(data);
                    }
                    break;
                case DDS_PF_RGB888:
                    /* fixme: support other [a]rgb formats */
                    {
                        colorFormat = ECF_R8G8B8;
                        CImageData* data = new CImageData(NULL,zeroDummy,mipSize,i,colorFormat,1);
                        file->read(data->getData(),data->getImageDataSizeInBytes());
                        images.push_back(data);

                        uint8_t* dataToManipulate = reinterpret_cast<uint8_t*>(data->getData());
                        for (uint32_t j=0; j<data->getImageDataSizeInBytes(); j+=3)
                        {
                            uint8_t byte1 = dataToManipulate[j+0];
                            uint8_t byte2 = dataToManipulate[j+1];
                            uint8_t byte3 = dataToManipulate[j+2];

                            dataToManipulate[j+0] = byte3;
                            dataToManipulate[j+1] = byte2;
                            dataToManipulate[j+2] = byte1;
                        }
                    }
                    break;
                case DDS_PF_ARGB1555:
                    /* fixme: support other [a]rgb formats */
                    {
                        colorFormat = ECF_A1R5G5B5;
                        CImageData* data = new CImageData(NULL,zeroDummy,mipSize,i,colorFormat,2);
                        file->read(data->getData(),data->getImageDataSizeInBytes());
                        images.push_back(data);
                    }
                    break;
                case DDS_PF_RGB565:
                    break;
                case DDS_PF_LA88:
                    /* fixme: support other [a]rgb formats */
                    {
                        colorFormat = ECF_R8G8;
                        CImageData* data = new CImageData(NULL,zeroDummy,mipSize,i,colorFormat,2);
                        file->read(data->getData(),data->getImageDataSizeInBytes());
                        images.push_back(data);
                    }
                    break;
                case DDS_PF_L8:
                case DDS_PF_A8:
                    /* fixme: support other [a]rgb formats */
                    {
                        colorFormat = ECF_R8;
                        CImageData* data = new CImageData(NULL,zeroDummy,mipSize,i,colorFormat,1);
                        file->read(data->getData(),data->getImageDataSizeInBytes());
                        images.push_back(data);
                    }
                    break;

                case DDS_PF_DXT1:
                case DDS_PF_DXT2:
                case DDS_PF_DXT3:
                case DDS_PF_DXT4:
                case DDS_PF_DXT5:
                    {
                        if (pixelFormat==video::DDS_PF_DXT2||pixelFormat==video::DDS_PF_DXT3)
                            colorFormat = video::ECF_RGBA_BC2;
                        else if (pixelFormat==video::DDS_PF_DXT4||pixelFormat==video::DDS_PF_DXT5)
                            colorFormat = video::ECF_RGBA_BC3;
                        else if (pixelFormat==video::DDS_PF_DXT1_ALPHA)
                            colorFormat = video::ECF_RGBA_BC1;
                        else if (pixelFormat==video::DDS_PF_DXT1)
                            colorFormat = video::ECF_RGB_BC1;

                        mipSize[0] += 3;
                        mipSize[1] += 3;
                        mipSize[0] &= 0xfffffc;
                        mipSize[1] &= 0xfffffc;
                        CImageData* data = new CImageData(NULL,zeroDummy,mipSize,i,colorFormat,1);
                        file->read(data->getData(),data->getImageDataSizeInBytes());
                        images.push_back(data);
                    }
                    break;

                default:
                case DDS_PF_UNKNOWN:
                    break;
            }
        }
	}


	return images;
}


//! creates a loader which is able to load dds images
IImageLoader* createImageLoaderDDS()
{
	return new CImageLoaderDDS();
}


} // end namespace video
} // end namespace irr

#endif

