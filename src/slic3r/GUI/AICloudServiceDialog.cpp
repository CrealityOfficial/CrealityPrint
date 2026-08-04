#include "AICloudServiceDialog.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "OptionsGroup.hpp"
#include "Plater.hpp"
#include "PartPlate.hpp"
#include "Tab.hpp"

#include <wx/wx.h> 
#include <wx/numformatter.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/tooltip.h>
#include <wx/richtext/richtextctrl.h>

#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/GCode/SmoothSpeedAndAcc.hpp"
#include "MsgDialog.hpp"
#include "format.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/spin_mutex.h>
#include <tbb/task_group.h>
#include <future>

namespace Slic3r {
namespace GUI {

CommWithAICloudService::CommWithAICloudService() {}
CommWithAICloudService::~CommWithAICloudService() {}

int CommWithAICloudService::getAIRecommendationSupportGeneration(const std::string& host, const std::string& url, const std::string& recordId, std::list<CommWithAICloudService::STRespData>& response, std::string& errorInfo)
{
    int nRet = -1;
    m_taskState = ENTS_Result;
    std::string base_url = host;
    std::string profile_url = url;
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(base_url + profile_url);
    updateHttp(&http);
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    json jIn;
    jIn["recordId"] = recordId;
    BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService getAIRecommendationSupportGeneration start,url=" << url << ",recordId=" << recordId;
    http.header("Content-Type", "application/json")
        //.header("__CXY_REQUESTID_", to_string(uuid))
        //.header("Host", base_url + profile_url)
        //.form_add_file("input_file", filePath, boost::filesystem::path(filePath).filename().string())
        //.form_add("output_format", "json")
        .set_post_body(jIn.dump())
        .on_complete([&](std::string body, unsigned status) {
            json j = json();
            try {
                //BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService getAIRecommendationSupportGeneration body=" << body;
                j = json::parse(body);
                if (j.contains("code") && j["code"].get<int>() != 0) {
                    nRet = -2;
                    errorInfo = j["msg"].get<std::string>();
                    BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService getAIRecommendationSupportGeneration code fail,body=" << body;
                    return;
                }
                if (j.contains("result")) 
                {
                    std::string result = j["result"].get<std::string>();
                    json jResult = json::parse(result);
                    for (auto& [key, value] : jResult["parameters"].items()) {
                        auto& param = value;
                        if (param.contains("part_name") && param.contains("enable_support") && param.contains("support_type")) {
                            STRespData stRespData;
                            stRespData.dataType = ENRespDataType::ENRDT_SupportGeneration;
                            stRespData.object_id = key;
                            stRespData.part_name = wxString::FromUTF8(param["part_name"].get<std::string>()).ToStdString();
                            stRespData.enable_support = param["enable_support"].get<int>();
                            stRespData.support_type = param["support_type"].get<std::string>();
                            response.emplace_back(std::move(stRespData));
                        }
                    }
                }
            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService getAIRecommendationSupportGeneration parse body fail";
                nRet = -1;
                errorInfo = "parse json error";
                return;
            }
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService getAIRecommendationSupportGeneration fail.err=" << error << ",status=" << status;
            errorInfo = error.empty() ? body : error;
            nRet = -2;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) { int i = 0;
            i     = progress.ulnow;
        })
        .perform_sync();

    if (nRet == 0) {
        m_taskState = ENTS_OK;
    } else {
        m_taskState = ENTS_FAIL;
    }
    updateHttp(nullptr);

    return nRet;
}

int CommWithAICloudService::getAIRecommendationZseamPainting(const std::string& host, const std::string& url, const std::string& recordId, std::list<STRespData>& response, std::string& errorInfo) {
    int nRet = -1;
    m_taskState             = ENTS_Result;
    std::string base_url    = host;
    std::string profile_url = url;
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(base_url + profile_url);
    //updateHttp(&http);
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    json               jIn;
    jIn["recordId"] = recordId;
    BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService getAIRecommendationZseamPainting start,url=" << url << ",recordId=" << recordId;
    http.header("Content-Type", "application/json")
        //.header("__CXY_REQUESTID_", to_string(uuid))
        //.header("Host", base_url + profile_url)
        //.form_add_file("input_file", filePath, boost::filesystem::path(filePath).filename().string())
        //.form_add("output_format", "json")
        .set_post_body(jIn.dump())
        .on_complete([&](std::string body, unsigned status) {
            json j = json();
            try {
                //BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService getAIRecommendationZseamPainting body=" << body;
                j = json::parse(body);
                if (j.contains("code") && j["code"].get<int>() != 0) {
                    nRet      = -2;
                    errorInfo = j["msg"].get<std::string>();
                    BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService getAIRecommendationZseamPainting code fail,body=" << body;
                    return;
                }

                if (j.contains("result") /*&& j["result"].contains("models")*/) {
                    std::string result = j["result"].get<std::string>();
                    json jResult = json::parse(result);
                    for (auto& [key, value] : jResult["models"].items()) {
                        if (value.contains("block_seam_domains")) {
                            STRespData stRespData;
                            stRespData.dataType = ENRespDataType::ENRDT_ZseamPainting;
                            stRespData.object_id = key;
                            auto& domains = value["block_seam_domains"];
                            if (domains.is_array()) {
                                for (int i = 0; i < domains.size(); ++i) {
                                    stRespData.block_seam_domains.push_back(domains[i]);
                                }
                            }
                            response.emplace_back(std::move(stRespData));
                        }
                    }
                }
            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService getAIRecommendationZseamPainting parse body fail";
                nRet = -1;
                errorInfo = "parse json error";
                return;
            }
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService getAIRecommendationZseamPainting fail.err=" << error
                                     << ",status=" << status;
            errorInfo = error.empty() ? body : error;
            nRet      = -2;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            int i = 0;
            i     = progress.ulnow;
        })
        .perform_sync();
    if (nRet == 0) {
        m_taskState = ENTS_OK;
    } else {
        m_taskState = ENTS_FAIL;
    }
    updateHttp(nullptr);
    return nRet;
}

CommWithAICloudService::ENTaskState CommWithAICloudService::getTaskState() {
    return m_taskState;
}

void CommWithAICloudService::cancel()
{
    m_mutexHttp.lock();
    if (m_http) {
        m_http->cancel();
        BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService cancel";
    }
    m_mutexHttp.unlock();
}
void CommWithAICloudService::reset() {
    m_taskState = ENTS_NULL; 
}

void CommWithAICloudService::updateHttp(Http* http)
{
    m_mutexHttp.lock();
    m_http = http;
    m_mutexHttp.unlock();
}

int CommWithAICloudService::addTask(const std::string& host, const std::string& url, const std::string& filePath, STTaskRespData& stAddTaskRespData)
{
    int nRet = -1;
    m_taskState = ENTS_AddTask;
    std::string base_url    = getApiUrl();
    std::string profile_url = "/process_3mf";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(host + url);
    updateHttp(&http);
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService addTask start,url=" << url;
    http.header("Content-Type", "multipart/form-data")
        //.header("__CXY_REQUESTID_", to_string(uuid))
        //.header("Host", host)
        //.header("__CXY_UID_", "2949699353")
        //.header("__CXY_TOKEN_", "53d6e029ba442a247a0b3b67b7d38033c4fa06f5f0cf0d54f73b6e9e52bbaddd")
        .form_add_file("input_file", filePath, boost::filesystem::path(filePath).filename().string())
        .form_add("output_format", "json")
        //.set_post_body(j.dump())
        .on_complete([&](std::string body, unsigned status) {
            json j = json();
            try {
                j = json::parse(body);

                if (j.contains("code"))
                    stAddTaskRespData.code = j["code"].get<int>();
                if (j.contains("msg"))
                    stAddTaskRespData.msg = j["msg"].get<std::string>();
                if (j.contains("reqId"))
                    stAddTaskRespData.reqId = j["reqId"].get<std::string>();
                if (j.contains("result"))
                    stAddTaskRespData.result = j["result"].get<std::string>();

            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService addTask parse body fail,url=" << host + url
                                         << ",body=" << body;
                nRet      = -1;
                stAddTaskRespData.code = -1;
                stAddTaskRespData.msg = "parse json error";
                return;
            }
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService addTask fail.err=" << error << ",status=" << status
                                     << ",body=" << body << ",url=" << host + url;
            stAddTaskRespData.code = -2;
            stAddTaskRespData.msg  = error.empty() ? body : error;
            nRet      = -2;
            if (status == 403) {
                stAddTaskRespData.code = 403;
                stAddTaskRespData.msg = "Cloud service encountered an error. Please try again.";
                nRet = 403;
            } else if (status == 6) {
                stAddTaskRespData.msg = "No network. Please retry.";
            }
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            int i = 0;
            i     = progress.ulnow;
        })
        .perform_sync();
    updateHttp(nullptr);
    return nRet;
}
int CommWithAICloudService::detail(const std::string& host, const std::string& url, const std::string& recordId, STTaskRespData& stDetailTaskRespData)
{
    int nRet                = -1;
    m_taskState             = ENTS_Detail;
    std::string base_url    = getApiUrl();
    std::string profile_url = "/process_3mf";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(host + url);
    updateHttp(&http);
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    json jIn;
    jIn["recordId"] = recordId;
    BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService detail start,url=" << url << ",recorId=" << recordId;
    http.header("Content-Type", "application/json")
        //.header("__CXY_REQUESTID_", to_string(uuid))
        //.header("Host", host)
        //.header("__CXY_UID_", "2949699353")
        //.header("__CXY_TOKEN_", "53d6e029ba442a247a0b3b67b7d38033c4fa06f5f0cf0d54f73b6e9e52bbaddd")
        //.form_add_file("input_file", filePath, boost::filesystem::path(filePath).filename().string())
        //.form_add("output_format", "json")
        .set_post_body(jIn.dump())
        .on_complete([&](std::string body, unsigned status) {
            json j = json();
            try {
                j = json::parse(body);

                if (j.contains("code"))
                    stDetailTaskRespData.code = j["code"].get<int>();
                if (j.contains("msg"))
                    stDetailTaskRespData.msg = j["msg"].get<std::string>();
                if (j.contains("reqId"))
                    stDetailTaskRespData.reqId = j["reqId"].get<std::string>();
                if (j.contains("result")) {
                    if (j["result"].contains("state"))
                        stDetailTaskRespData.state = j["result"]["state"].get<int>();
                    if (j["result"].contains("waitingCountAhead"))
                        stDetailTaskRespData.waitingCountAhead = j["result"]["waitingCountAhead"].get<int>();
                    if (j["result"].contains("waitingCountTotal"))
                        stDetailTaskRespData.waitingCountTotal = j["result"]["waitingCountTotal"].get<int>();
                    if (j["result"].contains("waitingLeftTime"))
                        stDetailTaskRespData.waitingLeftTime = j["result"]["waitingLeftTime"].get<int>();
                    if (j["result"].contains("waitingTime"))
                        stDetailTaskRespData.waitingTime = j["result"]["waitingTime"].get<int>();
                }
                if (stDetailTaskRespData.code != 0) {
                    nRet = -1;
                    BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService detail code fail,body="<< body;
                    return;
                } else {
                    if (stDetailTaskRespData.state == 4) {
                        nRet = 4;
                        stDetailTaskRespData.msg = "detail fail";                        
                        if (j["result"].contains("errorMsg")) {
                            json jerr;
                            std::string ss = j["result"]["errorMsg"].get<std::string>();
                            jerr = json::parse(ss);
                            if (jerr.contains("error"))
                                stDetailTaskRespData.msg = jerr["error"].get<std::string>();
                        }
                        if (j["result"].contains("httpStatusCode")) {
                            int httpStatusCode = j["result"]["httpStatusCode"].get<int>();
                            if (httpStatusCode == 406) {
                                stDetailTaskRespData.msg = "Cloud service cannot process this 3MF—contains assemblies or too many mesh triangles.";
                            }
                        }
                        BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService detail code fail,body=" << body;
                        return;
                    } else if (stDetailTaskRespData.state == 5) {
                        nRet = 5;
                        stDetailTaskRespData.msg = "detail timeout";
                        BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService detail code fail,body=" << body;
                        return;
                    } else if (stDetailTaskRespData.state == 6) {
                        nRet = 6;
                        stDetailTaskRespData.msg = "detail terminate";
                        BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService detail code fail,body=" << body;
                        return;
                    }
                }

            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService detail parse body fail, host=" << host << ",url=" << url
                                         << ",body=" << body;
                nRet                   = -1;
                stDetailTaskRespData.code = -1;
                stDetailTaskRespData.msg  = "parse json error";
                return;
            }
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "AICloudService CommWithAICloudService detail fail.err=" << error << ",status=" << status
                                     << ",body=" << body << ",url=" << url;
            stDetailTaskRespData.code = -2;
            stDetailTaskRespData.msg  = error.empty()?body:error;
            nRet                   = -2;
            if (status == 6) {
                stDetailTaskRespData.msg = "No network. Please retry.";
            }
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {
            int i = 0;
            i     = progress.ulnow;
        })
        .perform_sync();
    updateHttp(nullptr);

    return nRet;
}
int CommWithAICloudService::result(const std::string& host, const std::string& url, const std::string& recordId)
{
    int nRet = -1;
    return nRet;
}

std::string CommWithAICloudService::getApiUrl() 
{
    return "http://ai-test.crealitycloud.cn";
}

AICloudService_ResultListItem::AICloudService_ResultListItem(wxWindow* parent, wxSize itemSize)
    : wxPanel(parent, wxID_ANY)
{
    wxString bgColor  = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    this->SetMinSize(wxSize(FromDIP(-1), itemSize.GetHeight()));
    this->SetMaxSize(wxSize(FromDIP(-1), itemSize.GetHeight()));
    this->SetBackgroundColour(bgColor);
    m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(m_mainSizer);
    m_modelName = new wxStaticText(this, wxID_ANY, _L("Model name"));
    m_modelName->SetMinSize(itemSize);
    m_modelName->SetMaxSize(itemSize);
    m_modelName->SetFont(Label::Body_16);
    m_modelName->SetForegroundColour(fgColor);
    m_mainSizer->Add(m_modelName, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    auto panel = new wxPanel(this);
    panel->SetMinSize(wxSize(FromDIP(1), itemSize.GetHeight()));
    panel->SetMaxSize(wxSize(FromDIP(1), itemSize.GetHeight()));
    panel->SetBackgroundColour("#505052");
    panel->SetWindowStyle(wxBORDER_NONE);
    m_mainSizer->Add(panel);
    m_beforeProcessing = new wxStaticText(this, wxID_ANY, _L("Before processing"));
    m_beforeProcessing->SetMinSize(itemSize);
    m_beforeProcessing->SetMaxSize(itemSize);
    m_beforeProcessing->SetFont(Label::Body_16);
    m_beforeProcessing->SetForegroundColour(fgColor);
    m_mainSizer->Add(m_beforeProcessing, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    panel = new wxPanel(this);
    panel->SetMinSize(wxSize(FromDIP(1), itemSize.GetHeight()));
    panel->SetMaxSize(wxSize(FromDIP(1), itemSize.GetHeight()));
    panel->SetBackgroundColour("#505052");
    panel->SetWindowStyle(wxBORDER_NONE);
    m_mainSizer->Add(panel);
    m_afterProcessing = new wxStaticText(this, wxID_ANY, _L("After processing"));
    m_afterProcessing->SetMinSize(itemSize);
    m_afterProcessing->SetMaxSize(itemSize);
    m_afterProcessing->SetFont(Label::Body_16);
    m_afterProcessing->SetForegroundColour(fgColor);
    m_mainSizer->Add(m_afterProcessing, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));

    this->Layout();
    this->Fit();
}

wxString AICloudService_ResultListItem::wrapText(const wxString& text, wxDC& dc, int maxWidth)
{
    if (text.IsEmpty()) {
        return text;
    }
    
    wxString result;
    wxString currentLine;
    size_t len = text.Length();
    
    for (size_t i = 0; i < len; ++i) {
        wxChar ch = text[i];
        wxString chString;
        chString += wxUniChar(ch);
        
        if (ch == '\n') {
            if (!currentLine.IsEmpty()) {
                result += currentLine;
                currentLine.Clear();
            }
            result += '\n';
            continue;
        }
        
        wxString testLine = currentLine + chString;
        wxSize textSize = dc.GetTextExtent(testLine);
        
        if (textSize.GetWidth() > maxWidth) {
            if (!currentLine.IsEmpty()) {
                bool isChineseChar = (ch >= 0x4E00 && ch <= 0x9FFF) || 
                                     (ch >= 0x3400 && ch <= 0x4DBF) || 
                                     (ch >= 0x20000 && ch <= 0x2A6DF);
                
                if (isChineseChar) {
                    result += currentLine;
                    result += '\n';
                    currentLine = chString;
                } else {
                    bool foundSpace = false;
                    for (int j = currentLine.Length() - 1; j >= 0; --j) {
                        if (currentLine[j] == ' ') {
                            result += currentLine.Left(j);
                            result += '\n';
                            currentLine = currentLine.Mid(j + 1) + chString;
                            foundSpace = true;
                            break;
                        }
                    }
                    
                    if (!foundSpace) {
                        result += currentLine;
                        result += '\n';
                        currentLine = chString;
                    }
                }
            } else {
                result += chString;
                result += '\n';
            }
        } else {
            currentLine = testLine;
        }
    }
    
    if (!currentLine.IsEmpty()) {
        result += currentLine;
    }
    
    return result;
}

void AICloudService_ResultListItem::updateData(const wxString& modelName, const wxString& beforProcessing, const wxString& afterProcessing)
{
    wxClientDC dc(this);
    dc.SetFont(Label::Body_16);
    
    int maxWidth = FromDIP(355);
    
    wxString wrappedModelName = wrapText(modelName, dc, maxWidth);

    m_modelName->SetLabelText(wrappedModelName);
    m_beforeProcessing->SetLabelText(beforProcessing);
    m_afterProcessing->SetLabelText(afterProcessing);
    
    m_modelName->Wrap(m_columnWidth);
    
    this->Layout();
}

AICloudService_ResultDialog::AICloudService_ResultDialog(wxWindow* parent)
    : DPIDialog(parent, wxID_ANY, _L("AI cloud service"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    build_dialog();
}

void AICloudService_ResultDialog::build_dialog()
{
    this->SetMinSize(wxSize(FromDIP(1138), FromDIP(557)));
    this->SetMaxSize(wxSize(FromDIP(1138), FromDIP(557)));
    wxString bgColor  = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor  = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    SetFont(wxGetApp().normal_font());
    wxGetApp().UpdateDlgDarkUI(this);
    this->SetBackgroundColour(bgColor2);

    // SetBackgroundColour(*wxWHITE);

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(470)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(470)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(17));
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    m_scrolled_window = new wxScrolledWindow(panel);
    m_scrolled_window->SetMinSize(wxSize(FromDIP(1130), FromDIP(470)));
    m_scrolled_window->SetMaxSize(wxSize(FromDIP(1130), FromDIP(470)));
    m_scrolled_window->SetBackgroundColour(wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6");
    m_scrolled_window->SetScrollbars(1, 20, 1, 2);
    m_scrolled_window->SetWindowStyle(wxBORDER_NONE);
    panelSizer->Add(m_scrolled_window);
    panel->SetSizer(panelSizer);

    m_scrolled_window_sizer = new wxBoxSizer(wxVERTICAL);
    auto item = new AICloudService_ResultListItem(m_scrolled_window, wxSize(FromDIP(355), FromDIP(49)));
    item->updateData(_L("Model name"), _L("Before processing"), _L("After processing"));
    m_scrolled_window_sizer->Add(item);
    
    m_scrolled_window->SetSizer(m_scrolled_window_sizer);

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(48)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(48)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxBOTTOM, FromDIP(1));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    Button* btnDiscard = new Button(panel, _L("Discard"));
    btnDiscard->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    btnDiscard->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    btnDiscard->SetBorderColorNormal(wxColour(wxGetApp().dark_mode() ? "#FFFFFF" : "#000000"));
    btnDiscard->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                             std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                             std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    btnDiscard->Bind(wxEVT_LEFT_DOWN, &AICloudService_ResultDialog::on_discard_btn_clicked, this);
    panelSizer->AddStretchSpacer();
    panelSizer->Add(btnDiscard, 1, wxLEFT | wxALIGN_CENTER_VERTICAL);
    Button* btnApply = new Button(panel, _L("Apply"));
    btnApply->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    btnApply->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    btnApply->SetBorderColorNormal(wxColour(wxGetApp().dark_mode() ? "#FFFFFF" : "#000000"));
    btnApply->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Pressed),
                                            std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Hovered),
                                            std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Normal)));
    btnApply->Bind(wxEVT_LEFT_DOWN, &AICloudService_ResultDialog::on_apply_btn_clicked, this);
    panelSizer->Add(btnApply, 1, wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(16));
    panelSizer->AddStretchSpacer();

    this->Layout();
    this->Fit();
}
void AICloudService_ResultDialog::updateData(const std::list<CommWithAICloudService::STRespData>& datas)
{
    if (datas.size() > 0) {
        wxSize panelsize(wxSize(FromDIP(356*3+2), FromDIP(1)));
        static std::map<int, std::string> s_keys_map_SupportType{{stNormalAuto, "normal(auto)"},
                                                           {stTreeAuto, "tree(auto)"},
                                                           {stNormal, "normal(manual)"},
                                                           {stTree, "tree(manual)"}};
        for (auto &data : datas)
        {
            //if (param.contains("part_name") && param.contains("enable_support") && param.contains("support_type")) 
            {
                wxString ssBefore = "";
                wxString ssAfter = "";
                std::string name = data.part_name;
                if (!data.object_id.empty() && data.object_id != "-1") {
                    const ModelObject* modelObject = getModelObjectBy3mfPartId(atoi(data.object_id.c_str()));
                    if (modelObject != nullptr) {
                        name = data.part_name.empty() ? wxString::FromUTF8(modelObject->name).ToStdString() : data.part_name;
                            ModelObject* mo = const_cast<ModelObject*>(modelObject);
                        if (data.dataType == CommWithAICloudService::ENRespDataType::ENRDT_SupportGeneration) {
                            const ConfigOptionBool* enable_support = dynamic_cast<const ConfigOptionBool*>(
                                mo->config.option("enable_support"));
                            if (enable_support) {
                                ssBefore = _L("Enable support") + ": " + (enable_support && enable_support->value ? _L("Yes") : _L("No"));
                            } else {
                                enable_support = dynamic_cast<const ConfigOptionBool*>(
                                    wxGetApp().preset_bundle->prints.get_edited_preset().config.option("enable_support"));
                                ssBefore = _L("Enable support") + ": " + (enable_support && enable_support->value ? _L("Yes") : _L("No"));
                            }
                            const ConfigOption* support_type_opt = mo->config.option("support_type");
                            ssBefore = ssBefore + "\n" + _L("Type")+":";
                            if (support_type_opt) {
                                auto support_type = static_cast<SupportType>(support_type_opt->getInt());
                                if (s_keys_map_SupportType.find(support_type) != s_keys_map_SupportType.end()) {
                                    ssBefore += _L(s_keys_map_SupportType[support_type]);
                                }
                            } else {
                                auto support_type = wxGetApp().preset_bundle->prints.get_edited_preset().config.opt_enum<SupportType>(
                                    "support_type");
                                if (s_keys_map_SupportType.find(support_type) != s_keys_map_SupportType.end()) {
                                    ssBefore += _L(s_keys_map_SupportType[support_type]);
                                }
                            }
                            wxString type = (data.support_type == "tree")   ? _L("tree(auto)") :
                                            (data.support_type == "normal") ? _L("normal(auto)") :
                                                                              _L(data.support_type);
                            ssAfter = _L("Enable support") + ": " + (data.enable_support == 1 ? _L("Yes") : _L("No")) + "\n" + _L("Type") +
                                              wxString(": ") + type;
                        } else if (data.dataType == CommWithAICloudService::ENRespDataType::ENRDT_ZseamPainting) {
                            if (mo->volumes[0]->seam_facets.has_facets(*mo->volumes[0], EnforcerBlockerType::BLOCKER)) {
                                ssBefore = _L("Seam painting") + ": "+ _L("Yes");
                            } else {
                                ssBefore = _L("Seam painting") + ": "+ _L("No");
                            }
                            ssAfter = _L("Seam painting") + ":";
                            if (data.block_seam_domains.size() == 0) {
                                ssAfter += _L("No");
                            } else {
                                ssAfter += _L("Yes");
                            }
                        }
                    }
                }

                auto panel = new wxPanel(m_scrolled_window);
                panel->SetMinSize(panelsize);
                panel->SetMaxSize(panelsize);
                panel->SetBackgroundColour("#505052");
                panel->SetWindowStyle(wxBORDER_NONE);
                m_scrolled_window_sizer->Add(panel);
                auto item = new AICloudService_ResultListItem(m_scrolled_window, wxSize(FromDIP(355), FromDIP(96)));
                item->updateData(name, ssBefore, ssAfter);
                m_scrolled_window_sizer->Add(item);
            }
        }
    }
}
void AICloudService_ResultDialog::updateDataToModel(const std::list<CommWithAICloudService::STRespData>& datas, std::function<void(int, int)> funProcessCb)
{
    static t_config_enum_values s_keys_map_SupportType{{"normal(auto)", stNormalAuto},
                                                       {"tree(auto)", stTreeAuto},
                                                       {"normal(manual)", stNormal},
                                                       {"tree(manual)", stTree},
                                                       {"tree", stTreeAuto},
                                                       {"normal", stNormalAuto}};

    int count = m_dataCount;
    
    // 使用TBB并行处理数据
    std::vector<CommWithAICloudService::STRespData> dataVec(datas.begin(), datas.end());
    tbb::spin_mutex progressMutex;
    std::atomic<int> totalProgress{0};
    
    tbb::parallel_for(tbb::blocked_range<size_t>(0, dataVec.size()), 
        [&](const tbb::blocked_range<size_t>& range) {
            int localProgress = 0;
            
            for (size_t i = range.begin(); i != range.end(); ++i) {
                auto& data = dataVec[i];
                if (data.object_id.empty() || data.object_id == "-1")
                    continue;
                //const ModelObject* modelObject = getModelObjectBy3mfPartId(atoi(data.object_id.c_str()));
                std::vector<const ModelObject*> vtMO;
                getModelObjectBy3mfPartId(atoi(data.object_id.c_str()), vtMO);
                for (size_t i = 0; i < vtMO.size(); ++i) {
                    const ModelObject* modelObject = vtMO[i];
                    if (modelObject != nullptr) {
                        if (data.dataType == CommWithAICloudService::ENRespDataType::ENRDT_SupportGeneration) {
                            ModelObject* mo = const_cast<ModelObject*>(modelObject);
                            mo->config.set_key_value("enable_support", new ConfigOptionBool(data.enable_support == 0 ? false : true));
                            if (s_keys_map_SupportType.find(data.support_type) != s_keys_map_SupportType.end()) {
                                mo->config.set_key_value("support_type", new ConfigOptionEnum<SupportType>(
                                                                             (SupportType) s_keys_map_SupportType[data.support_type]));
                            }
                            if (funProcessCb && i == vtMO.size() - 1) {
                                localProgress += 100;
                            }
                        } else if (data.dataType == CommWithAICloudService::ENRespDataType::ENRDT_ZseamPainting) {
                            blockerModelFacets(modelObject, data.block_seam_domains, [&](int step, int) {
                                // 进度回调在blockerModelFacets内部处理
                            });
                            if (i == vtMO.size() - 1) {
                                localProgress += data.block_seam_domains.size();
                            }
                        }
                    }
                }
            }
            
            // 批量更新进度，减少锁竞争
            if (funProcessCb && localProgress > 0) {
                tbb::spin_mutex::scoped_lock lock(progressMutex);
                totalProgress += localProgress;
                funProcessCb(totalProgress.load(), count);
            }
        });

    for (auto tab : wxGetApp().model_tabs_list) {
        if (tab->type() == Preset::TYPE_MODEL) {
            TabPrintModel* tabPrintModel = dynamic_cast<TabPrintModel*>(tab);
            if (tabPrintModel != nullptr /*&& tabPrintModel->get_active_page() != nullptr*/)
                tabPrintModel->update_model_config();
        }
    }
    if (funProcessCb) {
        funProcessCb(count, count);
    }
}
int AICloudService_ResultDialog::getDataCount(const std::list<CommWithAICloudService::STRespData>& datas)
{
    m_dataCount = 0;
    for (auto data : datas) {
        if (data.object_id.empty() || data.object_id == "-1")
            continue;
        if (data.dataType == CommWithAICloudService::ENRespDataType::ENRDT_SupportGeneration) {
            m_dataCount += 100;
        } else if (data.dataType == CommWithAICloudService::ENRespDataType::ENRDT_ZseamPainting) {
            m_dataCount += data.block_seam_domains.size();
        }
    }
    return m_dataCount;
}

