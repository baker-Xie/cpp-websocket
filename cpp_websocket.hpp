//
// Created by u on 20-8-15.
//

/** This is a websocket class using boost::beast and jsoncpp
example:
int main()
{
    ws::server srv;
    Demo demo;
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
 */

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
#include <queue>
#include <thread>
#include <functional>
#include <iomanip>
#include <sys/time.h>

// boost library
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
//#include <boost/beast/ssl.hpp>

// JsonCPP library
#include <jsoncpp/json/json.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using std::vector;
using std::queue;
using std::string;
using std::map;
using std::shared_ptr;
using std::thread;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using namespace std::placeholders;

namespace ws
{
    // safe for flat_buffer
    typedef shared_ptr<beast::flat_buffer> PTR_buffer;
    // safe for vector
    typedef shared_ptr<vector<PTR_buffer>> PTR_buffer_seq;


    enum class pluginType
    {
        Session_Plugin = 0,
        Shared_Plugin = 1,
    };


    /** Abstract class to define the interface of plugins
     * You must override the function you want
     * Meanwhile, you can use template to represent owner class and caller class.
     * See example in
     * class SessionRemover
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
        virtual void onInit(void *caller)
        {

        }

        /** This function will be called when a connection between client and server established.
         * @param caller As above
         * @param endPoint_str The remote peer's IP:Port
         */
        virtual void onConnect(void *caller, string &endPoint_str)
        {

        }

        /** This function will be called when a connection between client and server closed.
         * @param caller As above
         * @param endPoint_str The remote peer's IP:Port
         */
        virtual void onClose(void *caller, string &endPoint_str)
        {

        }

        /** This function will be called when server finished a async_write operation.
         * @param caller As above
         * @param endPoint_str The remote peer's IP:Port
         * @bytes_transferred The size of sent bytes
         */
        virtual void onWrite(void *caller, string &endPoint_str, size_t bytes_transferred)
        {

        }

        /** This function will be called when server received message from client
         * @param caller As above
         * @param endPoint_str The remote peer's IP:Port
         * @PTR_buffer The received data in binary format
         * @is_text The data is binary or text
         */
        virtual void onMessage(void *caller, string &endPoint_str, const PTR_buffer data, bool is_text)
        {

        }

