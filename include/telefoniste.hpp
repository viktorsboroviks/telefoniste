#pragma once
#include <functional>
#include <string>

class Telefoniste {
public:
    using callback_t = std::function<std::string(const std::string& msg)>;
    const std::string DEFAULT_SOCKET_PATH = "/tmp/telefoniste.sock";

    Telefoniste(callback_t cb,
                const std::string& socket_path = DEFAULT_SOCKET_PATH) :
        _fd(_open_socket(socket_path)),
        _cb(cb)
    {
        // do nothing
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

    ~Telefoniste()
    {
        //        ::close(_server_fd);
    }

private:
    int _fd;
    callback_t _cb;

    static int _open_socket(const std::string& path)
    {
        //        ::unlink(path);
        //        int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        //        sockaddr_un addr{};
        //        addr.sun_family = AF_UNIX;
        //        std::strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);
        //        ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        //        ::listen(fd, 16);
        //        return fd;
        return 0;
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