void AICloudService_ResultDialog::on_discard_btn_clicked(wxEvent&) { EndModal(wxID_CANCEL); }
void AICloudService_ResultDialog::on_apply_btn_clicked(wxEvent&) { EndModal(wxID_OK); }
const ModelObject* AICloudService_ResultDialog::getModelObjectBy3mfPartId(int partId)
{
    const ModelObject* modelObject = nullptr;

    {
        typedef std::map<const ModelVolume*, int> VolumeToObjectIDMap;
        struct ObjectData
        {
            ModelObject const*  object;
            int                 backup_id;
            int                 object_id = 0;
            std::string         sub_path;
            bool                share_mesh = false;
            VolumeToObjectIDMap volumes_objectID;
        };
        typedef std::map<ModelObject const*, ObjectData> ObjectToObjectDataMap;
        std::map<void const*, std::pair<ObjectData*, ModelVolume const*>> m_shared_meshes;
        std::map<ModelVolume const*, std::pair<std::string, int>> m_volume_paths;
        ObjectToObjectDataMap objects_data;
        unsigned int object_id = 1;
        auto model = wxGetApp().model();
        for (ModelObject* obj : wxGetApp().model().objects) {
            if (obj == nullptr)
                continue;
            ObjectToObjectDataMap::iterator object_it = objects_data.begin();
            int backup_id = const_cast<Model&>(wxGetApp().model()).get_object_backup_id(*obj);
            object_it = objects_data.insert({obj, {obj, backup_id} }).first;
            auto & object_data = object_it->second;
            auto& volumes_objectID = object_data.volumes_objectID;
            unsigned int volume_id = object_id, volume_count = 0;
            for (ModelVolume* volume : obj->volumes) {
                if (volume == nullptr)
                    continue;
                volume_count++;
                {
                    auto iter = m_shared_meshes.find(volume->mesh_ptr().get());
                    if (iter != m_shared_meshes.end()) {
                        auto data = iter->second.first;
                        m_volume_paths.insert({volume, {data->sub_path, data->volumes_objectID.find(iter->second.second)->second}});
                        volumes_objectID.insert({volume, 0});
                        continue;
                    }
                    m_shared_meshes.insert({volume->mesh_ptr().get(), {&object_data, volume}});
                }
                volumes_objectID.insert({volume, volume_id});
                volume_id++;
            }
            object_id = volume_id;
            object_data.object_id = object_id;
            ++object_id;
        }
        for (const ObjectToObjectDataMap::value_type& obj_metadata : objects_data) {
            auto object_data = obj_metadata.second;
            const ModelObject* obj = object_data.object;
            if (obj != nullptr) {
                for (const ModelVolume* volume : obj_metadata.second.object->volumes) {
                    if (volume != nullptr) {
                        const VolumeToObjectIDMap&  objectIDs = obj_metadata.second.volumes_objectID;
                        VolumeToObjectIDMap::const_iterator it = objectIDs.find(volume);
                        if (it != objectIDs.end()) {
                            int volume_id = it->second;
                            if (/*m_share_mesh && */volume_id == 0)
                                volume_id = m_volume_paths.find(volume)->second.second;
                            if (volume_id == partId) {
                                modelObject = obj_metadata.second.object;
                                break;
                            }
                        }
                    }
                }
                if (modelObject != nullptr) {
                    break;
                }
            }
        }

    }
    return modelObject;
}

