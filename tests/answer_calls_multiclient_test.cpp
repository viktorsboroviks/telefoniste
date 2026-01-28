#include <cassert>
#include <string>
#include <thread>
#include <vector>
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

    const std::string socket_path = "/tmp/telefoniste_answer_calls_multi.sock";

    Socket server_sock(socket_path);
    Server server;
    server.answer_calls(server_sock,
                        [](const std::string& msg) { return msg; },
                        true);

    wait_for_socket(socket_path);

    const int clients = 8;
    std::vector<std::thread> threads;
    threads.reserve(clients);

    for (int i = 0; i < clients; ++i) {
        threads.emplace_back([&, i] {
            Socket client_sock(socket_path);
            const std::string payload = "client-" + std::to_string(i);
            const std::string resp = Client::call(client_sock, payload);
            assert(resp == payload);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    server.stop();
    return 0;
}
