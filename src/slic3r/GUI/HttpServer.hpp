#ifndef slic3r_Http_App_hpp_
#define slic3r_Http_App_hpp_

#include <iostream>
#include <mutex>
#include <stack>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include <set>
#include <memory>
#include "buildinfo.h"

#ifdef CUSTOMIZED
#define LOCALHOST_PORT      15888
#else 
#define LOCALHOST_PORT 13666
#endif // CUSTOMIZED

#define LOCALHOST_URL       "http://localhost:"

namespace Slic3r { namespace GUI {

class session;

class http_headers
{
    std::string method;
    std::string url;
    std::string version;

    std::map<std::string, std::string> headers;

    friend class session;
public:
    std::string get_url() { return url; }

    int content_length();

    void on_read_header(std::string line);

    void on_read_request_line(std::string line);
};

class HttpServer
{
    boost::asio::ip::port_type port;

public:
    class Response
    {
    public:
        virtual ~Response()                                   = default;
        virtual void write_response(std::stringstream& ssOut) = 0;
    };

    class ResponseNotFound : public Response
    {
    public:
        ~ResponseNotFound() override = default;
        void write_response(std::stringstream& ssOut) override;
    };
    class ResponseStaticFile : public Response
    {
    public:
        ~ResponseStaticFile() override = default;
        ResponseStaticFile(const std::string http_response) { m_http_response = http_response; }
        void write_response(std::stringstream& ssOut) override;
    private:
        std::string m_http_response;
    };
   
    class ResponseRedirect : public Response
    {
        const std::string location_str;

    public:
        ResponseRedirect(const std::string& location) : location_str(location) {}
        ~ResponseRedirect() override = default;
        void write_response(std::stringstream& ssOut) override;
    };
    HttpServer(boost::asio::ip::port_type port);
    ~HttpServer() { stop(); }
    boost::thread m_http_server_thread;
    bool          start_http_server = false;

    bool is_started() { return start_http_server; }
    void start();
    void stop();
    void set_request_handler(const std::function<std::shared_ptr<Response>(const std::string&)>& m_request_handler);
    int get_port() { return port; }
    static std::shared_ptr<Response> bbl_auth_handle_request(const std::string& url);
    static std::shared_ptr<Response> creality_handle_request(const std::string& url);
    
    #if defined(__linux__) || defined(__LINUX__)
    void sendFrame(const boost::system::error_code &ec,boost::asio::ip::tcp::socket &socket);
    std::mutex frame_mutex_;
    bool mjpeg_server_started = false;
    #endif
private:
    class IOServer
    {
    public:
        HttpServer&                        server;
        boost::asio::io_service            io_service;
        boost::asio::ip::tcp::acceptor     acceptor;
        std::set<std::shared_ptr<session>> sessions;

        IOServer(HttpServer& server) : server(server), acceptor(io_service, {boost::asio::ip::address_v4::loopback(), server.port}) {}

        void do_accept();

        void start(std::shared_ptr<session> session);
        void stop(std::shared_ptr<session> session);
        void stop_all();
    };
    friend class session;

    std::unique_ptr<IOServer> server_{nullptr};

    std::function<std::shared_ptr<Response>(const std::string&)> m_request_handler{&HttpServer::creality_handle_request};

};
using SocketPtr = std::shared_ptr<boost::asio::ip::tcp::socket>;
class session : public std::enable_shared_from_this<session>
{
    HttpServer::IOServer& server;
    boost::asio::ip::tcp::socket socket;

    boost::asio::streambuf buff;
    http_headers headers;
    
    void read_first_line();
    void read_next_line();
    void read_body();

    void sendFrame(const boost::system::error_code &ec,boost::asio::ip::tcp::socket &socket,bool is_rtsp=false);

private:
    boost::asio::deadline_timer *m_video_timer=nullptr;
    boost::asio::streambuf proxy_buff;
    boost::asio::steady_timer timer_;
    SocketPtr m_upstream_socket;
    bool m_proxy_done{false};

    void close_upstream();
    void fail_proxy(int status_code, const std::string& message);
    void arm_proxy_timeout(SocketPtr socket_ptr);
public:
    session(HttpServer::IOServer& server, boost::asio::ip::tcp::socket socket) : server(server), socket(std::move(socket)),timer_(server.io_service) {
        
    }
    ~session() {
        if(m_video_timer)
            delete m_video_timer;
    }
    void start();
    void stop();
    void handle_proxy_request(const std::string& url);
    void do_write_proxy(SocketPtr socket_ptr, const std::string& target_host,const std::string& target_path);
    void write_response(const std::string& response);
    void do_read(SocketPtr socket_ptr);
};

std::string url_get_param(const std::string& url, const std::string& key);

}};

#endif
