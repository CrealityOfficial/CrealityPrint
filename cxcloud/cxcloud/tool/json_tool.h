#pragma once

#include <list>
#include <optional>
#include <set>

#include <QtCore/QString>

#include <rapidjson/document.h>

namespace cxcloud {

  template<typename _Type>
  auto JsonGet(const rapidjson::Value& object, const char* key) -> std::optional<_Type>;

  template<>
  auto JsonGet<bool>(const rapidjson::Value& object, const char* key) -> std::optional<bool>;

  template<>
  auto JsonGet<int>(const rapidjson::Value& object, const char* key) -> std::optional<int>;

  template<>
  auto JsonGet<double>(const rapidjson::Value& object, const char* key) -> std::optional<double>;

  template<>
  auto JsonGet<const char*>(const rapidjson::Value& object,
                            const char* key) -> std::optional<const char*>;

  template<>
  auto JsonGet<QString>(const rapidjson::Value& object,
                        const char* key) -> std::optional<QString>;

  template<>
  auto JsonGet<std::set<QString>>(const rapidjson::Value& object,
                                  const char* key) -> std::optional<std::set<QString>>;

  template<>
  auto JsonGet<std::list<QString>>(const rapidjson::Value& object,
                                   const char* key) -> std::optional<std::list<QString>>;

}  // namespace cxcloud

#include "cxcloud/tool/json_tool.inl"
