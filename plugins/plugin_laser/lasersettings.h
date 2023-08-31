#ifndef LASERSETTINGS_H
#define LASERSETTINGS_H

#include <QObject>
/*
class LaserShapeSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int laserShapeX READ laserShapeX WRITE setLaserShapeX)
    Q_PROPERTY(int laserShapeY READ laserShapeY WRITE setLaserShapeY)
    Q_PROPERTY(int laserShapeSizeX READ laserShapeSizeX WRITE setLaserShapeSizeX)
    Q_PROPERTY(int laserShapeSizeY READ laserShapeSizeY WRITE setLaserShapeSizeY)
    Q_PROPERTY(int laserShapeRotate READ laserShapeRotate WRITE setLaserShapeRotate)
public:
    explicit LaserShapeSettings(QObject* parent = nullptr) 
    {
        m_shapeX = 0;
        m_shapeY = 0;
        m_shapeSizeX = 0;
        m_shapeSizeY = 0;
        m_shapeRotate = 0;
    }

    int laserShapeX()
    {
        return m_shapeX;
    }
    int laserShapeY()
    {
        return m_shapeY;
    }
    int laserShapeSizeX()
    {
        return m_shapeSizeX;
    }
    int laserShapeSizeY()
    {
        return m_shapeSizeY;
    }
    int laserShapeRotate()
    {
        return m_shapeRotate;
    }

    void setLaserShapeX(int shapeX)
    {
        m_shapeX = shapeX;
    }
    void setLaserShapeY(int shapeY)
    {
        m_shapeY = shapeY;
    }
    void setLaserShapeSizeX(int shapeSizeX)
    {
        m_shapeSizeX = shapeSizeX;
    }
    void setLaserShapeSizeY(int shapeSizeY)
    {
        m_shapeSizeY = shapeSizeY;
    }
    void setLaserShapeRotate(int shapeRotate)
    {
        m_shapeRotate = shapeRotate;
    }

signals:
    
public slots:

private:
    int m_shapeX;
    int m_shapeY;
    int m_shapeSizeX;
    int m_shapeSizeY;
    int m_shapeRotate;
};*/

class LaserSettings : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString laserOpenGode READ laserOpenGode WRITE setLaserOpenGode NOTIFY laserOpenGodeChanged)
    Q_PROPERTY(QString laserCloseGode READ laserCloseGode WRITE setLaserCloseGode NOTIFY laserCloseGodeChanged)
    Q_PROPERTY(int laserRunSpeed READ laserRunSpeed WRITE setLaserRunSpeed NOTIFY laserRunSpeedChanged)
    Q_PROPERTY(int laserEmptyRunSpeed READ laserEmptyRunSpeed WRITE setLaserEmptyRunSpeed NOTIFY laserEmptyRunSpeedChanged)
    Q_PROPERTY(int laserRunCount READ laserRunCount WRITE setLaserRunCount NOTIFY laserRunCountChanged)
    
#if 0
    Q_PROPERTY(unsigned int laserPowerMax READ laserPowerMax WRITE setLaserPowerMax)
    Q_PROPERTY(unsigned int laserWorkSpeed READ laserWorkSpeed WRITE setLaserWorkSpeed)
    Q_PROPERTY(unsigned int laserJogSpeed READ laserJogSpeed WRITE setLaserJogSpeed)
    Q_PROPERTY(float laserDensity READ laserDensity WRITE setLaserDensity)
#endif
    //Q_PROPERTY(int laserGcoStyle READ laserGcoStyle WRITE setLaserGcoStyle)
    Q_PROPERTY(int laserTotalNum READ laserTotalNum WRITE setLaserTotalNum)
    Q_PROPERTY(int laserPowerRate READ laserPowerRate WRITE setLaserPowerRate)
    Q_PROPERTY(int laserSpeedRate READ laserSpeedRate WRITE setLaserSpeedRate)
    Q_PROPERTY(int laserWorkSpeed READ laserWorkSpeed WRITE setLaserWorkSpeed)
    Q_PROPERTY(int laserJogSpeed READ laserJogSpeed WRITE setLaserJogSpeed)

