#ifndef _EXTRUDER_OUTPUT_MODEL_H
#define _EXTRUDER_OUTPUT_MODEL_H

#include <QtCore>
#include "PrinterOutputModel.h"

class ExtruderOutputModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool canPreHeatHotends READ canPreHeatHotends)
    //Q_PROPERTY(QString activeMaterial READ activeMaterial NOTIFY extruderConfigurationChanged)
    Q_PROPERTY(float targetHotendTemperature READ targetHotendTemperature NOTIFY targetHotendTemperatureChanged)
    Q_PROPERTY(float hotendTemperature READ hotendTemperature NOTIFY hotendTemperatureChanged)
    Q_PROPERTY(QString hotendID READ hotendID NOTIFY extruderConfigurationChanged)
    //Q_PROPERTY(QVariant extruderConfiguration READ extruderConfiguration NOTIFY extruderConfigurationChanged)
    Q_PROPERTY(bool isPreheating READ isPreheating NOTIFY isPreheatingChanged)
public:
    ExtruderOutputModel();
    ExtruderOutputModel(PrinterOutputModel* printer, int position);

public:
    bool canPreHeatHotends() {
        return true;
    }

    float targetHotendTemperature() {
        return _target_hotend_temperature;
    }
    void updateTargetHotendTemperature(float temperature) {
        if (_target_hotend_temperature != temperature)
        {
            _target_hotend_temperature = temperature;
            emit targetHotendTemperatureChanged();
        }
    }

    float hotendTemperature() {
        return _hotend_temperature;
    }
    void updateHotendTemperature(float temperature) {
        if (_hotend_temperature != temperature)
        {
            _hotend_temperature = temperature;
            emit hotendTemperatureChanged();
        }
    }

    QString hotendID() {
        return "";
    }

    bool isPreheating() {
        return _is_preheating;
    }

    void updateIsPreheating(bool pre_heating) {
        if (_is_preheating != pre_heating)
        {
            _is_preheating = pre_heating;
            emit isPreheatingChanged();
        }
    }

    int getPosition() const {
        return _position;
    }

    PrinterOutputModel* getPrinter() const {
        return _printer;
    }


private:
    int _position;
    float _target_hotend_temperature = 0;
    float _hotend_temperature = 0;
    bool _is_preheating = false;
    PrinterOutputModel* _printer = nullptr;

signals:
    void targetHotendTemperatureChanged();
    void hotendTemperatureChanged();
    void extruderConfigurationChanged();
    void isPreheatingChanged();

public slots:
    void preheatHotend(float temperature, float duration);
    void cancelPreheatHotend();
};
#endif