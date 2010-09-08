/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

namespace HRTLab {

class Main;

class Session {
    public:
        Session(Main *main);
        ~Session();

    protected:
        Main * const main;
};

}
#endif //MSRSESSION_H
