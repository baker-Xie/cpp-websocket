//
// Created by u on 20-8-15.
//

#ifndef CPP_WEBSOCKET_CPP_WEBSOCKET_HPP
#define CPP_WEBSOCKET_CPP_WEBSOCKET_HPP

// std library
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>

// boost library
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using std::vector;
using std::string;
using std::map;
using std::shared_ptr;


class listener;
class Session;
class BasePlugin;
class PluginManager;

enum class pluginType
{
    Session_Plugin = 0,
    Shared_Plugin = 1,
};

typedef shared_ptr<beast::flat_buffer> PTR_flat_buffer;

/** Abstract class to define the interface of plugins
 * You must override all pure virtual function
 * Meanwhile, you can use template to represent owner class and caller class.
 * See example in
 * 1. WebsocketServer/modules/SessionRemover.h
 * 2. WebsocketServer/plugin/Task/EchoTextMessage.h
 */
class BasePlugin
{
public:
    /** Constructions
     * @param type The pluginType
     * @param owner The object's pointer of owner
     */
    BasePlugin(pluginType type, void *owner) : _type(type), _owner(owner)
    {

    }

    virtual ~BasePlugin()
    {

    }

    /** onInit will be called during the construction method of the caller object. You can do some initialization
     * @param caller A pointer to represent who called this function. When use this pointer, you should convert to its actual pointer.
     * For example, if onInit is called by a Session class, then you should convert this pointer to (Session*)caller
     */
    virtual void onInit(void *caller) = 0;

    /** This function will be called when a connection between client and server established.
     * @param caller As above
     * @param endPoint_str The remote peer's IP:Port
     */
    virtual void onConnect(void *caller, string &endPoint_str) = 0;

    /** This function will be called when a connection between client and server closed.
     * @param caller As above
     * @param endPoint_str The remote peer's IP:Port
     */
    virtual void onClose(void *caller, string &endPoint_str) = 0;

    /** This function will be called when server finished a async_write operation.
     * @param caller As above
     * @param endPoint_str The remote peer's IP:Port
     * @bytes_transferred The size of sent bytes
     */
    virtual void onWrite(void *caller, string &endPoint_str, size_t bytes_transferred) = 0;

    /** This function will be called when server received text message from client
     * @param caller As above
     * @param endPoint_str The remote peer's IP:Port
     * @s The received string in text format
     */
    virtual void onReadText(void *caller, string &endPoint_str, const string &s) = 0;

    /** This function will be called when server received binary message from client
     * @param caller As above
     * @param endPoint_str The remote peer's IP:Port
     * @data The received data in binary format
     */
    virtual void onReadBinary(void *caller, string &endPoint_str, const PTR_flat_buffer data) = 0;

protected:
    pluginType _type;
    void *_owner; // if plugin is load in listener, owner is listener* , if plugin is load in session, owner is session*
};



/** The manager saves all plugins.
 * When call the following function, the manager will call the same function in all plugins
 */
class PluginManager
{
    std::vector<std::shared_ptr<BasePlugin>> _plugins;
public:
    // variable length template to pass argument
    template<typename ...Args>
    void onInit(Args &&...args)
    {
        for (auto &&plugin:_plugins)
        {
            plugin->onInit(std::forward<Args &&>(args)...);
        }
    }

    template<typename ...Args>
    void onConnect(Args &&...args)
    {
        for (auto &&plugin:_plugins)
        {
            plugin->onConnect(std::forward<Args &&>(args)...);
        }
    }

    template<typename ...Args>
    void onClose(Args &&...args)
    {
        for (auto &&plugin:_plugins)
        {
            plugin->onClose(std::forward<Args &&>(args)...);
        }
    }

    template<typename ...Args>
    void onWrite(Args &&...args)
    {
        for (auto &&plugin:_plugins)
        {
            plugin->onWrite(std::forward<Args &&>(args)...);
        }
    }

    template<typename ...Args>
    void onReadText(Args &&...args)
    {
        for (auto &&plugin:_plugins)
        {
            plugin->onReadText(std::forward<Args &&>(args)...);
        }
    }

    template<typename ...Args>
    void onReadBinary(Args &&...args)
    {
        for (auto &&plugin:_plugins)
        {
            plugin->onReadBinary(std::forward<Args &&>(args)...);
        }
    }

