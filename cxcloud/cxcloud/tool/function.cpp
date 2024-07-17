#include "cxcloud/tool/function.h"

#include <list>
#include <set>

#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>

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

}  // namespace cxcloud
