#ifndef XMLDOC_H
#define XMLDOC_H

#include <sstream>
#include <ostream>
#include <string>
#include <map>
#include <list>
#include <ctime>

namespace HRTLab {
    class Variable;
}

namespace MsrXml {

class Element {
    public:
        Element(const char *name, size_t indent = 0);
        ~Element();

        Element* createChild(const char *name);
        void setAttribute(const char *name, const char *value, size_t n = 0);
        void setAttribute(const char *name, const struct timespec&);
        void setAttribute(const char *name, const std::string& value);
        template<class T>
            void setAttribute(const char *name, const T& value);

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

template <class T>
void Element::setAttribute(const char *name, const T& value)
{
    std::ostringstream o;
    o << value;
    attr[name] = o.str();
}

}

#endif // XMLDOC_H
