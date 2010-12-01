/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef POINTER_H
#define POINTER_H

/////////////////////////////////////////////////////////////////////////////
    template<class T>
T* ptr_align(void *p)
{
    unsigned int mask = ~(sizeof(p) - 1);
    return reinterpret_cast<T*>(((unsigned int)p + ~mask) & mask);
}

#endif // POINTER_H
