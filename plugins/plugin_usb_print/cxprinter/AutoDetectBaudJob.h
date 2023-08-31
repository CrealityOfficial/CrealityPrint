#ifndef _AUTO_DETECT_BAUD_JOB_H
#define _AUTO_DETECT_BAUD_JOB_H

#include <QObject>
#include <QThread>
#include <QString>

class AutoDetectBaudJob : public QThread
{
    Q_OBJECT
public:
    AutoDetectBaudJob();
    AutoDetectBaudJob(const QString& serial_port);
    ~AutoDetectBaudJob();
    void run();

signals:
    void sigDetectBaud(int);

private:
    QString _serial_port;
    std::vector<int> _all_baud_rates = { 115200, 250000, 500000, 230400, 76800, 57600, 38400, 19200, 9600 };
};


#endif
