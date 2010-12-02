/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef LIB_POINTER_H
#define LIB_POINTER_H

/////////////////////////////////////////////////////////////////////////////
template<class T>
T* ptr_align(void *p)
{
    const size_t mask = sizeof(T*) - 1;
    return reinterpret_cast<T*>(((unsigned)p + mask) & ~mask);
}

#endif // LIB_POINTER_H