bool AICloudService_ResultDialog::getModelObjectBy3mfPartId(int partId, std::vector<const ModelObject*>& vtMO)
{
    bool               bRet        = false;
    const ModelObject* modelObject = nullptr;

    {
        typedef std::map<const ModelVolume*, int> VolumeToObjectIDMap;
        struct ObjectData
        {
            ModelObject const*  object;
            int                 backup_id;
            int                 object_id = 0;
            std::string         sub_path;
            bool                share_mesh = false;
            VolumeToObjectIDMap volumes_objectID;
        };
        typedef std::map<ModelObject const*, ObjectData>                  ObjectToObjectDataMap;
        std::map<void const*, std::pair<ObjectData*, ModelVolume const*>> m_shared_meshes;
        std::map<ModelVolume const*, std::pair<std::string, int>>         m_volume_paths;
        ObjectToObjectDataMap                                             objects_data;
        unsigned int                                                      object_id = 1;
        auto                                                              model     = wxGetApp().model();
        for (ModelObject* obj : wxGetApp().model().objects) {
            if (obj == nullptr)
                continue;
            ObjectToObjectDataMap::iterator object_it = objects_data.begin();
            int                             backup_id = const_cast<Model&>(wxGetApp().model()).get_object_backup_id(*obj);
            object_it                                 = objects_data.insert({obj, {obj, backup_id}}).first;
            auto&        object_data                  = object_it->second;
            auto&        volumes_objectID             = object_data.volumes_objectID;
            unsigned int volume_id = object_id, volume_count = 0;
            for (ModelVolume* volume : obj->volumes) {
                if (volume == nullptr)
                    continue;
                volume_count++;
                {
                    auto iter = m_shared_meshes.find(volume->mesh_ptr().get());
                    if (iter != m_shared_meshes.end()) {
                        auto data = iter->second.first;
                        m_volume_paths.insert({volume, {data->sub_path, data->volumes_objectID.find(iter->second.second)->second}});
                        volumes_objectID.insert({volume, 0});
                        continue;
                    }
                    m_shared_meshes.insert({volume->mesh_ptr().get(), {&object_data, volume}});
                }
                volumes_objectID.insert({volume, volume_id});
                volume_id++;
            }
            object_id             = volume_id;
            object_data.object_id = object_id;
            ++object_id;
        }
        for (const ObjectToObjectDataMap::value_type& obj_metadata : objects_data) {
            modelObject                    = nullptr;
            auto               object_data = obj_metadata.second;
            const ModelObject* obj         = object_data.object;
            if (obj != nullptr) {
                for (const ModelVolume* volume : obj_metadata.second.object->volumes) {
                    if (volume != nullptr) {
                        const VolumeToObjectIDMap&          objectIDs = obj_metadata.second.volumes_objectID;
                        VolumeToObjectIDMap::const_iterator it        = objectIDs.find(volume);
                        if (it != objectIDs.end()) {
                            int volume_id = it->second;
                            if (/*m_share_mesh && */ volume_id == 0)
                                volume_id = m_volume_paths.find(volume)->second.second;
                            if (volume_id == partId) {
                                modelObject = obj_metadata.second.object;
                                break;
                            }
                        }
                    }
                }
                if (modelObject != nullptr) {
                    vtMO.push_back(modelObject);
                    // break;
                }
            }
        }
    }
    return bRet;
}

