#ifndef Client_4408_H
#define Client_4408_H

#include "RemotePrinter.h"
#include "basickernelexport.h"
#include "RemotePrinterSession.h"

#include <QTimer>
#include <functional>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace creative_kernel
{
	class BASIC_KERNEL_API WsClient : public std::enable_shared_from_this<WsClient>
	{
	public:
		explicit WsClient(net::io_context& ioc, const std::string& ip, const std::string& strMac, const int& port);

	public:
		void start();
		void stop(std::function<void()> callback);
		void sendTextMessage(const std::string& message);
		std::function<void(const RemotePrinter&)> infoCallback = nullptr;
		std::function<void(const std::string&, const std::string&)> fileCallback = nullptr;
		std::function<void(std::string, std::string)> historyFileListCb = nullptr;
		std::function<void(const std::string&, const std::string&)> videoCallback = nullptr;
		//std::function<void(const std::string&, const std::string&)> materialBoxCb = nullptr;

	private:
		void onResolve(beast::error_code ec, tcp::resolver::results_type results);
		void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
		void onHandshake(beast::error_code ec);
		void onWriteGetfilelist(beast::error_code ec, std::size_t bytes_transferred);
		void onRead(beast::error_code ec, std::size_t bytes_transferred);
		void onClose(beast::error_code ec, std::function<void()> callback);
		void onWrite(beast::error_code ec, std::size_t bytes_transferred);
		void onFail(beast::error_code ec, char const* what);
		void ParseMsg(const std::string& message);

	private:
		tcp::resolver m_resolver;
		beast::flat_buffer m_readBuffer;

		std::string m_serverIp;
		std::vector<std::string> m_writeQueue;
		std::unique_ptr<websocket::stream<beast::tcp_stream>> m_ws;

		RemotePrinter m_printer;
		int m_serverPort = 9999;
		bool needStop = false;
		time_t m_offlineTime = 0;
	};
}


#endif