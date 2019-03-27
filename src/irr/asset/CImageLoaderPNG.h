// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageWriterPNG.h"

#ifdef _IRR_COMPILE_WITH_PNG_WRITER_

#include "CImageLoaderPNG.h"
#include "CColorConverter.h"
#include "IWriteFile.h"
#include "os.h" // for logging
#include "irr/asset/ICPUTexture.h"

#ifdef _IRR_COMPILE_WITH_LIBPNG_
#ifndef _IRR_USE_NON_SYSTEM_LIB_PNG_
	#include <png.h> // use system lib png
#else // _IRR_USE_NON_SYSTEM_LIB_PNG_
	#include "libpng/png.h" // use irrlicht included lib png
#endif // _IRR_USE_NON_SYSTEM_LIB_PNG_
#endif // _IRR_COMPILE_WITH_LIBPNG_

namespace irr
{
namespace asset
{

#ifdef _IRR_COMPILE_WITH_LIBPNG_
// PNG function for error handling
static void png_cpexcept_error(png_structp png_ptr, png_const_charp msg)
{
	os::Printer::log("PNG fatal error", msg, ELL_ERROR);
	longjmp(png_jmpbuf(png_ptr), 1);
}

// PNG function for warning handling
static void png_cpexcept_warning(png_structp png_ptr, png_const_charp msg)
{
	os::Printer::log("PNG warning", msg, ELL_WARNING);
}

// PNG function for file writing
void PNGAPI user_write_data_fcn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_size_t check;

	io::IWriteFile* file=(io::IWriteFile*)png_get_io_ptr(png_ptr);
	check=(png_size_t) file->write((const void*)data,(uint32_t)length);

	if (check != length)
		png_error(png_ptr, "Write Error");
}
#endif // _IRR_COMPILE_WITH_LIBPNG_

CImageWriterPNG::CImageWriterPNG()
{
#ifdef _DEBUG
	setDebugName("CImageWriterPNG");
#endif
}

bool CImageWriterPNG::writeAsset(io::IWriteFile* _file, const SAssetWriteParams& _params, IAssetWriterOverride* _override)
{
    if (!_override)
        getDefaultOverride(_override);
#ifdef _IRR_COMPILE_WITH_LIBPNG_
    SAssetWriteContext ctx{_params, _file};

    const asset::CImageData* image =
#   ifndef _DEBUG
        static_cast<const asset::CImageData*>(_params.rootAsset);
#   else
        dynamic_cast<const asset::CImageData*>(_params.rootAsset);
#   endif
    assert(image);

    io::IWriteFile* file = _override->getOutputFile(_file, ctx, {image, 0u});

	if (!file || !image)
		return false;

	// Allocate the png write struct
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, (png_error_ptr)png_cpexcept_error, (png_error_ptr)png_cpexcept_warning);
	if (!png_ptr)
	{
		os::Printer::log("PNGWriter: Internal PNG create write struct failure\n", file->getFileName().c_str(), ELL_ERROR);
		return false;
	}

	// Allocate the png info struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		os::Printer::log("PNGWriter: Internal PNG create info struct failure\n", file->getFileName().c_str(), ELL_ERROR);
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

	// for proper error handling
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	png_set_write_fn(png_ptr, file, user_write_data_fcn, NULL);

	// Set info
	switch(image->getColorFormat())
	{
		case asset::EF_B8G8R8A8_UNORM:
		case asset::EF_A1R5G5B5_UNORM_PACK16:
			png_set_IHDR(png_ptr, info_ptr,
				image->getSize().X, image->getSize().Y,
				8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		break;
		default:
			png_set_IHDR(png_ptr, info_ptr,
				image->getSize().X, image->getSize().Y,
				8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}

	int32_t lineWidth = image->getSize().X;
	switch(image->getColorFormat())
	{
	case asset::EF_R8G8B8_UNORM:
	case asset::EF_B5G6R5_UNORM_PACK16:
		lineWidth*=3;
		break;
	case asset::EF_B8G8R8A8_UNORM:
	case asset::EF_A1R5G5B5_UNORM_PACK16:
		lineWidth*=4;
		break;
	// TODO: Error handling in case of unsupported color format
	default:
		break;
	}
	uint8_t* tmpImage = new uint8_t[image->getSize().Y*lineWidth];
	if (!tmpImage)
	{
		os::Printer::log("PNGWriter: Internal PNG create image failure\n", file->getFileName().c_str(), ELL_ERROR);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	uint8_t* data = (uint8_t*)image->getData();
	switch(image->getColorFormat())
	{
	case asset::EF_R8G8B8_UNORM:
        video::CColorConverter::convert_R8G8B8toR8G8B8(data,image->getSize().Y*image->getSize().X,tmpImage);
		break;
	case asset::EF_B8G8R8A8_UNORM:
        video::CColorConverter::convert_A8R8G8B8toA8R8G8B8(data,image->getSize().Y*image->getSize().X,tmpImage);
		break;
	case asset::EF_B5G6R5_UNORM_PACK16:
        video::CColorConverter::convert_R5G6B5toR8G8B8(data,image->getSize().Y*image->getSize().X,tmpImage);
		break;
	case asset::EF_A1R5G5B5_UNORM_PACK16:
        video::CColorConverter::convert_A1R5G5B5toA8R8G8B8(data,image->getSize().Y*image->getSize().X,tmpImage);
		break;
#ifndef _DEBUG
		// TODO: Error handling in case of unsupported color format
	default:
		break;
#endif
	}

	// Create array of pointers to rows in image data

	//Used to point to image rows
	uint8_t** RowPointers = new png_bytep[image->getSize().Y];
	if (!RowPointers)
	{
		os::Printer::log("PNGWriter: Internal PNG create row pointers failure\n", file->getFileName().c_str(), ELL_ERROR);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		delete [] tmpImage;
		return false;
	}

	data=tmpImage;
	// Fill array of pointers to rows in image data
	for (uint32_t i=0; i<image->getSize().Y; ++i)
	{
		RowPointers[i]=data;
		data += lineWidth;
	}
	// for proper error handling
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		delete [] RowPointers;
		delete [] tmpImage;
		return false;
	}

	png_set_rows(png_ptr, info_ptr, RowPointers);

	if (image->getColorFormat()==asset::EF_B8G8R8A8_UNORM || image->getColorFormat()==asset::EF_A1R5G5B5_UNORM_PACK16)
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);
	else
	{
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	}

	delete [] RowPointers;
	delete [] tmpImage;
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return true;
#else
	return false;
#endif
}

} // namespace video
} // namespace irr

#endif


