#include "RemotePrinterManager.hpp"
#include <future>
#include <string>

namespace RemotePrint {

size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream)
{
    std::ifstream* file = static_cast<std::ifstream*>(stream);
    file->read(static_cast<char*>(ptr), size * nmemb);
    return file->gcount();
}

int progress_callback(void* ptr, curl_off_t totalToDownload, curl_off_t nowDownloaded, curl_off_t totalToUpload, curl_off_t nowUploaded)
{
    auto* progressCallback = static_cast<std::function<void(float)>*>(ptr);
    if (totalToUpload > 0 && progressCallback) {
        (*progressCallback)(static_cast<float>(nowUploaded) / totalToUpload * 100.0f);
    }
    return 0;
}

RemotePrinterManager::RemotePrinterManager()
{
//     m_pLanPrinterInterface = new LanPrinterInterface();
//     m_pOctoPrinterInterface = new OctoPrintInterface();
//     m_pKlipperInterface = new KlipperInterface();
    m_pKlipper4408Interface = new Klipper4408Interface();
    m_pKlipperCXInterface = new KlipperCXInterface();

    auto	t = std::thread(&RemotePrinterManager::uploadThread, this);
	t.detach();
}

RemotePrinterManager::~RemotePrinterManager()
{
    m_bExit = true;

//     delete m_pLanPrinterInterface;
//     delete m_pOctoPrinterInterface;
//     delete m_pKlipperInterface;
    delete m_pKlipper4408Interface;
    delete m_pKlipperCXInterface;
}

void RemotePrinterManager::uploadThread() 
{
    while (!m_bExit) {
        std::unique_lock<std::mutex> lock(m_mtxUpload);
        m_cvUpload.wait(lock, [this] { return !m_uploadTasks.empty() || m_bExit; });

        if (m_bExit) break;

        auto task = std::move(m_uploadTasks.front());
        m_uploadTasks.pop_front();
        lock.unlock();

        auto ipAddress = std::get<0>(task);
        auto filename = std::get<1>(task);
        auto filePath = std::get<2>(task);
        auto progress_call_back = std::get<3>(task);
        auto error_call_back = std::get<4>(task);
        auto onCompleteCall = std::get<5>(task);

        pushFile(ipAddress, filename, filePath, progress_call_back, error_call_back, onCompleteCall);
    }
}

void RemotePrinterManager::pushUploadTasks(const std::string& ipAddress, const std::string& fileName, const std::string& filePath, std::function<void(std::string, float)> progressCallback, std::function<void(std::string, int)> uploadStatusCallback, std::function<void(std::string, std::string)> onCompleteCallback) 
{
    std::lock_guard<std::mutex> lock(m_mtxUpload);
    m_uploadTasks.emplace_back(ipAddress, fileName, filePath, progressCallback, uploadStatusCallback, onCompleteCallback);
    m_cvUpload.notify_one();
}
void RemotePrinterManager::cancelUpload(const std::string& ipAddress) { 
    RemotePrinerType printerType = determinePrinterType(ipAddress); 
    switch (printerType) {
    case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
        m_pKlipper4408Interface->cancelSendFileToDevice();
        break;
    default: break;
    }
}

void RemotePrinterManager::pushFile(const std::string& ipAddress, const std::string& fileName, const std::string& filePath, 
    std::function<void(std::string, float)> progressCallback, 
    std::function<void(std::string, int)> uploadStatusCallback,
    std::function<void(std::string, std::string)> onCompleteCallback)
{
    RemotePrinerType printerType = determinePrinterType(ipAddress);

    auto sendFile = [&](auto* printerInterface, auto&&... args) {
        std::future<void> future = printerInterface->sendFileToDevice(std::forward<decltype(args)>(args)...,
            [=](float progress) {
                if (progressCallback) {
                    progressCallback(ipAddress, progress);
                }
            },
            [=](int statusCode) {
                if (uploadStatusCallback) {
                    uploadStatusCallback(ipAddress, statusCode);
                }
            },
            [=](std::string body){
                if(onCompleteCallback)onCompleteCallback(ipAddress, body);
            }
        );
        while (true) {
            auto status = future.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                break;
            }
        }
    };

    switch (printerType)
    {
        case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
            sendFile(m_pLanPrinterInterface, ipAddress, fileName, filePath);
            break;
        case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
            m_pOctoPrinterInterface->sendFileToDevice(ipAddress, "", filePath);
            break;
        case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
            sendFile(m_pKlipperInterface, ipAddress, 80, fileName, filePath);
            break;
        case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
            sendFile(m_pKlipper4408Interface, ipAddress, 80, fileName, filePath);
            break;
        case RemotePrinerType::REMOTE_PRINTER_TYPE_CX:
            sendFile(m_pKlipperCXInterface, ipAddress, 80, fileName, filePath);
            break;
        default:
            break;
    }
}

RemotePrinerType RemotePrinterManager::determinePrinterType(const std::string& ipAddress)
{
    if(ipAddress.find('.') !=-1)
        return RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;

    return RemotePrinerType::REMOTE_PRINTER_TYPE_CX;
}
}
