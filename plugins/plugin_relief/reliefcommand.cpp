#include "reliefcommand.h"

#include <map>
#include <mutex>
#include <set>

#include <QtCore/QCoreApplication>
#include <QtCore/QDirIterator>
#include <QtCore/QVariant>
#include <QtGui/QFontDatabase>

#include <qtusercore/string/resourcesfinder.h>

#include "interface/visualsceneinterface.h"

namespace {

  const std::map<ReliefOperateMode::ReliefType, QString> TYPE_NAME_MAP{
    { ReliefOperateMode::ReliefType::Text, QStringLiteral("text") },
    { ReliefOperateMode::ReliefType::Shape, QStringLiteral("shape") },
  };

  const std::map<QString, ReliefOperateMode::ReliefType> NAME_TYPE_MAP{
    { QStringLiteral("text"), ReliefOperateMode::ReliefType::Text },
    { QStringLiteral("shape"), ReliefOperateMode::ReliefType::Shape },
  };

  const std::map<ReliefOperateMode::Shape, QString> SHAPE_NAME_MAP{
    { ReliefOperateMode::Shape::Rectangle, QStringLiteral("rectangle") },
    { ReliefOperateMode::Shape::Circle, QStringLiteral("circle") },
    { ReliefOperateMode::Shape::Triangle, QStringLiteral("triangle") },
  };

  const std::map<QString, ReliefOperateMode::Shape> NAME_SHAPE_MAP{
    { QStringLiteral("rectangle"), ReliefOperateMode::Shape::Rectangle },
    { QStringLiteral("circle"), ReliefOperateMode::Shape::Circle },
    { QStringLiteral("triangle"), ReliefOperateMode::Shape::Triangle },
  };

}  // namespace

ReliefCommand::ReliefCommand(QObject* parent) : ToolCommand{ nullptr }, operate_mode_{ nullptr } {
  setSource(QStringLiteral("qrc:/relief/info.qml"));
  orderindex = 4;
}

ReliefCommand::~ReliefCommand() {
  if (operate_mode_) {
    operate_mode_.release()->deleteLater();
  }
}

void ReliefCommand::execute() {
    if (!operate_mode_) {
        operate_mode_ = std::make_unique<ReliefOperateMode>();

        connect(operate_mode_.get(), &ReliefOperateMode::typeChanged, this, &ReliefCommand::typeChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::shapeChanged, this, &ReliefCommand::shapeChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::textChanged, this, &ReliefCommand::textChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::fontSizeChanged, this, &ReliefCommand::fontSizeChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::wordSpaceChanged, this, &ReliefCommand::wordSpaceChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::lineSpaceChanged, this, &ReliefCommand::lineSpaceChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::heightChanged, this, &ReliefCommand::heightChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::distanceChanged, this, &ReliefCommand::distanceChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::embossTypeChanged, this, &ReliefCommand::embossTypeChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::reliefModelTypeChanged, this, &ReliefCommand::reliefModelTypeChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::boldChanged, this, &ReliefCommand::boldChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::italicChanged, this, &ReliefCommand::italicChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::posChanged, this, &ReliefCommand::posChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::needIndicateChanged, this, &ReliefCommand::needIndicateChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::hoverOnModelChanged, this, &ReliefCommand::hoverOnModelChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::canEmbossChanged, this, &ReliefCommand::canEmbossChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::fontConfigChanged, this, &ReliefCommand::fontConfigChanged);
        connect(operate_mode_.get(), &ReliefOperateMode::distanceEnabledChanged, this, &ReliefCommand::distanceEnabledChanged);
  }

  creative_kernel::setVisOperationMode(operate_mode_.get());
}

auto ReliefCommand::getTypeModel() const -> QAbstractItemModel* {
  constexpr auto NAME_ROLE = Qt::ItemDataRole::UserRole + 1;

  if (!type_model_) {
    type_model_ = std::make_unique<QStandardItemModel>();
    type_model_->setItemRoleNames({
      { NAME_ROLE, QByteArrayLiteral("name") },
    });

    auto* text_item = new QStandardItem{};
    text_item->setData(TYPE_NAME_MAP.at(ReliefOperateMode::ReliefType::Text), NAME_ROLE);

    // auto* shape_item = new QStandardItem{};
    // shape_item->setData(TYPE_NAME_MAP.at(ReliefOperateMode::ReliefType::Shape), NAME_ROLE);

    //type_model_->appendColumn({ text_item, shape_item });

    type_model_->appendColumn({ text_item });
  }

  return type_model_.get();
}

