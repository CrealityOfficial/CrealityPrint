#include "WebSocketProxy.hpp"

namespace Slic3r {
namespace GUI {
namespace WebSocketProxy {

/*
bool ShouldProxyAllWebSockets()
{
#ifdef __WXGTK__
    return true;
#else
    return false;
#endif
}
*/

wxString GetWebSocketProxyScript()
{
    const char* script =
        "(function(){"
        "if(window.top!==window){return;}"
        "if(window.__wsProxyInstalled){return;}"
        "window.__wsProxyInstalled=true;"
        "function __install(){"
        "if(!window.wx||!window.wx.postMessage||!window.WebSocket){return false;}"
        "var NativeWebSocket=window.WebSocket;"
        "var PAGE_TOKEN=(window.crypto&&typeof window.crypto.randomUUID==='function')"
        "?window.crypto.randomUUID():Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);"
        "var STATE_CONNECTING=(typeof NativeWebSocket.CONNECTING==='number')?NativeWebSocket.CONNECTING:0;"
        "var STATE_OPEN=(typeof NativeWebSocket.OPEN==='number')?NativeWebSocket.OPEN:1;"
        "var STATE_CLOSING=(typeof NativeWebSocket.CLOSING==='number')?NativeWebSocket.CLOSING:2;"
        "var STATE_CLOSED=(typeof NativeWebSocket.CLOSED==='number')?NativeWebSocket.CLOSED:3;"
        "function __makeSocketToken(id){"
        "return(window.crypto&&typeof window.crypto.randomUUID==='function')"
        "?window.crypto.randomUUID():PAGE_TOKEN+'-'+id+'-'+Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);"
        "}"
        "function __isIPv4(host){"
        "var parts=host.split('.');"
        "if(parts.length!==4){return false;}"
        "for(var i=0;i<parts.length;++i){"
        "if(!/^[0-9]+$/.test(parts[i])||Number(parts[i])>255){return false;}"
        "}"
        "return true;"
        "}"
        "function __shouldProxy(url){"
        "if(url==null){return false;}"
        "try{"
        "var parsed=new URL(String(url));"
        "if(!__isIPv4(parsed.hostname)){return false;}"
        "if(parsed.username||parsed.password){return false;}"
        "if(parsed.pathname!=='/'||parsed.search!==''||parsed.hash!==''){return false;}"
        "return parsed.protocol==='wss:'||(parsed.protocol==='ws:'&&parsed.port==='9999');"
        "}catch(e){return false;}"
        "}"
        "function ProxyWebSocket(url,protocols){"
        "this.url=url;this.protocol='';this.readyState=STATE_CONNECTING;"
        "this.binaryType='blob';this.extensions='';this.bufferedAmount=0;"
        "this.onopen=null;this.onmessage=null;this.onclose=null;this.onerror=null;"
        "var id=ProxyWebSocket.__nextId++,socketToken=__makeSocketToken(id);"
        "this.__id=id;this.__socketToken=socketToken;ProxyWebSocket.__sockets[id]=this;"
        "ProxyWebSocket.__send({type:'open',id:id,socketToken:socketToken,url:url,protocols:protocols});"
        "}"
        "ProxyWebSocket.__nextId=1;"
        "ProxyWebSocket.__sockets={};"
        "ProxyWebSocket.__send=function(msg){"
        "try{msg.pageToken=PAGE_TOKEN;window.wx.postMessage(JSON.stringify({command:'ws_proxy',payload:msg}));}catch(e){}"
        "};"
        "ProxyWebSocket.prototype.send=function(data){"
        "if(this.readyState!==STATE_OPEN&&this.readyState!==STATE_CONNECTING){throw new Error('WebSocket is not open');}"
        "ProxyWebSocket.__send({type:'send',id:this.__id,socketToken:this.__socketToken,data:String(data)});"
        "};"
        "ProxyWebSocket.prototype.close=function(code,reason){"
        "if(this.readyState===STATE_CLOSING||this.readyState===STATE_CLOSED){return;}"
        "this.readyState=STATE_CLOSING;"
        "ProxyWebSocket.__send({type:'close',id:this.__id,socketToken:this.__socketToken,code:code,reason:reason});"
        "};"
        "ProxyWebSocket.CONNECTING=STATE_CONNECTING;"
        "ProxyWebSocket.OPEN=STATE_OPEN;"
        "ProxyWebSocket.CLOSING=STATE_CLOSING;"
        "ProxyWebSocket.CLOSED=STATE_CLOSED;"
        "ProxyWebSocket.__dispatch=function(evt){"
        "var s=ProxyWebSocket.__sockets[evt.id];if(!s||evt.socketToken!==s.__socketToken){return;}"
        "switch(evt.event){"
        "case'open':s.readyState=STATE_OPEN;if(typeof s.onopen==='function'){s.onopen({type:'open',target:s});}break;"
        "case'message':if(typeof s.onmessage==='function'){s.onmessage({type:'message',data:evt.data,target:s});}break;"
        "case'close':s.readyState=STATE_CLOSED;delete ProxyWebSocket.__sockets[evt.id];if(typeof s.onclose==='function'){s.onclose({type:'close',code:evt.code||1000,reason:evt.reason||'',wasClean:!!evt.wasClean,target:s});}break;"
        "case'error':if(typeof s.onerror==='function'){s.onerror({type:'error',message:evt.message||'',target:s});}break;"
        "}"
        "};"
        "function HybridWebSocket(url,protocols){"
        "var proxy=__shouldProxy(url);"
        "if(proxy){return new ProxyWebSocket(url,protocols);}"
        "if(typeof protocols==='undefined'){return new NativeWebSocket(url);}"
        "return new NativeWebSocket(url,protocols);"
        "}"
        "HybridWebSocket.CONNECTING=STATE_CONNECTING;"
        "HybridWebSocket.OPEN=STATE_OPEN;"
        "HybridWebSocket.CLOSING=STATE_CLOSING;"
        "HybridWebSocket.CLOSED=STATE_CLOSED;"
        "if(NativeWebSocket.prototype){HybridWebSocket.prototype=NativeWebSocket.prototype;}"
        "window.__nativeWebSocketCallback=function(evt){"
        "if(!evt||evt.pageToken!==PAGE_TOKEN){return;}"
        "ProxyWebSocket.__dispatch(evt);"
        "};"
        "window.__nativeWebSocketCallbackBatch=function(events){"
        "if(!Array.isArray(events)){return;}"
        "for(var i=0;i<events.length;++i){"
        "try{window.__nativeWebSocketCallback(events[i]);}catch(e){}"
        "}"
        "};"
        "ProxyWebSocket.__send({type:'reset'});"
        "window.WebSocket=HybridWebSocket;"
        "return true;"
        "}"
        "if(!__install()){"
        "var __t=0,__max=200;"
        "var __timer=setInterval(function(){"
        "if(__install()||++__t>=__max){clearInterval(__timer);}"
        "},50);"
        "}"
        "})();";

    return wxString::FromUTF8(script);
}

} // namespace WebSocketProxy
} // namespace GUI
} // namespace Slic3r
