/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Receiver.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace HRTLab;

/////////////////////////////////////////////////////////////////////////////
Receiver::Receiver(unsigned int tid): tid(tid)
{
}

/////////////////////////////////////////////////////////////////////////////
Receiver::~Receiver()
{
}
