// Copyright (C) 2017- Mateusz 'DevSH' Kielan
// This file is part of the "IrrlichtBAW" engine.
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IMAGE_DATA_H_INCLUDED__
#define __C_IMAGE_DATA_H_INCLUDED__

#include "IReferenceCounted.h"
#include "string.h"
#include "SColor.h"
#include "IImage.h"

namespace irr
{
namespace video
{

class CImageData : public IReferenceCounted
{
        void*       data;

        uint32_t    minCoord[3];
        uint32_t    maxCoord[3];
        uint32_t    colorFormat     : 24;
        uint32_t    mipLevelHint    : 6;
        uint32_t    unpackAlignment : 2;

        //! Final
        CImageData() {}
    //protected:
        virtual ~CImageData()
        {
            if (data)
                free(data);
        }

        inline void setupMemory(void* inData, const bool& dataAllocatedWithMallocAndCanTake)
        {
            if (inData&&dataAllocatedWithMallocAndCanTake)
                data = inData;
            else
                setupMemory(inData);
        }

        inline void setupMemory(const void* inData)
        {
            size_t imgByteSize = getImageDataSizeInBytes();
            data = malloc(imgByteSize);
            if (inData)
                memcpy(data,inData,imgByteSize);
        }


        inline const void* getSliceRowPointer_helper(uint32_t slice, uint32_t row) const
        {
            if (getCompressedFormatBlockSize(getColorFormat())!=1)
                return NULL;

            if (row<minCoord[0]||row>=maxCoord[0])
                return NULL;
            if (slice<minCoord[1]||slice>=maxCoord[1])
                return NULL;

            size_t size[3] = {maxCoord[0]-minCoord[0],maxCoord[1]-minCoord[1],maxCoord[2]-minCoord[2]};
            row     -= minCoord[0];
            slice   -= minCoord[1];
            return reinterpret_cast<uint8_t*>(data)+(slice*size[1]+row)*getPitchIncludingAlignment();
        }

    public:
        CImageData(IImage* fromImage, const uint32_t& inMipLevel=0,
                   const bool& dataAllocatedWithMallocAndCanTake=false)
        {
            minCoord[0] = 0;
            minCoord[1] = 0;
            minCoord[2] = 0;
            maxCoord[0] = fromImage->getDimension().Width;
            maxCoord[1] = fromImage->getDimension().Height;
            maxCoord[2] = 1;

            colorFormat = fromImage->getColorFormat();
            mipLevelHint = inMipLevel;
            unpackAlignment = 1;

            setupMemory(fromImage->getData(),dataAllocatedWithMallocAndCanTake);
        }

        CImageData(const void* inData, uint32_t inMinCoord[3], uint32_t inMaxCoord[3],
                   const uint32_t& inMipLevel, const ECOLOR_FORMAT& inFmt,
                   const uint32_t& inUnpackLineAlignment=1)
        {
            memcpy(minCoord,inMinCoord,3*sizeof(uint32_t));
            memcpy(maxCoord,inMaxCoord,3*sizeof(uint32_t));

            mipLevelHint = inMipLevel;
            colorFormat = inFmt;
            unpackAlignment = inUnpackLineAlignment;

            setupMemory(inData);
        }

        CImageData(void* inData, uint32_t inMinCoord[3], uint32_t inMaxCoord[3],
                   const uint32_t& inMipLevel, const ECOLOR_FORMAT& inFmt,
                   const uint32_t& inUnpackLineAlignment,
                   const bool& dataAllocatedWithMallocAndCanTake)
        {
            memcpy(minCoord,inMinCoord,3*sizeof(uint32_t));
            memcpy(maxCoord,inMaxCoord,3*sizeof(uint32_t));

            mipLevelHint = inMipLevel;
            colorFormat = inFmt;
            unpackAlignment = inUnpackLineAlignment;

            setupMemory(inData,dataAllocatedWithMallocAndCanTake);
        }

        //!
        inline void forgetAboutData() {data = NULL;}

        //! Returns pointer to raw data
        inline void* getData() {return data;}
        inline const void* getData() const {return data;}

        //! Returns offset in width,height and depth of image slice.
        inline const uint32_t* getOffset() const {return minCoord;}
        inline const uint32_t* getSliceMin() const {return getOffset();}

        //! Returns width,height and depth of image slice.
        inline const uint32_t* getSliceMax() const {return maxCoord;}

        //!
        /*
        inline bool addOffset(const uint32_t offset[3])
        {
            if (getCompressedFormatBlockSize(getColorFormat())!=1)
                return false; //special error val

            (void)addOffset(offset);
            return true;
        }
        */
        inline void addOffset(const uint32_t offset[3])
        {
            for (size_t i=0; i<3; i++)
            {
                minCoord[i] += offset[i];
                maxCoord[i] += offset[i];
            }
        }

        //!
        inline uint32_t getSupposedMipLevel() const {return mipLevelHint;}
        //!
        inline void setSupposedMipLevel(const uint32_t& newMipLevel) {mipLevelHint = newMipLevel;}

        //! Returns bits per pixel.
        inline uint32_t getBitsPerPixel() const
        {
            return getBitsPerPixelFromFormat(static_cast<ECOLOR_FORMAT>(colorFormat));
        }

        //! Returns image data size in bytes
        inline size_t getImageDataSizeInBytes() const
        {
            uint32_t size[3] = {maxCoord[0]-minCoord[0],maxCoord[1]-minCoord[1],maxCoord[2]-minCoord[2]};
            const uint32_t blockAlignment = getCompressedFormatBlockSize(getColorFormat());

            if (blockAlignment!=1)
            {
                size[0] += blockAlignment-1;
                size[1] += blockAlignment-1;
                /*
                size[0] /= blockAlignment;
                size[1] /= blockAlignment;
                size[0] *= blockAlignment;
                size[1] *= blockAlignment;
                */
                size[0] &= ~(blockAlignment-1);
                size[1] &= ~(blockAlignment-1);

                return (size[0]*size[1]*getBitsPerPixel())/8*size[2];
            }
            else
            {
                size_t lineSize = getPitchIncludingAlignment();
                return lineSize*size[1]*size[2];
            }
        }

        //! Returns image data size in pixels
        inline size_t getImageDataSizeInPixels() const
        {
            size_t size[3] = {maxCoord[0]-minCoord[0],maxCoord[1]-minCoord[1],maxCoord[2]-minCoord[2]};
            return size[0]*size[1]*size[2];
        }

        //! Returns the color format
        inline ECOLOR_FORMAT getColorFormat() const {return static_cast<ECOLOR_FORMAT>(colorFormat);}

        //! Returns pitch of image
        inline uint32_t getPitch() const
        {
            return (getBitsPerPixel()*(maxCoord[0]-minCoord[0]))/8;
        }

        //!
        inline uint32_t getPitchIncludingAlignment() const
        {
            if (getCompressedFormatBlockSize(getColorFormat())!=1)
                return 0; //special error val

            return (getPitch()+unpackAlignment-1)/unpackAlignment;
        }

        //!
        inline uint32_t getUnpackAlignment() const {return unpackAlignment;}

        //!
        inline void* getSliceRowPointer(const uint32_t& slice, const uint32_t& row)
        {
            return const_cast<void*>(getSliceRowPointer_helper(slice,row)); // I know what I'm doing
        }
        inline const void* getSliceRowPointer(const uint32_t& slice, const uint32_t& row) const
        {
            return getSliceRowPointer_helper(slice,row);
        }
};

} // end namespace video
} // end namespace irr

#endif


