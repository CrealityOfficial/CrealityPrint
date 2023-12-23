#include "cxcloud/tool/function.h"

#include <list>
#include <set>

#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>

namespace cxcloud {

void ReplaceIllegalCharacters(QString& file_name, QChar const& legal_character) {
  static std::list<QChar> const ILLEGAL_CHARACTER_LIST{
#if defined(Q_OS_WIN)
    '\\', '/', ':', '*', '?', '<', '>', '|',
#elif defined(Q_OS_MAC)
    '/', ':', '.',
#elif defined(Q_OS_LINUX)
    '/',
#endif
  };

  for (auto const& illegal_character : ILLEGAL_CHARACTER_LIST) {
    file_name.replace(illegal_character, legal_character);
  }
}

void ConvertNonRepeatingPath(QString const& dir_path, QString& base_name) {
  QDirIterator dir_iter{ dir_path, QDir::Filter::Dirs
                                 | QDir::Filter::Files
                                 | QDir::Filter::NoDotAndDotDot
                                 | QDir::Filter::NoSymLinks };
  QRegExp const reg_exp{ QStringLiteral("%1(_[0-9]{1,}){0,1}").arg(base_name) };  // such as: xxx_1
  std::set<size_t> existed_index_set;

  while (dir_iter.hasNext()) {
    dir_iter.next();

    auto const local_base_name = dir_iter.fileInfo().baseName();
    if (reg_exp.exactMatch(local_base_name)) {
      auto const split_list = local_base_name.split(QStringLiteral("_"));
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

  size_t index = 0ull;
  while (existed_index_set.find(++index) != existed_index_set.cend()) {}
  base_name = QStringLiteral("%1_%2").arg(base_name).arg(QString::number(index));
  return;
}

}  // namespace cxcloud
