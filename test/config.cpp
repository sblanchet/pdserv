#include <iostream>
#include "../src/Config.h"

using namespace std;

int main(int argc, char **argv)
{
    PdServ::Config conf;

    if (argc > 1) {
        const char *err = conf.load(argv[1]);
        if (err) {
            cout << err << endl;
            return 1;
        }
    }

    PdServ::Config c(conf["lan"]);
    cout << (int)conf["ln"]["msr"]["port"] << endl;


    return 0;
}
