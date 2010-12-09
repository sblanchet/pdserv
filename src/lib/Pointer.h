/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_POINTER_H
#define LIB_POINTER_H

/////////////////////////////////////////////////////////////////////////////
template<class T>
inline T* ptr_align(void *p)
{
    const size_t mask = sizeof(T*) - 1;
    return reinterpret_cast<T*>(((unsigned)p + mask) & ~mask);
}

/////////////////////////////////////////////////////////////////////////////
inline size_t ptr_align(size_t len)
{
    const size_t mask = sizeof(void*) - 1;
    return (len + mask) & ~mask;
}

#endif // LIB_POINTER_H