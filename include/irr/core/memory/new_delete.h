// Copyright (C) 2018 Mateusz 'DevSH' Kielan
// This file is part of the "IrrlichtBAW Engine"
// For conditions of distribution and use, see copyright notice in irrlicht.h


#ifndef __IRR_NEW_DELETE_H_INCLUDED__
#define __IRR_NEW_DELETE_H_INCLUDED__

#include <memory>

#include "irr/macros.h"
#include "irr/core/alloc/aligned_allocator.h"



//! Allocator MUST provide a function with signature `pointer allocate(size_type n, size_type alignment, const_void_pointer hint=nullptr) noexcept`
#define _IRR_DEFAULT_ALLOCATOR_METATYPE                                 irr::core::aligned_allocator

namespace irr
{
namespace core
{
namespace impl
{

template<typename T, class Alloc=_IRR_DEFAULT_ALLOCATOR_METATYPE<T> >
struct AlignedWithAllocator
{
    template<typename... Args>
    static inline T*    new_(size_t align, Alloc& alloc, Args&&... args)
    {
        T* retval = alloc.allocate(1u,align);
        if (!retval)
            return nullptr;
        std::allocator_traits<Alloc>::construct(alloc,retval,std::forward<Args>(args)...);
        return retval;
    }
    template<typename... Args>
    static inline T*    new_(size_t align, Alloc&& alloc, Args&&... args)
    {
        return new_(align,static_cast<Alloc&>(alloc),std::forward<Args>(args)...);
    }
    static inline void  delete_(T* p, Alloc& alloc)
    {
        if (!p)
            return;
        std::allocator_traits<Alloc>::destroy(alloc,p);
        alloc.deallocate(p,1u);
    }
    static inline void  delete_(T* p, Alloc&& alloc=Alloc())
    {
        delete_(p,static_cast<Alloc&>(alloc));
    }


    static inline T*    new_array(size_t n, size_t align, Alloc& alloc)
    {
        T* retval = alloc.allocate(n,align);
        if (!retval)
            return nullptr;
        for (size_t dit=0; dit<n; dit++)
            std::allocator_traits<Alloc>::construct(alloc,retval+dit);
        return retval;
    }
    static inline T*    new_array(size_t n, size_t align, Alloc&& alloc=Alloc())
    {
        return new_array(n,align,static_cast<Alloc&>(alloc));
    }
    static inline void  delete_array(T* p, size_t n, Alloc& alloc)
    {
        if (!p)
            return;
        for (size_t dit=0; dit<n; dit++)
            std::allocator_traits<Alloc>::destroy(alloc,p+dit);
        alloc.deallocate(p,n);
    }
    static inline void  delete_array(T* p, size_t n, Alloc&& alloc=Alloc())
    {
        delete_array(p,n,static_cast<Alloc&>(alloc));
    }

    struct VA_ARGS_comma_workaround
    {
        VA_ARGS_comma_workaround(size_t align, Alloc _alloc = Alloc()) : m_align(align), m_alloc(_alloc) {}

        template<typename... Args>
        inline T*       new_(Args&&... args)
        {
            return AlignedWithAllocator::new_(m_align,m_alloc,std::forward<Args>(args)...);
        }


        size_t m_align;
        Alloc m_alloc;
    };
};

}
}
}

//use these by default instead of new and delete, single object (non-array) new takes constructor parameters as va_args
#define _IRR_NEW(_obj_type, ... )                               irr::core::impl::AlignedWithAllocator<_obj_type,_IRR_DEFAULT_ALLOCATOR_METATYPE<_obj_type> >::VA_ARGS_comma_workaround(_IRR_DEFAULT_ALIGNMENT(_obj_type)).new_(__VA_ARGS__)
#define _IRR_DELETE(_obj)                                       irr::core::impl::AlignedWithAllocator<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type,_IRR_DEFAULT_ALLOCATOR_METATYPE<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type> >::delete_(_obj)

#define _IRR_NEW_ARRAY(_obj_type,count)                         irr::core::impl::AlignedWithAllocator<_obj_type,_IRR_DEFAULT_ALLOCATOR_METATYPE<_obj_type> >::new_array(count,_IRR_DEFAULT_ALIGNMENT(_obj_type))
#define _IRR_DELETE_ARRAY(_obj,count)                           irr::core::impl::AlignedWithAllocator<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type,_IRR_DEFAULT_ALLOCATOR_METATYPE<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type> >::delete_array(_obj,count)

//! Extra Utility Macros for when you don't want to always have to deduce the alignment but want to use a specific allocator
//#define _IRR_ASSERT_ALLOCATOR_VALUE_TYPE(_obj_type,_allocator_type)     static_assert(std::is_same<_obj_type,_allocator_type::value_type>::value,"Wrong allocator value_type!")
#define _IRR_ASSERT_ALLOCATOR_VALUE_TYPE(_obj_type,_allocator_type)     static_assert(std::is_same<_obj_type,std::allocator_traits<_allocator_type >::value_type>::value,"Wrong allocator value_type!")

#define _IRR_NEW_W_ALLOCATOR(_obj_type,_allocator, ... )        irr::core::impl::AlignedWithAllocator<_obj_type,_IRR_DEFAULT_ALLOCATOR_METATYPE<_obj_type> >::VA_ARGS_comma_workaround(_IRR_DEFAULT_ALIGNMENT(_obj_type),_allocator).new_(__VA_ARGS__); \
                                                                    _IRR_ASSERT_ALLOCATOR_VALUE_TYPE(_obj_type,decltype(_allocator))
#define _IRR_DELETE_W_ALLOCATOR(_obj,_allocator)                irr::core::impl::AlignedWithAllocator<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type,_IRR_DEFAULT_ALLOCATOR_METATYPE<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type> >::delete_(_obj,_allocator); \
                                                                    _IRR_ASSERT_ALLOCATOR_VALUE_TYPE(std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type,decltype(_allocator))

#define _IRR_NEW_ARRAY_W_ALLOCATOR(_obj_type,count,_allocator)  irr::core::impl::AlignedWithAllocator<_obj_type,_IRR_DEFAULT_ALLOCATOR_METATYPE<_obj_type> >::new_array(count,_IRR_DEFAULT_ALIGNMENT(_obj_type),_allocator); \
                                                                    _IRR_ASSERT_ALLOCATOR_VALUE_TYPE(_obj_type,decltype(_allocator))
#define _IRR_DELETE_ARRAY_W_ALLOCATOR(_obj,count,_allocator)    irr::core::impl::AlignedWithAllocator<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type,_IRR_DEFAULT_ALLOCATOR_METATYPE<std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type> >::delete_array(_obj,count,_allocator); \
                                                                    _IRR_ASSERT_ALLOCATOR_VALUE_TYPE(std::remove_reference<std::remove_pointer<decltype(_obj)>::type>::type,decltype(_allocator))


namespace irr
{
namespace core
{
/**
//Maybe: Create a irr::AllocatedByDynamicAllocation class with a static function new[] like operator that takes an DynamicAllocator* parameter

template<class CRTP, class Alloc=aligned_allocator<CRTP> >
class IRR_FORCE_EBO AllocatedWithStatelessAllocator
{
    public:
};
*/

//! Special Class For providing deletion for things like C++11 smart-pointers
struct alligned_delete
{
    template<class T>
    void operator()(T* ptr) const noexcept(noexcept(ptr->~T()))
    {
        if (ptr)
        {
            ptr->~T();
            _IRR_ALIGNED_FREE(ptr);
        }
    }
};

}
}

#endif // __IRR_NEW_DELETE_H_INCLUDED__