    void AddPlugin(std::shared_ptr<BasePlugin> &plugin)
    {
        _plugins.emplace_back(plugin);
    }

    void AddPlugin(std::shared_ptr<BasePlugin> &&plugin)
    {
        _plugins.emplace_back(std::move(plugin));
    }

    void ClearPlugin()
    {
        _plugins.clear();
    }
};


// Accepts incoming connections and launches new sessions
class listener
{
private:
    asio::io_context &_ioc;                                  // ioc object referenced from main function
    tcp::acceptor _acceptor;                                // tcp acceptor
    map<const string, shared_ptr<Session>> _session_Set;     // a map to save all active sessions
    LogPrinter _logPrinter;

public:
    PluginManager pluginManager;                            // plugin manager which saves all plugins. All plugins in listener class will have effect in all sessions

public:
    listener(asio::io_context &ioc)
            : _ioc(ioc), _acceptor(ioc)
    {
        // add plugins
        // be careful, the SessionRemover should be the last plugin. Because these plugin will be called in sequence
        pluginManager.AddPlugin(shared_ptr<SessionRemover < listener, Session>>
        (new SessionRemover<listener, Session>(this)));
    }

    // Start accepting incoming connections
    bool listen(tcp::endpoint endpoint)
    {
        beast::error_code ec;

        // Open the acceptor
        _acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            _logPrinter.PrintError("open:" + ec.message());
            return false;
        }

        // Allow address reuse
        _acceptor.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            _logPrinter.PrintError("set_option:" + ec.message());
            return false;
        }

        // Bind to the server address
        _acceptor.bind(endpoint, ec);
        if (ec)
        {
            _logPrinter.PrintError("bind:" + ec.message());
            return false;
        }

        // Start listening for connections
        _acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            _logPrinter.PrintError("listen:" + ec.message());
            return false;
        }
        return true;
    }
    // run server


    void run()
    {
        do_accept();
    }


    // insert a session to session map when a session is established(connected)
    void addSession(const string &endPoint, shared_ptr<Session> &sp)
    {
        _session_Set.emplace(endPoint, sp);
    }

    // delete a session from session map when a session is closed. This function will be called by SessionRemover module
    void deleteSession(const string &endPoint)
    {
        _session_Set.erase(endPoint);
    }

    // return session map
    const map<const string, shared_ptr<Session>> &GetSessionList()
    {
        return _session_Set;
    }

private:
    // do the accept operation
    void do_accept()
    {
        // The new connection gets its own strand
        _acceptor.async_accept(asio::make_strand(_ioc), beast::bind_front_handler(&listener::on_accept, this));
    }

    // The callback function of async_accept. When accept operation finished, what to do
    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            _logPrinter.PrintError("accept:" + ec.message());
        }
        else
        {
            // Create the session and run it
            string endPoint = socket.remote_endpoint().address().to_string() + ":" +
                              std::to_string(socket.remote_endpoint().port());
            auto sp = std::make_shared<Session>(std::move(socket), pluginManager);
            addSession(endPoint, sp);
            sp->run();
        }

        // Accept another connection
        do_accept();
    }
};


// Session class, each websocket connection have a session, and each session have a strand.
// So no need to use threads synchronization
class Session : public std::enable_shared_from_this<Session>
{
private:
    bool _isRunning;                                // session running flag
    const std::string _remote_address;              // client's IP address
    const unsigned short _remote_port;              // client's port
    std::string _endpoint_str;                      // client's IP + ":" + port
    beast::websocket::stream<beast::tcp_stream> _ws;       // websocket object

    // tmp buffer for read text message/binary data
    beast::flat_buffer _read_buffer;

    // tmp buffer for write text message/binary data
    PTR_buffer_seq _write_buffer;

    // modules
    WriteTaskManager _writeTaskManager;
    IOSpeedCounter _speedCounter;
    LogPrinter _logPrinter;

