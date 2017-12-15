// Copyright (C) 2016 Mateusz "DeVsh" Kielan
// This file is part of the "Irrlicht Engine" and "Build A World".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// and on http://irrlicht.sourceforge.net/forum/viewtopic.php?f=2&t=49672

#ifndef __I_GPU_TRANSIENT_BUFFER_H_INCLUDED__
#define __I_GPU_TRANSIENT_BUFFER_H_INCLUDED__

#include "IGPUMappedBuffer.h"
#include "IVideoDriver.h"
#include "IDriverFence.h"
#include <vector>
#include "../source/Irrlicht/os.h"

class FW_Mutex;
class FW_ConditionVariable;

namespace irr
{


namespace video
{

//! Persistently Mapped buffer
/**
A) Only one thread can modify buffer descriptor
B) Only the main thread can grow the buffer!!!!!!!!! (OpenGL)
    B1) The grow can only occur if there are no pointers mapped to data
    B2) But can alloc from other thread if we agree to wait on Free() or GPU fences
**/
/*
// General Flow
Range = Buffer->Allocate()
Fill Range in Buffer in any way you like
Buffer->Commit(Range or SubRange); //CPU now STOPs writing to Buffer
//one subrange can only be committed once (no overlaps)

{ //Many GPU commands with arbitrary SubRanges
    if (!Buffer->queryRangeComitted(SubRange)) // optional, to check if CPU done using allocated mem
        JUMP AWAY;

    Driver->Use(SubRange);
    Buffer->fenceRangeUsedByGPU(SubRange); //one range can overlap other fenced ranges
}
//Afterwards when GPU is done using SubRange
Buffer->Free(SubRange);//any range, but no overlaps in freeing


We can split the load across several threads:

 ====(OPTIONAL)===== Thread 0 ====(OPTIONAL)====
 ===================  start   ===================
    MemRange = Buffer->Alloc(); //mutexed and thread safe
    PassToThread1(MemRange);
 ===================   end    ===================
 ====(OPTIONAL)===== Thread 0 ====(OPTIONAL)====

 =================== Thread 1 ===================
 ===================  start   ===================
    if (Thread 0 used)
        MemRange = GetFromThread0();
    else
        MemRange = Buffer->Alloc(); //mutexed and thread safe

    FillWithData(Buffer->getPointer()[MemRange]);
    Buffer->Commit(MemRange);
    PassToThread2(MemRange);
 ===================   end    ===================
 =================== Thread 1 ===================

 =================== Thread 2 ===================
 ===================  start   ===================
    MemRange = GetFromThread1();

    for (MANY DRAWCALLS XD AHAHAHA LOLOLOLOL)
    {
        // SubRange.start>=MemRange.end&&SubRange.end<=MemRange.end
        Driver->doCrazyGPUShit(SubRange);
        Driver->fenceRangeUsedByGPU(SubRange);
    }
    Buffer->Free(MemRange);
    //Many subranges of MemRange will be Returned to Free Pool
    //asynchronously on Alloc() when OpenGL completes the fences over them
    MemRange = INVALID;
 ===================   end    ===================
 =================== Thread 2 ===================

For Obvious reasons I recommend keeping fenc[]*ing and Free()'ing in the same thread
*/
/**
Thread Safeness:

    1) Creation and Deletion of IGPUTransientBuffer is NOT THREAD SAFE, and CAN ONLY BE DONE IN THE OPENGL THREAD
        (basically expose the object [handle] to other threads after creating it)
    2) Using the pointer returned by getRangePointer(size_t start) is NOT THREAD SAFE,
        UNLESS AT LEAST one sub-range remains allocated, that is Commit() has not been called on a Alloc()'ed subrange yet
    3) fenceRangeUsedByGPU() most probably should be calle donly by the OpenGL THREAD!!!!
    4) Alloc(,growIfTooSmall=true) CAN ONLY BE CALLED FROM THE OpenGL THREAD!!!
    5) Alloc(), Commit(), Place(), Free(), fenceRangeUsedByGPU() and DefragDescriptor() are all thread safe
    6) Using EWP_WAIT_FOR_CPU_UNMAP bit on any call while having un-Commit()'ed ranges (which will not be Commit()'ed by other Threads) will DEADLOCK
    7) Using EWP_WAIT_FOR_GPU_FREE bit on Alloc() while having un-Free()'ed ranges (which will not be Free()'ed by other Threads) will DEADLOCK

**/
class IGPUTransientBuffer : public virtual IReferenceCounted
{
    public:
        struct Allocation
        {
            enum E_ALLOCATION_STATE
            {
                EAS_FREE = 0,
                EAS_ALLOCATED,
                EAS_PENDING_RENDER_CMD
            };
            E_ALLOCATION_STATE state;
            IDriverFence* fence;
            size_t start;
            size_t end;
        };

        enum E_ALLOC_RETURN_STATUS
        {
            EARS_SUCCESS = 0,
            ///EARS_TIMEOUT, Future
            EARS_FAIL,
            EARS_ERROR
        };
        enum E_WAIT_POLICY
        {
            EWP_DONT_WAIT,
            EWP_WAIT_FOR_CPU_UNMAP=1,
            EWP_WAIT_FOR_GPU_FREE=2
        };


