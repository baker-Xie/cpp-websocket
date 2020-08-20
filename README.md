## Dependency
- boost 1.73 (build from source)
- jsoncpp (sudo apt install libjsoncpp-dev)

## features
- async
- easy to use API interface
- inline print log plugin
- inline network speed plugin
- user-define plugin support

## usage
```
#include "cpp_websocket.hpp"
int main()
{
    ws::server srv;
    srv.on_connect = [&](ws::Session *caller, string end_point)
    {

    };
    srv.on_disconnect = [&](ws::Session *caller, string end_point)
    {

    };

    srv.on_message = [&](ws::Session *caller, string end_point, ws::PTR_buffer data, bool is_text)
    {

    };
    srv.listen_block("127.0.0.1", 8001);
}
```