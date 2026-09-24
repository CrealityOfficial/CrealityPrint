# Cloud device MQTT recovery tests

These tests use an injected transport and do not connect to Creality Cloud.

```sh
cmake -S tests/cloud_device_mqtt -B /tmp/cloud-mqtt-tests
cmake --build /tmp/cloud-mqtt-tests
ctest --test-dir /tmp/cloud-mqtt-tests --output-on-failure
```

On Windows, use a writable build directory and a Visual Studio developer shell.
The target is also registered under the repository's `tests/CMakeLists.txt`.
Assertions remain enabled in Release builds.

Covered: initial connect/subscribe/monitor failure, repeated disconnect callbacks,
single-worker reconnect, restoring the latest selected DN, ignoring old callbacks,
authentication failure followed by credential renewal, target changes during an
in-flight connect, and nonblocking shutdown while a connect is pending.

Related frontend tests in the CrealityCommunity repository:

```sh
cd DMgr
node --test tests/cloud-device-state.test.mjs
node --experimental-vm-modules --test --test-concurrency=1 tests/detail-resource-lifecycle.test.mjs tests/memory-lifecycle.test.mjs
```

The frontend coalesces HTTP snapshots, ignores responses for replaced devices or
credentials, preserves previous data on failures, and normalizes both MQTT array
and HTTP object telemetry formats.

Implementation: one background worker owns the Paho transport. A failed attempt
is destroyed before another begins; connecting, subscribing, adding/removing a
device monitor and RPC acknowledgements never block the UI thread. Transport
callbacks only enqueue bounded notifications. `Shutdown()` invalidates delivery
immediately and returns a completion future; retained worker state is released
after bounded in-flight I/O ends. A successful QoS 0 monitor publish is reported
as `monitoring`, and only a matching device update marks it `live`. Snapshot polling
continues independently of MQTT and printer online flags.

Manual acceptance still required with an authenticated cloud device:

- Open while offline, restore networking, verify temperatures recover without restarting.
- Disconnect a working session, restore networking, verify re-subscribe and monitor restoration.
- Switch A to B during a retry; only B's pushes/snapshots may update B.
- Refresh the token, change accounts/regions, log out, and close during connection attempts.
- Check `[CloudDeviceMQTT]` state/retry/error logs; no credentials or payloads are logged.

Validation performed: Windows compilation of the session, Paho adapter and
PrinterMgrView translation units; standalone CTest; 24 frontend tests; Vite
production build. No full desktop executable link or live cloud outage test was
performed as part of this change.
