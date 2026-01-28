#include <cassert>
#include <string>
#include <thread>

#include <unistd.h>

#include "telefoniste.hpp"

int main()
{
    using namespace telefoniste;

    const std::string socket_path = "/tmp/telefoniste_answer_calls.sock";

    Socket server_sock(socket_path);
    Telefoniste server;

    server.answer_calls(server_sock, [](const std::string& msg) { return msg; });

    // wait for socket file to appear
    for (int i = 0; i < 100; ++i) {
        if (::access(socket_path.c_str(), F_OK) == 0)
            break;
        ::usleep(1000 * 10); // 10ms
    }

    Socket client_sock(socket_path);
    const std::string payload = "answer-calls-test";
    const std::string resp = call(client_sock, payload);
    assert(resp == payload);

    server.stop();
    return 0;
}
