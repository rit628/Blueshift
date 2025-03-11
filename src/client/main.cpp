#include "libCE/Client.hpp"
#include <climits>
#include <string>
#include <unistd.h>

int main(int argc, char* argv[]) {
    std::string name;
    if (argc == 2) {
        name = argv[1];
    }
    else {        
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX+1);
        name = hostname;
    }
    auto client = Client(name);
    client.start();
}