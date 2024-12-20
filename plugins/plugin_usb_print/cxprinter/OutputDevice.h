#ifndef _OUTPUT_DEVICE_H
#define _OUTPUT_DEVICE_H

#include <QtCore>

class OutputDevice : public QObject
{
    Q_OBJECT
public:
    OutputDevice();
    OutputDevice(const std::string& id);

private:
    std::string _id;
    std::string _name = "Unknown Device";
    std::string _short_description = "Unknown Device";
    std::string _description = "Do something with an unknown device";
    std::string _icon_name = "generic_device";
    int _priority = 0;

public:
    std::string getId() const { return _id; }
    std::string getName() const { return _id; }
    void setName(const std::string& name) { _name = name; }
    std::string getShortDescription() const { return _short_description; }
    void setShortDescription(const std::string& short_description) { _short_description = short_description; }
    std::string getDescription() const { return _description; }
    void setDescription(const std::string& description) { _description = description; }
    std::string getIconName() const { return _icon_name; }
    void setIconName(const std::string& icon_name) { _icon_name = icon_name; }

    virtual void requestWrite(const std::string& file_name);
    virtual void sendCommand(const std::string& command);
    virtual void pausePrint();
    virtual void resumePrint();
    virtual void cancelPrint();
};
#endif