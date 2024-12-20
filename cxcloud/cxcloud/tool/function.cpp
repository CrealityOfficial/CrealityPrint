#include "cxcloud/tool/function.h"

#include <list>
#include <set>

#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QDebug>

#include <boost/asio.hpp>

#include "cxcloud/tool/settings.h"

namespace cxcloud {

  auto ReplaceIllegalCharacters(QString& file_name, QChar const& legal_character) -> void {
    static const auto ILLEGAL_CHARACTER_LIST = std::list<QChar>{
#if defined(Q_OS_WIN)
      '\\', '/', ':', '*', '?', '<', '>', '|',
#elif defined(Q_OS_MAC)
      '/', ':', '.',
#elif defined(Q_OS_LINUX)
      '/',
#endif
    };

    for (const auto& illegal_character : ILLEGAL_CHARACTER_LIST) {
      file_name.replace(illegal_character, legal_character);
    }
  }

  auto ConvertNonRepeatingPath(QString const& dir_path, QString& base_name) -> void {
    QDirIterator dir_iter{ dir_path, QDir::Filter::Dirs
                                   | QDir::Filter::Files
                                   | QDir::Filter::NoDotAndDotDot
                                   | QDir::Filter::NoSymLinks };
    //  such as xxx_1
    const auto reg_exp = QRegExp{ QStringLiteral("%1(_[0-9]{1,}){0,1}").arg(base_name) };
    auto existed_index_set = std::set<size_t>{};

    while (dir_iter.hasNext()) {
      dir_iter.next();

      const auto local_base_name = dir_iter.fileInfo().baseName();
      if (reg_exp.exactMatch(local_base_name)) {
        const auto split_list = local_base_name.split(QStringLiteral("_"));
        if (split_list.empty()) {
          existed_index_set.emplace(0ull);
        } else {
          existed_index_set.emplace(split_list.last().toULongLong());
        }
      }
    }

    if (existed_index_set.empty()) {
      return;
    }

    auto index = 0ull;
    while (existed_index_set.find(++index) != existed_index_set.cend()) {}
    base_name = QStringLiteral("%1_%2").arg(base_name).arg(QString::number(index));
    return;
  }

  auto ModelRestrictionToString(ModelRestriction moderation) -> QString {
    static const std::map<ModelRestriction, QString> MODEL_RESTRICTION_STRING_MAP{
      { ModelRestriction::HIDE   , QStringLiteral("hide")   },
      { ModelRestriction::BLUR   , QStringLiteral("blur")   },
      { ModelRestriction::IGNORE_, QStringLiteral("ignore") },
    };

    const auto iter = MODEL_RESTRICTION_STRING_MAP.find(moderation);
    if (iter == MODEL_RESTRICTION_STRING_MAP.cend()) {
      return MODEL_RESTRICTION_STRING_MAP.at(DEFAULT_MODEL_RESTRICTION);
    }

    return iter->second;
  }

  auto StringToModelRestriction(const QString& moderation) -> ModelRestriction {
    static const std::map<QString, ModelRestriction> STRING_MODEL_RESTRICTION_MAP{
      { QStringLiteral("hide")  , ModelRestriction::HIDE    },
      { QStringLiteral("blur")  , ModelRestriction::BLUR    },
      { QStringLiteral("ignore"), ModelRestriction::IGNORE_ },
    };

    const auto iter = STRING_MODEL_RESTRICTION_MAP.find(moderation);
    if (iter == STRING_MODEL_RESTRICTION_MAP.cend()) {
      return DEFAULT_MODEL_RESTRICTION;
    }

    return iter->second;
  }

  auto CheckNetworkAvailable() -> bool {
    constexpr auto baidu  = "www.baidu.com";
    constexpr auto google = "www.google.com";

    const auto is_mainland_china = ServerTypeToRealServerType(CloudSettings{}.getServerType()) ==
                                   RealServerType::MAINLAND_CHINA;

    auto io_context = boost::asio::io_context{};
    auto resolver   = boost::asio::ip::tcp::resolver{ io_context };
    auto socket     = boost::asio::ip::tcp::socket{ io_context };

    try {
      boost::asio::connect(socket, resolver.resolve(is_mainland_china ? baidu : google, "80"));
    } catch (const std::exception& exception) {
      qDebug() << QStringLiteral("network: %1").arg(QString::fromLocal8Bit(exception.what()));
      return false;
    }

    return true;
  }

}  // namespace cxcloud