        static IGPUTransientBuffer* createMappedTransientBuffer(IVideoDriver* driver, const size_t& bufsize=0x100000u, const E_GPU_BUFFER_ACCESS& accessPattern=EGBA_WRITE,
                                                                const bool& inCPUMem=true, const bool& growable=false, const bool& autoFlush=true, const bool& threadSafe=true, core::LeakDebugger* dbgr=NULL)
        {
            IGPUMappedBuffer* buffer = driver->createPersistentlyMappedBuffer(bufsize,NULL,accessPattern,true,inCPUMem);
            IGPUTransientBuffer* retval = new IGPUTransientBuffer(driver,buffer,growable,autoFlush,threadSafe,dbgr);
			buffer->drop();
			return retval;
        }
        static IGPUTransientBuffer* createMappedTransientBuffer(IVideoDriver* driver, IGPUMappedBuffer* buffer, const bool& growable=false, const bool& autoFlush=true, const bool& threadSafe=true, core::LeakDebugger* dbgr=NULL)
        {
            return new IGPUTransientBuffer(driver,buffer,growable,autoFlush,threadSafe,dbgr);
        }
        static IGPUTransientBuffer* createTransientBuffer(IVideoDriver* driver, const size_t& bufsize=0x100000u, const E_GPU_BUFFER_ACCESS& accessPattern=EGBA_WRITE,
                                                          const bool& inCPUMem=true, const bool& canModifySubData=false, const bool& growable=false, const bool& autoFlush=true, const bool& threadSafe=true, core::LeakDebugger* dbgr=NULL)
        {
            IGPUBuffer* buffer = driver->createGPUBuffer(bufsize,NULL,canModifySubData,inCPUMem,accessPattern);
            IGPUTransientBuffer* retval = new IGPUTransientBuffer(driver,buffer,growable,autoFlush,threadSafe,dbgr);
			buffer->drop();
			return retval;
        }

        IGPUBuffer* getUnderlyingBuffer() {return underlyingBuffer;}
        //
        bool Validate();
        void PrintDebug(bool needsMutex=true);
        //
        //do more defragmentation in alloc()
        E_ALLOC_RETURN_STATUS Alloc(size_t &offsetOut, const size_t &maxSize, const size_t& alignment=32, E_WAIT_POLICY waitPolicy=EWP_DONT_WAIT, bool growIfTooSmall = false);
        //
        bool Commit(const size_t& start, const size_t& end);
        //
        bool Place(size_t &offsetOut, const void* data, const size_t& dataSize, const size_t& alignment=32, const E_WAIT_POLICY &waitPolicy=EWP_DONT_WAIT, const bool &growIfTooSmall = false);
        //! Unless memory is being used by GPU it will be returned to free pool straight away
        //! Useful if you dont end up using comitted memory by GPU
        bool Free(const size_t& start, const size_t& end);
        // GPU side calls
        bool fenceRangeUsedByGPU(const size_t& start, const size_t& end);
        //
        inline bool queryRangeCommitted(const size_t& start, const size_t& end)
        {
            return queryRange(start,end,Allocation::EAS_PENDING_RENDER_CMD);
        }
        bool queryRange(const size_t& start, const size_t& end, const Allocation::E_ALLOCATION_STATE& state);
        //
        bool waitRangeFences(const size_t& start, const size_t& end, size_t timeOutNs);

        void DefragDescriptor();

        const size_t& getFreeSpace() const {return totalFreeSpace;}
        const size_t& getTrueFreeSpace() const {return totalTrueFreeSpace;}
    private:
        ~IGPUTransientBuffer();
        IGPUTransientBuffer(IVideoDriver* driver, IGPUBuffer* buffer, const bool& growable, const bool& autoFlush, const bool& threadSafe, core::LeakDebugger* dbgr=NULL);
        FW_Mutex* mutex;
        FW_ConditionVariable* allocationChanged;
        size_t lastChanged;
        FW_Mutex* allocMutex;

        size_t totalFreeSpace;
        size_t totalTrueFreeSpace;
        size_t largestFreeChunkSize; //! unifinished
        size_t trueLargestFreeChunkSize; //! unfinished

		bool flushOnWait;
        const bool canGrow;
        IVideoDriver* Driver;
        IGPUBuffer* underlyingBuffer;
        IGPUMappedBuffer* underlyingBufferAsMapped;

        std::vector<Allocation> allocs;
        inline bool invalidState(const Allocation& a)
        {
            if (a.fence)
            {
                return a.state==Allocation::EAS_ALLOCATED;
            }
            else
            {
                return false;
            }
        }
        inline bool findFirstChunk(uint32_t& index, const size_t& start)
        {
            uint32_t startIx = 0;
            uint32_t endIx = allocs.size();
            while (startIx<endIx)
            {
                uint32_t m = (startIx+endIx)>>1;
                if (allocs[m].start>start)
                    endIx = m;
                else if (allocs[m].end<=start)
                    startIx = m+1;
                else
                {
        #ifdef _EXTREME_DEBUG
                    if (m>=allocs.size()||(!(start>=allocs[m].start&&start<allocs[m].end)))
                    {
                        os::Printer::log("IGPUTransientBuffer::findFirstChunk Binary Search FAILED!",ELL_ERROR);
                        return false;
                    }
        #endif
                    index = m;
                    return true;
                }
            }

        #ifdef _EXTREME_DEBUG
            if (startIx>endIx)
            {
                os::Printer::log("IGPUTransientBuffer::findFirstChunk Binary Search FAILED!",ELL_ERROR);
                return false;
            }
        #endif

            index =  endIx;
            return true;
        }
        bool validate_ALREADYMUTEXED();


        core::LeakDebugger* leakTracker;
};

} // end namespace scene
} // end namespace irr

#endif

