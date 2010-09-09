/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>

namespace HRTLab {

class Main;

class Session {
    public:
        Session(Main *main);
        virtual ~Session();

        virtual std::string getName() const;
        virtual std::string getClientName() const;
        virtual size_t getCountIn() const;
        virtual size_t getCountOut() const;
        virtual struct timespec getLoginTime() const;

    protected:
        Main * const main;
};

}
#endif //MSRSESSION_H
