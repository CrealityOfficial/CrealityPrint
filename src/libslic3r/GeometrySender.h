#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#else
#include <cerrno>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace GeometryDebugerSender
{
struct Point3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Color
{
    int r = -1;
    int g = -1;
    int b = -1;

    static Color rgb(int red, int green, int blue)
    {
        return Color{red, green, blue};
    }

    bool isValid() const
    {
        return r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255;
    }
};

using Polyline = std::vector<Point3>;
using PolylineGroup = std::vector<Polyline>;
using PointSet = std::vector<Point3>;
using PointSetGroup = std::vector<PointSet>;
using TriangleFacet = std::array<Point3, 3>;
using TriangleSoup = std::vector<TriangleFacet>;
using TriangleSoupGroup = std::vector<TriangleSoup>;

namespace detail
{
#ifdef _WIN32
using Socket = SOCKET;
static constexpr Socket InvalidSocket = INVALID_SOCKET;
#else
using Socket = int;
static constexpr Socket InvalidSocket = -1;
#endif

inline void closeSocket(Socket socket)
{
#ifdef _WIN32
    ::closesocket(socket);
#else
    ::close(socket);
#endif
}

inline int lastSocketError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline std::string escapeJson(const std::string& text)
{
    std::string escaped;
    escaped.reserve(text.size() + 8);

    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }

    return escaped;
}

inline void appendPoint(std::ostringstream& stream, const Point3& point)
{
    stream << '[' << point.x << ',' << point.y << ',' << point.z << ']';
}

inline void appendColor(std::ostringstream& stream, const Color& color)
{
    if (color.isValid())
    {
        stream << ",\"color\":[" << color.r << ',' << color.g << ',' << color.b << ']';
    }
}

inline void appendGroupColor(std::ostringstream& stream, const Color& color)
{
    if (color.isValid())
    {
        stream << "\"color\":[" << color.r << ',' << color.g << ',' << color.b << "],";
    }
}

inline void appendPointArray(std::ostringstream& stream, const std::vector<Point3>& points)
{
    stream << '[';
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        if (i > 0)
        {
            stream << ',';
        }
        appendPoint(stream, points[i]);
    }
    stream << ']';
}

inline void appendFacetArray(std::ostringstream& stream, const TriangleSoup& facets)
{
    stream << '[';
    for (std::size_t i = 0; i < facets.size(); ++i)
    {
        if (i > 0)
        {
            stream << ',';
        }

        stream << '[';
        appendPoint(stream, facets[i][0]);
        stream << ',';
        appendPoint(stream, facets[i][1]);
        stream << ',';
        appendPoint(stream, facets[i][2]);
        stream << ']';
    }
    stream << ']';
}

inline void appendPolylineGroup(std::ostringstream& stream, const PolylineGroup& polylines, const std::vector<Color>& colors)
{
    stream << '[';
    for (std::size_t i = 0; i < polylines.size(); ++i)
    {
        if (i > 0)
        {
            stream << ',';
        }

        stream << '{';
        if (i < colors.size())
        {
            appendGroupColor(stream, colors[i]);
        }
        stream << "\"vertices\":";
        appendPointArray(stream, polylines[i]);
        stream << '}';
    }
    stream << ']';
}

inline void appendPointSetGroup(std::ostringstream& stream, const PointSetGroup& pointSets, const std::vector<Color>& colors)
{
    stream << '[';
    for (std::size_t i = 0; i < pointSets.size(); ++i)
    {
        if (i > 0)
        {
            stream << ',';
        }

        stream << '{';
        if (i < colors.size())
        {
            appendGroupColor(stream, colors[i]);
        }
        stream << "\"vertices\":";
        appendPointArray(stream, pointSets[i]);
        stream << '}';
    }
    stream << ']';
}

