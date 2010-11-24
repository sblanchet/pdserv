/*****************************************************************************
 * $Id$
 *****************************************************************************/

#include "Attribute.h"

#include <cstring>
#include <algorithm>
#include <sstream>
#include <locale>

using namespace MsrProto;

/////////////////////////////////////////////////////////////////////////////
void Attr::clear()
{
    id = 0;
    _id.clear();
    attrMap.clear();
}

/////////////////////////////////////////////////////////////////////////////
void Attr::adjust(ptrdiff_t delta)
{
    for (AttrMap::iterator it = attrMap.begin(); it != attrMap.end(); it++) {
        it->second.name += delta;
        it->second.value += delta;
    }
}

/////////////////////////////////////////////////////////////////////////////
void Attr::insert(const char *name)
{
    //cout << "Binary attribute: Name=" << std::string(name, nameLen) << endl;
    AttrPtrs a = {name, 0};
    attrMap.insert(std::pair<size_t,AttrPtrs>(strlen(name), a));
}

/////////////////////////////////////////////////////////////////////////////
void Attr::insert(const char *name, char *value)
{
    //cout << "Value attribute: Name=" << std::string(name, nameLen)
        //<< ", Value=" << std::string(value, valueLen)
        //<< endl;
    size_t len = strlen(name);

    if (len == 2 and !strncmp(name, "id", 2)) {
        _id.assign(value);
        id = &_id;
        return;
    }

    AttrPtrs a = {name, value};
    attrMap.insert(std::pair<size_t,AttrPtrs>(len, a));
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::find(const char *name, char * &value) const
{
    size_t len = strlen(name);
    std::pair<AttrMap::const_iterator, AttrMap::const_iterator>
        ret(attrMap.equal_range(len));

    for (AttrMap::const_iterator it(ret.first); it != ret.second; it++) {
        if (!strncasecmp(name, it->second.name, len)) {
            value = it->second.value;
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::isEqual(const char *name, const char *s) const
{
    char *value;

    if (find(name, value))
        return !strcasecmp(value, s);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::isTrue(const char *name) const
{
    char *value;

    if (!(find(name, value)))
        return false;

    size_t len = strlen(value);

    if (len == 1)
        return *value == '1';

    if (len == 4)
        return !strncasecmp(value, "true", 4);

    if (len == 2)
        return !strncasecmp(value, "on", 2);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::getString(const char *name, std::string &s) const
{
    char *value;

    s.clear();

    if (!(find(name, value)))
        return false;

    char *pptr, *eptr = value + strlen(value);
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
bool Attr::getUnsigned(const char *name, unsigned int &i) const
{
    char *value;

    if (!(find(name, value)))
        return false;

    i = strtoul(value, 0, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool Attr::getUnsignedList(const char *name,
        std::list<unsigned int> &intList) const
{
    char *value;

    if (!(find(name, value)))
        return false;

    std::istringstream is(value);
    is.imbue(std::locale::classic());

    while (is) {
        unsigned int i;
        char comma;

        is >> i >> comma;
        intList.push_back(i);
    }

    return true;
}