bool AICloudService_ResultDialog::blockerModelFacets(int partId, const std::vector<int>& vtFacetIdx, std::function<void(int, int)> funProcessCb)
{
    bool bRet = false;
    do {
        const ModelObject* mo = getModelObjectBy3mfPartId(partId);
        if (mo == nullptr)
            break;

        TriangleSelector ts(mo->volumes[0]->mesh());
        //std::vector<indexed_triangle_set> facets_per_type;
        //ts.get_facets(facets_per_type);

        // 使用TBB并行处理面片设置
        tbb::parallel_for(tbb::blocked_range<size_t>(0, vtFacetIdx.size(), 1000),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i != range.end(); ++i) {
                    if (vtFacetIdx[i] >= 0) {
                        ts.set_facet(vtFacetIdx[i], EnforcerBlockerType::BLOCKER);
                    }
                }
            });
        
        mo->volumes[0]->seam_facets.set(ts);

        if (funProcessCb) {
            // 批量更新进度，减少回调频率
            for (int i = 0; i <= vtFacetIdx.size(); i += 100) {
                funProcessCb(std::min(i, (int)vtFacetIdx.size()), vtFacetIdx.size());
            }
            // 确保最终进度为100%
            funProcessCb(vtFacetIdx.size(), vtFacetIdx.size());
        }
    } while (0);
    return bRet;
}

bool AICloudService_ResultDialog::blockerModelFacets(const ModelObject* mo, const std::vector<int>& vtFacetIdx, std::function<void(int, int)> funProcessCb)
{
    bool bRet = false;
    //do {
    //    if (mo == nullptr)
    //        break;

    //    TriangleSelector ts(mo->volumes[0]->mesh());
    //    //std::vector<indexed_triangle_set> facets_per_type;
    //    //ts.get_facets(facets_per_type);

    //    for (int i = 0; i < vtFacetIdx.size(); ++i) {
    //        if (vtFacetIdx[i] >= 0 /*&& idx < facets_per_type.size()*/) {
    //            ts.set_facet(vtFacetIdx[i], EnforcerBlockerType::BLOCKER);
    //            mo->volumes[0]->seam_facets.set(ts);
    //            if (funProcessCb&& i% 100 ==0)
    //                funProcessCb(i, i);
    //        }
    //    }

    //    if (funProcessCb)
    //        funProcessCb(vtFacetIdx.size(), vtFacetIdx.size());
    //} while (0);
    do {
        if (mo == nullptr)
            break;

        TriangleSelector ts(mo->volumes[0]->mesh());
        // std::vector<indexed_triangle_set> facets_per_type;
        // ts.get_facets(facets_per_type);

        // 使用TBB并行处理面片设置
        tbb::parallel_for(tbb::blocked_range<size_t>(0, vtFacetIdx.size(), 1000), [&](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                if (vtFacetIdx[i] >= 0) {
                    ts.set_facet(vtFacetIdx[i], EnforcerBlockerType::BLOCKER);
                }
            }
        });

        mo->volumes[0]->seam_facets.set(ts);

        if (funProcessCb) {
            // 批量更新进度，减少回调频率
            for (int i = 0; i <= vtFacetIdx.size(); i += 100) {
                funProcessCb(std::min(i, (int) vtFacetIdx.size()), vtFacetIdx.size());
            }
            // 确保最终进度为100%
            funProcessCb(vtFacetIdx.size(), vtFacetIdx.size());
        }
    } while (0);
    return bRet;
}