    protected:
        pluginType _type;
        void *_owner; // if plugin is load in listener, owner is listener* , if plugin is load in session, owner is session*
    };


    /** The manager saves all plugins.
     * When call the following function, the manager will call the same function in all plugins
     */
    class PluginManager
    {
        vector<shared_ptr<BasePlugin>> _plugins;
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
        void onMessage(Args &&...args)
        {
            for (auto &&plugin:_plugins)
            {
                plugin->onMessage(std::forward<Args &&>(args)...);
            }
        }

        void AddPlugin(shared_ptr<BasePlugin> &plugin)
        {
            _plugins.emplace_back(plugin);
        }

        void AddPlugin(shared_ptr<BasePlugin> &&plugin)
        {
            _plugins.emplace_back(move(plugin));
        }

        void ClearPlugin()
        {
            _plugins.clear();
        }
    };


    /** This plugin is used for Remove Session instance when onClose
     */
    template<typename Owner, typename Caller>
    class SessionRemover : public BasePlugin
    {
    public:
        SessionRemover(Owner *p) : BasePlugin(pluginType::Session_Plugin, p)
        {

        }

        void onConnect(void *caller, string &endPoint_str)
        {

        }

        void onMessage(void *caller, string &endPoint_str, PTR_buffer data, bool is_text)
        {

        }

        void onClose(void *caller, string &endPoint_str)
        {

        }

    private:

    };


    /** This plugin is used for store user callback function
    */
    template<typename Owner, typename Caller>
    class UserCallBack : public BasePlugin
    {
    public:
        UserCallBack(Owner *p) : BasePlugin(pluginType::Session_Plugin, p)
        {
        }

        void onConnect(void *caller, string &endPoint_str)
        {
            auto f = GetOwner()->on_connect;
            if (f)
            {
                f((Caller*)caller, endPoint_str);
            }
        }

        void onClose(void *caller, string &endPoint_str)
        {
            auto f = GetOwner()->on_disconnect;
            if (f)
            {
                f((Caller*)caller, endPoint_str);
            }
        }

        void onMessage(void *caller, string &endPoint_str, PTR_buffer data, bool is_text)
        {
            auto f = GetOwner()->on_message;
            if (f)
            {
                f((Caller*)caller, endPoint_str, data, is_text);
            }
        }

    private:
        Owner *GetOwner()
        {
            return (Owner *) _owner;
        }
    };


    /**
     * This class is a helper class which is used to print some information. Do not use this class
     */
    class LogPrinter
    {
    public:
        void setEnable(bool enable)
        {
            _enable = enable;
        }

        void SetEndPoint(string &_endpoint_str)
        {
            this->_endpoint_str = _endpoint_str;
        }

        // T mean string& or string&&
        template<typename T>
        void PrintLog(T &&log, bool highlight = false)
        {
            if (!_enable)
                return;
            if (_shouldLineFeed)
            {
                _shouldLineFeed = false;
                cout << "\n";
            }
            if (highlight)
                cout << "\033[32m[INFO ][" + _endpoint_str + "] " + log << "\033[0m\n";
            else
                cout << "\033[0m[INFO ][" + _endpoint_str + "] " + log << "\033[0m\n";
        }

        template<typename T>
        void PrintError(T &&log)
        {
            if (!_enable)
                return;
            if (_shouldLineFeed)
            {
                _shouldLineFeed = false;
                cout << "\n";
            }
            cout << "\033[31m[Error][" + _endpoint_str + "] " + log << "\033[0m\n";
        }

        void PrintNetworkSpeed(double KB_s)
        {
            if (!_enable)
                return;
            _shouldLineFeed = true;
            auto func = [this](string &&log)
            {
                cout << "\r[INFO ][" + _endpoint_str + "] " << log << flush;
            };

            std::stringstream stream;
            if (KB_s >= 1 && KB_s < 1000)
            {
                stream << std::fixed << std::setprecision(1) << KB_s << "KB/s";
                func(stream.str());
            }
            else if (KB_s < 1)
            {
                stream << std::fixed << std::setprecision(1) << KB_s * 1024 << "B/s";
                func(stream.str());
            }
            else if (KB_s >= 1000)
            {
                stream << std::fixed << std::setprecision(1) << KB_s / 1024 << "MB/s";
                func(stream.str());
            }
        }

    private:
        string _endpoint_str = "";
        bool _shouldLineFeed = false;
        bool _enable = true;
    };

    /**
     * This class is a helper class which is used to buffer Websocket send data. Do not use this class
     */
    class WriteTaskManager
    {
        /**
         * @binary true, if you want to write a binary data / false if you want to write text message
         * @is_copy false(default), then you shall make your data under data_pointer available, until the callback function is called. Then you can release your data in callback
         * true, if you don't want to manage your data lifetime
         * @data_pointer The pointer to your data address
         * @size data total size
         * @callback A callback function which have no parameter and void return type. When the writeTask is done, the callback function will be called, and you can manually release your
         * data under data_pointer
         */
        struct WriteTask
        {
            bool is_binary;
            PTR_buffer_seq data;
        };

    public:
        ~WriteTaskManager()
        {
            clear();
        }

        void push_text(const string &text)
        {
            // create a new char[] to save the string data, and save it in a shared_ptr
            auto data = std::make_shared<beast::flat_buffer>();
            beast::ostream(*data) << text;
            _writeTaskQueue.emplace(
                    WriteTask{false, std::make_shared<vector<PTR_buffer>>(vector<PTR_buffer>{data})}
            );
        }

        void push_text(PTR_buffer text)
        {
            _writeTaskQueue.emplace(
                    WriteTask{false, std::make_shared<vector<PTR_buffer>>(vector<PTR_buffer>{text})}
            );
        }

        void push_binary(PTR_buffer_seq data)
        {
            _writeTaskQueue.emplace(WriteTask{true, data});
        }

        void push_binary(PTR_buffer data)
        {
            _writeTaskQueue.emplace(
                    WriteTask{true, std::make_shared<vector<PTR_buffer>>(vector<PTR_buffer>{data})}
            );
        }

        const WriteTask &front()
        {
            return _writeTaskQueue.front();
        }

        void pop()
        {
            _writeTaskQueue.pop();
        }

        void clear()
        {
            queue<WriteTask>().swap(_writeTaskQueue);
        }

        const bool empty()
        {
            return _writeTaskQueue.empty();
        }

        const size_t size()
        {
            return _writeTaskQueue.size();
        }

    private:
        queue<WriteTask> _writeTaskQueue;
    };


    /**
     * This class is used for calculate the Network download/upload speed.
     * You have 2 mode to calculate the network speed you want.
     *
     * Mode 1:
     * Give t1(start time),t2(end_time) and b1,b2,b3,... this class will calculate a estimated speed in the past duration.
     * t1/t2 means the time you call the method SetStart/GetSpeed, and t1/t2 will be calculated automatically.
     *
     * For example:
     * NetSpeed spd;
     * spd.SetMode(1);
     * spd.SetStart(500); // statistics cycle = 500ms
     * spd.SetData(1000);
     * spd.SetData(1072);
     * spd.SetData(1072);
     * double speed2 = spd.GetSpeed(); // Get Speed in the last 0.5 sec KB/s
     *
     * Mode 2:
     * Give t1(start time),b1,b2,b3,... and a callback function, this class will calculate a estimate speed in every duration.
     *
     * For example:
     * void handler(double speed)
     * {
     *     std::cout << speed << std::endl;
     * }
     *
     * NetSpeed spd;
     * spd.SetMode(2);
     * spd.Register_Callback(&handler);
     * spd.SetDurationAsync(2000); // get a estimate speed in every 2 second
     * spd.StartAsync();
     * spd.SetDataAsync(1000);
     * spd.SetDataAsync(2000);
     * ...
     * spd.SetDataAsync(1000); // whenever you want to insert new data
     * ...
     * spd.EndAsync();
     */
    class IOSpeedCounter
    {
    public:
        IOSpeedCounter() : _mode2_running(false), _mode(1)
        {

        }

        void SetMode(int mode)
        {
            if (mode == 1)
                _mode = 1;
            if (mode == 2)
                _mode = 2;
        }

        /// region for mode 1
        void SetStart(int duration = 1000)
        {
            if (_mode != 1)
                return;
            this->Clear();
            _duration = duration;
            _last_cycle_time = GetNowTime();
            _last_duration_bytes = 0;
            _last_duration_time = 0;
        }

        /**
         * Return true means that, when you call SetData, at least one duration have been past since your last SetData return true
         * @param bytes
         * @return
         */
        bool SetData(unsigned long long bytes)
        {
            if (_mode != 1)
                return false;
            auto now_time = GetNowTime();
            if (now_time - _last_cycle_time > _duration * 1000)
            {
                // save bytes/duration time in the last period of time
                _last_duration_bytes = _this_cycle_bytes;
                auto tmp = now_time - (now_time - _last_cycle_time) % (_duration * 1000);
                _last_duration_time = tmp - _last_cycle_time;

                _this_cycle_bytes = bytes;
                _last_cycle_time = tmp;
                return true;
            }
            _this_cycle_bytes += bytes;
            _total_bytes += bytes;
            return false;
        }

        double GetSpeed()
        {
            if (_mode != 1)
                return 0;
            unsigned long long endTime = GetNowTime();
            unsigned long long time_cost = (endTime - _last_cycle_time) + _last_duration_time;
            double kb = fmax(0, (double) (_this_cycle_bytes + _last_duration_bytes)) / 1024;
            double sec = (double) time_cost / 1e6;
            return kb / sec;
        }
        /// region for mode 1

        /// region for mode 2 TODO:Need Implementation
        /*
        void SetDurationAsync(int duration = 1000)
        {

        }

        void Register_Callback(void* p)
        {

        }

        void StartAsync()
        {

        }

        void SetDataAsync()
        {

        }

        void EndAsync()
        {

        }
         */

        void Clear()
        {
            _beginTime = 0;
            _this_cycle_bytes = 0;
            _total_bytes = 0;
            _last_cycle_time = 0;
            _duration = 1000;
        }

        /**
        * @return Now Timestamp in MicroSecond
        */
        unsigned long long GetNowTime()
        {
            struct timeval temp;
            gettimeofday(&temp, nullptr);
            return temp.tv_sec * 1e6 + temp.tv_usec;
        }

    private:

    private:
        int _mode;
        // for mode 1
        unsigned long long _beginTime;
        unsigned long long _last_cycle_time;

        unsigned long long _this_cycle_bytes;
        unsigned long long _total_bytes;

        unsigned long long _last_duration_bytes;
        unsigned long long _last_duration_time;

        // for mode 2
        bool _mode2_running;
        unsigned int _duration = 1000;
    };

    /**
     * Session class, each websocket connection have a session, and each session have a strand.
     * So no need to use threads synchronization
     */
    class Session : public std::enable_shared_from_this<Session>
    {
    private:
        bool _isRunning;                                // session running flag
        const string _remote_address;              // client's IP address
        const unsigned short _remote_port;              // client's port
        string _endpoint_str;                      // client's IP + ":" + port
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
        void SendTextMessage(const string &text)
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
                                string(BOOST_BEAST_VERSION_STRING) +
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
        void on_read(beast::error_code ec, size_t bytes_transferred)
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
            shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>(std::move(_read_buffer));
            _shared_pluginManager.onMessage(this, _endpoint_str, buffer, _ws.got_text());

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
        void on_write(beast::error_code ec, size_t bytes_transferred)
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


    /**
     * Accepts incoming connections and launches new sessions
     */
    class listener
    {
    private:
        asio::io_context &_ioc;                                  // ioc object referenced from main function
        tcp::acceptor _acceptor;                                // tcp acceptor
        map<const string, shared_ptr<Session>> _session_Set;     // a map to save all active sessions
        LogPrinter _logPrinter;

    public:
        // callback function
        std::function<void(Session *caller, string endpoint, const PTR_buffer data, bool is_text)> on_message;
        std::function<void(Session *caller, string endpoint)> on_connect;
        std::function<void(Session *caller, string endpoint)> on_disconnect;
        PluginManager pluginManager;                            // plugin manager which saves all plugins. All plugins in listener class will have effect in all sessions

    public:
        listener(asio::io_context &ioc) : _ioc(ioc), _acceptor(ioc)
        {
            on_message = nullptr;
            on_connect = nullptr;
            on_disconnect = nullptr;

            // add plugins
            // be careful, the SessionRemover should be the last plugin. Because these plugin will be called in sequence
            pluginManager.AddPlugin(
                    shared_ptr<UserCallBack<listener, Session>>(new UserCallBack<listener, Session>(this)));
            pluginManager.AddPlugin(
                    shared_ptr<SessionRemover<listener, Session>>(new SessionRemover<listener, Session>(this)));
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

        shared_ptr<Session> GetSession(const string endpoint)
        {
            if (_session_Set.count(endpoint) > 0)
                return _session_Set[endpoint];
            else
                return nullptr;
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


    class server
    {
    public:
        server(int threads_num = 1) : _ioc{std::max(threads_num, 1)}
        {
            _threads_num = std::max(threads_num, 1);
            _running = false;
            on_message = nullptr;
            on_connect = nullptr;
            on_disconnect = nullptr;
        }

        void listen(string address_str, const unsigned short port, bool wss = false, bool block = false)
        {
            if (_running)
                return;

            auto address = asio::ip::make_address(address_str);

            // Create a listening port.
            _listener = std::make_shared<listener>(_ioc);
            // If port is available ,then run server
            if (_listener->listen(tcp::endpoint{address, port}))
                _listener->run();
            else
                return;

            setCallback();

            for (auto i = 0; i < _threads_num; ++i)
                _thread_pool.emplace_back(
                        [this]
                        {
                            this->_ioc.run();
                        }
                );
            _running = true;
        }

        void listen_block(string address_str, const unsigned short port, bool wss = false)
        {
            if (_running)
                return;

            auto address = asio::ip::make_address(address_str);

            // Create a listening port.
            _listener = std::make_shared<listener>(_ioc);
            // If port is available ,then run server
            if (_listener->listen(tcp::endpoint{address, port}))
                _listener->run();
            else
                return;

            setCallback();

            for (auto i = 0; i < _threads_num - 1; ++i)
                _thread_pool.emplace_back(
                        [this]
                        {
                            this->_ioc.run();
                        }
                );
            _running = true;
            this->_ioc.run();
        }

        void close()
        {
            _ioc.stop();
            for (auto &&t:_thread_pool)
            {
                if (t.joinable())
                    t.join();
            }
            _running = false;
        }

        void SendBinaryMessage(PTR_buffer data, const string endpoint = "")
        {
            if (endpoint == "")
            {
                for (auto &&sess:_listener->GetSessionList())
                {
                    sess.second->SendBinaryMessage(data);
                }
            }
            else
            {
                _listener->GetSession(endpoint)->SendBinaryMessage(data);
            }
        }

        void SendTextMessage(const string &text, const string endpoint = "")
        {
            if (endpoint == "")
            {
                for (auto &&sess:_listener->GetSessionList())
                {
                    sess.second->SendTextMessage(text);
                }
            }
            else
            {
                _listener->GetSession(endpoint)->SendTextMessage(text);
            }
        }

    private:
        void setCallback()
        {
            _listener->on_connect = on_connect;
            _listener->on_disconnect = on_disconnect;
            _listener->on_message = on_message;
        }

    public:
        std::function<void(Session *caller, string endpoint, const PTR_buffer data, bool is_text)> on_message;
        std::function<void(Session *caller, string endpoint)> on_connect;
        std::function<void(Session *caller, string endpoint)> on_disconnect;

    private:
        bool _running;
        int _threads_num;
        vector<thread> _thread_pool;
        asio::io_context _ioc;
        shared_ptr<listener> _listener;
    };

    // help function
    string to_string(const PTR_buffer data)
    {
        return string((const char *)data->data().data(), data->size());
    }

    Json::Value json_load(const PTR_buffer data)
    {
        return Json::Value(to_string(data));
    }

    Json::Value json_load(const string& str)
    {
        return Json::Value(str);
    }

    string json_dump(const Json::Value& json)
    {
        Json::StreamWriterBuilder builder;
        builder.settings_["indentation"] = "";
        return Json::writeString(builder, json);
    }

    void buf_write(const PTR_buffer data, void* p, size_t size)
    {
        beast::ostream(*data).write(reinterpret_cast<const char*>(p), size);
    }

    void buf_write(const PTR_buffer data, const string& str)
    {
        beast::ostream(*data).write(reinterpret_cast<const char*>(str.c_str()), str.size());
    }

    void buf_write(const PTR_buffer data, const Json::Value& json)
    {
        auto str = json_dump(json);
        beast::ostream(*data).write(reinterpret_cast<const char*>(str.c_str()), str.size());
    }
}

#endif //CPP_WEBSOCKET_CPP_WEBSOCKET_HPP
