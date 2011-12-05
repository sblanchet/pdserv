/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv package.
 *
 *  pdserv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pdserv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pdserv. See COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <map>
#include <vector>

namespace PdServ {
    class Variable;
    class Signal;
    class Parameter;
}

namespace MsrProto {

class Variable;
class Channel;
class Parameter;

/////////////////////////////////////////////////////////////////////////////
class DirectoryNode {
    public:
        DirectoryNode(DirectoryNode *parent,
                const std::string& name);
//        ~DirectoryNode();

        std::string path() const;

        DirectoryNode * const parent;
        const std::string name;

        const Channel *findChannel(const std::string &path) const;
        const Parameter *findParameter(const std::string &path) const;

        void insert(const Variable *v);
        DirectoryNode *create(size_t index, size_t nelem,
                size_t ndims, const size_t *dim);

    protected:
        DirectoryNode *getRoot();

        const Variable* getVariable() const;


        DirectoryNode();

        const Variable *variable;

        class Path {
            public:
                Path(const std::string& s);

                DirectoryNode* create(DirectoryNode *, char& hide);

                const Parameter *findParameter(const DirectoryNode *);
                const Channel *findChannel(const DirectoryNode *);

            private:
                const std::string &path;
                size_t offset;

                bool getDir(std::string&);
                bool getDir(std::string&, char &hide);

                const Variable *findVariable(const DirectoryNode *);
        };

    private:
        typedef std::map<std::string, DirectoryNode*> Entry;
        Entry entry;
};

/////////////////////////////////////////////////////////////////////////////
class VariableDirectory: public DirectoryNode {
    public:
        VariableDirectory();
        ~VariableDirectory();

        bool insert(const PdServ::Signal *s, bool traditional);
        bool insert(const PdServ::Parameter *p, bool traditional);

       typedef std::vector<const Channel*> Channels;
       typedef std::vector<const Parameter*> Parameters;

       const Channels& getChannels() const;
       const Channel * getChannel(unsigned int) const;

       const Parameters& getParameters() const;
       const Parameter * getParameter(unsigned int) const;
       const Parameter * getParameter(const PdServ::Parameter *p) const;

    private:
       Channels channels;
       Parameters parameters;

       typedef std::map<const PdServ::Parameter *, const Parameter *>
           ParameterMap;
       ParameterMap parameterMap;
};

}

#endif  // DIRECTORY_H