AICloudService_ProgressDialog::AICloudService_ProgressDialog(wxWindow* parent)
    : DPIDialog(parent, wxID_ANY, _L("In cloud service processing"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    build_dialog();
}

void AICloudService_ProgressDialog::build_dialog()
{
    this->SetMinSize(wxSize(FromDIP(600), FromDIP(358)));
    this->SetMaxSize(wxSize(FromDIP(600), FromDIP(358)));
    wxString bgColor  = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor  = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    SetFont(wxGetApp().normal_font());
    wxGetApp().UpdateDlgDarkUI(this);
    this->SetBackgroundColour(bgColor);

    // SetBackgroundColour(*wxWHITE);

    // paraseLimitStr(data.serialize(), )

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(8));
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    panelSizer->AddStretchSpacer();
    m_timeoutText = new wxStaticText(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_timeoutText->SetMinSize(wxSize(FromDIP(100), FromDIP(-1)));
    m_timeoutText->SetMaxSize(wxSize(FromDIP(100), FromDIP(-1)));
    m_timeoutText->SetForegroundColour(wxGetApp().dark_mode() ? "#3C3C3C" : "#F2F2F2");
    panelSizer->Add(m_timeoutText, 1, wxRIGHT | wxALIGN_RIGHT, FromDIP(40));

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(96)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(96)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(32));
    panelSizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(panelSizer);
    m_headText = new wxStaticText(panel, wxID_ANY, _L("In the processing of cloud AI services"));
    panelSizer->Add(m_headText, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(40));

    m_info = new wxStaticText(panel, wxID_ANY, "");
    m_info->SetMinSize(wxSize(FromDIP(520), FromDIP(32)));
    m_info->SetMaxSize(wxSize(FromDIP(520), FromDIP(32)));
    panelSizer->Add(m_info, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(40));

    m_queueText = new wxStaticText(panel, wxID_ANY, "");
    m_queueText->SetMinSize(wxSize(FromDIP(520), FromDIP(64)));
    m_queueText->SetMaxSize(wxSize(FromDIP(520), FromDIP(64)));
    panelSizer->Add(m_queueText, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(40));

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(64)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(64)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    panelSizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(panelSizer);
    panelSizer->AddStretchSpacer();
    m_progressBar = new ProgressBar(panel);
    m_progressBar->SetMinSize(wxSize(FromDIP(-1), FromDIP(10)));
    m_progressBar->SetMaxSize(wxSize(FromDIP(-1), FromDIP(10)));
    m_progressBar->SetBackgroundColour(bgColor);
    m_progressBar->SetProgressForedColour(wxGetApp().dark_mode() ? wxColour("#505052") : wxColour("#E1E4E9"));
    m_progressBar->SetProgressBackgroundColour(wxColour("#1FCA63"));
    m_progressBar->SetRadius(5);
    m_progressBar->SetValue(1);
    panelSizer->Add(m_progressBar, 1, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(40));

    wxPanel* textPanel = new wxPanel(panel, wxID_ANY);
    textPanel->SetMinSize(wxSize(FromDIP(-1), FromDIP(32)));
    textPanel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(32)));
    textPanel->SetFont(Label::Body_13);
    textPanel->SetBackgroundColour(bgColor);
    textPanel->SetForegroundColour(fgColor);
    wxBoxSizer* textPanelSizer = new wxBoxSizer(wxHORIZONTAL);
    textPanel->SetSizer(textPanelSizer);
    panelSizer->Add(textPanel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    m_progressText = new wxStaticText(textPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_progressText->SetMinSize(wxSize(FromDIP(100), FromDIP(-1)));
    m_progressText->SetMaxSize(wxSize(FromDIP(100), FromDIP(-1)));
    m_progressText->SetForegroundColour(fgColor);
    textPanelSizer->AddStretchSpacer();
    textPanelSizer->Add(m_progressText, 1, wxRIGHT | wxALIGN_RIGHT, FromDIP(40));

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->AddStretchSpacer();
    main_sizer->Add(panel, 1, wxEXPAND | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(16));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    m_btnCancel = new Button(panel, _L("Cancel"));
    m_btnCancel->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    m_btnCancel->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    m_btnCancel->SetBorderColorNormal(wxColour(wxGetApp().dark_mode() ? "#FFFFFF" : "#000000"));
    m_btnCancel->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                             std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                             std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    m_btnCancel->Bind(wxEVT_LEFT_DOWN, &AICloudService_ProgressDialog::on_cancel_btn_clicked, this);
    panelSizer->AddStretchSpacer();
    panelSizer->Add(m_btnCancel, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL);
    panelSizer->AddStretchSpacer();
    this->Bind(wxEVT_CLOSE_WINDOW, ([this](wxCloseEvent& evt) {
        if (m_bEnable)
            EndModal(wxID_CANCEL);
    }));

    this->Layout();
    this->Fit();
}

void AICloudService_ProgressDialog::updateProgress(const wxString& info, int step)
{
    wxGetApp().CallAfter([=] {     
        if (m_progressBar && m_info) {
            m_info->SetLabelText(info);
            if (step != m_progressBar->m_step) {
                if (m_progressBar->m_max == 100) {
                    m_progressBar->SetValue(step);
                    m_progressText->SetLabel(std::to_string(step) + "%");
                } else {
                    int tempStep = int((float) step / m_progressBar->m_max * 100);
                    if (tempStep != m_nStep && tempStep % 5 == 0) {
                        m_progressBar->SetValue(step);
                        m_progressText->SetLabel(std::to_string(tempStep) + "%");
                        m_nStep = tempStep;
                    }
                }
            }
            if (step == m_progressBar->m_max) {
                EndModal(wxID_OK);
            }
        }
    });
}
void AICloudService_ProgressDialog::updateQueue(const wxString& errorInfo, int waitingCountAhead, int waitingCountTotal)
{
    wxGetApp().CallAfter([=] {
        if (errorInfo.empty()) {
            if (waitingCountTotal == 0) {
                m_queueText->SetLabel("");
            } else {
                m_queueText->SetLabel(GUI::format_wxstr(_L("Currently queued at position %1%, total %2%"), std::to_string(waitingCountAhead), std::to_string(waitingCountTotal)));
            }
        } else {
            m_queueText->SetForegroundColour("#FF0000");
            m_queueText->SetLabel(errorInfo);
        }
    });
}

void AICloudService_ProgressDialog::updateTimeout(int step, int timeout) 
{
    wxGetApp().CallAfter([=] { 
        m_timeoutText->SetLabel(_L("Timeout") + ": " + std::to_string(step) + "s");
    });

}

void AICloudService_ProgressDialog::reset()
{
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    m_timeoutText->SetLabel("");
    m_headText->SetLabel("");
    m_info->SetLabel("");
    m_queueText->SetForegroundColour(fgColor);
    m_queueText->SetLabel("");
    m_progressBar->SetValue(0);
    m_progressText->SetLabel("");
    m_nStep = 0;
}

void AICloudService_ProgressDialog::updateHeadText(const wxString& head) {
    m_headText->SetLabel(head);
}

void AICloudService_ProgressDialog::setProcessMax(int max) {
    m_progressBar->m_max = max;
}
void AICloudService_ProgressDialog::disable()
{
    m_btnCancel->Show(false);
    m_bEnable = false;
}

void AICloudService_ProgressDialog::on_cancel_btn_clicked(wxEvent&) { EndModal(wxID_CANCEL); }

AICloudService_QueueDialog::AICloudService_QueueDialog(wxWindow* parent)
    : DPIDialog(parent, wxID_ANY, _L("Tips"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    build_dialog();
}

void AICloudService_QueueDialog::build_dialog()
{
    this->SetMinSize(wxSize(FromDIP(600), FromDIP(358)));
    this->SetMaxSize(wxSize(FromDIP(600), FromDIP(358)));
    wxString bgColor  = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor  = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    SetFont(wxGetApp().normal_font());
    wxGetApp().UpdateDlgDarkUI(this);
    this->SetBackgroundColour(bgColor);

    // SetBackgroundColour(*wxWHITE);

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetFont(Label::Body_16);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(61));
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    wxStaticText* head = new wxStaticText(panel, wxID_ANY, _L("In the processing of cloud AI services, you need to queue up and wait"));
    panelSizer->AddStretchSpacer();
    panelSizer->Add(head, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(58)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(58)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    auto waitBtn = new HoverBorderIcon(panel, "", wxGetApp().dark_mode() ? "loading" : "loading", wxDefaultPosition, wxSize(FromDIP(56),FromDIP(56)),
                                     wxTE_PROCESS_ENTER);
    waitBtn->SetMinSize(wxSize(FromDIP(56), FromDIP(56)));
    waitBtn->SetMaxSize(wxSize(FromDIP(56), FromDIP(56)));
    waitBtn->SetBorderColorNormal(wxColour(bgColor));
    waitBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                              std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                              std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    waitBtn->Enable(false);
    panelSizer->AddStretchSpacer();
    panelSizer->Add(waitBtn);
    panelSizer->AddStretchSpacer();

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetFont(Label::Body_16);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    m_queueText = new wxStaticText(panel, wxID_ANY, GUI::format_wxstr(_L("Currently queued at position %%1%%, total %%2%%"), 0, 0));
    panelSizer->AddStretchSpacer();
    panelSizer->Add(m_queueText, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(15));
    panelSizer->AddStretchSpacer();

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(32)));
    panel->SetFont(Label::Body_13);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->AddStretchSpacer();
    main_sizer->Add(panel, 1, wxEXPAND | wxBOTTOM | wxALIGN_CENTER_VERTICAL, FromDIP(16));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    Button* btnCancel = new Button(panel, _L("Cancel"));
    btnCancel->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    btnCancel->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    btnCancel->SetBorderColorNormal(wxColour(wxGetApp().dark_mode() ? "#FFFFFF" : "#000000"));
    btnCancel->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(bgColor), StateColor::Pressed),
                                         std::pair<wxColour, int>(wxColour(bgColor), StateColor::Hovered),
                                         std::pair<wxColour, int>(wxColour(bgColor), StateColor::Normal)));
    btnCancel->Bind(wxEVT_LEFT_DOWN, &AICloudService_QueueDialog::on_cancel_btn_clicked, this);
    panelSizer->AddStretchSpacer();
    panelSizer->Add(btnCancel, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL);
    panelSizer->AddStretchSpacer();

    this->Layout();
    this->Fit();
}

