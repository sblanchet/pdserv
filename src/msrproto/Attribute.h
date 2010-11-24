/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef MSRATTRIBUTE_H
#define MSRATTRIBUTE_H

#include <map>
#include <list>
#include <string>

namespace MsrProto {

        class Attr {
            public:
                void clear();

                void insert(const char *name);
                void insert(const char *name, char *attr);
                void adjust(ptrdiff_t delta);

                bool find(const char *name, char * &value) const;
                bool isEqual(const char *name, const char *s) const;
                bool isTrue(const char *name) const;
                bool getString(const char *name, std::string &s) const;
                bool getUnsigned(const char *name, unsigned int &i) const;
                bool getUnsignedList(const char *name,
                        std::list<unsigned int> &i) const;

                const std::string *id;

            private:
                std::string _id;
                
                struct AttrPtrs {
                    const char *name;
                    char *value;
                };

                typedef std::multimap<size_t,AttrPtrs> AttrMap;
                AttrMap attrMap;
        };
}
#endif //MSRATTRIBUTE_H
