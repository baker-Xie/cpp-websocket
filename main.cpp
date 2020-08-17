#include <iostream>
#include "cpp_websocket.hpp"

int main()
{
    ws::server srv;
    srv.on_connect = [&srv](ws::Session *caller, string end_point)
    {
        srv.SendBinaryMessage();
    };

    srv.on_disconnect = [&srv](ws::Session *caller, string end_point)
    {
        srv.SendBinaryMessage();
    };

    srv.on_message = [&srv](ws::Session *caller, string end_point, ws::PTR_buffer data, bool is_text)
    {
        srv.SendBinaryMessage();
    };
    srv.listen_block("127.0.0.1", 8001);
}