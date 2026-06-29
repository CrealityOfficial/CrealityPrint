#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mqtt/async_client.h>
#include <mqtt/callback.h>
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <memory>

/**
 * @brief MQTT客户端封装类
 * 
 * 封装了Paho MQTT C++库，提供简单的发布和订阅接口
 */
class MQTTClient {
public:
    // 消息到达回调函数类型
    using MessageCallback = std::function<void(const std::string& topic, 
                                              const std::string& payload)>;
    
    // 连接状态回调函数类型
    using ConnectionCallback = std::function<void(bool connected)>;
    using ConnectionLostCallback = std::function<void(const std::string& cause)>;

    /**
     * @brief 构造函数
     * @param server_address MQTT服务器地址，格式：tcp://host:port
     * @param client_id 客户端ID，如果为空则自动生成
     */
    MQTTClient(const std::string& server_address, 
               const std::string& client_id = "");
    
    /**
     * @brief 析构函数
     */
    ~MQTTClient();
    
    // 删除拷贝构造函数和赋值运算符
    MQTTClient(const MQTTClient&) = delete;
    MQTTClient& operator=(const MQTTClient&) = delete;
    
    /**
     * @brief 连接到MQTT服务器
     * @param username 用户名（可选）
     * @param password 密码（可选）
     * @param clean_session 是否清理会话
     * @param keep_alive 保活间隔（秒）
     * @return 是否连接成功
     */
    bool connect(const std::string& username = "", 
                 const std::string& password = "", 
                 bool clean_session = true,
                 int keep_alive = 60);
    
    /**
     * @brief 断开连接
     */
    void disconnect();
    
    /**
     * @brief 发布消息
     * @param topic 主题
     * @param payload 消息内容
     * @param qos 服务质量等级（0,1,2）
     * @param retained 是否保留消息
     * @return 是否发布成功
     */
    bool publish(const std::string& topic, 
                 const std::string& payload, 
                 int qos = 1, 
                 bool retained = false);
    
    /**
     * @brief 订阅主题
     * @param topic 主题，支持通配符
     * @param qos 服务质量等级
     * @param callback 消息到达时的回调函数
     * @return 是否订阅成功
     */
    bool subscribe(const std::string& topic, 
                   int qos = 1, 
                   MessageCallback callback = nullptr);
    
    /**
     * @brief 取消订阅
     * @param topic 主题
     * @return 是否取消成功
     */
    bool unsubscribe(const std::string& topic);
    
    /**
     * @brief 设置连接状态回调
     * @param callback 回调函数
     */
    void setConnectionCallback(ConnectionCallback callback);

    /**
     * @brief 设置连接断开回调
     * @param callback 回调函数
     */
    void setConnectionLostCallback(ConnectionLostCallback callback);
    
    /**
     * @brief 检查是否已连接
     * @return 连接状态
     */
    bool isConnected() const;

    /**
     * @brief 获取最近一次错误信息
     * @return 错误文本
     */
    std::string getLastError() const;

private:
    // 内部回调处理类
    class ActionCallback : public mqtt::callback {
    public:
        ActionCallback(MQTTClient& client);
        ~ActionCallback();
        // 连接丢失回调
        void connection_lost(const std::string& cause) override;
        
        // 消息到达回调
        void message_arrived(mqtt::const_message_ptr msg) override;
        
        // 消息发送完成回调（暂不使用）
        void delivery_complete(mqtt::delivery_token_ptr token) override;
        
    private:
        MQTTClient& client_;
    };
    
    std::string server_address_;
    std::string client_id_;
    std::unique_ptr<mqtt::async_client> client_;
    std::unique_ptr<ActionCallback> callback_;
    
    // 订阅主题和对应的回调函数
    std::map<std::string, MessageCallback> topic_callbacks_;
    
    // 连接状态回调
    ConnectionCallback connection_callback_;
    ConnectionLostCallback connection_lost_callback_;
    
    // 连接选项
    mqtt::connect_options conn_opts_;
    std::string last_error_;
};

#endif // MQTT_CLIENT_H
