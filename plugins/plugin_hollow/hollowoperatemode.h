#ifndef HOLLOWOPERATEMODE_H_
#define HOLLOWOPERATEMODE_H_

#include <set>

#include <QtCore/QTimer>

#include "qtuser3d/scene/sceneoperatemode.h"
#include "data/modeln.h"
#include "operation/moveoperatemode.h"

class HollowOperateMode : public creative_kernel::MoveOperateMode {
  Q_OBJECT;

public:
  explicit HollowOperateMode(QObject* parent = nullptr);
  virtual ~HollowOperateMode();

public:
  void hollow(float thinkness, int type = 0);
  bool isPopupVisible();

  Q_INVOKABLE void accept();
  Q_INVOKABLE void cancel();

public:
  Q_INVOKABLE QString getMessageText();
  void setMessageText(const QString& message);

public:
  Q_INVOKABLE void setFillEnabled(bool fill);
  Q_INVOKABLE bool isFillEnabled() const;

public:
  Q_INVOKABLE float getFillRatio() const;
  Q_INVOKABLE void setFillRatio(float ratio);

protected:
  virtual void onAttach() override;
  virtual void onDettach() override;

private:
  void setNoSelectedModelsVisiable(bool visiable);
  void recordModelsVisibility();
  void resetModelsVisibility() const;

private:
  Q_SLOT void jobFinished(bool successed);
  Q_SLOT void timerUpdate();

private:
  std::set<creative_kernel::ModelN*> unvisible_model_set_;

  QTimer timer_;
  float unvisible_height_percent_;
  float model_max_height_;

  float fill_ratio_;
  bool fill_enabled_;

  bool popup_visible_;
  QString message_text_;
};

#endif // HOLLOWOPERATEMODE_H_
