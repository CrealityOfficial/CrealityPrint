#ifndef DUMPTOOL_WIZARDUI_HPP_
#define DUMPTOOL_WIZARDUI_HPP_

#include <QGuiApplication>
#include <QFontMetrics>
#include <QObject>
#include <QCursor>
#include <QPoint>
#include <QFont>

class WizardUi : public QObject {
  Q_OBJECT;

public:
  explicit WizardUi(QObject* parent = nullptr)
    : QObject(parent) {
  }

  virtual ~WizardUi() {
  }

  Q_INVOKABLE QPoint cursorGlobalPos() const {
    return QCursor::pos();
  }

  Q_INVOKABLE float getScreenScaleFactor() {
#ifdef __APPLE__
    QFont font = qApp->font();
    font.setPointSize(12);
    qApp->setFont(font);
    return 1;
#else
    QFont font = qApp->font();
    font.setPointSize(10);
    qApp->setFont(font);
    //int ass = QFontMetrics(font).ascent();
    float fontPixelRatio = QFontMetrics(qApp->font()).ascent() / 11.0;
    fontPixelRatio = int(fontPixelRatio * 4) / 4.0;
    return fontPixelRatio;
#endif
  }
};

#endif // !DUMPTOOL_WIZARDUI_HPP_