public:
    explicit LaserSettings(QObject *parent = nullptr);
    QString laserOpenGode();
    QString laserCloseGode();
    int laserRunSpeed();
    int laserEmptyRunSpeed();
    int laserRunCount();

    /*int laserGcoStyle()
    {
        return m_gco_style;
    }*/

    int laserTotalNum()
    {
        return m_total_num;
    }
    int laserPowerRate()
    {
        return m_power_rate;
    }
    int laserSpeedRate()
    {
        return m_speed_rate;
    }
    int laserJogSpeed()
    {
        return m_jog_speed;
    }
    int laserWorkSpeed()
    {
        return m_work_speed;
    }

    /*void setLaserGcoStyle(int gcoStyle)
    {
        m_gco_style = gcoStyle;
    }*/
    void setLaserTotalNum(int totalNum)
    {
        m_total_num = totalNum;
    }
    void setLaserPowerRate(int powerRate)
    {
        m_power_rate = powerRate;     
    }
    void setLaserSpeedRate(int speedRate)
    {
        m_speed_rate = speedRate;
    }
    void setLaserWorkSpeed( int workSpeed)
    {
        m_work_speed = workSpeed;
    }
    void setLaserJogSpeed( int jogSpeed)
    {
        m_jog_speed = jogSpeed;
    }
#if 0
    unsigned int laserPowerMax()
    {
        return m_power_max;
    }
    unsigned int laserWorkSpeed()
    {
        return m_work_speed;
    }
    unsigned int laserJogSpeed()
    {
        return m_jog_speed;
    }
    float laserDensity()
    {
        return m_density;
    }
    /*float laserOffsetX()
    {

    }
    float laserOffsetY()
    {

    }*/
#endif
    void setLaserOpenGode(QString gcode);
    void setLaserCloseGode(QString gcode);
    void setLaserRunSpeed(int speed);
    void setLaserEmptyRunSpeed(int speed);
    void setLaserRunCount(int count);
#if 0
    void setLaserPowerMax(unsigned int powerMax)
    {
        m_power_max = powerMax;
    }
    void setLaserWorkSpeed(unsigned int workSpeed)
    {
        m_work_speed = workSpeed;
    }
    void setLaserJogSpeed(unsigned int jogSpeed)
    {
        m_jog_speed = jogSpeed;
    }
    void setLaserDensity(float density)
    {
        m_density = density;
    }
    /*void setLaserOffsetX(float offsetX)
    {
        m_offset_x = offsetX;
    }
    void setLaserOffsetY(float offsetY)
    {
        m_offset_y = offsetY;
    }*/
#endif
signals:
    void laserOpenGodeChanged();
    void laserCloseGodeChanged();
    void laserRunSpeedChanged();
    void laserEmptyRunSpeedChanged();
    void laserRunCountChanged();

public slots:

private:
    QString m_openGcode;
    QString m_closeGcode;
    int m_runSpeed;
    int m_emptyRunSpeed;
    int m_runCount;
#if 0
    unsigned int m_power_max; /*设置最大功率，按照百分比换算，例如设置50%功率，0.5*1000 */
    unsigned int m_work_speed; /*工作速度 1000mm/min */
    unsigned int m_jog_speed;  /*空驶速度 2000mm/min */
    float m_density; /*雕刻分辨率， line/mm */
    //float m_offset_x; /*图片中心位置 X*/
    //float m_offset_y; /*图片中心位置 Y*/
#endif
    int m_total_num = 1;// 雕刻次数(0 - 1000)
    int m_power_rate = 80;// 雕刻功率： 0 - 100 (0 或 超出范围之外： 机型默认), 通过改变G1 主轴功率(S)实现
    int m_speed_rate = 100; // 雕刻速度(深度)： 0 - 100 (0 或 超出范围之外： 机型默认), 通过改变G1 主轴速度(F)实现
    int m_jog_speed = 3000; // 空走速度（G0）
    int m_work_speed = 1500;// 工作速度（G1）
    int m_gco_style = 2;//GRBLStyle = 1, MarlinStyle = 2
};

#endif // LASERSETTINGS_H
