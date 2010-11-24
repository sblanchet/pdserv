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
Inbuf::Inbuf(Session *s, size_t bufMax): session(s), bufLenMax(bufMax)
{
    parseState = FindStart;
    buf = bptr = pptr = eptr = new char[bufIncrement];
    bufEnd = buf + bufIncrement;
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
    return bufEnd - eptr;
}

/////////////////////////////////////////////////////////////////////////////
void Inbuf::parse(size_t n)
{
    eptr += n;

//    cout << __LINE__ << __PRETTY_FUNCTION__ << ' ' 
//        << std::string(bptr, eptr - bptr) << endl;

//    cout << "   ->" << std::string(buf, eptr - buf) << "<-";
//    cout << "bptr:" << (void*)bptr << " eptr:" << (void*)eptr << endl;

    tokenize();

    if (bptr == eptr) {
//        cout << "Finished parsing buffer; resetting it" << endl;
        bptr = eptr = buf;
    }
    else if (eptr == bufEnd) {
        // Command is not complete and there is not enough space at the end
        // of the buffer for new data
        ptrdiff_t delta = 0;
//        cout << "not enough space " << std::string(buf, eptr - buf) << endl;
//        cout << "not enough space " << std::string(bptr, eptr - bptr) << endl;

        if (bptr == buf) {
            // Buffer is completely full
//            cout << "// Buffer is completely full" << endl;

            size_t bufLen = bufEnd - buf;

            if (bufLen < bufLenMax) {

                bufLen += bufIncrement;

                buf = new char[bufLen];
                bufEnd = buf + bufLen;

                delta = buf - bptr;

                std::copy(bptr, eptr, buf);
                eptr = buf + (eptr - bptr);

                delete[] bptr;

                bptr = buf;
            }
            else {
                // Buffer size exceeded limit. Discard everything
//                cout << "// Buffer size exceeded limit. Discard everything"
//                    << endl;
                eptr = buf;
                parseState = FindStart;
            }
        }
        else {
            // There is some space before the current command.
//            cout << "// There is some space before the current command." << endl;
            std::copy(bptr, eptr, buf);

            delta = buf - bptr;

            eptr = buf + (eptr - bptr);
            bptr = buf;
//            cout << "    " << std::string(bptr, eptr - bptr) << endl;
        }

        // Because the pointers have moved, start parsing from the beginning
        // again
        attr.adjust(delta);
        commandPtr += delta;
        pptr += delta;
        attrName += delta;
        attrValue += delta;
//        cout << "adjusted " << delta << commandPtr << endl;
    }
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
                bptr = std::find(bptr, eptr, '<');
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
                    quote = 0;
                    attrValue = pptr;
                }
                else {
                    attrValue = ++pptr;
                }

                parseState = GetValue;
                // no break here

            case GetValue:
                //cout << __LINE__ << "GetValue" << endl;
                if (quote)
                    pptr = std::find(pptr, const_cast<char*>(eptr), quote);
                else {
                    while (pptr != eptr) {
                        if (isspace(*pptr))
                            break;
                        pptr++;
                    }
                }

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
