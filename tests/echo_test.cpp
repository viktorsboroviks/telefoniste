#include <cassert>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "telefoniste.hpp"

int main()
{
    using namespace telefoniste;

    const std::string socket_path = "/tmp/telefoniste_test.sock";

    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;

    std::thread server([&] {
        Socket server_sock(socket_path);
        server_sock.open_server();

        {
            std::lock_guard<std::mutex> lock(mutex);
            ready = true;
        }
        cv.notify_one();

        const int client_fd = ::accept(server_sock.fd, nullptr, nullptr);
        if (client_fd == -1)
            std::perror("accept");
        assert(client_fd != -1);

        const std::string msg = Socket::receive_msg(client_fd);
        Socket::send_msg(client_fd, msg);
        ::close(client_fd);

        server_sock.close();
    });

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return ready; });
    }

    Socket client_sock(socket_path);
    const std::string payload = "echo-test";
    const std::string resp = call(client_sock, payload);
    assert(resp == payload);

    server.join();
    return 0;
}