auto ReliefCommand::getShapeModel() const -> QAbstractItemModel* {
  constexpr auto NAME_ROLE = Qt::ItemDataRole::UserRole + 1;

  if (!shape_model_) {
    shape_model_ = std::make_unique<QStandardItemModel>();
    shape_model_->setItemRoleNames({
      { NAME_ROLE, QByteArrayLiteral("name") },
    });

    auto* rectangle_item = new QStandardItem{};
    rectangle_item->setData(SHAPE_NAME_MAP.at(ReliefOperateMode::Shape::Rectangle), NAME_ROLE);

    auto* circle_item = new QStandardItem{};
    circle_item->setData(SHAPE_NAME_MAP.at(ReliefOperateMode::Shape::Circle), NAME_ROLE);

    auto* triangle_item = new QStandardItem{};
    triangle_item->setData(SHAPE_NAME_MAP.at(ReliefOperateMode::Shape::Triangle), NAME_ROLE);

    shape_model_->appendColumn({ rectangle_item, circle_item, triangle_item });
  }

  return shape_model_.get();
}

auto ReliefCommand::getFontModel() const -> QAbstractItemModel* {
  constexpr auto NAME_ROLE = Qt::ItemDataRole::UserRole + 1;
  constexpr auto PATH_ROLE = Qt::ItemDataRole::UserRole + 2;

  if (!font_model_) {
    static std::map<QString, QString> name_path_map{};
    static std::once_flag init_once;
    std::call_once(init_once, [] {
      static const auto BIN_PATH = QCoreApplication::applicationDirPath();
      static const std::set<QString> dir_paths{
        BIN_PATH,
        QStringLiteral("%1/resources").arg(BIN_PATH),
        QStringLiteral("%1/Resources/resources").arg(BIN_PATH.left(BIN_PATH.lastIndexOf('/'))),
        QStringLiteral("%1/resources").arg(qtuser_core::getOrCreateAppDataLocation()),
      };

      for (const auto& dir_path : dir_paths) {
        QDirIterator dir_iter{ QStringLiteral("%1/fonts").arg(dir_path) };
        while (dir_iter.hasNext()) {
          dir_iter.next();
          const auto file_info = dir_iter.fileInfo();
          if (file_info.isFile() && file_info.suffix() == QStringLiteral("ttf")) {
            name_path_map.emplace(file_info.baseName(), file_info.absoluteFilePath());
          }
        }
      }
    });

    font_model_ = std::make_unique<QStandardItemModel>();
    font_model_->setItemRoleNames({
      { NAME_ROLE, QByteArrayLiteral("name") },
      { PATH_ROLE, QByteArrayLiteral("path") },
    });

    QList<QStandardItem*> item_list{};
    for (const auto& pair : name_path_map) {
      auto* item = new QStandardItem{};
      item->setData(pair.first, NAME_ROLE);
      item->setData(pair.second, PATH_ROLE);
      item_list.push_back(item);
    }

    font_model_->appendColumn(std::move(item_list));
  }

  return font_model_.get();
}

QStringList ReliefCommand::fliterFontFamilies(QStringList fontFamilies)
{
  QStringList& fs = fontFamilies;
  QStringList discardFs;
  discardFs << "Bookshelf Symbol 7" <<
              "Marlett" <<
              "MS Outlook" <<
              "MS Reference Specialty" <<
              "Symbol" <<
              "Webdings" <<
              "Wingdings" <<
              "Wingdings 2" <<
              "Wingdings 3" <<
              "MT Extra" <<
              "Cambria Math" <<
              "Javanese Text" <<
              "Modern No. 20" <<
              "Script" <<
              "Segoe Print" <<
              "Segoe Script" <<
              "新宋体" <<
              "Snap ITC" <<
              "Source Han Sans CN" <<
              "思源黑体 CN Bold" <<
              "思源黑体 CN Normal";
  
  for (int i = 0, count = fs.count(); i < count; ++i)
  {
    int index = discardFs.indexOf(fs[i]);
    if (index != -1)
    {
      fs.removeAt(i);
      discardFs.removeAt(index);
      i--;
      count--;
    }
    if (discardFs.isEmpty())
    break;
  }

  return fs;
}

QString ReliefCommand::getType() const {
  if (!operate_mode_) {
    return TYPE_NAME_MAP.at(ReliefOperateMode::ReliefType::Text);
  }

  const auto iter = TYPE_NAME_MAP.find(operate_mode_->getType());
  if (iter == TYPE_NAME_MAP.cend()) {
    return TYPE_NAME_MAP.at(ReliefOperateMode::ReliefType::Text);
  }

  return iter->second;
}

void ReliefCommand::setType(const QString& type) {
  if (!operate_mode_ || type == getType()) {
    return;
  }

  const auto iter = NAME_TYPE_MAP.find(type);
  if (iter == NAME_TYPE_MAP.cend()) {
    return;
  }

  operate_mode_->setType(iter->second);
  typeChanged();
}

QString ReliefCommand::getShape() const {
  if (!operate_mode_) {
    return SHAPE_NAME_MAP.at(ReliefOperateMode::Shape::Rectangle);
  }

  const auto iter = SHAPE_NAME_MAP.find(operate_mode_->getShape());
  if (iter == SHAPE_NAME_MAP.cend()) {
    return SHAPE_NAME_MAP.at(ReliefOperateMode::Shape::Rectangle);
  }

  return iter->second;
}