inline void appendTriangleSoupGroup(std::ostringstream& stream, const TriangleSoupGroup& triangleSoups, const std::vector<Color>& colors)
{
    stream << '[';
    for (std::size_t i = 0; i < triangleSoups.size(); ++i)
    {
        if (i > 0)
        {
            stream << ',';
        }

        stream << '{';
        if (i < colors.size())
        {
            appendGroupColor(stream, colors[i]);
        }
        stream << "\"facets\":";
        appendFacetArray(stream, triangleSoups[i]);
        stream << '}';
    }
    stream << ']';
}
}

class GeometrySender
{
public:
    static GeometrySender& instance()
    {
        static GeometrySender sender;
        return sender;
    }

    GeometrySender(const GeometrySender&) = delete;
    GeometrySender& operator=(const GeometrySender&) = delete;

    bool configure(const std::string& host, std::uint16_t port)
    {
        if (host.empty() || port == 0)
        {
            m_lastError = "Host must not be empty and port must not be zero";
            return false;
        }

        close();
        m_host = host;
        m_port = port;
        return true;
    }

    bool sendPolylineGroupObject(
        const std::string& name,
        const PolylineGroup& polylines,
        const std::vector<Color>& colors = std::vector<Color>())
    {
        if (!validatePointCollectionGroups(name, polylines, colors, 2, "Polyline"))
        {
            return false;
        }

        std::ostringstream json;
        json << "{\"name\":\"" << detail::escapeJson(name) << "\",\"type\":\"polyline\",\"groups\":";
        detail::appendPolylineGroup(json, polylines, colors);
        json << '}';
        return sendJsonLine(json.str());
    }

    bool sendPointSetGroupObject(
        const std::string& name,
        const PointSetGroup& pointSets,
        const std::vector<Color>& colors = std::vector<Color>())
    {
        if (!validatePointCollectionGroups(name, pointSets, colors, 1, "Point set"))
        {
            return false;
        }

        std::ostringstream json;
        json << "{\"name\":\"" << detail::escapeJson(name) << "\",\"type\":\"pointset\",\"groups\":";
        detail::appendPointSetGroup(json, pointSets, colors);
        json << '}';
        return sendJsonLine(json.str());
    }

    bool sendTriangleSoupGroupObject(
        const std::string& name,
        const TriangleSoupGroup& triangleSoups,
        const std::vector<Color>& colors = std::vector<Color>())
    {
        if (name.empty() || triangleSoups.empty())
        {
            m_lastError = "Triangle soup object requires a name and at least one group";
            return false;
        }
        if (!colors.empty() && colors.size() != triangleSoups.size())
        {
            m_lastError = "Triangle soup color count must be zero or match group count";
            return false;
        }
        for (const TriangleSoup& triangleSoup : triangleSoups)
        {
            if (triangleSoup.empty())
            {
                m_lastError = "Each triangle soup group requires at least one triangle facet";
                return false;
            }
        }

        std::ostringstream json;
        json << "{\"name\":\"" << detail::escapeJson(name) << "\",\"type\":\"trianglesoup\",\"groups\":";
        detail::appendTriangleSoupGroup(json, triangleSoups, colors);
        json << '}';
        return sendJsonLine(json.str());
    }

    void close()
    {
        if (m_socket != detail::InvalidSocket)
        {
            detail::closeSocket(m_socket);
            m_socket = detail::InvalidSocket;
        }
    }

    const std::string& lastError() const
    {
        return m_lastError;
    }

private:
    GeometrySender()
    {
#ifdef _WIN32
        WSADATA data;
        m_socketReady = WSAStartup(MAKEWORD(2, 2), &data) == 0;
        if (!m_socketReady)
        {
            setLastSocketError("WSAStartup failed");
        }
#else
        m_socketReady = true;
#endif
    }

    ~GeometrySender()
    {
        close();

#ifdef _WIN32
        if (m_socketReady)
        {
            WSACleanup();
        }
#endif
    }

