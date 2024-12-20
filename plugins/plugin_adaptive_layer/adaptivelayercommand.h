#ifndef ADAPTIVELAYERCOMMAND_H
#define ADAPTIVELAYERCOMMAND_H

#include <QPointer>

#include "qtusercore/plugin/toolcommand.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/module/pickable.h"
#include "adaptivelayermode.h"

class AdaptiveLayerMode;
class AdaptiveLayerCommand : public ToolCommand
                    //, public qtuser_3d::SelectorTracer 
{
  Q_OBJECT
  Q_PROPERTY(float speedVal READ speedVal WRITE setSpeedVal NOTIFY valChanged);
  Q_PROPERTY(int radiusVal READ radiusVal WRITE setRadiusVal NOTIFY valChanged);
  Q_PROPERTY(bool keepMin READ keepMin WRITE setKeepMin NOTIFY valChanged);
public:
  explicit AdaptiveLayerCommand(QObject* parent = nullptr);
  virtual ~AdaptiveLayerCommand();
  Q_INVOKABLE virtual bool checkSelect() override;
  Q_INVOKABLE void execute();


  Q_INVOKABLE void adapted();
  Q_INVOKABLE void smooth();
  Q_INVOKABLE void reset();

public:
  float speedVal() const { return m_speedVal; }
  void setSpeedVal(const float& val) { m_speedVal = val; }

  int radiusVal() const { return m_radius; }
  void setRadiusVal(const int& val) { m_radius = val; }

  bool keepMin() const { return m_bKeepMin; }
  void setKeepMin(const bool& val) { m_bKeepMin = val; }

Q_SIGNALS:
    void valChanged();
private:
  AdaptiveLayerMode * operation_mode_;
  float m_speedVal;
  int m_radius;
  bool m_bKeepMin;
};

#endif // ADAPTIVELAYERCOMMAND_H
