#include "HttpServer.hpp"
#include <boost/log/trivial.hpp>
#include "GUI_App.hpp"
#include "libslic3r/Thread.hpp"
#include "slic3r/Utils/Http.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"
#include <boost/regex.hpp>
#if defined(__linux__) || defined(__LINUX__)
#include "video/WebRTCDecoder.h"
#include <boost/asio/steady_timer.hpp>
#endif
namespace pt = boost::property_tree;
namespace Slic3r {
namespace GUI {

int http_headers::content_length()
{
    auto request = headers.find("content-length");
    if (request != headers.end()) {
        std::stringstream ssLength(request->second);
        int               content_length;
        ssLength >> content_length;
        return content_length;
    }
    return 0;
}

void http_headers::on_read_header(std::string line)
{
    // std::cout << "header: " << line << std::endl;

    std::stringstream ssHeader(line);
    std::string       headerName;
    std::getline(ssHeader, headerName, ':');

    std::string value;
    std::getline(ssHeader, value);
    headers[headerName] = value;
}

void http_headers::on_read_request_line(std::string line)
{
    std::stringstream ssRequestLine(line);
    ssRequestLine >> method;
    ssRequestLine >> url;
    ssRequestLine >> version;

    std::cout << "request for resource: " << url << std::endl;
}

std::string url_get_param(const std::string& url, const std::string& key)
{
    size_t start = url.find(key);
    if (start == std::string::npos) return "";
    size_t eq = url.find('=', start);
    if (eq == std::string::npos) return "";
    std::string key_str = url.substr(start, eq - start);
    if (key_str != key)
        return "";
    start += key.size() + 1;
    size_t end = url.find('&', start);
    if (end == std::string::npos) end = url.length(); // Last param
    std::string result = url.substr(start, end - start);
    return result;
}

std::string url_get_param_ignore(const std::string& url, const std::string& key)
{
    size_t start = url.find(key);
    if (start == std::string::npos) return "";
    size_t eq = url.find('=', start);
    if (eq == std::string::npos) return "";
    std::string key_str = url.substr(start, eq - start);
    if (key_str != key)
        return "";
    start += key.size() + 1;
    size_t end = url.length(); // Last param
    std::string result = url.substr(start, end - start);
    return result;
}

void session::start()
{
    read_first_line();
}

void session::stop()
{
    boost::system::error_code ignored_ec;
    socket.shutdown(boost::asio::socket_base::shutdown_both, ignored_ec);
    socket.close(ignored_ec);
}

void session::read_first_line()
{
    auto self(shared_from_this());

    async_read_until(socket, buff, '\r', [this, self](const boost::beast::error_code& e, std::size_t s) {
        if (!e) {
            std::string  line, ignore;
            std::istream stream{&buff};
            std::getline(stream, line, '\r');
            std::getline(stream, ignore, '\n');
            headers.on_read_request_line(line);
            read_next_line();
        } else if (e != boost::asio::error::operation_aborted) {
            server.stop(self);
        }
    });
}

void session::read_body()
{
    auto self(shared_from_this());

    int                                nbuffer = 1000;
    std::shared_ptr<std::vector<char>> bufptr  = std::make_shared<std::vector<char>>(nbuffer);
    async_read(socket, boost::asio::buffer(*bufptr, nbuffer),
               [this, self](const boost::beast::error_code& e, std::size_t s) { server.stop(self); });
}

void session::read_next_line()
{
    auto self(shared_from_this());

    async_read_until(socket, buff, '\r', [this, self](const boost::beast::error_code& e, std::size_t s) {
        if (!e) {
            std::string  line, ignore;
            std::istream stream{&buff};
            std::getline(stream, line, '\r');
            std::getline(stream, ignore, '\n');
            headers.on_read_header(line);

            if (line.length() == 0) {
                if (headers.content_length() == 0) {
                    std::cout << "Request received: " << headers.method << " " << headers.get_url();
                    if (headers.method == "OPTIONS") {
                        // Ignore http OPTIONS
                        server.stop(self);
                        return;
                    }

                    const std::string url_str = Http::url_decode(headers.get_url());
                    if (url_str.find("/proxy") == 0){
                        // 处理代理请求
                        this->handle_proxy_request(url_str);
                        return ;
                    }
                    else if (url_str.find("/videostream") == 0) {
                        #if defined(__linux__) || defined(__LINUX__)
                        std::string   ip           = url_get_param(url_str, "ip");
                        std::string timestamp = url_get_param(url_str, "timestamp");
                        std::string video_url = (boost::format("http://%1%:8000/call/webrtc_local") % ip).str();
                        HttpServer *pServer = wxGetApp().get_server();
                        std::cout << "start webrtc!"<<timestamp<<"\n";
                        pServer->mjpeg_server_started = true;
                        WebRTCDecoder::GetInstance()->startPlay(video_url); 
                        
                        const std::string header =
                                    "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: multipart/x-mixed-replace; boundary=boundarydonotcross\r\n"
                                    "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
                                    "pragma:no-cache\r\n"
                                    "Access-Control-Allow-Origin: *\r\n"
                                    "Connection: close\r\n\r\n";
                        async_write(socket,  boost::asio::buffer(header),[this, self,pServer](const boost::beast::error_code& e, std::size_t s) {
                            if(!e)
                            {
                                if(!this->m_video_timer)
                                {
                                    this->m_video_timer = new boost::asio::deadline_timer(server.io_service, boost::posix_time::milliseconds(100));
                                }
                                std::cout << "start send!\r\n";
                                this->sendFrame(e,socket);
                            }else{
                                pServer->mjpeg_server_started = false;
                                std::cout << "end send!\r\n";
                            }
                            
                        });
                        #endif
                    }else{
                        const auto        resp    = server.server.m_request_handler(url_str);
                        std::stringstream ssOut;
                        resp->write_response(ssOut);
                        std::shared_ptr<std::string> str = std::make_shared<std::string>(ssOut.str());
                        async_write(socket, boost::asio::buffer(str->c_str(), str->length()),
                                    [this, self, str](const boost::beast::error_code& e, std::size_t s) {
                            std::cout << "done" << std::endl;
                            server.stop(self);
                        });
                    }
                } else {
                    read_body();
                }
            } else {
                read_next_line();
            }
        } else if (e != boost::asio::error::operation_aborted) {
            server.stop(self);
        }
    });
}
#if defined(__linux__) || defined(__LINUX__)
void session::sendFrame(const boost::system::error_code &ec,boost::asio::ip::tcp::socket &socket)
{
        //this->frame_mutex_.lock();
        std::vector<unsigned char> copy_frame = WebRTCDecoder::GetInstance()->getFrameData();
        //this->frame_mutex_.unlock();
        if(copy_frame.size()==0)
        {
            this->m_video_timer->cancel();
            this->m_video_timer->expires_at(this->m_video_timer->expires_at()+boost::posix_time::milliseconds(150));
            this->m_video_timer->async_wait([&](const auto& error) {
                    if (!error) {
                        //std::cout << "start send frame!\r\n";
                        this->sendFrame(ec,socket);
                    }
                });
            return;
        }
        if(WebRTCDecoder::GetInstance()->isStop())
        {
            //break;
        }
        
        //std::cout << "send!\n"<<copy_frame.size();
        const std::string frame_header =
                    "--boundarydonotcross\r\n"
                    "Content-Type: image/jpeg\r\n"
                    "Content-Length: " + 
                    std::to_string(copy_frame.size()) + "\r\n\r\n";
        
        std::vector<boost::asio::const_buffer> buffers;
        buffers.emplace_back(boost::asio::buffer(frame_header));
        buffers.emplace_back(boost::asio::buffer(copy_frame));
        //std::cout << "start send!\r\n";
        boost::asio::async_write(socket,  buffers,[this,&socket](const boost::beast::error_code& ec, std::size_t s) {
            
            if(!ec)
            {
                //std::cout << "send!"<<this->m_video_timer<<"\r\n";
                //std::this_thread::sleep_for(std::chrono::milliseconds(80));
                //this->sendFrame(ec,socket);
                this->m_video_timer->cancel();
                this->m_video_timer->expires_at(this->m_video_timer->expires_at()+boost::posix_time::milliseconds(150));
                this->m_video_timer->async_wait([&](const auto& error) {
                    if (!error) {
                        //std::cout << "start send frame!\r\n";
                        this->sendFrame(ec,socket);
                    }
                });
            }else{
                //this->mjpeg_server_started = false;
                std::cout << "end send!\r\n";
            }
            
        });
}
#endif
void HttpServer::IOServer::do_accept()
{
    acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (!acceptor.is_open()) {
            return;
        }

        if (!ec) {
            const auto ss = std::make_shared<session>(*this, std::move(socket));
            start(ss);
        }

        do_accept();
    });
}

