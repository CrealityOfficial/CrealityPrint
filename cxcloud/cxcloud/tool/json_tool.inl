#pragma once

namespace cxcloud {

  template<typename _Type>
  auto JsonGet(const rapidjson::Value& object, const char* key) -> std::optional<_Type> {
    return std::nullopt;
  }

  template<>
  auto JsonGet<bool>(const rapidjson::Value& object, const char* key) -> std::optional<bool> {
    if (object.HasMember(key) && object[key].IsBool()) {
      return std::make_optional(object[key].GetBool());
    }

    return std::nullopt;
  }

  template<>
  auto JsonGet<int>(const rapidjson::Value& object, const char* key) -> std::optional<int> {
    if (object.HasMember(key) && object[key].IsInt()) {
      return std::make_optional(object[key].GetInt());
    }

    return std::nullopt;
  }

  template<>
  auto JsonGet<double>(const rapidjson::Value& object, const char* key) -> std::optional<double> {
    if (object.HasMember(key) && object[key].IsNumber()) {
      return std::make_optional(object[key].GetDouble());
    }

    return std::nullopt;
  }

  template<>
  auto JsonGet<const char*>(const rapidjson::Value& object,
                            const char* key) -> std::optional<const char*> {
    if (object.HasMember(key) && object[key].IsString()) {
      return std::make_optional(object[key].GetString());
    }

    return std::nullopt;
  }

  template<>
  auto JsonGet<QString>(const rapidjson::Value& object,
                        const char* key) -> std::optional<QString> {
    if (object.HasMember(key) && object[key].IsString()) {
      return std::make_optional(QString{ object[key].GetString() });
    }

    return std::nullopt;
  }

  template<>
  auto JsonGet<std::set<QString>>(const rapidjson::Value& object,
                                  const char* key) -> std::optional<std::set<QString>> {
    if (!object.HasMember(key) || !object[key].IsArray()) {
      return std::nullopt;
    }

    const auto array = object[key].GetArray();
    if (array.Empty()) {
      return std::nullopt;
    }

    auto set = std::set<QString>{};
    for (const auto& value : array) {
      if (value.IsString()) {
        set.emplace(QString{ value.GetString() });
      }
    }

    return set.empty() ? std::nullopt : std::make_optional(std::move(set));
  }

  template<>
  auto JsonGet<std::list<QString>>(const rapidjson::Value& object,
                                   const char* key) -> std::optional<std::list<QString>> {
    if (!object.HasMember(key) || !object[key].IsArray()) {
      return std::nullopt;
    }

    const auto array = object[key].GetArray();
    if (array.Empty()) {
      return std::nullopt;
    }

    auto list = std::list<QString>{};
    for (const auto& value : array) {
      if (value.IsString()) {
        list.emplace_back(QString{ value.GetString() });
      }
    }

    return list.empty() ? std::nullopt : std::make_optional(std::move(list));
  }

}  // namespace cxcloud
