#ifndef _PRINTER_JOB_OUTPUT_MODEL_H
#define _PRINTER_JOB_OUTPUT_MODEL_H

#include <QtCore>
#include "GenericOutputController.h"

class PrintJobOutputModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString key READ key NOTIFY keyChanged)
    Q_PROPERTY(int timeTotal READ timeTotal NOTIFY timeTotalChanged)
    Q_PROPERTY(int timeElapsed READ timeElapsed NOTIFY timeElapsedChanged)
    Q_PROPERTY(int progress READ progress NOTIFY timeElapsedChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
    PrintJobOutputModel();
    PrintJobOutputModel(PrinterOutputController* output_controller, std::string name);

public:
    QString key() const { return _key.c_str(); }
    void updateKey(const std::string& key) {
        if (_key != key)
        {
            _key = key;
            emit keyChanged();
        }
    }

    int timeTotal() const {
        return _time_total;
    }
    void updateTimeTotal(const int& new_time_total){
        if (_time_total != new_time_total)
        {
            _time_total = new_time_total;
            emit timeTotalChanged();
        }
    }

    int timeElapsed() const {
        return _time_elapsed;
    }
    void updateTimeElapsed(const int& new_time_total) {
        if (_time_total != new_time_total)
        {
            _time_elapsed = new_time_total;
            emit timeElapsedChanged();
        }
    }

    int progress() const {
        auto result = _time_elapsed * 100 / std::max(_time_total, 1);
        return std::min(result, 100);
    }

    QString state() const {
        return _state.c_str(); 
    }
    void updateState(const std::string& state) {
        if (_state != state)
        {
            _state = state;
            emit stateChanged();
        }
    }

    QString name() const {
        return _name.c_str();
    }
    void updateName(const std::string& name) {
        if (_name != name)
        {
            _name = name;
            emit nameChanged();
        }
    }
public slots:
    void setState(QString state);

signals:
    void keyChanged();
    void timeTotalChanged();
    void timeElapsedChanged();
    void stateChanged();
    void nameChanged();

private:
    std::string _state;
    std::string _key;
    std::string _name;
    int _time_total = 0;
    int _time_elapsed = 0;
    PrinterOutputController* _output_controller = nullptr;
};
#endif