void AICloudService_QueueDialog::updateQueue(bool waiting, int waitingCountAhead, int waitingCountTotal)
{
    wxGetApp().CallAfter([=] {
        if (waiting) {
            if (!this->IsVisible()) {
                m_queueText->SetLabel(GUI::format_wxstr(_L("Currently queued at position %1%, total %2%"), waitingCountAhead, waitingCountTotal));
                m_showModelRet = ShowModal();
            } else {
                m_queueText->SetLabel(GUI::format_wxstr(_L("Currently queued at position %1%, total %2%"), waitingCountAhead, waitingCountTotal));
            }
        } else {
            EndModal(wxID_OK);
        }
    });
}

int AICloudService_QueueDialog::getShowModelRet() {
    return m_showModelRet;
}

void AICloudService_QueueDialog::on_cancel_btn_clicked(wxEvent&) { EndModal(wxID_CANCEL); }


AICloudService_TipDialog::AICloudService_TipDialog(wxWindow* parent)
    : DPIDialog(parent, wxID_ANY, _L("Tips"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    build_dialog();
}

void AICloudService_TipDialog::build_dialog()
{
    this->SetMinSize(wxSize(FromDIP(600), FromDIP(358)));
    this->SetMaxSize(wxSize(FromDIP(600), FromDIP(358)));
    wxString bgColor = wxGetApp().dark_mode() ? "#313131" : "#FFFFFF";
    wxString bgColor2 = wxGetApp().dark_mode() ? "#2B2B2B" : "#EFF0F6";
    wxString fgColor = wxGetApp().dark_mode() ? "#FFFFFF" : "#30373D";
    this->SetFont(Label::Head_20);
    wxGetApp().UpdateDlgDarkUI(this);
    this->SetBackgroundColour(bgColor);

    //SetBackgroundColour(*wxWHITE);

    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(40)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(40)));
    panel->SetFont(Label::Body_16);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxTOP | wxALIGN_CENTER_VERTICAL, FromDIP(40));
    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    wxStaticText* head = new wxStaticText(panel, wxID_ANY, _L("We are about to utilize AI cloud services to process the current project"));
    panelSizer->Add(head, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(138)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(138)));
    if (wxGetApp().app_config->get("language").find("zh") == 0) {
        panel->SetFont(Label::Body_16);
    } else {
        panel->SetFont(Label::Body_13);
    }
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP(40));
    panelSizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(panelSizer);
    wxStaticText* textInfo = new wxStaticText(panel, wxID_ANY, 
        _L("Prompt:\n1. We will collect necessary account, device, and usage information to provide core functions such as cloud algorithms and project synchronization. "
            "Your data will be securely protected and will not be disclosed or used for unrelated purposes. By continuing to use, you agree to our Privacy Policy\n"
            "2. The efficiency of cloud services depends on the network environment and cloud computing capabilities, and processing may be slightly slower"));
    int height = FromDIP(106);
    if (wxGetApp().app_config->get("language").find("zh") == 0) {
        height = FromDIP(106);
    } else {
        height = FromDIP(116);
    }
    textInfo->SetMinSize(wxSize(FromDIP(-1), height));
    textInfo->SetMaxSize(wxSize(FromDIP(-1), height));
    textInfo->SetBackgroundColour(bgColor);
    textInfo->SetForegroundColour(fgColor);
    panelSizer->Add(textInfo, 0, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    panelSizer->AddSpacer(FromDIP(5));
    wxString url = "";
    if (wxGetApp().app_config->get("language").find("zh") == 0) {
        url = _L("https://wiki.creality.com/zh/software/6-0/privacy");
    } else {
        url = _L("https://wiki.creality.com/en/software/6-0/privacy");
    }
    wxHyperlinkCtrl* privacyLink = new wxHyperlinkCtrl(panel, wxID_ANY, 
        _L("<<Privacy Policy>>"), url);
    privacyLink->SetBackgroundColour(bgColor);
    privacyLink->SetNormalColour(wxColour(0, 0, 255)); // 蓝色文本
    privacyLink->SetVisitedColour(wxColour(128, 0, 128)); // 访问后紫色
    privacyLink->SetFont(wxFontInfo().Bold()); // 粗体
    panelSizer->Add(privacyLink, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(26)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(26)));
    if (wxGetApp().app_config->get("language").find("zh") == 0) {
        panel->SetFont(Label::Body_16);
    } else {
        panel->SetFont(Label::Body_13);
    }
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(40));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    m_ckAIRecommendationSupportGeneration = new ::CheckBox(panel);
    m_ckAIRecommendationSupportGeneration->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_ckAIRecommendationSupportGeneration->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_ckAIRecommendationSupportGeneration->SetValue(true);
    m_ckAIRecommendationSupportGeneration->setItemClickedCb([bgColor, this]() {
        if (m_ckAIRecommendationSupportGeneration->GetValue()) {
            m_okBtn->Enable(true);
            m_okBtn->SetBackgroundColor(wxColour("#1FCA63"));
        } else if (!m_ckAIRecommendationZseamPainting->GetValue()) {
            m_okBtn->Enable(false);
            m_okBtn->SetBackgroundColor(bgColor);
        }
    });
    panelSizer->Add(m_ckAIRecommendationSupportGeneration);
    wxStaticText* text = nullptr;
    text = new wxStaticText(panel, wxID_ANY, _L("AI Recommendation Support generation"));
    panelSizer->Add(text, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    m_ckAIRecommendationZseamPainting = new ::CheckBox(panel);
    m_ckAIRecommendationZseamPainting->SetMinSize(wxSize(FromDIP(24), FromDIP(24)));
    m_ckAIRecommendationZseamPainting->SetMaxSize(wxSize(FromDIP(24), FromDIP(24)));
    m_ckAIRecommendationZseamPainting->SetValue(false);
    m_ckAIRecommendationZseamPainting->setItemClickedCb([bgColor, this]() {
        if (m_ckAIRecommendationZseamPainting->GetValue()) {
            m_okBtn->Enable(true);
            m_okBtn->SetBackgroundColor(wxColour("#1FCA63"));
        } else if (!m_ckAIRecommendationSupportGeneration->GetValue()) {
            m_okBtn->Enable(false);
            m_okBtn->SetBackgroundColor(bgColor);
        }
    });
    m_ckAIRecommendationZseamPainting->Show(false);
    panelSizer->Add(m_ckAIRecommendationZseamPainting);
    text = new wxStaticText(panel, wxID_ANY, _L("AI Recommendation Z-seam painting"));
    text->Show(false);
    panelSizer->Add(text, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));

    panel = new wxPanel(this, wxID_ANY);
    panel->SetMinSize(wxSize(FromDIP(-1), FromDIP(64)));
    panel->SetMaxSize(wxSize(FromDIP(-1), FromDIP(64)));
    panel->SetFont(Label::Body_16);
    panel->SetBackgroundColour(bgColor);
    panel->SetForegroundColour(fgColor);
    main_sizer->Add(panel, 1, wxEXPAND | wxLEFT | wxALIGN_CENTER_VERTICAL, FromDIP(1));
    panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(panelSizer);
    m_okBtn = new Button(panel, _L("Ok"));
    m_okBtn->SetMinSize(wxSize(FromDIP(104), FromDIP(32)));
    m_okBtn->SetMaxSize(wxSize(FromDIP(104), FromDIP(32)));
    m_okBtn->SetFont(Label::Body_16);
    m_okBtn->SetBorderColor(wxColour(wxGetApp().dark_mode() ? "#6C6E71" : "#A6ACB4"));
    m_okBtn->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Pressed),
                                         std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Hovered),
                                         std::pair<wxColour, int>(wxColour("#1FCA63"), StateColor::Normal)));
    m_okBtn->Bind(wxEVT_LEFT_DOWN, &AICloudService_TipDialog::on_ok_btn_clicked, this);
    panelSizer->AddStretchSpacer();
    panelSizer->Add(m_okBtn, 1, wxEXPAND | wxTOP | wxALIGN_CENTER_HORIZONTAL, FromDIP(32));
    panelSizer->AddStretchSpacer();
   
    this->Layout();
    this->Fit();
}
bool AICloudService_TipDialog::getCheckedSupportGeneration()
{
    if (m_ckAIRecommendationSupportGeneration != nullptr && m_ckAIRecommendationSupportGeneration->GetValue()) {
        return true;
    }
    return false;
}
bool AICloudService_TipDialog::getCheckedZseamPainting()
{
    if (m_ckAIRecommendationZseamPainting != nullptr && m_ckAIRecommendationZseamPainting->GetValue()) {
        return true;
    }
    return false;
}

void AICloudService_TipDialog::on_ok_btn_clicked(wxEvent&) { 
    EndModal(wxID_OK); 
}

AICloudService::AICloudService()
{
}
AICloudService::~AICloudService() { shutdown(); }

AICloudService* AICloudService::getInstance()
{
    static AICloudService instance;
    return &instance;
}

// 添加清理方法，在语言切换时调用
void AICloudService::cleanup()
{
    // 停止所有异步操作
    m_bRunning.store(false);
    // 释放progressDlg，避免持有对旧MainFrame的引用
    m_progressDlg.reset();
    // 清空命令队列
    m_mutexLstAICmd.lock();
    m_lstAICmd.clear();
    m_mutexLstAICmd.unlock();
}

