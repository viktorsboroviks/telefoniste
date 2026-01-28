#include <cassert>
#include <string>
#include <unistd.h>

#include "telefoniste.hpp"

static void wait_for_socket(const std::string& path)
{
    for (int i = 0; i < 200; ++i) {
        if (::access(path.c_str(), F_OK) == 0)
            return;
        ::usleep(1000 * 5); // 5ms
    }
    assert(false && "socket did not appear");
}

int main()
{
    using namespace telefoniste;

    const std::string socket_path = "/tmp/telefoniste_answer_calls_large.sock";

    Socket server_sock(socket_path);
    Server server;
    server.answer_calls(server_sock,
                        [](const std::string& msg) { return msg; },
                        true);

    wait_for_socket(socket_path);

    Socket client_sock(socket_path);
    const std::string payload(1 << 20, 'x');
    const std::string resp = Client::call(client_sock, payload);
    assert(resp.size() == payload.size());
    assert(resp == payload);

    server.stop();
    return 0;
}
