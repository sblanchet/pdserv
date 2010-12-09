/*****************************************************************************
 * $Id$
 *****************************************************************************/

#ifndef TASK_H
#define TASK_H

namespace HRTLab {

class Main;
class Session;
class Signal;

class Task {
    public:
        Task(Main *main, unsigned int tid, double sampleTime);
        ~Task();

        Main * const main;

        const unsigned int tid;
        const double sampleTime;

    private:
};

}
#endif // TASK_H