void HttpServer::IOServer::start(std::shared_ptr<session> session)
{
    sessions.insert(session);
    session->start();
}

void HttpServer::IOServer::stop(std::shared_ptr<session> session)
{
    sessions.erase(session);
    session->stop();
}

void HttpServer::IOServer::stop_all()
{
    for (auto s : sessions) {
        s->stop();
    }
    sessions.clear();
}


HttpServer::HttpServer(boost::asio::ip::port_type port) : port(port) {}
bool is_port_in_use(unsigned short port) {
    try {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::acceptor acceptor(io_context);
        acceptor.open(boost::asio::ip::tcp::v4());
        acceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));
        acceptor.close(); // 不需要持续监听，关闭 acceptor
        return false; // 如果没有异常，端口未被占用
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return true; // 捕获异常，说明端口被占用
    }
}
void HttpServer::start()
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    
    boost::system::error_code ec;
    //#ifndef _WIN32
    // 尝试绑定到指定端口
    for(int i=1;i<=20;i++)
    {
        if(is_port_in_use(port))
        {
            port = port+i;
        }else{
            break;
        }
        if(i==20)
        {
            return;
        }
    }
    //#endif
    
    BOOST_LOG_TRIVIAL(info) << "start_http_service...:"<<port;
    start_http_server    = true;
    m_http_server_thread = create_thread([this] {
        set_current_thread_name("http_server");
        try {
            server_ = std::make_unique<IOServer>(*this);
            server_->acceptor.listen();

            server_->do_accept();
            //this->m_video_timer = new boost::asio::deadline_timer(server_->io_service, boost::posix_time::milliseconds(100));
            server_->io_service.run();
        }catch(boost::system::system_error& e)
        {
            start_http_server = false;
        }
    });
}

