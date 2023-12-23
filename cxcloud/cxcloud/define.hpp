#pragma once

#ifndef CXCLOUD_DEFINE_HPP_
#define CXCLOUD_DEFINE_HPP_

#include <QString>

namespace cxcloud {

/// @brief Some http request behaviors will have differences at defferent application.
///        Such as recommend model list in model library or newest application version at website.
enum class ApplicationType : int {
  CREATIVE_PRINT = 11,
  HALOT_BOX      = 12,
};

enum class ServerType : int {
	MAINLAND_CHINA = 0,
	INTERNATIONAL = 1,
};

struct UserBaseInfo {
  QString token{};
  QString user_id{};
};

struct UserDetailInfo {
  QString avatar{};
  QString nick_name{};
  double max_storage_space{0.0};
  double used_storage_space{0.0};
};

}  // namespace cxcloud

#endif  // CXCLOUD_DEFINE_HPP_
