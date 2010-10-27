/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRATTRIBUTE_H
#define MSRATTRIBUTE_H

#include <cstddef>
#include <map>
#include <list>
#include <string>

namespace MsrProto {

        class Attr {
            public:
                void clear();
                void insert(const char *name, size_t nameLen);
                void insert(const char *name, size_t nameLen,
                        char *attr, size_t attrLen);
                bool find(const char *name,
                        char * &value, size_t &valueLen);
                bool isEqual(const char *name, const char *s);
                bool isTrue(const char *name);
                bool getString(const char *name, std::string &s);
                bool getUnsigned(const char *name, unsigned int &i);
                bool getUnsignedList(const char *name,
                        std::list<unsigned int> &i);

                const std::string *id;

            private:
                std::string _id;
                
                struct AttrPtrs {
                    const char *name;
                    size_t nameLen;
                    char *value;
                    size_t valueLen;
                };

                typedef std::multimap<size_t,AttrPtrs> AttrMap;
                AttrMap attrMap;
        };
}
#endif //MSRATTRIBUTE_H