void HttpServer::stop()
{
    if(!start_http_server)
        return;
    start_http_server = false;
    if (server_) {
        server_->acceptor.close();
        server_->stop_all();
        server_->io_service.stop();
    }
    if (m_http_server_thread.joinable())
        m_http_server_thread.join();
    server_.reset();
}

void HttpServer::set_request_handler(const std::function<std::shared_ptr<Response>(const std::string&)>& request_handler)
{
    this->m_request_handler = request_handler;
}
// 根据文件扩展名确定Content-Type
std::string get_content_type(const std::string& file_extension) {
    static std::map<std::string, std::string> content_types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".jpg", "image/jpeg"},
        {".png", "image/png"},  
        {".svg", "image/svg+xml"},
        {".ttf", "font/ttf"}
        // 可以继续添加更多的文件扩展名和对应的Content-Type映射
    };
    auto it = content_types.find(file_extension);
    if (it!= content_types.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

// 构建HTTP响应消息
std::string build_http_response(int status_code, const std::string& content_type, const std::string& content) {
    std::stringstream response;
    switch (status_code) {
    case 200:
        response << "HTTP/1.1 200 OK\r\n";
        break;
    case 404:
        response << "HTTP/1.1 404 Not Found\r\n";
        break;
    case 500:
        response << "HTTP/1.1 500 Internal Server Error\r\n";
        break;
        // 可以添加更多的状态码处理逻辑
    }
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Access-Control-Allow-Origin: " << "*" << "\r\n";
    response << "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n";
    response << "pragma:no-cache\r\n";
    response << "\r\n";
    response << content;
    return response.str();
}
// 解析URL，提取路径和参数
void parse_url(const std::string& url, std::string& path, std::string& params) {
    // 简单的正则表达式模式，用于匹配URL中的路径和查询参数部分
    // 这里的正则表达式是一种简化示意，实际应用中可根据更严格的URL规范进一步完善
    boost::regex pattern("(/[^?]*)?(\\?.*)?");
    boost::smatch matches;

    if (boost::regex_match(url, matches, pattern)) {
        if (matches.size() >= 1) {
            path = matches[1];
            if (matches.size() >= 2) {
                params = matches[2];
            } else {
                params = "";
            }
        }
    }
}
void session::do_write_proxy(SocketPtr proxysocket, const std::string& target_host,const std::string& target_path)
{
    // 构造 HTTP 请求
        std::string request = "GET " + target_path + " HTTP/1.1\r\n";
        request += "Host: " + target_host + "\r\n";
        request += "Connection: close\r\n\r\n";

        std::shared_ptr<std::string> str = std::make_shared<std::string>(request);
        // 发送请求
        //boost::asio::write(socket, boost::asio::buffer(request));
        proxysocket->async_send(boost::asio::buffer(str->c_str(), str->length()), [str](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                
                std::cout << "Request sent successfully: " << bytes_transferred << " bytes\n";
                //boost::asio::streambuf response;
                //boost::asio::read_until(*proxysocket, response, "\r\n");
            } else {
                //this->write_response(
                //        build_http_response(500, "text/plain", "Internal server error: Unable to connect to target host"));
                
            }
        });
        timer_.expires_after(std::chrono::seconds(5));
}
void session::write_response(const std::string& response)
{
    auto self(shared_from_this());
    std::shared_ptr<std::string> str = std::make_shared<std::string>(response);
                        async_write(socket, boost::asio::buffer(str->c_str(), str->length()),
                                    [this, self, str](const boost::beast::error_code& e, std::size_t s) {
                            if (e) {
                                std::cerr << "Error writing response: " << e.message() << "\n";
                                server.stop(self);
                                return;
                            }else{
                                std::cerr << "Response sent successfully: " << s << " bytes\n";
                                server.stop(self);
                                return;
                            }
                        });
}
void session::do_read(SocketPtr socket_ptr)
{
    auto self(shared_from_this());
    async_read_until(*socket_ptr, proxy_buff, '\r', [this, self,socket_ptr](const boost::beast::error_code& e, std::size_t s) {
        timer_.cancel();
        if (!e) {
            std::string  line, ignore;
            std::istream stream{&proxy_buff};
            std::getline(stream, line, '\r');
            //{}
            std::getline(stream, ignore, '\n');
            timer_.expires_after(std::chrono::seconds(5));
            async_read(*socket_ptr, proxy_buff,[this](const boost::system::error_code& ec, size_t) {
                //if(ec) {
                //    std::cerr << "Error reading response: " << ec.message() << "\n";
                //    return;
                //}
                timer_.cancel();
                std::ostringstream response_body;
                response_body << "HTTP/1.1 200 OK\r\n";
                response_body << "Content-Type: application/json\r\n"; 
                response_body << "Access-Control-Allow-Origin: *\r\n";
                
                std::istream response_stream1{&proxy_buff};
                std::string allLine,line;
                bool body_started = false;
                while (std::getline(response_stream1, line))
                {
                    if(line == "\r"&&!body_started)
                    {
                        body_started = true; // 开始读取body
                        continue; // 跳过空行
                    }
                    if(body_started)
                        allLine += line;
                }
                try{
                    nlohmann::json json_data = nlohmann::json::parse(allLine);
                    std::string body = json_data.dump();
                    response_body << "Content-Length: " << body.length() << "\r\n";
                    response_body << "connection: keep-alive\r\n";
                    response_body << "\r\n"; // 结束头部
                    response_body <<  body;
                    write_response(response_body.str());
                    }catch (nlohmann::json::parse_error& e) {
                        std::cerr << "JSON parse error: " << e.what() << "\n";
                        write_response(
                            build_http_response(500, "text/plain", "Internal server error: JSON parse error"));
                        return;
                    }
                });
            // read body
            //std::cout << "Response received: " << line << "\n";
        } else if (e != boost::asio::error::operation_aborted) {
            server.stop(self);
        }
    });
    timer_.expires_after(std::chrono::seconds(5));
                   
}
void session::handle_proxy_request(const std::string& url)
{
    BOOST_LOG_TRIVIAL(info) << "Proxy request: " << url;

    // 解析目标服务器地址和路径
    std::string target_host = url_get_param(url, "host");
    std::string target_path = url_get_param_ignore(url, "path");
    //std::string target_path = url_get_param(url, "path");


    if (target_host.empty() || target_path.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Invalid proxy request: missing host or path";
        return ;
    }

    try {
       
        SocketPtr socket_ptr = nullptr;
        auto it = m_proxy_sockets.find(target_host);
            if(it != m_proxy_sockets.end() && it->second->is_open())
            {
                socket_ptr = it->second;
            }else{
                socket_ptr = std::make_shared<tcp::socket>(server.io_service);
                m_proxy_sockets.emplace(target_host,socket_ptr);
            }
        
        socket_ptr->async_connect(
            tcp::endpoint(boost::asio::ip::address::from_string(target_host), 81),
            [this,socket_ptr,target_host,target_path](const boost::system::error_code& ec) {
                if (!ec) {
                    // 连接成功，可以继续使用 socket_ptr
                    timer_.cancel();
                    
                     this->do_write_proxy(socket_ptr,target_host, target_path);
                     this->do_read(socket_ptr);
                }else{
                    std::cerr << "Error connecting to target host: " << ec.message() << "\n";
                    write_response(
                        build_http_response(500, "text/plain", "Internal server error: Unable to connect to target host"));
                }
            });
        timer_.expires_after(std::chrono::seconds(5));
        timer_.async_wait([this,socket_ptr](std::error_code ec) {
            if (!ec) {
                std::cout << "Operation timed out!" << std::endl;
                //write_response(
                //        build_http_response(500, "text/plain", "timeout: Unable to connect to target host"));
                
                socket_ptr->close(); // 超时后关闭socket连接
               // 超时取消后的清理工作（如果有）
            } else {
                // 处理其他错误情况（理论上不应该发生）
            }
        });

        // 返回代理响应
        return ;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Proxy request failed: " << e.what();
        return ;
    }
}

