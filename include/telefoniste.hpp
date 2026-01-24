#pragma once
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>

class Telefoniste {
public:
    using callback_t = std::function<std::string(const std::string& msg)>;
    inline static const std::string DEFAULT_SOCKET_PATH =
            "/tmp/telefoniste.sock";
    inline static const int MAX_CONNECTIONS = 16;

    explicit Telefoniste(
            callback_t cb,
            const std::string& socket_path = DEFAULT_SOCKET_PATH) :
        _fd(_open_socket(socket_path)),
        _socket_path(socket_path),
        _cb(cb)
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
        // TODO: for every new message open a dedicated thread
        //        while (true) {
        //            std::string msg = _receive_msg();
        //            std::string resp = _cb(msg);
        //            _send_msg(resp);
        //        }
    }

private:
    int _fd;
    const std::string _socket_path;
    callback_t _cb;

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

    static const std::string _receive_msg()
    {
        // accept connection, read message, close connection
        return "";
    }

    static void _send_msg(const std::string& msg)
    {
        // write message to fd
    }
};
