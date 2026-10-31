# WebSocket fallback regression tests

Run with:

```sh
cmake -S tests/websocket_proxy -B out/websocket-fallback-tests
cmake --build out/websocket-fallback-tests
ctest --test-dir out/websocket-fallback-tests --output-on-failure
```

The executor tracks transport failures per original WSS host/port. After three
failures, subsequent opens for that endpoint use plain WS on port 9999. The
frontend still reconnects normally and sends its initialization requests again.
A handshake alone does not reset failures: a message received at least 30 seconds
after opening does. Local closes, superseded sessions and UI queue overflow do
not count. A page reset clears the fallback state. Device HTTPS/video settings
are not rewritten, since those services may still require TLS.

These tests cover the threshold, endpoint isolation, saturation, recovery and
page reset policy. They do not emulate a printer or verify WebView2 delivery.
Hardware validation should force three WSS read failures, confirm the logged
WS fallback, and verify initialization data/CFS and video token retrieval. Also
check a device with port 9999 unavailable and verify ordinary reconnect handling.
