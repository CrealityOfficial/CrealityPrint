#ifndef __test_helper_hpp_
#define __test_helper_hpp_

#include <string>

#include "nlohmann/json.hpp"

namespace Test {

class CmdChannel;

extern bool enable_test;

void mark_app_ready();

enum class Button : wxWindowID {
    Slice       = 9000,
    ExportGCode = 9001
};

inline constexpr wxWindowID toWxWinId(Button b) noexcept { return static_cast<wxWindowID>(b); }

class TestHelper
{
    using Cmd_Type = int(nlohmann::json, std::string&, std::string&);

public:
    static TestHelper& instance()
    {
        static TestHelper ins(Test::enable_test);
        return ins;
    }
    std::function<std::string(std::string, std::string)> call_cmd = [](std::string, std::string) -> std::string {
        return "";
    }; // Singleton interface
    
private:
    TestHelper(bool enable);
    ~TestHelper();
    std::string call_cmd_inner(std::string cmd, std::string json_str);
    void register_cmd();
    void init_cmd_channel(short port);
    void cmd_respone(std::string cmd, nlohmann::json ret);

private:
    std::unordered_map<std::string, std::function<Cmd_Type>> m_cmd2func;
    std::unordered_map<std::string, std::function<Cmd_Type>> m_inner_cmd2func;
    CmdChannel* m_cmd_channel = nullptr;
};

inline void Init(bool enable)
{
    enable_test = enable;
    TestHelper::instance();
}

inline TestHelper& Visitor() { return TestHelper::instance(); }

inline void EVENT_SPREAD(std::string event_name, std::string param = "")
{
    if (Test::enable_test) {
        nlohmann::json output;
        output["event"] = event_name;
        output["param"] = param;
        Test::Visitor().call_cmd("event_spread", output.dump(-1, ' ', true));
    }
}

} // Test namespace
#endif // __test_helper_hpp_
