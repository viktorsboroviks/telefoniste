# telefoniste

Minimal IPC helper for simple server/client command exchange

- c++ header-only lib: server, client
- python module: client
- uses
  - UNIX sockets

## c++ usage

Server:

```cpp
#include "telefoniste.hpp"

int main() {
    using namespace telefoniste;
    Socket server_sock("/tmp/telefoniste.sock");
    Server server;
    server.answer_calls(server_sock, [](const std::string& msg) { return msg; });
    // ... later
    server.stop();
}
```

Client:

```cpp
#include "telefoniste.hpp"

int main() {
    using namespace telefoniste;
    Socket client_sock("/tmp/telefoniste.sock");
    std::string resp = Client::call(client_sock, "ping");
}
```

## python usage

```python
from telefoniste import Client, Socket

sock = Socket("/tmp/telefoniste.sock")
resp = Client.call(sock, b"ping")
```

## build and test

```bash
make test
```

## lint and format

```bash
make lint
make format
```
