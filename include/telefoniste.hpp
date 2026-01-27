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
        _server_fd(_open_socket(socket_path)),
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
        ::close(_server_fd);
        ::unlink(_socket_path.c_str());
    }

    void run()
    {
        while (true) {
            int client_fd = ::accept(_server_fd, nullptr, nullptr);
            if (client_fd == -1)
                std::perror("accept");
            assert(client_fd != -1);

            if (_new_thread_per_cb) {
                // no concurrency around _handle_client, _receive, _send
                // as each client owns its own resources and gets own client_fd
                std::thread(&Telefoniste::_handle_client, this, client_fd)
                        .detach();
            }
            else {
                _handle_client(client_fd);
            }
        }
    }

private:
    const int _server_fd;
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

    void _handle_client(int client_fd)
    {
        std::string msg  = _receive(client_fd);
        std::string resp = _cb(msg);
        _send(client_fd, resp);
        ::close(client_fd);
    }

    static void _readn(int src_fd, void* const p_dst, size_t n)
    {
        size_t total = 0;
        while (total < n) {
            const ssize_t count = ::read(src_fd, p_dst + total, n - total);
            switch (count) {
                case -1:
                    std::perror("read");
                    assert(false);
                    break;
                case 0:
                    std::fprintf(stderr, "read: unexpected eof\n");
                    assert(false);
                    break;
                default:
                    assert(count > 0);
                    break;
            }
            total += count;
        }
    }

    static void _writen(int dest_fd, const void* const p_src, size_t n)
    {
        size_t total = 0;
        while (total < n) {
            const ssize_t count = ::write(dest_fd, p_src + total, n - total);
            if (count == -1) {
                std::perror("write");
                assert(false);
            }
            total += count;
        }
    }

    static std::string _receive(int client_fd)
    {
        // read mesage length from header in network byte order
        uint32_t msg_nlen      = 0;
        char* const p_msg_nlen = reinterpret_cast<char* const>(&msg_nlen);
        _readn(client_fd, p_msg_nlen, sizeof(msg_nlen));

        // convert length from network byte order to host byte order
        // ntohl() - network to host long (big-endian)
        uint32_t msg_len = ntohl(msg_nlen);

        // read message body
        std::string msg(msg_len, '\0');
        _readn(client_fd, &msg[0], msg_len);
        return msg;
    }

    static void _send(int client_fd, const std::string& msg)
    {
        // write message length in header in network byte order
        uint32_t msg_len = static_cast<uint32_t>(msg.size());

        // htonl() - host to network long (big-endian, 4 bytes)
        uint32_t msg_nlen     = htonl(msg_len);
        char* const p_msg_len = reinterpret_cast<char* const>(&msg_nlen);
        _writen(client_fd, p_msg_len, sizeof(msg_nlen));

        // write message body
        _writen(client_fd, msg.data(), msg_len);
    }
};
