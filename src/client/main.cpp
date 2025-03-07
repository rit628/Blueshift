#include "libCE/Client.hpp"
#include <climits>
#include <string>
#include <unistd.h>

int main() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX+1);
    auto client = Client(hostname);
    client.start();
}