#ifndef PlotterSettings_H
#define PlotterSettings_H

#include <QObject>

class PlotterSettings : public QObject
{
    Q_OBJECT
    //Q_PROPERTY(int plotterPenDiameter READ plotterPenDiameter WRITE setPlotterPenDiameter NOTIFY plotterPenDiameterChanged)
    //Q_PROPERTY(int plotterSafeHeight READ plotterSafeHeight WRITE setPlotterSafeHeight NOTIFY plotterSafeHeightChanged)
    //Q_PROPERTY(int plotterDrawSpeed READ plotterDrawSpeed WRITE setPlotterDrawSpeed NOTIFY plotterDrawSpeedChanged)
    //Q_PROPERTY(int plotterTravelSpeed READ plotterTravelSpeed WRITE setPlotterTravelSpeed NOTIFY plotterTravelSpeedChanged)
    //Q_PROPERTY(int plotterPathType READ plotterPathType WRITE setPlotterPathType NOTIFY plotterPathTypeChanged)
    Q_PROPERTY(int plotterTotalNum READ plotterTotalNum WRITE setPlotterTotalNum)
    Q_PROPERTY(int plotterPowerRate READ plotterPowerRate WRITE setPlotterPowerRate)
    Q_PROPERTY(int plotterSpeedRate READ plotterSpeedRate WRITE setPlotterSpeedRate)
    Q_PROPERTY(int plotterWorkSpeed READ plotterWorkSpeed WRITE setPlotterWorkSpeed)
    Q_PROPERTY(int plotterJogSpeed READ plotterJogSpeed WRITE setPlotterJogSpeed)
    Q_PROPERTY(int plotterWorkDepth READ plotterWorkDepth WRITE setPlotterWorkDepth)
    Q_PROPERTY(int plotterTravelHeight READ plotterTravelHeight WRITE setPlotterTravelHeight)
public:
    explicit PlotterSettings(QObject *parent = nullptr);

    //int plotterPenDiameter();
    //int plotterSafeHeight();
    //int plotterDrawSpeed();
    //int plotterTravelSpeed();
    //int plotterPathType();

    //void setPlotterPenDiameter(int diameter);
    //void setPlotterSafeHeight(int height);
    //void setPlotterDrawSpeed(int drawSpeed);
    //void setPlotterTravelSpeed(int travelSpeed);
    //void setPlotterPathType(int pathType);



    int plotterTotalNum()
    {
        return m_total_num;
    }
    int plotterPowerRate()
    {
        return m_power_rate;
    }
    int plotterSpeedRate()
    {
        return m_speed_rate;
    }
    int plotterJogSpeed()
    {
        return m_jog_speed;
    }
    int plotterWorkSpeed()
    {
        return m_work_speed;
    }

    int plotterWorkDepth()
    {
        return m_work_depth;
    }

    int plotterTravelHeight()
    {
        return m_travle_height;
    }


    void setPlotterTotalNum(int totalNum)
    {
        m_total_num = totalNum;
    }
    void setPlotterPowerRate(int powerRate)
    {
        m_power_rate = powerRate;
    }
    void setPlotterSpeedRate(int speedRate)
    {
        m_speed_rate = speedRate;
    }
    void setPlotterWorkSpeed(int workSpeed)
    {
        m_work_speed = workSpeed;
    }
    void setPlotterJogSpeed(int jogSpeed)
    {
        m_jog_speed = jogSpeed;
    }
    void setPlotterWorkDepth(int workDepth)
    {
        m_work_depth = workDepth;
    }
    void setPlotterTravelHeight(int travelHeight)
    {
        m_travle_height = travelHeight;
    }

signals:
    void plotterPenDiameterChanged();
    void plotterSafeHeightChanged();
    void plotterDrawSpeedChanged();
    void plotterTravelSpeedChanged();
    void plotterPathTypeChanged();
public slots:

private:

    //int m_penDiameter;
    //int m_safeHeight;
    //int m_drawSpeed;
    //int m_travelSpeed;
    //int m_pathType;

    int m_total_num = 1;// ��̴���(0 - 1000)
    int m_power_rate = 80;// ��̹��ʣ� 0 - 100 (0 �� ������Χ֮�⣺ ����Ĭ��), ͨ���ı�G1 ���Ṧ��(S)ʵ��
    int m_speed_rate = 100; // ����ٶ�(���)�� 0 - 100 (0 �� ������Χ֮�⣺ ����Ĭ��), ͨ���ı�G1 �����ٶ�(F)ʵ��
    int m_jog_speed = 3000; // �����ٶȣ�G0��
    int m_work_speed = 120;// �����ٶȣ�G1��
    int m_gco_style = 2;//GRBLStyle = 1, MarlinStyle = 2
    int m_work_depth = 1;  //������
    int m_travle_height = 5;  //���߸߶�
};

#endif // PlotterSettings_H
