#include "slic3r/GUI/print_manage/App/WebSocketFallbackPolicy.hpp"
#include <cstdlib>
#include <iostream>
using Slic3r::GUI::WebSocketProxy::WebSocketFallbackPolicy;

void check(bool value, const char* message)
{
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    WebSocketFallbackPolicy policy;
    const std::string device = "192.168.1.3:443";
    check(!policy.UsePlaintext(device), "First connection must prefer WSS");
    policy.Failed(device);
    policy.Failed(device);
    check(!policy.UsePlaintext(device), "Allow transient WSS failures");
    policy.Failed(device);
    check(policy.UsePlaintext(device), "Repeated failures must switch the next connection");
    check(!policy.UsePlaintext("192.168.1.4:443"), "Failures must not affect another device");
    check(!policy.UsePlaintext("192.168.1.3:8443"), "Failures must not affect another endpoint");
    for (int i = 0; i < 100; ++i) policy.Failed(device);
    check(policy.Failed(device) == 3, "Failure count must saturate");
    policy.Clear();
    check(!policy.UsePlaintext(device), "A new page must retry WSS");
    policy.Failed(device);
    policy.Failed(device);
    policy.Healthy(device);
    policy.Failed(device);
    check(!policy.UsePlaintext(device), "Stable recovery must break a failure streak");
    std::cout << "WebSocket fallback tests passed\n";
}
