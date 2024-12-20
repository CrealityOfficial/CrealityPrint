#ifndef _PRINTER_OUTPUT_MODEL_H
#define _PRINTER_OUTPUT_MODEL_H

#include <QtCore>
#include "PrintJobOutputModel.h"
#include "ExtruderOutputModel.h"
#include "PrinterOutputController.h"

class PrinterOutputModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* activePrintJob READ activePrintJob NOTIFY activePrintJobChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QList<QObject*> extruders READ extruders)
    Q_PROPERTY(float bedTemperature READ bedTemperature NOTIFY bedTemperatureChanged)
    Q_PROPERTY(float targetBedTemperature READ targetBedTemperature NOTIFY targetBedTemperatureChanged)
    Q_PROPERTY(bool isPreheating READ isPreheating NOTIFY isPreheatingChanged)
public:
    PrinterOutputModel();
    PrinterOutputModel(PrinterOutputController* output_controller, int number_of_extruders);

public:
    QObject* activePrintJob() { 
        return _active_print_job; 
    }
    QString name() const;
    void updateName(std::string name) {
        if (_name != name)
        {
            _name = name;
            emit nameChanged();
        }
    }

    QList<QObject*> extruders() const;

    void updateActivePrintJob(PrintJobOutputModel* print_job);

    float bedTemperature() {
        return _bed_temperature;
    }
    void updateBedTemperature(float temperature) {
        if (_bed_temperature != temperature)
        {
            _bed_temperature = temperature;
            emit bedTemperatureChanged();
        }
    }

    float targetBedTemperature() {
        return _target_bed_temperature;
    }
    void updateTargetBedTemperature(float temperature) {
        if (_target_bed_temperature != temperature)
        {
            _target_bed_temperature = temperature;
            emit targetBedTemperatureChanged();
        }
    }

    bool isPreheating() {
        return _is_preheating;
    }
    void updateIsPreheating(const bool& pre_heating) {
        if (_is_preheating != pre_heating)
        {
            _is_preheating = pre_heating;
            emit isPreheatingChanged();
        }
    }

    PrinterOutputController* getController() const {
        return _controller;
    }

signals:
    void nameChanged();
    void activePrintJobChanged();
    void bedTemperatureChanged();
    void targetBedTemperatureChanged();
    void isPreheatingChanged();

public slots:
    void homeHead();
    void homeBed();
    void sendRawCommand(QString command);
    void moveHead(float x= 0, float y = 0, float z = 0, float speed = 3000);
    void preheatBed(float temperature, float duration);

protected:
    PrintJobOutputModel* _active_print_job = nullptr;

private:
    float _bed_temperature = 0;
    float _target_bed_temperature = 0;
    std::string _name;
    std::string _key;
    std::string _unique_name;
    PrinterOutputController* _controller = nullptr;
    bool _is_preheating = false;
public:
    std::vector<ExtruderOutputModel*> _extruders;
};
#endif