    // shared plugin manager(reference) and private plugin manager
    PluginManager &_shared_pluginManager;
    PluginManager _pluginManager;

public:
    /** Construction method
     * @param socket The socket object created in listener. This socket is used in listener to accept connections
     * @param Shared_pluginManager The shared plugin manager.
     */
    explicit Session(tcp::socket &&socket, PluginManager &shared_pluginManager) :
            _remote_address(socket.remote_endpoint().address().to_string()),
            _remote_port(socket.remote_endpoint().port()),
            _ws(std::move(socket)),
            _shared_pluginManager(shared_pluginManager)
    {
        _isRunning = false;
        _endpoint_str = _remote_address + ":" + std::to_string(_remote_port);
        _logPrinter.SetEndPoint(_endpoint_str);
        _speedCounter.SetStart();

        // add plugin
        //_pluginManager.AddPlugin(shared_ptr<EchoTextMessage<Session,Session>>(new EchoTextMessage<Session,Session>(this)));
        // call the plugins
        _pluginManager.onInit(this);
        _shared_pluginManager.onInit(this);
    }


    // Get on the correct executor
    void run()
    {
        // Avoid run twice
        if (_isRunning)
            return;
        _isRunning = true;

        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        // The asio::dispatch or asio::post method will put the handler function into message queue, thus avoid multi-thread race problem
        asio::dispatch(_ws.get_executor(), beast::bind_front_handler(&Session::on_run, shared_from_this()));
    }


    /** Send Text Messages to the client. The text is not sent immediately but this function will push a task into a queue, so it a non-block async method.
     * When the text is sent is undetermined.
     * @param text The text to be sent. Usually use json format
     */
    // This function will push a task into a queue, so it a non-block async method
    void SendTextMessage(const std::string &text)
    {
        asio::dispatch(_ws.get_executor(), [text, self = shared_from_this()]
                       {
                           self->_writeTaskManager.push_text(text);
                           self->do_write();
                       }
        );
    }


    /** Send Binary Messages to the client. The data is not sent immediately but this function will push a task into a queue, so it a non-block async method.
     * @param data The data to be sent.
     */
    // This function will push a task into a queue, so it a non-block async method
    // Send binary data represented by unsigned char array
    void SendBinaryMessage(const PTR_buffer data)
    {
        asio::dispatch(_ws.get_executor(), [data, self = shared_from_this()]
                       {
                           self->_writeTaskManager.push_binary(data);
                           // Are we already writing? We should wait this write operation finished(handler will be invoked)
                           if (self->_writeTaskManager.size() > 1)
                               return;
                           self->do_write();
                       }
        );
    }


    /** Send Binary Messages to the client. The data is not sent immediately but this function will push a task into a queue, so it a non-block async method.
     * @param data The data to be sent. PTR_buffer_seq is a shared_ptr type of vector<PTR_buffer>
     */
    // This function will push a task into a queue, so it a non-block async method
    // Send binary data represented by vector<unsigned char array>. All the unsigned char array in vector will be concatenated together, and send out
    void SendBinaryMessage(const PTR_buffer_seq data)
    {
        asio::dispatch(_ws.get_executor(), [data, self = shared_from_this()]
                       {
                           self->_writeTaskManager.push_binary(data);
                           // Are we already writing? We should wait this write operation finished(handler will be invoked)
                           if (self->_writeTaskManager.size() > 1)
                               return;
                           self->do_write();
                       }
        );
    }

private:
    /** Start the asynchronous operation
     */
    void on_run()
    {
        // Set suggested timeout settings for the websocket
        _ws.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        _ws.set_option(beast::websocket::stream_base::decorator(
                [](beast::websocket::response_type &res)
                {
                    res.set(beast::http::field::server,
                            std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-server-async");
                }));
        // Accept the websocket handshake
        _ws.async_accept(beast::bind_front_handler(&Session::on_accept, shared_from_this()));
    }


    /**
     * This is a callback function of async_accept, you should not call this function directly.
     * All the parameter in this function will be filled by another function, so you don't need to pass these parameters
     * @param ec When async_accept operation finished, the error code you receive
     */
    void on_accept(beast::error_code ec)
    {
        if (ec)
        {
            on_close(ec);
            return _logPrinter.PrintError("accept:" + ec.message());
        }

        _logPrinter.PrintLog("Connection established", true);

        // call the plugins
        _pluginManager.onConnect(this, _endpoint_str);
        _shared_pluginManager.onConnect(this, _endpoint_str);

        // Read a message
        do_read();
    }


    /** This is a callback function of async_close, you should not call this function directly.
     * All the parameter in this function will be filled by another function, so you don't need to pass these parameters
     * @param ec When operation finished, the error code you receive
     */
    void on_close(beast::error_code ec)
    {
        // call the plugins
        _pluginManager.onClose(this, _endpoint_str);
        _shared_pluginManager.onClose(this, _endpoint_str);
    }


    /** do the read operation
     */
    void do_read()
    {
        // Read a message into our buffer
        _read_buffer.consume(_read_buffer.size());
        _ws.async_read(_read_buffer, beast::bind_front_handler(&Session::on_read, shared_from_this()));
    }


    /** This is a callback function of async_receive, you should not call this function directly.
     * All the parameter in this function will be filled by another function, so you don't need to pass these parameters
     * @param ec When operation finished, the error code you receive
     * @param bytes_transferred When operation finished, how many bytes you received
     */
    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        // display the data transfer speed
        if (_speedCounter.SetData(bytes_transferred))
        {
            _logPrinter.PrintNetworkSpeed(_speedCounter.GetSpeed());
        }

        // This indicates that the session was closed
        if (ec == beast::websocket::error::closed)
        {
            _logPrinter.PrintLog("Connection gracefully closed", true);
            on_close(ec);
            return;
        }
        if (ec)
        {
            _logPrinter.PrintError("read:" + ec.message());
            if (ec.value() == 104 || ec.value() == 107 || ec.value() == 2)
            {
                on_close(ec);
                _logPrinter.PrintLog("Connection closed", true);
            }
            return;
        }

        // call the plugins
        if (_ws.got_text())
        {
            const string s((const char *) _read_buffer.data().data(), _read_buffer.size());
            _pluginManager.onReadText(this, _endpoint_str, s);
            _shared_pluginManager.onReadText(this, _endpoint_str, s);
        }
        else
        {
            shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>(std::move(_read_buffer));
            _shared_pluginManager.onReadBinary(this, _endpoint_str, buffer);
        }

        do_read();
    }


