#ifndef slic3r_GUI_DeviceVideoDownloader_hpp_
#define slic3r_GUI_DeviceVideoDownloader_hpp_

#include "TimeLapseShareTypes.hpp"

#include <atomic>

namespace Slic3r { namespace GUI { namespace TimeLapseShare {

class DeviceVideoDownloader final
{
public:
    // The callback runs on the calling thread. Setting cancel to true aborts
    // libcurl at its next progress or write callback.
    static DownloadResult Download(const DownloadRequest& request, const std::atomic<bool>& cancel, ProgressCallback progress_callback = {});
};

}}} // namespace Slic3r::GUI::TimeLapseShare

#endif /* slic3r_GUI_DeviceVideoDownloader_hpp_ */