void AICloudService::run()
{
    //std::shared_ptr<AICloudService_ResultDialog> resultDlg2 = std::make_shared<AICloudService_ResultDialog>(wxGetApp().mainframe);
    //auto model = wxGetApp().model();
    //std::vector<int> vtIdx = {1, 2, 4};
    //resultDlg2->blockerModelFacets(1, vtIdx); 

    //return;
    std::shared_ptr<AICloudService_TipDialog> tipDlg = std::make_shared<AICloudService_TipDialog>(wxGetApp().mainframe);
    tipDlg->Center();
    if (tipDlg->ShowModal() != wxID_OK)
        return;
    if (!tipDlg->getCheckedSupportGeneration() && !tipDlg->getCheckedZseamPainting())
        return;
    
    //std::shared_ptr<AICloudService_QueueDialog> queueDlg = std::make_shared<AICloudService_QueueDialog>(wxGetApp().mainframe);
    //queueDlg->Center();
    //if (queueDlg->ShowModal() != wxID_OK)
    //    return;

    m_host = getAIUrl();

    m_progressDlg = std::make_shared<AICloudService_ProgressDialog>(wxGetApp().mainframe);
    m_progressDlg->Center();
    m_progressDlg->updateProgress(_L("Preparing data"), 20);
    //m_queueDlg = std::make_shared<AICloudService_QueueDialog>(wxGetApp().mainframe);
    //m_queueDlg->Center();
    AICloudService::getInstance()->startup();
    AICloudService::getInstance()->doAIRecommendation(tipDlg->getCheckedSupportGeneration(), tipDlg->getCheckedZseamPainting());
    if (m_progressDlg->ShowModal() != wxID_OK) {
        AICloudService::getInstance()->shutdown();
        return;
    }

    std::shared_ptr<AICloudService_ResultDialog> resultDlg = std::make_shared<AICloudService_ResultDialog>(wxGetApp().mainframe);
    std::list<CommWithAICloudService::STRespData> lstRespData;
    AICloudService::getInstance()->getRespData(lstRespData);
    AICloudService::getInstance()->shutdown();
    resultDlg->updateData(lstRespData);
    resultDlg->Center();
    if (resultDlg->ShowModal() != wxID_OK) {
        return;
    }

    m_progressDlg->reset();
    m_progressDlg->updateHeadText(_L("In the application of AI predictive data"));
    m_progressDlg->SetTitle(_L("In the application of AI predictive data"));
    int max = resultDlg->getDataCount(lstRespData);
    m_progressDlg->setProcessMax(max);
    m_progressDlg->disable();
    
    // 使用std::thread异步执行数据更新，确保m_progressDlg不会被提前销毁
    auto progressDlgPtr = m_progressDlg; // 延长shared_ptr的生命周期
    std::thread([progressDlgPtr, &resultDlg, &lstRespData]() {
        resultDlg->updateDataToModel(lstRespData, [progressDlgPtr](int step, int count) {
            // 使用wxCallAfter确保UI更新在主线程执行
            wxGetApp().CallAfter([progressDlgPtr, step]() {
                if (progressDlgPtr) {
                    progressDlgPtr->updateProgress("", step);
                }
            });
        });
        
        // 数据更新完成后，关闭进度对话框
        wxGetApp().CallAfter([progressDlgPtr]() {
            if (progressDlgPtr) {
                progressDlgPtr->EndModal(wxID_OK);
            }
        });
    }).detach();
    
    m_progressDlg->Center();
    m_progressDlg->ShowModal();
}

void AICloudService::doAIRecommendation(bool bAIRecommendationSupportGeneration, bool bAIRecommendationZseamPainting)
{
    m_mutexLstAICmd.lock();
    m_bAIRecommendationSupportGeneration = bAIRecommendationSupportGeneration;
    m_bAIRecommendationZseamPainting = bAIRecommendationZseamPainting;
    m_lstAICmd.push_back(ENAICmd::ENAIC_SAVE_3MF);
    m_lstAICmd.push_back(ENAICmd::ENAIC_CLOUD_PROCESS);
    m_mutexLstAICmd.unlock();
}

void AICloudService::getRespData(std::list<CommWithAICloudService::STRespData>& lstRespData)
{ 
    m_mutexRespData.lock();
    lstRespData = m_lstRespData; 
    m_mutexRespData.unlock();
}

int AICloudService::startup()
{
    BOOST_LOG_TRIVIAL(warning) << "AICloudService startup";
    if (m_bRunning) {
        return 1;
    }
    m_thread = std::thread(&AICloudService::onRun, this);
    m_thread.detach();
    return 0;
}
void AICloudService::shutdown()
{
    BOOST_LOG_TRIVIAL(warning) << "AICloudService shutdown";
    if (!m_bRunning.load())
        return;
    m_bRunning.store(false);
    std::unique_lock<std::mutex> lock(m_mutexQuit);
    m_cvQuit.wait(lock, [this]() { return m_bStoped.load(); });
    lock.unlock();
    BOOST_LOG_TRIVIAL(warning) << "AICloudService shutdown end";
}