void ReliefCommand::setShape(const QString& shape) {
  if (!operate_mode_ || shape == getShape()) {
    return;
  }

  const auto iter = NAME_SHAPE_MAP.find(shape);
  if (iter == NAME_SHAPE_MAP.cend()) {
    return;
  }

  operate_mode_->setShape(iter->second);
  shapeChanged();
}

QString ReliefCommand::getText() const {
  return operate_mode_ ? operate_mode_->getText() : QString{};
}

void ReliefCommand::setText(const QString& text) {
  if (operate_mode_ && operate_mode_->getText() != text) {
    operate_mode_->setText(text);
    textChanged();
  }
}

QString ReliefCommand::getFont() const {
  return operate_mode_ ? operate_mode_->getFont() : QString{};
}

void ReliefCommand::setFont(const QString& text) {
  if (operate_mode_ && operate_mode_->getFont() != text) {
    operate_mode_->setFont(text);
    fontChanged();
  }
}

int ReliefCommand::getFontSize() const {
  return operate_mode_ ? operate_mode_->fontSize() : 1;
}

void ReliefCommand::setFontSize(int size) {
  if (operate_mode_ && operate_mode_->fontSize() != size) {
    operate_mode_->setFontSize(size);
  }
}

int ReliefCommand::getWordSpace() const {
  return operate_mode_ ? operate_mode_->wordSpace() : 50;
}

void ReliefCommand::setWordSpace(int space) {
  if (operate_mode_ && operate_mode_->wordSpace() != space) {
    operate_mode_->setWordSpace(space);
  }
}

int ReliefCommand::getLineSpace() const {
  return operate_mode_ ? operate_mode_->lineSpace() : 50;
}

void ReliefCommand::setLineSpace(int space) {
  if (operate_mode_ && operate_mode_->lineSpace() != space) {
    operate_mode_->setLineSpace(space);
  }
}

float ReliefCommand::getHeight() const {
  return operate_mode_ ? operate_mode_->height() : 50;
}

void ReliefCommand::setHeight(float height) {
  if (operate_mode_ && operate_mode_->height() != height) {
    operate_mode_->setHeight(height);
  }
}

float ReliefCommand::getDistance() const {
  return operate_mode_ ? operate_mode_->distance() : 0;
}

void ReliefCommand::setDistance(float distance)  {
  if (operate_mode_ && operate_mode_->distance() != distance) {
    operate_mode_->setDistance(distance);
  }
}

int ReliefCommand::embossType() const
{
  return operate_mode_->embossType();
}

void ReliefCommand::setEmbossType(int type)
{
  operate_mode_->setEmbossType(type);
}

int ReliefCommand::reliefModelType() const
{
  return operate_mode_->reliefModelType();
}

void ReliefCommand::setReliefModelType(int type)
{
  operate_mode_->setReliefModelType(type);
}

bool ReliefCommand::getBold() const
{
  return operate_mode_->bold();
}

void ReliefCommand::setBold(bool bold)
{
  operate_mode_->setBold(bold);
}

bool ReliefCommand::getItalic() const
{
  return operate_mode_->italic();
}

void ReliefCommand::setItalic(bool italic)
{
  operate_mode_->setItalic(italic);
}

QPoint ReliefCommand::getPos() const
{
  return operate_mode_->pos();
}

bool ReliefCommand::needIndicate() const
{
  return operate_mode_->needIndicate();
}

bool ReliefCommand::hoverOnModel() const
{
  return operate_mode_->hoverOnModel();
}

bool ReliefCommand::canEmboss() const
{
  return operate_mode_->canEmboss();
}

bool ReliefCommand::distanceEnabled() const
{
  return operate_mode_->distanceEnabled();
}

void ReliefCommand::onThemeChanged(creative_kernel::ThemeCategory theme) {
  switch (theme) {
    case creative_kernel::ThemeCategory::tc_dark: {
      setDisabledIcon(QStringLiteral("qrc:/relief/image/execute_disabled.svg"));
      setEnabledIcon(QStringLiteral("qrc:/relief/image/execute_enabled.svg"));
      setHoveredIcon(QStringLiteral("qrc:/relief/image/execute_hovered.svg"));
      setPressedIcon(QStringLiteral("qrc:/relief/image/execute_pressed.svg"));
      break;
    }
    case creative_kernel::ThemeCategory::tc_light: {
      setDisabledIcon(QStringLiteral("qrc:/relief/image/execute_disabled_light.svg"));
      setEnabledIcon(QStringLiteral("qrc:/relief/image/execute_enabled_light.svg"));
      setHoveredIcon(QStringLiteral("qrc:/relief/image/execute_hovered_light.svg"));
      setPressedIcon(QStringLiteral("qrc:/relief/image/execute_pressed_light.svg"));
      break;
    }
    case creative_kernel::ThemeCategory::tc_num:
    default: {
      break;
    }
  }
}

void ReliefCommand::onLanguageChanged(creative_kernel::MultiLanguage language) {
  setName(QCoreApplication::translate("ReliefPlugin", "Emboss"));
}
