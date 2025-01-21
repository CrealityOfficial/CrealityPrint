#ifndef CUSTOM_EVENTS_20241125_HPP
#define CUSTOM_EVENTS_20241125_HPP

#include <array>
#include <wx/debug.h>
#include <wx/event.h>
#include "slic3r/GUI/print_manage/DeviceDB.hpp"


namespace Slic3r {

namespace GUI {
class DeviceDataEvent : public wxEvent
{
public:
    DeviceDataEvent(wxEventType type, const RemotePrint::DeviceDB::Data& data, wxObject* origin = nullptr) : wxEvent(0, type), m_data(data)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
        SetEventObject(origin);
    }

    DeviceDataEvent(const DeviceDataEvent& event) : wxEvent(event), m_data(event.m_data) {}

    virtual wxEvent* Clone() const wxOVERRIDE { return new DeviceDataEvent(*this); }

    const RemotePrint::DeviceDB::Data& GetData() const { return m_data; }

private:
    RemotePrint::DeviceDB::Data m_data;
};
}
}

#endif // CUSTOM_EVENTS_20241125_HPP
