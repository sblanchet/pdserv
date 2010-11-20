/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "config.h"

#include "Inbuf.h"
#include "Session.h"

#include <algorithm>

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
Inbuf::Inbuf(Session *s): session(s)
{
    bufLen = 1024;
    buf = bptr = pptr = eptr = new char[bufLen];
}

/////////////////////////////////////////////////////////////////////////////
Inbuf::~Inbuf()
{
    delete[] buf;
}

/////////////////////////////////////////////////////////////////////////////
char *Inbuf::bufptr() const
{
    return eptr;
}

/////////////////////////////////////////////////////////////////////////////
size_t Inbuf::free() const
{
    return bufLen + (eptr - buf);
}

/////////////////////////////////////////////////////////////////////////////
void Inbuf::parse(size_t n)
{
    //cout << __LINE__ << __PRETTY_FUNCTION__ << ' ' << (void*)buf << endl;
    //cout << "   ->" << std::string(buf, eptr - buf) << "<-" << endl;

    eptr += n;
    tokenize();

    if (eptr == buf + bufLen) {
        if (bptr == buf) {
            bufLen += 1024;

            buf = new char[bufLen];

            std::copy(bptr, eptr, buf);
            eptr = buf + (eptr - bptr);

            delete[] bptr;

            bptr = buf;
        }
        else {
            std::copy(bptr, eptr, buf);
            eptr = buf + (eptr - bptr);
        }

        parseState = FindStart;
    }

    if (bptr == eptr)
        bptr = eptr = buf;
}


/////////////////////////////////////////////////////////////////////////////
void Inbuf::tokenize()
{
    while (true) {
        switch (parseState) {
            case FindStart:
                //cout << __LINE__ << "FindStart" << endl;

                attr.clear();
                
                // Move forward in the buffer until '<' is found
                bptr = std::find(bptr, const_cast<char*>(eptr), '<');
                if (bptr == eptr)
                    return;

                pptr = bptr + 1;
                commandPtr = pptr;
                parseState = GetCommand;
                //cout << "Found command at " << (void*)commandPtr << endl;
                // no break here

            case GetCommand:
                //cout << __LINE__ << "GetCommand" << endl;
                while (pptr != eptr and (isalpha(*pptr) or *pptr == '_'))
                    pptr++;

                if (pptr == eptr)
                    return;

                parseState = GetToken;
                // no break here

            case GetToken:
                //cout << __LINE__ << "GetToken" << endl;
                while (pptr != eptr and isspace(*pptr))
                    *pptr++ = '\0';

                if (pptr == eptr)
                    return;

                if (!isalpha(*pptr)) {
                    if (*pptr == '>') {
                        *pptr = '\0';
                        bptr = pptr + 1;
                        //cout << __LINE__ << "processCommand()" << endl;
                        //cout << "Command is " << std::string(commandPtr) << endl;
                        session->processCommand(commandPtr, attr);
                    }
                    else if (*pptr == '/') {
                        if (eptr - pptr < 2)
                            return;

                        if (pptr[1] == '>') {
                            *pptr = '\0';
                            bptr = pptr + 2;
                            //cout << __LINE__ << "processCommand()" << endl;
                            //cout << "Command is " << std::string(commandPtr) << endl;
                            session->processCommand(commandPtr, attr);
                        }
                        else {
                            bptr = pptr + 1;
                        }
                    }
                    else {
                        bptr = pptr;
                    }
                    parseState = FindStart;
                    break;
                }

                attrName = pptr;
                parseState = GetAttribute;

            case GetAttribute:
                //cout << __LINE__ << "GetAttribute" << endl;
                while (pptr != eptr and isalpha(*pptr))
                    pptr++;

                if (pptr == eptr)
                    return;

                if (*pptr == '=') {
                    // Value attribute
                    *pptr++ = '\0';
                }
                else {
                    if (isspace(*pptr)) {
                        // Binary attribute
                        *pptr++ = '\0';
                        //cout << __LINE__ << "GetAttribute = "
                            //<< std::string(attrName) << endl;
                        attr.insert(attrName);
                    }
                    parseState = GetToken;
                    break;
                }

                parseState = GetQuote;
                // no break here

            case GetQuote:
                //cout << __LINE__ << "GetQuote" << endl;
                if (pptr == eptr)
                    return;

                quote = *pptr;
                if (quote != '\'' and quote != '"') {
                    bptr = pptr;
                    parseState = FindStart;
                    break;
                }

                pptr++;
                attrValue = pptr;
                parseState = GetValue;
                // no break here

            case GetValue:
                //cout << __LINE__ << "GetValue" << endl;
                pptr = std::find(pptr, const_cast<char*>(eptr), quote);

                if (pptr == eptr)
                    return;

                *pptr++ = '\0';

                //cout << __LINE__ << "GetAttribute = "
                    //<< std::string(attrName) << '=' << std::string(attrValue) << endl;
                attr.insert(attrName, attrValue);
                
                parseState = GetToken;
        }
    }
}
