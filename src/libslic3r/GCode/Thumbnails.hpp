#ifndef slic3r_GCodeThumbnails_hpp_
#define slic3r_GCodeThumbnails_hpp_

#include "../Point.hpp"
#include "../PrintConfig.hpp"
#include "ThumbnailData.hpp"

#include <vector>
#include <memory>
#include <string_view>

#include <boost/beast/core/detail/base64.hpp>

namespace Slic3r {
    enum class ThumbnailError : int { InvalidVal, OutOfRange, InvalidExt };
    using ThumbnailErrors = enum_bitmask<ThumbnailError>;
    ENABLE_ENUM_BITMASK_OPERATORS(ThumbnailError);
}

namespace Slic3r::GCodeThumbnails {

struct CompressedImageBuffer
{
    void       *data { nullptr };
    size_t      size { 0 };

    unsigned int pos_s = 0; //for cr_png
    unsigned int pos_e = 0; //for cr_png
    unsigned int pos_h = 0; //for cr_png

    virtual ~CompressedImageBuffer() {}
    virtual std::string_view tag() const = 0;
};

std::string get_hex(const unsigned int input);
std::string rjust(std::string input, unsigned int width, char fill_char);
std::unique_ptr<CompressedImageBuffer> compress_thumbnail(const ThumbnailData &data, GCodeThumbnailsFormat format);
std::string get_error_string(const ThumbnailErrors& errors);
inline std::string encode_thumbnail_to_png_base64(const ThumbnailData &data)
{
    if (!data.is_valid())
        return {};

    auto compressed = compress_thumbnail(data, GCodeThumbnailsFormat::PNG);
    if (!compressed || !compressed->data || compressed->size == 0)
        return {};

    std::string encoded;
    encoded.resize(boost::beast::detail::base64::encoded_size(compressed->size));
    encoded.resize(boost::beast::detail::base64::encode((void *) encoded.data(), (const void *) compressed->data,
                                                        compressed->size));
    return encoded;
}


typedef std::vector<std::pair<GCodeThumbnailsFormat, Vec2d>> GCodeThumbnailDefinitionsList;
using namespace std::literals;
std::pair<GCodeThumbnailDefinitionsList, ThumbnailErrors> make_and_check_thumbnail_list(const std::string& thumbnails_string, const std::string_view def_ext = "PNG"sv);
std::pair<GCodeThumbnailDefinitionsList, ThumbnailErrors> make_and_check_thumbnail_list(const ConfigBase &config);


template<typename WriteToOutput, typename ThrowIfCanceledCallback>
inline void export_thumbnails_to_file(ThumbnailsGeneratorCallback&                                thumbnail_cb,
                                      int                                                         plate_id,
                                      const std::vector<std::pair<GCodeThumbnailsFormat, Vec2d>>& thumbnails_list,
                                      WriteToOutput                                               output,
                                      ThrowIfCanceledCallback                                     throw_if_canceled,
                                      int                          layer_count = 0)
{
    // Write thumbnails using base64 encoding
    if (thumbnail_cb == nullptr)
        return;
    short i = 0;
    bool first_ColPic = true;
    for (const auto& [format, size] : thumbnails_list) {
        static constexpr const size_t max_row_length = 78;
        ThumbnailsList                thumbnails     = thumbnail_cb(ThumbnailsParams{{size}, true, true, true, true, plate_id});
        for (const ThumbnailData &data : thumbnails) {
            if (data.is_valid()) {
                auto compressed = compress_thumbnail(data, format);
                if (compressed->data && compressed->size) {
                    if (format == GCodeThumbnailsFormat::BTT_TFT) {
                        // write BTT_TFT header
                        output((";" + rjust(get_hex(data.width), 4, '0') + rjust(get_hex(data.height), 4, '0') + "\r\n").c_str());
                        output((char *) compressed->data);
                        if (i == (thumbnails_list.size() - 1))
                            output("; bigtree thumbnail end\r\n\r\n");
                    }
                    else if (format == GCodeThumbnailsFormat::ColPic) {
                        if (first_ColPic) {
                            output((boost::format("\n\n;gimage:%s\n\n") % reinterpret_cast<char*>(compressed->data)).str().c_str());
                        } else {
                            output((boost::format("\n\n;simage:%s\n\n") % reinterpret_cast<char*>(compressed->data)).str().c_str());
                        }
                        first_ColPic = false;
                    } 
                    else {
                        output("; THUMBNAIL_BLOCK_START\n");
                        std::string encoded;
                        encoded.resize(boost::beast::detail::base64::encoded_size(compressed->size));
                        encoded.resize(boost::beast::detail::base64::encode((void *) encoded.data(), (const void *) compressed->data,
                                                                            compressed->size));                        
                        
                        if (format == GCodeThumbnailsFormat::CR_PNG)
                        {
                            output((boost::format("\n;\n; %s begin %d*%d %d %d %d %d\n") % compressed->tag() % data.width % data.height % encoded.size() % compressed->pos_s % compressed->pos_e % layer_count)
                                .str()
                                .c_str());
                        }else
                        {
                            output((boost::format("\n;\n; %s begin %dx%d %d\n") % compressed->tag() % data.width % data.height % encoded.size())
                                    .str()
                                    .c_str()); 
                        }                       
                        while (encoded.size() > max_row_length) {
                            output((boost::format("; %s\n") % encoded.substr(0, max_row_length)).str().c_str());
                            encoded = encoded.substr(max_row_length);
                        }

                        // Orca write remaining ecoded data
                        if (encoded.size() > 0)
                            output((boost::format("; %s\n") % encoded).str().c_str());

                        output((boost::format("; %s end\n") % compressed->tag()).str().c_str());
                        output("; THUMBNAIL_BLOCK_END\n\n");
                    }
                    throw_if_canceled();
                }
            }
        }
        i++;
    }
}

template<typename WriteToOutput>
inline void write_base64_image_block(const std::string& encoded,
                                     const char*        tag,
                                     const char*        block_tag,
                                     unsigned int       width,
                                     unsigned int       height,
                                     const char*        extra,
                                     WriteToOutput      output)
{
    if (encoded.empty())
        return;

    static constexpr const size_t max_row_length = 78;
    output((boost::format("; %s_BLOCK_START\n") % block_tag).str().c_str());
    output((boost::format("\n;\n; %s begin %dx%d %d format=png%s\n") % tag % width % height % encoded.size() % extra)
               .str()
               .c_str());

    std::string remaining = encoded;
    while (remaining.size() > max_row_length) {
        output((boost::format("; %s\n") % remaining.substr(0, max_row_length)).str().c_str());
        remaining = remaining.substr(max_row_length);
    }
    if (!remaining.empty())
        output((boost::format("; %s\n") % remaining).str().c_str());

    output((boost::format("; %s end\n") % tag).str().c_str());
    output((boost::format("; %s_BLOCK_END\n\n") % block_tag).str().c_str());
}

template<typename WriteToOutput, typename ThrowIfCanceledCallback>
inline void export_plate_object_bitmaps_to_file(const PlateObjectBitmapData& bitmaps,
                                                WriteToOutput                output,
                                                ThrowIfCanceledCallback      throw_if_canceled)
{
    if (!bitmaps.is_valid())
        return;

    const std::string top_png_base64 = encode_thumbnail_to_png_base64(bitmaps.top);
    write_base64_image_block(top_png_base64, "plate_top_image", "PLATE_TOP_IMAGE", bitmaps.top.width, bitmaps.top.height, "", output);
    throw_if_canceled();

    const std::string pick_png_base64 = encode_thumbnail_to_png_base64(bitmaps.pick);
    write_base64_image_block(pick_png_base64, "plate_pick_image", "PLATE_PICK_IMAGE", bitmaps.pick.width, bitmaps.pick.height, " id_encoding=rgb24_le", output);
    throw_if_canceled();
}
} // namespace Slic3r::GCodeThumbnails

#endif // slic3r_GCodeThumbnails_hpp_
