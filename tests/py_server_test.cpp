#include <atomic>
#include <cassert>
#include <string>
#include <unistd.h>

#include "telefoniste.hpp"

int main()
{
    using namespace telefoniste;

    const std::string socket_path = "/tmp/telefoniste_py_test.sock";

    std::atomic<bool> served{false};

    Socket server_sock(socket_path);
    Server server;

    server.answer_calls(
            server_sock,
            [&served](const std::string& msg) {
                served.store(true);
                return msg;
            },
            false);

    // wait for socket file to appear
    for (int i = 0; i < 200; ++i) {
        if (::access(socket_path.c_str(), F_OK) == 0)
            break;
        ::usleep(1000 * 5); // 5ms
    }

    // wait until a client is served
    for (int i = 0; i < 200; ++i) {
        if (served.load())
            break;
        ::usleep(1000 * 5); // 5ms
    }

    server.stop();
    return 0;
}
