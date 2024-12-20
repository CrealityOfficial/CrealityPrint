# 设置定制类型(类型名与对应 cmake 文件对应), 为空则不定制
# set(CUSTOM_TYPE "Piocreat")
# set(CUSTOM_TYPE "PiocreatChenfei")

if (NOT DEFINED CUSTOM_TYPE)
  set(CUSTOM_TYPE "")
endif()

# 添加定制化宏
function(AppendCustomizedMarco marco_name)
  set(_CUSTOMIZED_MARCOS_DEFINE "${_CUSTOMIZED_MARCOS_DEFINE}\n#define ${marco_name}" PARENT_SCOPE)
endfunction()

function(AppendCustomizedMarcoNoWrap marco_name)
  set(_CUSTOMIZED_MARCOS_DEFINE "${_CUSTOMIZED_MARCOS_DEFINE}#define ${marco_name}" PARENT_SCOPE)
endfunction()

# 添加定制化宏字符串
function(AppendCustomizedMarcoString marco_name marco_string)
  set(_CUSTOMIZED_MARCOS_DEFINE "${_CUSTOMIZED_MARCOS_DEFINE}\n#define ${marco_name} \"${marco_string}\"" PARENT_SCOPE)
endfunction()


# 目前有效的定制化宏:
# CUSTOMIZED             # 当前为定制化程序 (默认添加)
# CUSTOM_TYPE            # 当前定制化程序类型 (默认添加)
# CUSTOM_MACHINE_LIST    # 使用自定义的设备列表
# CUSTOM_MATERIAL_LIST   # 使用自定义的耗材列表
# CUSTOM_CXCLOUD_ENABLED # 开启创想云功能 (登录, 个人中心, 模型库, 下载管理, 上传&下载模型/切片)
# CUSTOM_UN_UPGRADEABLE  # 不可更新 (只在创想云功能开启时生效)
# CUSTOM_UN_FEEDBACKABLE # 不可反馈
# CUSTOM_UN_TEACHABLE    # 不可查看教程
# CUSTOM_HOLLOW_ENABLED       # 开启掏空功能
# CUSTOM_HOLLOW_FILL_ENABLED  # 开启掏空功能的填充选项 (只在掏空功能开启时生效)
# CUSTOM_SLICE_HEIGHT_SETTING_ENABLED # 开启切片高度设置功能 (成飞定制)
# CUSTOM_PARTITION_PRINT_ENABLED      # 开启分区打印功能 (成飞定制)

set(_CUSTOMIZED_MARCOS_DEFINE "")
if (NOT ${CUSTOM_TYPE} STREQUAL "")
  message(STATUS "build 3rd customized project")
  AppendCustomizedMarcoNoWrap(CUSTOMIZED)
  AppendCustomizedMarcoString(CUSTOM_TYPE ${CUSTOM_TYPE})
  include(customized/${CUSTOM_TYPE}.cmake)
endif()