void AICloudService::onRun()
{
    m_bRunning.store(true);
    std::list<ENAICmd> lstAICmd;
    while (m_bRunning.load())
    {
        lstAICmd.clear();
        m_mutexLstAICmd.lock();
        lstAICmd = m_lstAICmd;
        m_lstAICmd.clear();
        m_mutexLstAICmd.unlock();

        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        int ret = 0;
        for (auto cmd : lstAICmd)
        {
            if (ret != 0) {
                break;
            }
            if (!m_bRunning.load()) {
                break;
            }
            switch (cmd) {
            case ENAICmd::ENAIC_SAVE_3MF: {
                ret = doSave3mf();
            } break;
            case ENAICmd::ENAIC_CLOUD_PROCESS: doCloudProcess(); break;
            default: break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    m_bStoped.store(true);
    m_cvQuit.notify_one();
    BOOST_LOG_TRIVIAL(warning) << "AICloudService thread quited";
}

int AICloudService::doSave3mf()
{
    m_progressDlg->updateProgress(_L("Preparing data"), 20);
    boost::filesystem::path temp_folder(data_dir() + "/" + PRESET_USER_DIR + "/" + "Temp/AIRecommendation");
    m_3mfPath = temp_folder.string() + "/AIRecommendation.3mf";
    boost::system::error_code ec;
    try {
        if (fs::exists(temp_folder))
            fs::remove_all(temp_folder);
        if (!boost::filesystem::exists(temp_folder)) {
            if (!boost::filesystem::create_directories(temp_folder, ec)) {
            }
        }
    } catch (...) {
        return -1;
    }
    //if (!wxGetApp().plater()->up_to_date(false, true)) {
    std::promise<void> prom;
    std::future<void>  fut = prom.get_future();
    wxGetApp().CallAfter([this, &prom]() {
        wxGetApp().plater()->export_3mf(m_3mfPath, SaveStrategy::Silence | SaveStrategy::SplitModel | SaveStrategy::ShareMesh | SaveStrategy::FullPathSources /*SaveStrategy::Backup*/);
        prom.set_value();
    });
    while (fut.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
       // wxSafeYield();
    }
    //    wxGetApp().plater()->up_to_date(true, true);
    //}
    size_t size = fs::file_size(m_3mfPath);
    size_t maxSize = 200 * 1024 * 1024;
    if (size >= maxSize) {
        m_progressDlg->updateQueue(GUI::format_wxstr(_L("3MF project too large (%1% MB); cloud service cannot process it."), boost::str(boost::format("%.2f") % (float(size)/(1024*1024)))), 0, 0);
        return -2;
    }

    return 0;
}

int AICloudService::doCloudProcess()
{
    m_progressDlg->updateProgress(_L("In cloud service computing"), 80);
    json j = json();
    std::string filePath = m_3mfPath;
    m_mutexRespData.lock();
    m_lstRespData.clear();
    m_mutexRespData.unlock();
    commSupportService.reset();
    bool bSupportServiceWaitting = false;
    std::atomic<int> supportWaitingCountAhead = 0;
    std::atomic<int> supportWaitingCountTotal = 0;
    std::list<CommWithAICloudService::STRespData> lstRespData;
    int retCommSupportService = 0;
    int retCommZSeamService = 0;
    m_errMsg = "";
    
    // 使用std::future来管理异步任务
    std::future<int> futureSupport;
    std::future<int> futureZSeam;
    
    if (m_bAIRecommendationSupportGeneration) {
        futureSupport = std::async(std::launch::async, [&,this](){
            m_mutexHost.lock();
            std::string host = m_host;
            m_mutexHost.unlock();
            std::string error;
            int nRet = -1;
            CommWithAICloudService::STTaskRespData stAddTaskRespData;
            if ((nRet = commSupportService.addTask(host, "/ai-support/addTask", filePath, stAddTaskRespData)) != 0) {
                retCommSupportService = nRet;
                m_mutexErrMsg.lock();
                m_errMsg = stAddTaskRespData.msg;
                m_mutexErrMsg.unlock();
                return nRet;
            }

            BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService /ai-support/addTask end,result=" << stAddTaskRespData.result;
            CommWithAICloudService::STTaskRespData stDetailRespData;
            do {
                if (!m_bRunning.load())
                    break;
                stDetailRespData = {};
                nRet = commSupportService.detail(host, "/ai-support/detail", stAddTaskRespData.result, stDetailRespData);
                BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService /ai-support/detail end, nRet=" << nRet
                                           << ",state=" << stDetailRespData.state
                                           << ",waitingCountAhead=" << stDetailRespData.waitingCountAhead
                                           << ",waitingCountTotal=" << stDetailRespData.waitingCountTotal;
                supportWaitingCountAhead.store(stDetailRespData.waitingCountAhead);
                supportWaitingCountTotal.store(stDetailRespData.waitingCountTotal);
                if (nRet == 0) {
                    if (stDetailRespData.state == 0 || stDetailRespData.state == 1 || stDetailRespData.state == 2) {
                        bSupportServiceWaitting = true;
                        for (int i = 0; i < 1; ++i) {
                            if (!m_bRunning.load())
                                return -1;
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }
                    } else if (stDetailRespData.state == 4 || stDetailRespData.state == 5 || stDetailRespData.state == 6) {
                        nRet = -2;
                        break;
                    }
                }
            } while (nRet == 0 && (stDetailRespData.state==0||stDetailRespData.state == 1||stDetailRespData.state==2) && m_bRunning.load());
            if (nRet != 0 || !m_bRunning.load()) {
                if (m_bRunning.load()) {
                    m_mutexErrMsg.lock();
                    m_errMsg = stDetailRespData.msg;
                    m_mutexErrMsg.unlock();
                    retCommSupportService = nRet;
                }
                return nRet;
            }
            nRet = commSupportService.getAIRecommendationSupportGeneration(host, "/ai-support/result", stAddTaskRespData.result, lstRespData, error);
            BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService /ai-support/result end,nRet=" << nRet << ",respDataCount=" << lstRespData.size();
            if (nRet != 0) {
                m_mutexErrMsg.lock();
                m_errMsg = error;
                m_mutexErrMsg.unlock();
                retCommSupportService = nRet;
                return -1;
            }
            return 0;
        });
    }

    commZSeamService.reset();
    bool bZSeamServiceWaitting = false;
    std::atomic<int> zseamWaitingCountAhead = 0;
    std::atomic<int> zseamWaitingCountTotal = 0;
    if (m_bAIRecommendationZseamPainting) {
        futureZSeam = std::async(std::launch::async, [&,this]() {
            m_mutexHost.lock();
            std::string host = m_host;
            m_mutexHost.unlock();
            std::list<CommWithAICloudService::STRespData> lstRespData2;
            std::string error;
            int nRet = -1;
            CommWithAICloudService::STTaskRespData stAddTaskRespData;
            //if ((nRet = commZSeamService.getZseamAddTask(filePath, stAddTaskRespData)) != 0) {
            if ((nRet = commZSeamService.addTask(host, "/ai-zgap/addTask", filePath, stAddTaskRespData)) != 0) {
                retCommZSeamService = nRet;
                m_mutexErrMsg.lock();
                m_errMsg = stAddTaskRespData.msg;
                m_mutexErrMsg.unlock();
                return nRet;
            }
            BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService /ai-zgap/addTask end,result=" << stAddTaskRespData.result;

            CommWithAICloudService::STTaskRespData stDetailRespData;
            do {
                if (!m_bRunning.load())
                    break;
                stDetailRespData = {};
                //nRet = commZSeamService.getZseamDetail(stAddTaskRespData.result, stDetailRespData);
                nRet = commZSeamService.detail(host, "/ai-zgap/detail", stAddTaskRespData.result, stDetailRespData);
                BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService /ai-zgap/detail end, nRet=" << nRet
                                           << ",state=" << stDetailRespData.state
                                           << ",waitingCountAhead=" << stDetailRespData.waitingCountAhead
                                           << ",waitingCountTotal=" << stDetailRespData.waitingCountTotal;
                zseamWaitingCountAhead.store(stDetailRespData.waitingCountAhead);
                zseamWaitingCountTotal.store(stDetailRespData.waitingCountTotal);
                if (nRet == 0) {
                    if (stDetailRespData.state == 0 || stDetailRespData.state == 1 || stDetailRespData.state == 2) {
                        bZSeamServiceWaitting = true;
                        for (int i = 0; i < 1; ++i) {
                            if (!m_bRunning.load())
                                return -1;
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }
                    } else if (stDetailRespData.state == 4 || stDetailRespData.state == 5 || stDetailRespData.state == 6) {
                        nRet = -2;
                        break;
                    }
                }
            } while (nRet == 0 && (stDetailRespData.state == 0 || stDetailRespData.state == 1 || stDetailRespData.state == 2) &&
                     m_bRunning.load());
            if (nRet != 0 || !m_bRunning.load()) {
                if (m_bRunning.load()) {
                    m_mutexErrMsg.lock();
                    m_errMsg = stDetailRespData.msg;
                    m_mutexErrMsg.unlock();
                    retCommZSeamService = nRet;
                }
                return nRet;
            }
            nRet = commZSeamService.getAIRecommendationZseamPainting(host, "/ai-zgap/result", stAddTaskRespData.result, lstRespData2, error);
            BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService /ai-zgap/result end, nRet=" << nRet << ",respDataCount=" << lstRespData2.size();
            if (nRet != 0) {
                m_mutexErrMsg.lock();
                m_errMsg = error;
                m_mutexErrMsg.unlock();
                retCommZSeamService = nRet;
                return -1;
            }
            lstRespData.insert(lstRespData.end(), lstRespData2.begin(), lstRespData2.end());
            return 0;
        });
    }
    int timeout = 5 * 60;
    int curTime = 0;
    
    // 检查是否有任务需要执行
    bool hasSupportTask = m_bAIRecommendationSupportGeneration && futureSupport.valid();
    bool hasZSeamTask = m_bAIRecommendationZseamPainting && futureZSeam.valid();
    
    // 如果没有任务需要执行，直接返回成功
    if (!hasSupportTask && !hasZSeamTask) {
        return 0;
    }
    
    while (m_bRunning.load()) {
        if (curTime < timeout * 2) {
            if (curTime % 2 == 0) {
                m_progressDlg->updateTimeout(curTime / 2 + 1, timeout);
            }
            
            // 检查任务状态
            auto supportStatus = hasSupportTask ? futureSupport.wait_for(std::chrono::milliseconds(0)) : std::future_status::ready;
            auto zseamStatus = hasZSeamTask ? futureZSeam.wait_for(std::chrono::milliseconds(0)) : std::future_status::ready;
            
            // 检查任务是否出错
            if (retCommSupportService != 0 || retCommZSeamService != 0) {
                commSupportService.cancel();
                commZSeamService.cancel();
                std::string ssem;
                m_mutexErrMsg.lock();
                ssem = m_errMsg;
                m_mutexErrMsg.unlock();
                wxString strErrMsg = _L(ssem);
                if (ssem == "Cloud service encountered an error. Please try again.") {
                    strErrMsg = _L("Cloud service encountered an error. Please try again.");
                } else if (ssem == "No network. Please retry.") {
                    strErrMsg = _L("No network. Please retry.");
                } else if (ssem == "Cloud service cannot process this 3MF—contains assemblies or too many mesh triangles.") {
                    strErrMsg = _L("Cloud service cannot process this 3MF—contains assemblies or too many mesh triangles.");
                }
                m_progressDlg->updateQueue(strErrMsg, 0, 0);
                BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService retCommSupportService=" << retCommSupportService << ",retCommZSeamService=" << retCommZSeamService;
                return -1;
            }
            
            // 更新队列信息
            if ((commSupportService.getTaskState() == CommWithAICloudService::ENTS_Detail && bSupportServiceWaitting) ||
                (commZSeamService.getTaskState() == CommWithAICloudService::ENTS_Detail && bZSeamServiceWaitting)) {
                m_progressDlg->updateQueue("", supportWaitingCountAhead.load()+zseamWaitingCountAhead.load(), supportWaitingCountTotal.load()+zseamWaitingCountTotal.load());
            } else {
                if (commSupportService.getTaskState() == CommWithAICloudService::ENTS_FAIL ||
                    commZSeamService.getTaskState() == CommWithAICloudService::ENTS_FAIL){
                    commSupportService.cancel();
                    commZSeamService.cancel();
                    BOOST_LOG_TRIVIAL(warning) << "AICloudService CommWithAICloudService commSupportService.taskState=" << commSupportService.getTaskState()
                                               << ",commZSeamService.taskState=" << commZSeamService.getTaskState();
                    return -1;
                }
            }
            
            // 检查任务是否完成
            if (supportStatus == std::future_status::ready && zseamStatus == std::future_status::ready) {
                // 两个任务都完成了
                break;
            }
        } else {
            // 超时处理
            m_progressDlg->updateQueue("timeout", 0, 0);
            
            // 取消服务请求
            commSupportService.cancel();
            commZSeamService.cancel();
            
            // 等待一小段时间让任务有机会退出
            if (hasSupportTask && futureSupport.valid()) {
                futureSupport.wait_for(std::chrono::milliseconds(100));
            }
            if (hasZSeamTask && futureZSeam.valid()) {
                futureZSeam.wait_for(std::chrono::milliseconds(100));
            }
            
            return -1;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ++curTime;
    }
    
    // 获取任务结果
    if (hasSupportTask && futureSupport.valid()) {
        try {
            futureSupport.get();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "AICloudService support task exception: " << e.what();
        }
    }
    
    if (hasZSeamTask && futureZSeam.valid()) {
        try {
            futureZSeam.get();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "AICloudService zseam task exception: " << e.what();
        }
    }
    m_mutexLstAICmd.lock();
    m_lstRespData = lstRespData;
    m_mutexLstAICmd.unlock();
    m_progressDlg->updateProgress(_L("finished"), 100);
    return 0; 
}
std::string AICloudService::getAIUrl() {
    std::string url;
    std::string version_type = get_vertion_type();
    std::string country_code = wxGetApp().app_config->get_country_code();
    // 当 PROJECT_VERSION_EXTRA 为 Dev 时，云端交互统一指向 Dev 接口
    {
        std::string extra = std::string(PROJECT_VERSION_EXTRA);
        if (boost::algorithm::iequals(extra, std::string("Dev"))) {
            return "http://ai-test.crealitycloud.cn";
        }
    }
    if (version_type == "Alpha") {
        if (country_code == "CN") {
            url = "http://ai-test.crealitycloud.cn";
        } else {
            url = "http://ai-test.crealitycloud.cn";
        }
    } else {
        if (country_code == "CN") {
            url = "https://ai-cn.crealitycloud.cn";
        } else {
            url = "https://ai-usa.crealitycloud.com";
        }
    }
    return url;
}

} // GUI
} // Slic3r