std::shared_ptr<HttpServer::Response> HttpServer::creality_handle_request(const std::string& url)
{
    BOOST_LOG_TRIVIAL(warning) << "creality server: get_response:"<<url;
    std::string path;
    std::string params;
    parse_url(url, path, params);
    boost::filesystem::path currentPath = boost::filesystem::path(resources_dir()).append("web");
    
    if (path.find("/resources") == 0) {
        currentPath = boost::filesystem::path(resources_dir()).parent_path();
    }
    if (path.find("/creality_presets") == 0) {
        currentPath = fs::temp_directory_path();
    }
    if(path.find("/login") == 0) {
        std::string   result_url = url_get_param(url, "result_url");
        std::string   code       = url_get_param(url, "code");
         std::string location_str = (boost::format("%1%?result=success") % result_url).str();
        Http::set_extra_headers(wxGetApp().get_extra_header());
        std::string base_url              = get_cloud_api_url();
        Http http = Http::post(base_url + "/api/cxy/account/v2/oauthLogin");
        json        j;
        j["code"]  = code;
        j["clientId"]  = "f9c302ecc29c59a0a6e921ff39a073ca";
        j["redirecturi"]  = (boost::format("https://www.crealitycloud.com/oauth?back_url=http://localhost:%1%/login") % wxGetApp().get_server_port()).str();;
        if(code==""){
            location_str = (boost::format("%1%?result=fail?code=1000") % result_url).str();
            return std::make_shared<ResponseRedirect>(location_str);
        }
        //boost::uuids::uuid uuid = boost::uuids::random_generator()();
        http.header("Content-Type", "application/json")
            //              .header("__CXY_REQUESTID_", to_string(uuid))
                        .timeout_connect(5)
                        .timeout_max(15)
                        .set_post_body(j.dump())
                        .on_complete([&](std::string body, unsigned status) {
                            if(status!=200){
                                location_str = (boost::format("%1%?result=fail") % result_url).str();
                                return false;
                            }
                            std::stringstream oss;
                            try{
                                json j = json::parse(body);
                                int code = j["code"];
                                if(code==0)
                                {
                                    std::string region = wxGetApp().app_config->get("region");
                                    auto user_file = fs::path(data_dir()).append("user_info.json");
                                    json data_node = j["result"];
                                    json r;
                                    r["token"] = data_node["token"];
                                    r["nickName"] = data_node["user_info"]["nickName"];
                                    r["avatar"] = data_node["user_info"]["avatar"];
                                    r["userId"] = data_node["userId"];
                                    r["region"] = region;
                                    boost::nowide::ofstream c;
                                    c.open(user_file.string(), std::ios::out | std::ios::trunc);
                                    c << std::setw(4) << r << std::endl;
                                    c.close();
                                    UserInfo user;
                                    user.token = data_node["token"];
                                    user.nickName = data_node["user_info"]["nickName"];
                                    user.avatar = data_node["user_info"]["avatar"];
                                    user.userId = data_node["userId"];
                                    GUI::wxGetApp().CallAfter([user]() { wxGetApp().post_login_status_cmd(true, user); });
                                }else{
                                    GUI::wxGetApp().CallAfter([]() { wxGetApp().post_login_status_cmd(false, {}); });
                                    location_str = (boost::format("%1%?result=fail&code=%2%") % result_url % code).str();
                                }
                            }catch(std::exception& e)
                            {
                                GUI::wxGetApp().CallAfter([]() { wxGetApp().post_login_status_cmd(false, {}); });
                                location_str = (boost::format("%1%?result=fail") % result_url).str();
                                return false;
                            }
                            
                            return true;
                            
                        }).perform_sync();        
        
        return std::make_shared<ResponseRedirect>(location_str);
    }
    

    std::string request_path = currentPath.append(path=="/"?"index.html":path).string();
    try{
        fs::path file_path(request_path);

        if (!fs::exists(file_path) ||!fs::is_regular_file(file_path)) {
            // 文件不存在或者不是常规文件，返回404错误
            return std::make_shared<ResponseNotFound>();
        }

        boost::nowide::ifstream file(file_path.string(),std::ios::binary);
        if(!file)
        {
            return std::make_shared<ResponseNotFound>();
        }
        std::string file_content =  std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        //file.close();

        std::string file_extension = fs::extension(file_path);
        std::string content_type = get_content_type(file_extension);
        std::string http_response = build_http_response(200, content_type, file_content);

        return std::make_shared<ResponseStaticFile>(http_response);
    } catch (std::exception& e) {
        std::cerr << "Exception in connection handler: " << e.what() << std::endl;
        // 发生异常返回500错误给客户端
        std::string error_response = build_http_response(500, "text/plain", "Internal server error");
        try {
            return std::make_shared<ResponseNotFound>();
        } catch (std::exception& inner_e) {
            std::cerr << "Error sending error response: " << inner_e.what() << std::endl;
            return std::make_shared<ResponseNotFound>();
        }
    }
}
std::shared_ptr<HttpServer::Response> HttpServer::bbl_auth_handle_request(const std::string& url)
{
    BOOST_LOG_TRIVIAL(info) << "thirdparty_login: get_response";

    if (boost::contains(url, "access_token")) {
        std::string   redirect_url           = url_get_param(url, "redirect_url");
        std::string   access_token           = url_get_param(url, "access_token");
        std::string   refresh_token          = url_get_param(url, "refresh_token");
        std::string   expires_in_str         = url_get_param(url, "expires_in");
        std::string   refresh_expires_in_str = url_get_param(url, "refresh_expires_in");
        NetworkAgent* agent                  = wxGetApp().getAgent();

        unsigned int http_code;
        std::string  http_body;
        int          result = agent->get_my_profile(access_token, &http_code, &http_body);
        if (result == 0) {
            std::string user_id;
            std::string user_name;
            std::string user_account;
            std::string user_avatar;
            try {
                json user_j = json::parse(http_body);
                if (user_j.contains("uidStr"))
                    user_id = user_j["uidStr"].get<std::string>();
                if (user_j.contains("name"))
                    user_name = user_j["name"].get<std::string>();
                if (user_j.contains("avatar"))
                    user_avatar = user_j["avatar"].get<std::string>();
                if (user_j.contains("account"))
                    user_account = user_j["account"].get<std::string>();
            } catch (...) {
                ;
            }
            json j;
            j["data"]["refresh_token"]      = refresh_token;
            j["data"]["token"]              = access_token;
            j["data"]["expires_in"]         = expires_in_str;
            j["data"]["refresh_expires_in"] = refresh_expires_in_str;
            j["data"]["user"]["uid"]        = user_id;
            j["data"]["user"]["name"]       = user_name;
            j["data"]["user"]["account"]    = user_account;
            j["data"]["user"]["avatar"]     = user_avatar;
            agent->change_user(j.dump());
            if (agent->is_user_login()) {
                wxGetApp().request_user_login(1);
            }
            GUI::wxGetApp().CallAfter([] { wxGetApp().ShowUserLogin(false); });
            std::string location_str = (boost::format("%1%?result=success") % redirect_url).str();
            return std::make_shared<ResponseRedirect>(location_str);
        } else {
            std::string error_str    = "get_user_profile_error_" + std::to_string(result);
            std::string location_str = (boost::format("%1%?result=fail&error=%2%") % redirect_url % error_str).str();
            return std::make_shared<ResponseRedirect>(location_str);
        }
    } else {
        return std::make_shared<ResponseNotFound>();
    }
}

void HttpServer::ResponseStaticFile::write_response(std::stringstream& ssOut)
{
    ssOut << m_http_response;
}
void HttpServer::ResponseNotFound::write_response(std::stringstream& ssOut)
{
    const std::string sHTML = "<html><body><h1>404 Not Found</h1><p>There's nothing here.</p></body></html>";
    ssOut << "HTTP/1.1 404 Not Found" << std::endl;
    ssOut << "content-type: text/html" << std::endl;
    ssOut << "content-length: " << sHTML.length() << std::endl;
    ssOut << std::endl;
    ssOut << sHTML;
}

void HttpServer::ResponseRedirect::write_response(std::stringstream& ssOut)
{
    const std::string sHTML = "<html><body><p>redirect to url </p></body></html>";
    ssOut << "HTTP/1.1 302 Found" << std::endl;
    ssOut << "Location: " << location_str << std::endl;
    ssOut << "content-type: text/html" << std::endl;
    ssOut << "content-length: " << sHTML.length() << std::endl;
    ssOut << std::endl;
    ssOut << sHTML;
}

} // GUI
} //Slic3r
