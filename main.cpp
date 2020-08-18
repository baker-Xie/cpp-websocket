
#include "cpp_websocket.hpp"
#include <opencv2/opencv.hpp>
#include <atomic>
#include <tuple>

using std::tuple;

tuple<bool, vector<unsigned char>> EncodeImage(cv::Mat &src, const std::string format, int w = 0, int h = 0)
{
    vector<unsigned char> vecImg;
    cv::Mat tmp;
    if (w != 0 && h != 0)
        cv::resize(src, tmp, cv::Size(w, h));
    // 30 mean jpg quality 0~100 default = 95
    std::vector<int> vecCompression_params = vector<int>{CV_IMWRITE_JPEG_QUALITY, 80};
    if (format == ".jpg" || format == ".png")
        return {cv::imencode(format, tmp, vecImg, vecCompression_params), vecImg};
    else
        return {false, vecImg};
}


class Demo
{
public:
    Demo(ws::server &srv) : _srv(srv)
    {
        _t = nullptr;
        _stop = false;
    }

    void MainLoop()
    {
        cv::VideoCapture cap;
        cap.open("/home/u/RECORD/20200410/test2/test2_1_video.mkv");
        cv::VideoCapture cap2;
        cap2.open("/home/u/RECORD/20200410/test2/test2_2_video.mkv");
        cv::Mat img1;
        cv::Mat img2;
        Json::Value j1;
        while (!_stop)
        {
            cap.read(img1);
            cap2.read(img2);
            j1["path"] = "/Video0";
            Json::Value tmp;
            tmp["timestamp"] = 0;
            tmp["width"] = cap.get(CV_CAP_PROP_FRAME_WIDTH);
            tmp["height"] = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
            tmp["format"] = "raw";
            tmp["comments"] = "";
            j1["metadata"] = tmp;
            string s1 = ws::json_dump(j1);

            auto s1_size = s1.length();
            j1["path"] = "/Video1";
            tmp["comments"] = "1";
            j1["metadata"] = tmp;
            string s2 = ws::json_dump(j1);
            auto s2_size = s2.length();

            auto sp = std::make_shared<beast::flat_buffer>();

            int tag = 0;
            int length;
            auto d = sizeof(int);
            ws::buf_write(sp,&tag,sizeof(int));
            ws::buf_write(sp,&s1_size,sizeof(int));
            ws::buf_write(sp,s1);

            if (0)
            {
                auto[result, data] = EncodeImage(img1, ".jpg", 640, 360);
                tag = 1;
                ws::buf_write(sp,&tag,sizeof(int));
                length = data.size();
                ws::buf_write(sp,&length,sizeof(int));
                ws::buf_write(sp,data.data(),data.size());
            }
            else
            {
                cv::cvtColor(img1, img1, CV_BGR2RGBA);
                tag = 1;
                beast::ostream(*sp).write(reinterpret_cast<const char *>(&tag), sizeof(int)); //write image
                int length = img1.rows * img1.cols * img1.channels();
                beast::ostream(*sp).write(reinterpret_cast<const char *>(&length), sizeof(int)); //write length
                beast::ostream(*sp).write(reinterpret_cast<const char *>(img1.data), length); //write value
            }

            tag = 0;
            beast::ostream(*sp).write(reinterpret_cast<const char *>(&tag), sizeof(int)); //write json
            beast::ostream(*sp).write(reinterpret_cast<const char *>(&s2_size), sizeof(int)); //write length
            beast::ostream(*sp).write(reinterpret_cast<const char *>(s2.c_str()), s2_size); //write value

            if (0)
            {
                auto[result, data] = EncodeImage(img2, ".jpg", 640, 360);
                tag = 1;
                beast::ostream(*sp).write(reinterpret_cast<const char *>(&tag), sizeof(int)); //write image
                length = data.size();
                beast::ostream(*sp).write(reinterpret_cast<const char *>(&length), sizeof(int)); //write length
                beast::ostream(*sp).write(reinterpret_cast<const char *>(data.data()), data.size());
                auto size = sp->data().size();
            }
            else
            {
                cv::cvtColor(img2, img2, CV_BGR2RGBA);
                tag = 1;
                beast::ostream(*sp).write(reinterpret_cast<const char *>(&tag), sizeof(int)); //write image
                int length = img2.rows * img2.cols * img2.channels();
                beast::ostream(*sp).write(reinterpret_cast<const char *>(&length), sizeof(int)); //write length
                beast::ostream(*sp).write(reinterpret_cast<const char *>(img2.data), length); //write value
            }
            auto k = sp->size();
            _srv.SendBinaryMessage(sp);
            usleep(1e5);
        }
        return;
    }

    void on_connect(ws::Session *caller, string end_point)
    {
        _stop = false;
        _t = new thread(&Demo::MainLoop, this);
    }

    void on_disconnect(ws::Session *caller, string end_point)
    {
        _stop = true;
        if (_t)
        {
            if (_t->joinable())
                _t->join();
            delete _t;
            _t = nullptr;
        }
    }

    void on_message(ws::Session *caller, string end_point, ws::PTR_buffer data, bool is_text)
    {

    }

private:
    std::atomic_bool _stop;
    ws::server &_srv;
    std::thread *_t;
};

int main()
{
    ws::server srv;
    Demo demo(srv);
    srv.on_connect = std::bind(&Demo::on_connect, &demo, _1, _2);
    srv.on_disconnect = std::bind(&Demo::on_disconnect, &demo, _1, _2);
    srv.on_message = std::bind(&Demo::on_message, &demo, _1, _2, _3, _4);
    srv.listen_block("127.0.0.1", 8001);
}