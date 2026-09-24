"""Loopback HTTP integration tests; optionally serve a directory of real presets."""
import argparse
import collections
import ctypes
import http.server
import os
import pathlib
import subprocess
import tempfile
import threading
import time


class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self):
        super().__init__(("127.0.0.1", 0), Handler)
        self.lock = threading.Lock()
        self.active = 0
        self.peak = 0
        self.requests = collections.Counter()
        self.payloads = []


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        index = int(self.path.rsplit("/", 1)[1])
        server = self.server
        with server.lock:
            server.active += 1
            server.peak = max(server.peak, server.active)
            server.requests[self.path] += 1
        counted = True
        try:
            payload = server.payloads[index % len(server.payloads)]
            broken = self.path.startswith("/broken/")
            self.send_response(200)
            self.send_header("Content-Length", str(len(payload) + (100 if broken else 0)))
            self.end_headers()
            # Keep the request active long enough to measure overlapping downloads.
            if self.path.startswith("/slow/"):
                self.wfile.write(payload[:1])
                self.wfile.flush()
                time.sleep(1)
                payload = payload[1:]
            else:
                time.sleep(0.015)
            # Stop counting before the client can finish and dispatch another request.
            with server.lock:
                server.active -= 1
                counted = False
            self.wfile.write(payload)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass
        finally:
            if counted:
                with server.lock:
                    server.active -= 1


def run_case(client, root, mode, count, payloads):
    server = Server()
    server.payloads = payloads
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        output = root / (mode + "_用户预设")
        if mode != "open_failure":
            output.mkdir()
        proc = subprocess.run(
            [str(client), f"http://127.0.0.1:{server.server_port}", str(output), str(count), mode],
            capture_output=True, timeout=40,
        )
        assert proc.returncode == 0, (mode, proc.returncode, proc.stdout, proc.stderr)
        assert server.peak <= (1 if mode == "zero" else 3), (mode, server.peak)
        if mode in ("batch", "zero", "real"):
            assert server.peak >= min(count, 1 if mode == "zero" else 3)
        for index in range(count):
            file = output / f"{index}.json"
            if mode in ("cancel", "open_failure") or (mode == "failure" and index == 0):
                assert not file.exists(), (mode, "partial file remains", index)
            else:
                assert file.read_bytes() == payloads[index % len(payloads)], (mode, index)
        if mode == "cancel":
            assert sum(server.requests.values()) <= 3, "cancelled queue kept starting requests"
        # No lingering file handles: all completed outputs must be movable on Windows.
        if output.exists():
            output.rename(output.with_name(output.name + "_done"))
        print(f"PASS {mode}: {count} files, peak active requests={server.peak}")
    finally:
        server.shutdown()
        server.server_close()
        worker.join()


def main():
    if os.name == "nt":
        # Children inherit this: report loader/crash errors through exit status,
        # without opening interactive Windows error dialogs during tests.
        ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002)
    parser = argparse.ArgumentParser()
    parser.add_argument("client", type=pathlib.Path)
    parser.add_argument("--presets", type=pathlib.Path)
    args = parser.parse_args()
    payloads = [b'{"name":"test","values":["1","2"]}', b"x" * 90000]
    with tempfile.TemporaryDirectory(prefix="preset-download-test-") as tmp:
        root = pathlib.Path(tmp)
        for mode, count in [("batch", 1000), ("failure", 8), ("cancel", 244), ("open_failure", 8), ("zero", 4), ("empty", 0)]:
            run_case(args.client.resolve(), root, mode, count, payloads)
        if args.presets:
            actual = [p.read_bytes() for p in sorted(args.presets.rglob("*")) if p.is_file() and p.suffix != ".xlsx"]
            assert actual, "no presets found"
            run_case(args.client.resolve(), root, "real", len(actual), actual)


if __name__ == "__main__":
    main()
