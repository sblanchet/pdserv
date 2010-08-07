#ifndef MAIN_H
#define MAIN_H

namespace RTLab {

class Main {
    public:
        Main(int argc, const char *argv[], int nst);
        ~Main();

        void update(int st);

    private:
        const int nst;
};

}
#endif // MAIN_H
