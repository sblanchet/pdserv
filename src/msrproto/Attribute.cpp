/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Attribute.h"

#include <cstring>
#include <algorithm>
#include <sstream>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
void Attr::clear()
{
    id = 0;
    _id.clear();
    attrMap.clear();
}

/////////////////////////////////////////////////////////////////////////////
void Attr::insert(const char *name, size_t nameLen)
{
    //cout << "Binary attribute: Name=" << std::string(name, nameLen) << endl;
    AttrPtrs a = {name, nameLen, 0, 0};
    attrMap.insert(std::pair<size_t,AttrPtrs>(nameLen,a));
}

/////////////////////////////////////////////////////////////////////////////
void Attr::insert(const char *name, size_t nameLen,
                        char *value, size_t valueLen)
{
    //cout << "Value attribute: Name=" << std::string(name, nameLen)
        //<< ", Value=" << std::string(value, valueLen)
        //<< endl;
    if (nameLen == 2 and !strncmp(name, "id", 2)) {
        _id.assign(value, valueLen);
        id = &_id;
        return;
    }

    AttrPtrs a = {name, nameLen, value, valueLen};
    attrMap.insert(std::pair<size_t,AttrPtrs>(nameLen,a));
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::find(const char *name, char * &value, size_t &valueLen)
{
    size_t len = strlen(name);
    std::pair<AttrMap::iterator, AttrMap::iterator>
        ret(attrMap.equal_range(len));

    for (AttrMap::iterator it(ret.first); it != ret.second; it++) {
        if (!strncmp(name, it->second.name, len)) {
            value = it->second.value;
            valueLen = it->second.valueLen;
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::isEqual(const char *name, const char *s)
{
    char *value;
    size_t valueLen;

    if (find(name, value, valueLen))
        return !strncasecmp(value, s, valueLen);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::isTrue(const char *name)
{
    char *value;
    size_t valueLen;

    if (!(find(name, value, valueLen)))
        return false;

    if (valueLen == 1)
        return *value == '1';

    if (valueLen == 4)
        return !strncasecmp(value, "true", 4);

    if (valueLen == 2)
        return !strncasecmp(value, "on", 2);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::getString(const char *name, std::string &s)
{
    char *value;
    size_t valueLen;

    s.clear();

    if (!(find(name, value, valueLen)))
        return false;

    value[valueLen] = 0;

    char *pptr, *eptr = value + valueLen;
    while ((pptr = std::find(value, eptr, '&')) != eptr) {
        s.append(value, pptr - value);
        size_t len = eptr - pptr;
        if (len > 4 and !strncmp(pptr, "&gt;", 4)) {
            s.append(1, '>');
            value = pptr + 4;
        }
        else if (len > 4 and !strncmp(pptr, "&lt;", 4)) {
            s.append(1, '<');
            value = pptr + 4;
        }
        else if (len > 5 and !strncmp(pptr, "&amp;", 5)) {
            s.append(1, '&');
            value = pptr + 5;
        }
        else if (len > 6 and !strncmp(pptr, "&quot;", 6)) {
            s.append(1, '"');
            value = pptr + 6;
        }
        else if (len > 6 and !strncmp(pptr, "&apos;", 6)) {
            s.append(1, '\'');
            value = pptr + 6;
        }
        else {
            s.append(1, '&');
            value = pptr + 1;
        }
    }

    s.append(value, eptr - value);
    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::getUnsigned(const char *name, unsigned int &i)
{
    char *value;
    size_t valueLen;

    if (!(find(name, value, valueLen)))
        return false;

    value[valueLen] = 0;

    i = strtoul(value, 0, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::getUnsignedList(const char *name,
        std::list<unsigned int> &intList)
{
    char *value;
    size_t valueLen;

    if (!(find(name, value, valueLen)))
        return false;

    value[valueLen] = 0;

    std::istringstream is(value);
    while (is) {
        unsigned int i;
        char comma;

        is >> i >> comma;
        intList.push_back(i);
    }

    return true;
}
