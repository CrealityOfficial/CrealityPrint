# ---------- dumptool 要上传的内容选项 [beg] ----------

if(NOT DEFINED DUMPTOOL_OPTION_DUMP_FILE)
  set(DUMPTOOL_OPTION_DUMP_FILE "true") # 本次生成的 dump 文件
endif()

if(NOT DEFINED DUMPTOOL_OPTION_SCENE_FILE)
  set(DUMPTOOL_OPTION_SCENE_FILE "true") # 最后一次切片的 scene 文件
endif()

if(NOT DEFINED DUMPTOOL_OPTION_INFO_JSON)
  set(DUMPTOOL_OPTION_INFO_JSON "true") # 携带显卡型号, opengl 版本 & 供应商 & 渲染器 信息的 json 文件
endif()

# ---------- dumptool 要上传的内容选项 [end] ----------

if(NOT DEFINED DUMPTOOL_ACCESS_KEY_ID)
  set(DUMPTOOL_ACCESS_KEY_ID $ENV{DUMPTOOL_ACCESS_KEY_ID})
endif()

if(NOT DEFINED DUMPTOOL_ACCESS_KEY_SECRET)
  set(DUMPTOOL_ACCESS_KEY_SECRET $ENV{DUMPTOOL_ACCESS_KEY_SECRET})
endif()

if(NOT DEFINED DUMPTOOL_ENDPOINT)
  set(DUMPTOOL_ENDPOINT "http://oss-cn-hangzhou.aliyuncs.com")
endif()

if(NOT DEFINED DUMPTOOL_BUCKET)
  set(DUMPTOOL_BUCKET "c-smart-upgrade-pkg-test")
endif()

if(NOT DEFINED DUMPTOOL_ENDPOINT_FOREIGN)
  set(DUMPTOOL_ENDPOINT_FOREIGN "oss-us-east-1.aliyuncs.com")
endif()

if(NOT DEFINED DUMPTOOL_BUCKET_FOREIGN)
  set(DUMPTOOL_BUCKET_FOREIGN "upgrade-pkg")
endif()

if(NOT DEFINED DUMPTOOL_LOG_DIR)
  set(DUMPTOOL_LOG_DIR "slice_logs/")
endif()
