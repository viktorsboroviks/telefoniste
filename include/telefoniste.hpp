#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

class Telefoniste {
public:
    using callback_t = std::function<std::string(const std::string& msg)>;
    inline static const std::string DEFAULT_SOCKET_PATH =
            "/tmp/telefoniste.sock";
    inline static const int MAX_CONNECTIONS = 16;

    explicit Telefoniste(
            callback_t cb,
            bool new_thread_per_cb         = true,
            const std::string& socket_path = DEFAULT_SOCKET_PATH) :
        _fd(_open_socket(socket_path)),
        _socket_path(socket_path),
        _cb(cb),
        _new_thread_per_cb(new_thread_per_cb)
    {
    }

    // disable copying/assignment to prevent double-closing the fd
    Telefoniste(const Telefoniste&)            = delete;
    Telefoniste& operator=(const Telefoniste&) = delete;

    ~Telefoniste()
    {
        ::close(_fd);
        ::unlink(_socket_path.c_str());
    }

    void run()
    {
        // TODO: review
        while (true) {
            int client_fd = ::accept(_fd, nullptr, nullptr);
            if (client_fd == -1)
                std::perror("accept");
            assert(client_fd != -1);

            if (_new_thread_per_cb) {
                std::thread(&Telefoniste::_handle_client, this, client_fd)
                        .detach();
            }
            else {
                _handle_client(client_fd);
            }
        }
    }

private:
    const int _fd;
    const std::string _socket_path;
    const callback_t _cb;
    const bool _new_thread_per_cb;

    static int _open_socket(const std::string& path)
    {
        // initial checks
        assert(!path.empty());
        // avoid silent truncation, sun_path size varies by os
        // (typically 104 on mac, 108 on linux)
        assert(path.length() < sizeof(sockaddr_un::sun_path));

        // remove socket file if it exists, this resolves
        // "address already in use" errors if the program crashed previously
        // and the socket file was not removed
        ::unlink(path.c_str());

        // create fd endpoint
        int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd == -1)
            std::perror("socket");
        assert(fd != -1);

        // bind = associate fd with path
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        int bind_retval =
                ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (bind_retval != 0)
            std::perror("bind");
        assert(bind_retval == 0);

        // listen for connections
        int listen_retval = ::listen(fd, MAX_CONNECTIONS);
        if (listen_retval != 0)
            std::perror("listen");
        assert(listen_retval == 0);
        return fd;
    }

    // TODO: review if needed
    void _handle_client(int client_fd)
    {
        std::string msg  = _receive(client_fd);
        std::string resp = _cb(msg);
        _send(client_fd, resp);
        ::close(client_fd);
    }

    // TODO: review
    static std::string _receive(int fd)
    {
        uint32_t net_len = 0;
        size_t total     = 0;
        char* header     = reinterpret_cast<char*>(&net_len);

        while (total < sizeof(net_len)) {
            ssize_t count =
                    ::read(fd, header + total, sizeof(net_len) - total);
            if (count == -1) {
                std::perror("read header");
                assert(count != -1);
            }
            if (count == 0) {
                std::fprintf(stderr, "read header: unexpected eof\n");
                assert(count != 0);
            }
            total += count;
        }

        // ntohl() - network to host long (big-endian, 4 bytes)
        uint32_t len = ntohl(net_len);

        std::string msg(len, '\0');
        total = 0;
        while (total < len) {
            ssize_t count = ::read(fd, &msg[total], len - total);
            if (count == -1) {
                std::perror("read body");
                assert(count != -1);
            }
            if (count == 0) {
                std::fprintf(stderr, "read body: unexpected eof\n");
                assert(count != 0);
            }
            total += count;
        }
        return msg;
    }

    // TODO: review
    static void _send(int fd, const std::string& msg)
    {
        uint32_t len = static_cast<uint32_t>(msg.size());
        assert(msg.size() == len && "msg too big");

        // htonl() - host to network long (big-endian, 4 bytes)
        uint32_t net_len   = htonl(len);
        const char* header = reinterpret_cast<const char*>(&net_len);
        size_t total       = 0;
        while (total < sizeof(net_len)) {
            ssize_t written =
                    ::write(fd, header + total, sizeof(net_len) - total);
            if (written == -1) {
                std::perror("write header");
                assert(written != -1);
            }
            total += written;
        }

        total = 0;
        while (total < len) {
            ssize_t written = ::write(fd, msg.data() + total, len - total);
            if (written == -1) {
                std::perror("write body");
                assert(written != -1);
            }
            total += written;
        }
    }
};
