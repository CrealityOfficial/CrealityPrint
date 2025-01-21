#ifndef DOWNLOAD_SERVICE_H
#define DOWNLOAD_SERVICE_H

#include "../Utils/Http.hpp"
#include <memory>
#include <string>
#include <boost/filesystem.hpp>

namespace Slic3r {
namespace GUI {
class DownloadService : public std::enable_shared_from_this<DownloadService>
{
private:
	struct priv;
public:
    DownloadService(std::string                    ID,
                    std::string                    url,
                    const std::string&             filename,
                    const boost::filesystem::path& dest_folder,
                    std::function<void(int)>       progress_cb = nullptr,
                    std::function<void(std::string)>      complete_cb = nullptr,
                    std::function<void(void)>      error_cb    = nullptr);
    DownloadService(DownloadService&& other) noexcept;
    ~DownloadService();

	void get();
	void cancel();
	void pause();
	void resume();
private:
	std::unique_ptr<priv> p;
};
}
}
#endif
