#include "lasersettings.h"

LaserSettings::LaserSettings(QObject *parent) : QObject(parent)
{
    m_openGcode = "M106 S255";
    m_closeGcode = "M107";
    m_runSpeed = 50;
    m_emptyRunSpeed = 100;
    m_runCount = 1;


 /*   m_power_max = 1 * 1000;
    m_work_speed = 1000;
    m_jog_speed = 2000;
    m_density = 10;*/
    //m_offset_x = 450;
    //m_offset_y = 450;
}
QString LaserSettings::laserOpenGode(){
    return m_openGcode;
}
QString LaserSettings::laserCloseGode(){
    return m_closeGcode;
}
int LaserSettings::laserRunSpeed(){
    return m_runSpeed;
}
int LaserSettings::laserEmptyRunSpeed(){
    return m_emptyRunSpeed;
}
int LaserSettings::laserRunCount(){
    return m_runCount;
}

void LaserSettings::setLaserOpenGode(QString gcode){
    m_openGcode=gcode;
}
void LaserSettings::setLaserCloseGode(QString gcode){
    m_closeGcode=gcode;
}
void LaserSettings::setLaserRunSpeed(int speed){
    m_runSpeed=speed;
}
void LaserSettings::setLaserEmptyRunSpeed(int speed){
    m_emptyRunSpeed=speed;
}
void LaserSettings::setLaserRunCount(int count){
    m_runCount=count;
}
