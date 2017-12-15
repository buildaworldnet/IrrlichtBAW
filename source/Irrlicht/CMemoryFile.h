// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_MEMORY_READ_FILE_H_INCLUDED__
#define __C_MEMORY_READ_FILE_H_INCLUDED__

#include "IReadFile.h"
#include "IWriteFile.h"
#include "irrString.h"

namespace irr
{

namespace io
{

	/*!
		Class for reading and writing from memory.
	*/
	class CMemoryFile : public IReadFile, public IWriteFile
	{
	    protected:
            //! Destructor
            virtual ~CMemoryFile();

        public:
            //! Constructor
            CMemoryFile(void* memory, long len, const io::path& fileName, bool deleteMemoryWhenDropped);

            //! returns how much was read
            virtual int32_t read(void* buffer, uint32_t sizeToRead);

            //! returns how much was written
            virtual int32_t write(const void* buffer, uint32_t sizeToWrite);

            //! changes position in file, returns true if successful
            virtual bool seek(long finalPos, bool relativeMovement = false);

            //! returns size of file
            virtual long getSize() const;

            //! returns where in the file we are.
            virtual long getPos() const;

            //! returns name of file
            virtual const io::path& getFileName() const;

        private:

            void *Buffer;
            long Len;
            long Pos;
            io::path Filename;
            bool deleteMemoryWhenDropped;
	};

} // end namespace io
} // end namespace irr

#endif

