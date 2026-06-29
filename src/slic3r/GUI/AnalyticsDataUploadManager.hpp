#ifndef ANALYTICS_DATA_UPLOAD_MANAGER_HPP
#define ANALYTICS_DATA_UPLOAD_MANAGER_HPP

#include <functional>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <map>
#include <future>
#include <string>

#include "nlohmann/json.hpp"
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r {
namespace GUI {

class PartPlate;  // 前置声明

// when to upload analytics data
enum class AnalyticsUploadTiming {
    ON_CLICK_START_PRINT_CMD,        // when user clicks the ("start print" or "send only") command(on SendToPrinter front page)
    ON_SLICE_PLATE_CMD,    //when user clicks the (slice or slice all) command
    ON_FIRST_LAUNCH,      // when software is launched for the first time (when "AppData\Roaming\Creality" directory first created)
    ON_PREFERENCES_CHANGED,    //when user close the preference dialog
    ON_SOFTWARE_LAUNCH,      //when creality slicer launch, it could possibly launch multiple times every day
    ON_SOFTWARE_CRASH,        // when software crash and then reboot
    ON_SOFTWARE_CLOSE        // when software close
};

// what kind of data to upload
enum class AnalyticsDataEventType {
    ANALYTICS_GLOBAL_PRINT_PARAMS,
    ANALYTICS_OBJECT_PRINT_PARAMS,
    ANALYTICS_SLICE_PLATE,
    ANALYTICS_FIRST_LAUNCH,
    ANALYTICS_PREFERENCES_CHANGED,
    ANALYTICS_SOFTWARE_LAUNCH,
    ANALYTICS_SOFTWARE_CRASH,
    ANALYTICS_BAD_ALLOC,
    ANALYTICS_SOFTWARE_CLOSE,
    ANALYTICS_DEVICE_INFO,
    ANALYTICS_ACCOUNT_DEVICE_INFO,
    ANALYTICS_ONLINE_MODELS,
    ANALYTICS_PREPARE,
    ANALYTICS_PREVIEW,
    ANALYTICS_DEVICE,
    ANALYTICS_CLICK_HOME_PAGE_PROJECTS,
    ANALYTICS_CLICK_HOME_PAGE_ONLINE_PARAMS,
    ANALYTICS_CLICK_HOME_PAGE_TUTORIALS,
    ANALYTICS_CLICK_HOME_PAGE_PERSON_CENTER,
    ANALYTICS_CLICK_HOME_PAGE_FEEDBACK,
    ANALYTICS_CLICK_HOME_PAGE_MAKENOW,
    ANALYTICS_CLICK_HOME_PAGE_CREALITYMALL,
    ANALYTICS_MODEL_ACTION_ADD,
    ANALYTICS_MODEL_ACTION_ADD_PLATE,
    ANALYTICS_MODEL_ACTION_MOVE,
    ANALYTICS_MODEL_ACTION_ROTATE,
    ANALYTICS_MODEL_ACTION_AUTO_ORIENT,
    ANALYTICS_MODEL_ACTION_ARRANGE_ALL,
    ANALYTICS_MODEL_ACTION_LAY_ON_FACE,
    ANALYTICS_MODEL_ACTION_SPLIT_TO_OBJECTS,
    ANALYTICS_MODEL_ACTION_SPLIT_TO_PARTS,
    ANALYTICS_MODEL_ACTION_SCALE,
    ANALYTICS_MODEL_ACTION_HOLLOW,
    ANALYTICS_MODEL_ACTION_ADD_HOLE,
    ANALYTICS_MODEL_ACTION_CUT,
    ANALYTICS_MODEL_ACTION_BOOLEAN,
    ANALYTICS_MODEL_ACTION_MEASURE,
    ANALYTICS_MODEL_ACTION_SUPPORT_PAINT,
    ANALYTICS_MODEL_ACTION_ZSEAM_PAINT,
    ANALYTICS_MODEL_ACTION_VARIABLE_LAYER,
    ANALYTICS_MODEL_ACTION_PAINT,
    ANALYTICS_MODEL_ACTION_EMBOSS,
    ANALYTICS_MODEL_ACTION_ASSEMBLY_VIEW,
    ANALYTICS_AI_SERVICE_CALL,
    ANALYTICS_GOTO_WIKI,
    ANALYTICS_GOTO_SUPPORT,
    ANALYTICS_TAB_HOME,
    ANALYTICS_SLICE_SINGLE_COMPLETE,
    ANALYTICS_SLICE_ALL_COMPLETE,
    ANALYTICS_PRINT_SEND,
    ANALYTICS_PRINT_BEGIN,
    ANALYTICS_PRINT_ERROR,
    // Click events
    ANALYTICS_CLICK_SEND_SINGLE,
    ANALYTICS_CLICK_SEND_MULTI,
    // File project events
    ANALYTICS_FILE_PROJECT_NEW,
    ANALYTICS_FILE_PROJECT_OPEN,
    ANALYTICS_FILE_PROJECT_SAVE,
    ANALYTICS_FILE_PROJECT_SAVE_AS,
    // File model events
    ANALYTICS_FILE_IMPORT_MODEL,
    ANALYTICS_FILE_EXPORT_MODEL,
    // File preset events
    ANALYTICS_FILE_IMPORT_PRESET,
    ANALYTICS_FILE_EXPORT_PRESET,
    // File GCode events
    ANALYTICS_FILE_EXPORT_GCODE_SINGLE,
    ANALYTICS_FILE_EXPORT_GCODE_ALL,
    // Model action events
    ANALYTICS_MODEL_BOOLEAN,
    // OTA update events
    ANALYTICS_OTA_DOWNLOAD_START,
    ANALYTICS_OTA_DOWNLOAD_CANCEL,
    ANALYTICS_OTA_UPDATE_START,
    ANALYTICS_OTA_UPDATE_CANCEL,
    ANALYTICS_OTA_UPDATE_SUCCESS,
    ANALYTICS_OTA_UPDATE_FAIL,
    ANALYTICS_PRINT_VIDEO_CONNECT
};

struct AnalyticsEventPayload {
    AnalyticsDataEventType type;
    nlohmann::json data;
};

struct AnalyticsProjectInfo {
    std::string url;
    std::string file_id;
    std::string file_format;
    std::string model_id;
    std::string name;

    bool is_valid = false;
};

class AnalyticsDataUploadManager
{
public:
    static AnalyticsDataUploadManager& getInstance()
    {
        static std::unique_ptr<AnalyticsDataUploadManager> instance;
        static std::once_flag flag;
        std::call_once(flag, []() {
            instance.reset(new AnalyticsDataUploadManager());
        });
        return *instance;
    }

    ~AnalyticsDataUploadManager();

    void triggerUploadTasks(AnalyticsUploadTiming triggerTiming, const std::vector<AnalyticsDataEventType>& dataEventTypes, int plate_idx = 0, const std::string& device_mac = "");
    void triggerUploadTasksWithPayload(const AnalyticsEventPayload& payload, int plate_idx = 0, const std::string& device_mac = "");

    void mark_analytics_project_info(const std::string& full_url,
                                               const std::string& model_id,
                                               const std::string& file_id,
                                               const std::string& file_format,
                                               const std::string& name);

    void set_analytics_project_info_valid(bool valid);
    void clear_analytics_project_info();

    static void uploadSlice822ClickEvent(const std::string& module, int id=1);

    // ============================================================
    // 创想云神策埋点上报接口（新系统 - 独立区域）
    // 与原有的 Firebase Analytics 上报到不同的服务器地址
    // ============================================================
    
    /**
     * @brief 测试与服务器的连接性
     * 
     * @return true 如果连接成功
     * @note 仅用于调试，检查是否能连接到创想云服务器
     */
    static bool test_sensors_connection();
    
    /**
     * @brief 发送打印发送事件到创想云（event_key 按格式自动映射：3mf→print_002）
     * 
     * @param plate_idx 盘索引
     * @note 自动获取所有必需参数并转换为神策 SDK 格式发送到对应服务器
     */
    void send_print_send_event(int plate_idx);
    
    /**
     * @brief 统一的打印事件发送条件判断
     * 
     * 封装所有不上报的条件检查（模型修改、文件格式白名单等），
     * 供 send_print_send_event 和 cache_slice_info 两处统一使用。
     * 后续新增条件只需在此方法内追加。
     * 
     * @return true 如果满足所有上报条件
     */
    bool should_send_print_event() const;
    
    /**
     * @brief 发送神策埋点数据到创想云服务器（通用接口）
     * 
     * @param payload 完整的埋点数据 JSON 对象（由调用方封装好）
     * @note 会自动根据版本和地区选择正确的上报地址
     */
    void send_sensors_payload_to_creality(const nlohmann::json& payload);

    // ============================================================
    // 3MF文件指纹管理
    // ============================================================
    
    /**
     * @brief 计算3MF文件指纹（同步）
     * @param file_path 3MF文件路径
     * @return MD5指纹字符串（32位十六进制）
     */
    std::string computeModelFingerprint(const std::string& file_path);

    /**
     * @brief 异步计算3MF文件指纹并自动设置（不阻塞调用线程）
     * @param file_path 3MF文件路径
     * @param full_url 文件完整URL
     * @param file_format 文件格式
     * @param name 文件名
     * @note 计算完成后自动在主线程调用 mark_analytics_project_info
     */
    void computeAndSetModelIdAsync(
        const std::string& file_path,
        const std::string& full_url,
        const std::string& file_format,
        const std::string& name);

    /**
     * @brief 获取已缓存的指纹
     * @param file_path 3MF文件路径
     * @return 指纹字符串，如果未计算过返回空字符串
     */
    std::string getCachedFingerprint(const std::string& file_path);

    /**
     * @brief 清除所有指纹缓存
     */
    void clearFingerprintCache();

    // ============================================================
    // 项目几何体修改追踪（黑名单法）
    // ============================================================
    
    /**
     * @brief 几何体修改类型枚举（黑名单操作）
     */
    enum class ModelModifyType {
        REPAIR,              // 修复模型
        SIMPLIFY,            // 简化模型
        HOLLOW,              // 抽壳
        ADD_HOLE,            // 打洞
        CUT,                 // 剪切
        BOOLEAN,             // 布尔操作
        EMBOSS,              // 浮雕
        VARIABLE_LAYER,      // 可变层高
        SUPPORT_PAINT,       // 支撑绘制
        ZSEAM_PAINT,         // Z缝绘制
        SPLIT_OBJECTS,       // 分割为对象
        SPLIT_PARTS,         // 分割为部件
        ADD_PART,            // 添加部件
        DELETE_PART,         // 删除部件
        HEIGHT_RANGE,         // 高度范围修改
        ADD_NEGATIVE_VOLUME,  // Add negative volume
        ADD_SUPPORT_BLOCKER,  // Add support blocker
        ADD_SUPPORT_ENFORCER  // Add support enforcer
    };

    /**
     * @brief 项目几何体修改追踪器（全局单例）
     * 
     * 功能：
     * - 追踪项目是否被本质修改（几何体改变）
     * - 使用黑名单法：只有黑名单操作才会标记
     * - 每次导入新3MF时自动复位
     * - 阶段3：缓存切片信息（printer_info, slice_param）
     * - 按盘索引存储，每个盘独立缓存
     * - 包含切片参数采集功能（低侵入式设计）
     */
    class ProjectModificationTracker {
    public:
        // ============================================================
        // 参数采集功能
        // ============================================================
        
        // 参数类型枚举
        enum class ParamType {
            Float, FloatFirst, Int, IntFirst, Bool, BoolFirst,
            String, StringFirst, StringMulti, Percent, PercentFirst,
            FloatOrPercent, FloatOrPercentFirst, Enum
        };

        // 参数定义结构
        struct ParamDef {
            const char* config_key;
            const char* output_key;
            ParamType type;
        };

        // 采集参数（静态方法，供外部调用）
        // @param config DynamicPrintConfig引用
        // @return 采集到的参数JSON
        static nlohmann::json collect_params(const DynamicPrintConfig& config);

        // 采集对象/部件修改参数（阶段4新增）
        // @param plate PartPlate指针（用于获取当前盘的对象）
        // @param plate_idx 盘索引（用于日志）
        // @return obj_list JSON数组
        static nlohmann::json collect_obj_params(PartPlate* plate, int plate_idx);

    private:
        // 内部辅助函数
        static void add_param(const DynamicPrintConfig& config,
                              nlohmann::json& output,
                              const char* config_key, 
                              const char* output_key, 
                              ParamType type);
        static void collect_printer_params(const DynamicPrintConfig& config, nlohmann::json& output);
        static void collect_filament_params(const DynamicPrintConfig& config, nlohmann::json& output);
        static void collect_process_params(const DynamicPrintConfig& config, nlohmann::json& output);

        // 参数定义表（数组大小必须与CSV一致！）
        // printer_params: 5个, filament_params: 60个, process_params: 113个
        static const ParamDef s_printer_params[];
        static constexpr size_t s_printer_params_count = 5;
        static const ParamDef s_filament_params[];
        static constexpr size_t s_filament_params_count = 60;
        static const ParamDef s_process_params[];
        static constexpr size_t s_process_params_count = 187;

    private:
        bool m_is_modified = false;
        std::vector<ModelModifyType> m_modify_history;
        mutable std::mutex m_mutex;
        
        // 阶段3：切片信息缓存（按盘索引存储）
        std::map<int, std::string> m_printer_info;   // key=盘索引, value=JSON字符串
        std::map<int, std::string> m_slice_param;    // key=盘索引, value=JSON字符串
        std::map<int, std::string> m_filament_info;  // key=盘索引, value=JSON字符串
        
        // 【新增】当前盘的 task_id（供 GCode.cpp 读取并写入 G-code header）
        std::string m_current_task_id;
        
        // 【新增】当前盘索引（供 GCode.cpp 写入占位符时嵌入盘号）
        int m_current_plate_id = -1;
        
        // 【新增】按盘索引存储的 task_id（导出时生成，发送时复用，保证时间戳一致）
        std::map<int, std::string> m_plate_task_ids;
        
    public:
        static ProjectModificationTracker& getInstance();
        
        // 阶段2：标记修改（操作成功后调用）
        void mark_modified(ModelModifyType type);
        
        // 阶段2：查询是否被修改
        bool is_essentially_modified() const;
        
        // 阶段2+3：复位（导入新3MF时调用，清除所有状态）
        void reset();
        
        // 阶段2：获取修改历史（调试用）
        const std::vector<ModelModifyType>& get_history() const;
        
        // 阶段3：缓存切片信息（按盘索引）
        void cache_slice_info(int plate_idx, 
                              const std::string& printer_info,
                              const std::string& slice_param,
                              const std::string& filament_info);
        
        // 阶段3：获取已缓存的切片信息
        std::string get_printer_info(int plate_idx) const;
        std::string get_slice_param(int plate_idx) const;
        std::string get_filament_info(int plate_idx) const;
        // 导出全部GCode场景：合并所有盘的 objects + plates，global_param 取第0盘
        std::string get_merged_slice_param(int plate_count) const;
        // 返回已缓存的盘数（用于 export_all 场景推断 plate_count）
        size_t get_cached_plate_count() const;
        
        // 【新增】设置/获取当前 task_id（供 GCode.cpp 写入 G-code header）
        void set_current_task_id(const std::string& task_id);
        std::string get_current_task_id() const;
        
        // 【新增】设置/获取当前盘索引（供 GCode.cpp 写入占位符时嵌入盘号）
        void set_current_plate_id(int plate_id);
        int get_current_plate_id() const;
        
        // 【新增】生成 task_id（格式：{device_id}_{timestamp_ms}_{plate_id:02d}）
        static std::string generate_task_id(int plate_id);
        // 【新增】重载：使用外部传入的 timestamp 生成 task_id（用于多盘共享同一时间戳）
        static std::string generate_task_id(int plate_id, long long timestamp_ms);
        
        // 【新增】替换 gcode 文件中的 task_id 占位符为真实值（原地覆盖，不移动文件内容）
        static bool replace_task_id_placeholder_in_gcode(const std::string& gcode_path, const std::string& task_id);
        
        // 【新增】按盘索引存取 task_id（导出时写入，发送时复用，确保 gcode 和事件的时间戳一致）
        void set_plate_task_id(int plate_index, const std::string& task_id);
        std::string get_plate_task_id(int plate_index);
        
        // 【新增】清除指定盘的 task_id 缓存（用于强制重新生成）
        void clear_plate_task_id(int plate_index);
    };

private:
    AnalyticsDataUploadManager();
    
    AnalyticsDataUploadManager(const AnalyticsDataUploadManager&)            = delete;
    AnalyticsDataUploadManager& operator=(const AnalyticsDataUploadManager&) = delete;
    
    // 初始化环境和地区配置（仅在首次调用时执行一次）
    void init_sensors_config_if_needed();
    
    // 缓存的配置信息
    bool m_sensors_config_initialized = false;
    std::string m_sensors_upload_url;

    void processUploadData(AnalyticsDataEventType dataEventType, int plate_idx, const std::string& device_mac);

    void uploadGlobalPrintParams(int plate_idx, const std::string& device_mac);
    void uploadObjectPrintParams(int plate_idx,const std::string& device_mac);
    void uploadSlicePlateEventData();
    void uploadFirstLaunchEventData();
    void uploadPreferencesChangedData();
    void uploadSoftwareLaunchData();
    void uploadSoftwareCrashData();
    void uploadSoftwareBadAlloc();
    void uploadSoftwareCloseData();
    void uploadDeviceInfoData();
    void uploadAccountDeviceInfoData();
    void uploadOnlineModelsEvent();
    void uploadPrepareEvent();
    void uploadPreviewEvent();
    void uploadDeviceEvent();
    void uploadClickHomePageProjectsEvent();
    void uploadClickHomePageOnlineParamsEvent();
    void uploadClickHomePageTutorialsEvent();
    void uploadClickHomePagePersonCenterEvent();
    void uploadClickHomePageFeedbackEvent();
    void uploadClickHomePageMakenowEvent();
    void uploadClickHomePageCrealitymallEvent();
    void uploadModelActionAddEvent();
    void uploadModelActionAddPlateEvent();
    void uploadModelActionMoveEvent();
    void uploadModelActionRotateEvent();
    void uploadModelActionAutoOrientEvent();
    void uploadModelActionArrangeAllEvent();
    void uploadModelActionLayOnFaceEvent();
    void uploadModelActionSplitToObjectsEvent();
    void uploadModelActionSplitToPartsEvent();
    void uploadModelActionScaleEvent();
    void uploadModelActionHollowEvent();
    void uploadModelActionAddHoleEvent();
    void uploadModelActionCutEvent();
    void uploadModelActionBooleanEvent();
    void uploadModelActionMeasureEvent();
    void uploadModelActionSupportPaintEvent();
    void uploadModelActionZseamPaintEvent();
    void uploadModelActionVariableLayerEvent();
    void uploadModelActionPaintEvent();
    void uploadModelActionEmbossEvent();
    void uploadModelActionAssemblyViewEvent();
    void uploadAiServiceCallEvent();

    void track_model_action(const std::string& event_name, nlohmann::json& js);
    
    // Delayed sending of print_send event to ensure frontend page is ready
    void track_model_action_delayed_print_send(const nlohmann::json& js);
    void on_delayed_print_send_timer(nlohmann::json js);

private:
    AnalyticsProjectInfo m_analytics_project_info;

    // 指纹缓存：文件路径 -> 指纹
    std::unordered_map<std::string, std::string> m_fingerprint_cache;
    mutable std::mutex m_fingerprint_mutex;

    // 底层MD5计算函数
    std::string computeMD5(const std::string& file_path, size_t chunk_size = 1024 * 1024);

};

} // namespace GUI
} // namespace Slic3r

#endif // ANALYTICS_DATA_UPLOAD_MANAGER_HPP
