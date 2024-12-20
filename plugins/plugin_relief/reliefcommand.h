#pragma once

#include <memory>

#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>
#include <QtGui/QStandardItemModel>
#include <qobject.h>

#include <qtusercore/module/creativeinterface.h>
#include <qtusercore/plugin/toolcommand.h>

#include <data/interface.h>

#include "reliefoperatemode.h"


class ReliefCommand : public ToolCommand, public creative_kernel::UIVisualTracer {
  Q_OBJECT;

 public:
  explicit ReliefCommand(QObject* parent = nullptr);
  virtual ~ReliefCommand() override;

 public:
  Q_INVOKABLE void execute();

  auto getTypeModel() const -> QAbstractItemModel*;
  Q_PROPERTY(QAbstractItemModel* typeModel READ getTypeModel CONSTANT);

  auto getShapeModel() const -> QAbstractItemModel*;
  Q_PROPERTY(QAbstractItemModel* shapeModel READ getShapeModel CONSTANT);

  auto getFontModel() const -> QAbstractItemModel*;
  Q_PROPERTY(QAbstractItemModel* fontModel READ getFontModel CONSTANT);

 public:  // R/W parameter
  Q_INVOKABLE QStringList fliterFontFamilies(QStringList fontFamilies);

	QString getType() const;
	void setType(const QString& type);
  Q_SIGNAL void typeChanged() const;
  Q_PROPERTY(QString type READ getType WRITE setType NOTIFY typeChanged);

  QString getShape() const;
  void setShape(const QString& shape);
  Q_SIGNAL void shapeChanged() const;
  Q_PROPERTY(QString shape READ getShape WRITE setShape NOTIFY shapeChanged);

  auto getText() const -> QString;
  auto setText(const QString& text) -> void;
  Q_SIGNAL void textChanged() const;
  Q_PROPERTY(QString text READ getText WRITE setText NOTIFY textChanged);

  QString getFont() const;
  void setFont(const QString& text);
  Q_SIGNAL void fontChanged() const;
  Q_PROPERTY(QString font READ getFont WRITE setFont NOTIFY fontChanged);
  
  int getFontSize() const;
  void setFontSize(int text);
  Q_SIGNAL void fontSizeChanged() const;
  Q_PROPERTY(int fontSize READ getFontSize WRITE setFontSize NOTIFY fontSizeChanged);
  
  int getWordSpace() const;
  void setWordSpace(int space);
  Q_SIGNAL void wordSpaceChanged() const;
  Q_PROPERTY(int wordSpace READ getWordSpace WRITE setWordSpace NOTIFY wordSpaceChanged);

  int getLineSpace() const;
  void setLineSpace(int lineSpace);
  Q_SIGNAL void lineSpaceChanged() const;
  Q_PROPERTY(int lineSpace READ getLineSpace WRITE setLineSpace NOTIFY lineSpaceChanged);

  float getHeight() const;
  void setHeight(float height);
  Q_SIGNAL void heightChanged() const;
  Q_PROPERTY(float height READ getHeight WRITE setHeight NOTIFY heightChanged);

  float getDistance() const;
  void setDistance(float distance);
  Q_SIGNAL void distanceChanged() const;
  Q_PROPERTY(float distance READ getDistance WRITE setDistance NOTIFY distanceChanged);

  int embossType() const;
  void setEmbossType(int type);
  Q_SIGNAL void embossTypeChanged();
  Q_PROPERTY(int embossType READ embossType WRITE setEmbossType NOTIFY embossTypeChanged);

  int reliefModelType() const;
  void setReliefModelType(int type);
  Q_SIGNAL void reliefModelTypeChanged();
  Q_PROPERTY(int reliefModelType READ reliefModelType WRITE setReliefModelType NOTIFY reliefModelTypeChanged);

  bool getBold() const;
  void setBold(bool bold);
  Q_SIGNAL void boldChanged() const;
  Q_PROPERTY(bool bold READ getBold WRITE setBold NOTIFY boldChanged);
  
  bool getItalic() const;
  void setItalic(bool italic);
  Q_SIGNAL void italicChanged() const;
  Q_PROPERTY(bool italic READ getItalic WRITE setItalic NOTIFY italicChanged);
  
  QPoint getPos() const;
  Q_SIGNAL void posChanged() const;
  Q_PROPERTY(QPoint pos READ getPos NOTIFY posChanged);

  bool needIndicate() const;
  Q_SIGNAL void needIndicateChanged();
  Q_PROPERTY(bool needIndicate READ needIndicate NOTIFY needIndicateChanged);

  bool hoverOnModel() const;
  Q_SIGNAL void hoverOnModelChanged();
  Q_PROPERTY(bool hoverOnModel READ hoverOnModel NOTIFY hoverOnModelChanged);

  bool canEmboss() const;
  Q_SIGNAL void canEmbossChanged();
  Q_PROPERTY(bool canEmboss READ canEmboss NOTIFY canEmbossChanged);

  bool distanceEnabled() const;
  Q_SIGNAL void distanceEnabledChanged();
  Q_PROPERTY(bool distanceEnabled READ distanceEnabled NOTIFY distanceEnabledChanged);

  Q_SIGNAL void fontConfigChanged();
  
 protected:
	 void onThemeChanged(creative_kernel::ThemeCategory theme) override;
	 void onLanguageChanged(creative_kernel::MultiLanguage language) override;

 private:
  mutable std::unique_ptr<QStandardItemModel> type_model_{ nullptr };
  mutable std::unique_ptr<QStandardItemModel> font_model_{ nullptr };
  mutable std::unique_ptr<QStandardItemModel> shape_model_{ nullptr };
  std::unique_ptr<ReliefOperateMode>          operate_mode_{ nullptr };
};