    /** do the write operation
     */
    void do_write()
    {
        // Are we already writing? We should wait this write operation finished(handler will be invoked)
        if (_writeTaskManager.size() > 1)
            return;

        // We are not currently writing, so we can get the following task to send immediately
        _ws.binary(_writeTaskManager.front().is_binary);
        auto raw_buffer_seq = _writeTaskManager.front().data;
        vector<asio::const_buffer> buffer_seq_view;
        for (auto &&d:*raw_buffer_seq)
        {
            buffer_seq_view.emplace_back(asio::buffer(d->data()));
        }
        _write_buffer = raw_buffer_seq;
        _ws.async_write(buffer_seq_view, beast::bind_front_handler(&Session::on_write, shared_from_this()));
    }

    /** This is a callback function of async_write, you should not call this function directly.
     * All the parameter in this function will be filled by another function, so you don't need to pass these parameters
     * @param ec When operation finished, the error code you receive
     * @param bytes_transferred When operation finished, how many bytes you sent
     */
    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            _logPrinter.PrintError("write:" + ec.message());
            on_close(ec);
            return;
        }

        // display the data transfer speed
        if (_speedCounter.SetData(bytes_transferred))
        {
            _logPrinter.PrintNetworkSpeed(_speedCounter.GetSpeed());
        }

        // Remove the last Task from the queue
        _writeTaskManager.pop();

        // call the plugins
        _pluginManager.onWrite(this, _endpoint_str, bytes_transferred);
        _shared_pluginManager.onWrite(this, _endpoint_str, bytes_transferred);

        // Send the next message if any
        if (!_writeTaskManager.empty())
        {
            _ws.binary(_writeTaskManager.front().is_binary);
            auto raw_buffer_seq = _writeTaskManager.front().data;
            vector<asio::const_buffer> buffer_seq_view;
            for (auto &&d:*raw_buffer_seq)
            {
                buffer_seq_view.emplace_back(asio::buffer(d->data()));
            }
            _write_buffer = raw_buffer_seq;
            _ws.async_write(buffer_seq_view, beast::bind_front_handler(&Session::on_write, shared_from_this()));
        }
    }
};


namespace ws
{
    class server
    {
        void listen(string endpoint)
        {

        }

        void close()
        {

        }

        void SenBinaryMessage()
        {

        }

        void SendTextMessage()
        {

        }

    private:
    };
}

#endif //CPP_WEBSOCKET_CPP_WEBSOCKET_HPP
