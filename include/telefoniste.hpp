#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

namespace telefoniste {

inline static const std::string DEFAULT_SOCKET_PATH = "/tmp/telefoniste.sock";
inline static const int MAX_CONNECTIONS             = 16;
using callback_t = std::function<std::string(const std::string& msg)>;

class Socket {
public:
    int fd = -1;
    std::string path;
    bool is_open     = false;
    bool rm_on_close = false;

    explicit Socket(const std::string& path = DEFAULT_SOCKET_PATH) :
        path(path)
    {
    }

    // disable copying/assignment to prevent double-closing the fd
    Socket(const Socket&)            = delete;
    Socket& operator=(const Socket&) = delete;

    ~Socket()
    {
        close();
    }

    void close()
    {
        if (!is_open)
            return;
        if (fd != -1) {
            ::close(fd);
        }
        fd      = -1;
        is_open = false;
        if (!rm_on_close) {
            return;
        }
        if (!path.empty()) {
            ::unlink(path.c_str());
        }
        rm_on_close = false;
    }

    void open_server()
    {
        assert(!is_open);
        _open(true);

        // bind = associate fd with path
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        const int bind_retval =
                ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (bind_retval != 0)
            std::perror("bind");
        assert(bind_retval == 0);

        // listen for connections
        const int listen_retval = ::listen(fd, MAX_CONNECTIONS);
        if (listen_retval != 0)
            std::perror("listen");
        assert(listen_retval == 0);

        rm_on_close = true;
    }

    void open_client()
    {
        assert(!is_open);
        _open();

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        const int retval_connect = ::connect(
                fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (retval_connect == -1)
            std::perror("connect");
        assert(retval_connect == 0);
    }

    static void readn(int src_fd, void* const p_dst, size_t n)
    {
        size_t total = 0;
        while (total < n) {
            const ssize_t count = ::read(
                    src_fd, static_cast<char*>(p_dst) + total, n - total);
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

    void readn(void* const p_dst, size_t n) const
    {
        readn(fd, p_dst, n);
    }

    static void writen(int dest_fd, const void* const p_src, size_t n)
    {
        size_t total = 0;
        while (total < n) {
            const ssize_t count =
                    ::write(dest_fd,
                            static_cast<const char*>(p_src) + total,
                            n - total);
            if (count == -1) {
                std::perror("write");
                assert(false);
            }
            total += count;
        }
    }

    void writen(const void* const p_src, size_t n) const
    {
        writen(fd, p_src, n);
    }

    static std::string receive_msg(int client_fd)
    {
        // read mesage length from header in network byte order
        uint32_t msg_nlen      = 0;
        char* const p_msg_nlen = reinterpret_cast<char* const>(&msg_nlen);
        readn(client_fd, p_msg_nlen, sizeof(msg_nlen));

        // convert length from network byte order to host byte order
        // ntohl() - network to host long (big-endian)
        uint32_t msg_len = ntohl(msg_nlen);

        // read message body
        std::string msg(msg_len, '\0');
        readn(client_fd, &msg[0], msg_len);
        return msg;
    }

    std::string receive_msg() const
    {
        return receive_msg(fd);
    }

    static void send_msg(int client_fd, const std::string& msg)
    {
        // write message length in header in network byte order
        uint32_t msg_len = static_cast<uint32_t>(msg.size());

        // htonl() - host to network long (big-endian, 4 bytes)
        uint32_t msg_nlen     = htonl(msg_len);
        const char* p_msg_len = reinterpret_cast<const char*>(&msg_nlen);
        writen(client_fd, p_msg_len, sizeof(msg_nlen));

        // write message body
        writen(client_fd, msg.data(), msg_len);
    }

    void send_msg(const std::string& msg) const
    {
        send_msg(fd, msg);
    }

private:
    void _open(bool replace_file_if_exist = false)
    {
        if (is_open)
            return;

        // initial checks
        assert(!path.empty());
        // avoid silent truncation, sun_path size varies by os
        // (typically 104 on mac, 108 on linux)
        assert(path.length() < sizeof(sockaddr_un::sun_path));

        // remove socket file if it exists, this resolves
        // "address already in use" errors if the program crashed previously
        // and the socket file was not removed
        if (replace_file_if_exist)
            ::unlink(path.c_str());

        // create fd endpoint
        fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd == -1)
            std::perror("socket");
        assert(fd != -1);

        is_open = true;
    }
};

class Server {
public:
    ~Server()
    {
        stop();
    }

    void answer_calls(Socket& s,
                      callback_t answer_cb,
                      bool answer_in_new_thread = true)
    {
        if (s.is_open)
            perror("socket already open");
        assert(!s.is_open);

        if (_thread.joinable())
            perror("telefoniste already answering calls");
        assert(!_thread.joinable());

        _stop.store(false);
        p_socket = &s;

        _thread = std::thread([this, answer_cb, answer_in_new_thread] {
            p_socket->open_server();
            const int server_fd = p_socket->fd;

            while (!_stop.load()) {
                int client_fd = ::accept(server_fd, nullptr, nullptr);
                if (client_fd == -1) {
                    // if interrupted by stop request do not print to stderr
                    if (_stop.load()) {
                        break;
                    }
                    std::perror("accept");
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (answer_in_new_thread) {
                    // no concurrency around _answer, _receive, _send
                    // as each client owns its own resources and gets own
                    // client_fd
                    std::thread([answer_cb, client_fd] {
                        _answer(client_fd, answer_cb);
                    }).detach();
                }
                else {
                    _answer(client_fd, answer_cb);
                }
            }
        });
    }

    void stop()
    {
        _stop.store(true);
        // close server fd to unblock accept()
        if (p_socket != nullptr) {
            p_socket->close();
        }
        if (_thread.joinable()) {
            _thread.join();
        }
        p_socket = nullptr;
    }

private:
    Socket* p_socket = nullptr;
    std::thread _thread;
    std::atomic<bool> _stop{false};

    static void _answer(int client_fd, callback_t answer_cb)
    {
        std::string msg  = Socket::receive_msg(client_fd);
        std::string resp = answer_cb(msg);
        Socket::send_msg(client_fd, resp);
        ::close(client_fd);
    }
};

class Client {
public:
    static std::string call(Socket& s, const std::string& msg)
    {
        s.open_client();
        s.send_msg(msg);
        std::string resp = s.receive_msg();
        s.close();
        return resp;
    }
};

};  // namespace telefoniste