    template <typename Group>
    bool validatePointCollectionGroups(
        const std::string& name,
        const Group& groups,
        const std::vector<Color>& colors,
        std::size_t minimumPointCount,
        const char* label)
    {
        if (name.empty() || groups.empty())
        {
            m_lastError = std::string(label) + " object requires a name and at least one group";
            return false;
        }
        if (!colors.empty() && colors.size() != groups.size())
        {
            m_lastError = std::string(label) + " color count must be zero or match group count";
            return false;
        }
        for (const auto& group : groups)
        {
            if (group.size() < minimumPointCount)
            {
                m_lastError = std::string(label) + " group has too few points";
                return false;
            }
        }

        return true;
    }

    bool ensureConnected()
    {
        if (!m_socketReady)
        {
            m_lastError = "Socket subsystem is not initialized";
            return false;
        }

        if (m_socket != detail::InvalidSocket)
        {
            return true;
        }

        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        const std::string portText = std::to_string(m_port);
        const int status = getaddrinfo(m_host.c_str(), portText.c_str(), &hints, &result);
        if (status != 0)
        {
            m_lastError = "getaddrinfo failed: " + std::to_string(status);
            return false;
        }

        detail::Socket connectedSocket = detail::InvalidSocket;
        for (addrinfo* item = result; item != nullptr; item = item->ai_next)
        {
            connectedSocket = ::socket(item->ai_family, item->ai_socktype, item->ai_protocol);
            if (connectedSocket == detail::InvalidSocket)
            {
                continue;
            }

#ifdef _WIN32
            const int connectResult = ::connect(
                connectedSocket,
                item->ai_addr,
                static_cast<int>(item->ai_addrlen));
#else
            const int connectResult = ::connect(
                connectedSocket,
                item->ai_addr,
                static_cast<socklen_t>(item->ai_addrlen));
#endif
            if (connectResult == 0)
            {
                break;
            }

            detail::closeSocket(connectedSocket);
            connectedSocket = detail::InvalidSocket;
        }

        freeaddrinfo(result);

        if (connectedSocket == detail::InvalidSocket)
        {
            setLastSocketError("connect failed");
            return false;
        }

        m_socket = connectedSocket;
        return true;
    }

    bool sendJsonLine(const std::string& json)
    {
        if (!ensureConnected())
        {
            return false;
        }

        const std::string line = json + '\n';
        if (!sendAll(line.data(), static_cast<int>(line.size())))
        {
            close();
            return false;
        }

        return true;
    }

    bool sendAll(const char* data, int byteCount)
    {
        int sent = 0;
        while (sent < byteCount)
        {
            const auto result = ::send(m_socket, data + sent, byteCount - sent, 0);
            if (result < 0)
            {
                setLastSocketError("send failed");
                return false;
            }
            if (result == 0)
            {
                m_lastError = "send failed: connection closed";
                return false;
            }
            sent += static_cast<int>(result);
        }

        return true;
    }

    void setLastSocketError(const std::string& context)
    {
        m_lastError = context + ": " + std::to_string(detail::lastSocketError());
    }

    std::string m_host = "127.0.0.1";
    std::uint16_t m_port = 45454;
    detail::Socket m_socket = detail::InvalidSocket;
    std::string m_lastError;
    bool m_socketReady = false;
};

inline bool configure(const std::string& host, std::uint16_t port)
{
    return GeometrySender::instance().configure(host, port);
}

inline bool sendPolylineGroupObject(
    const std::string& name,
    const PolylineGroup& polylines,
    const std::vector<Color>& colors = std::vector<Color>())
{
    return GeometrySender::instance().sendPolylineGroupObject(name, polylines, colors);
}

inline bool sendPointSetGroupObject(
    const std::string& name,
    const PointSetGroup& pointSets,
    const std::vector<Color>& colors = std::vector<Color>())
{
    return GeometrySender::instance().sendPointSetGroupObject(name, pointSets, colors);
}

inline bool sendTriangleSoupGroupObject(
    const std::string& name,
    const TriangleSoupGroup& triangleSoups,
    const std::vector<Color>& colors = std::vector<Color>())
{
    return GeometrySender::instance().sendTriangleSoupGroupObject(name, triangleSoups, colors);
}

inline void close()
{
    GeometrySender::instance().close();
}

inline const std::string& lastError()
{
    return GeometrySender::instance().lastError();
}
}
