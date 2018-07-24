
// Copyright (C) 2016 Mateusz "DeVsh" Kielan
// This file is part of the "Irrlicht Engine" and "Build A World".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// and on http://irrlicht.sourceforge.net/forum/viewtopic.php?f=2&t=49672

#ifndef __I_BUFFER_H_INCLUDED__
#define __I_BUFFER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "stdint.h"

namespace irr
{
namespace core
{

class IBuffer : public virtual IReferenceCounted
{
    public:
        //! size in BYTES
        virtual const uint64_t& getSize() const = 0;

        virtual const uint64_t& getLastTimeReallocated() const {return lastTimeReallocated;}
    protected:
        uint64_t lastTimeReallocated;
    private:
        //
};

} // end namespace scene
} // end namespace irr

#endif

