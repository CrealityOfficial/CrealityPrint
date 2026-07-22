#!/usr/bin/env python3
"""Stress-test a fixed localhost proxy (mirrors HttpServer.cpp fix) against Sermoon :81.

Fixed behavior:
- new upstream socket per request (Connection: close)
- write then read-to-EOF
- always close client socket after response / error / timeout
"""

from __future__ import annotations

import argparse
import asyncio
import json
import subprocess
import time
import urllib.parse
from typing import Optional, Tuple

HOST = "127.0.0.1"
DEFAULT_PORT = 13667
PRINTER = "192.168.31.73"
PATH = "/protocal.csp?fname=Info&opt=main&function=get"


def count_close_wait(port: int) -> int:
    out = subprocess.check_output(["netstat", "-ano"], text=True, errors="ignore")
    n = 0
    for line in out.splitlines():
        if f":{port}" in line and "CLOSE_WAIT" in line.upper():
            n += 1
    return n


async def upstream_fetch(host: str, path: str, timeout: float = 5.0) -> Tuple[int, bytes]:
    reader: Optional[asyncio.StreamReader] = None
    writer: Optional[asyncio.StreamWriter] = None
    try:
        reader, writer = await asyncio.wait_for(asyncio.open_connection(host, 81), timeout=timeout)
        req = (
            f"GET {path} HTTP/1.1\r\n"
            f"Host: {host}\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()
        writer.write(req)
        await writer.drain()
        raw = await asyncio.wait_for(reader.read(), timeout=timeout)
        if b"\r\n\r\n" not in raw:
            return 502, b"Invalid upstream response"
        body = raw.split(b"\r\n\r\n", 1)[1].replace(b"\r", b"")
        data = json.loads(body.decode("utf-8", "replace"))
        dumped = json.dumps(data).encode()
        return 200, dumped
    except asyncio.TimeoutError:
        return 504, b"timeout"
    except Exception as e:
        return 502, str(e).encode()
    finally:
        if writer is not None:
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass


async def handle_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
    peer = writer.get_extra_info("peername")
    try:
        # Read HTTP request headers only (deviceMgr uses GET).
        buf = b""
        while b"\r\n\r\n" not in buf:
            chunk = await asyncio.wait_for(reader.read(4096), timeout=5)
            if not chunk:
                break
            buf += chunk
            if len(buf) > 65536:
                break
        line = buf.split(b"\r\n", 1)[0].decode("latin1", "replace")
        parts = line.split()
        if len(parts) < 2 or not parts[1].startswith("/proxy"):
            body = b"not found"
            resp = (
                b"HTTP/1.1 404 Not Found\r\nContent-Length: "
                + str(len(body)).encode()
                + b"\r\nConnection: close\r\n\r\n"
                + body
            )
            writer.write(resp)
            await writer.drain()
            return

        qs = urllib.parse.urlparse(parts[1]).query
        params = urllib.parse.parse_qs(qs)
        host = (params.get("host") or [""])[0]
        path = (params.get("path") or [""])[0]
        if not host or not path:
            body = b"missing host or path"
            resp = (
                b"HTTP/1.1 400 Bad Request\r\nContent-Length: "
                + str(len(body)).encode()
                + b"\r\nConnection: close\r\n\r\n"
                + body
            )
            writer.write(resp)
            await writer.drain()
            return

        status, payload = await upstream_fetch(host, path)
        reason = {
            200: b"OK",
            400: b"Bad Request",
            502: b"Bad Gateway",
            504: b"Gateway Timeout",
        }.get(status, b"Error")
        ctype = b"application/json" if status == 200 else b"text/plain"
        resp = (
            f"HTTP/1.1 {status} ".encode()
            + reason
            + b"\r\nContent-Type: "
            + ctype
            + b"\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: "
            + str(len(payload)).encode()
            + b"\r\nConnection: close\r\n\r\n"
            + payload
        )
        writer.write(resp)
        await writer.drain()
    except Exception as e:
        try:
            body = str(e).encode()
            writer.write(
                b"HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
                + str(len(body)).encode()
                + b"\r\nConnection: close\r\n\r\n"
                + body
            )
            await writer.drain()
        except Exception:
            pass
    finally:
        # Always close client — this is the CLOSE_WAIT fix.
        try:
            writer.close()
            await writer.wait_closed()
        except Exception:
            pass
        _ = peer


async def client_once(port: int, host: str, path: str) -> bool:
    q = urllib.parse.urlencode({"host": host, "path": path})
    url_path = f"/proxy?{q}"
    reader, writer = await asyncio.open_connection(HOST, port)
    try:
        writer.write(f"GET {url_path} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n".encode())
        await writer.drain()
        raw = await asyncio.wait_for(reader.read(), timeout=8)
        ok = raw.startswith(b"HTTP/1.1 200") and b"model" in raw
        if not ok:
            head = raw[:120].replace(b"\r", b"").replace(b"\n", b" ")
            print("fail_sample:", head)
        return ok
    finally:
        writer.close()
        await writer.wait_closed()


async def run_stress(port: int, concurrency: int, rounds: int) -> None:
    server = await asyncio.start_server(handle_client, HOST, port)
    sockets = ", ".join(str(s.getsockname()) for s in server.sockets or [])
    print(f"fixed proxy listening on {sockets}")

    before = count_close_wait(port)
    print(f"CLOSE_WAIT before: {before}")

    ok = 0
    fail = 0
    t0 = time.time()
    for r in range(rounds):
        results = await asyncio.gather(
            *[client_once(port, PRINTER, PATH) for _ in range(concurrency)],
            return_exceptions=True,
        )
        for x in results:
            if x is True:
                ok += 1
            else:
                fail += 1
        print(f"round {r+1}/{rounds}: ok={ok} fail={fail} close_wait={count_close_wait(port)}")

    await asyncio.sleep(1.0)
    after = count_close_wait(port)
    elapsed = time.time() - t0
    print(f"DONE ok={ok} fail={fail} elapsed={elapsed:.1f}s CLOSE_WAIT after={after}")
    if fail:
        raise SystemExit(2)
    if after > before + 5:
        print("WARNING: CLOSE_WAIT grew unexpectedly")
        raise SystemExit(3)
    print("PASS: proxy survived stress without CLOSE_WAIT leak")
    server.close()
    await server.wait_closed()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=DEFAULT_PORT)
    ap.add_argument("--concurrency", type=int, default=30)
    ap.add_argument("--rounds", type=int, default=10)
    args = ap.parse_args()
    asyncio.run(run_stress(args.port, args.concurrency, args.rounds))


if __name__ == "__main__":
    main()
