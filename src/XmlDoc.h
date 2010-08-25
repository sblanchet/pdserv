#ifndef XMLDOC_H
#define XMLDOC_H

#include <ostream>
#include <string>
#include <map>
#include <list>

namespace MsrXml {

class Element {
    public:
        Element(const char *name, size_t indent = 0);
        ~Element();

        Element* createChild(const char *name);
        void setAttribute(const char *name, const char *value, size_t n = 0);

        friend std::ostream& operator<<(std::ostream& os, const Element& el);

    private:
        const std::string name;
        const size_t indent;

        typedef std::map<const std::string, std::string> Attribute;
        Attribute attr;
        typedef std::list<Element*> Children;
        Children children;
};

std::ostream& operator<<(std::ostream& os, const Element& el);

}

#endif // XMLDOC_H
