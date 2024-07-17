#ifndef CREATIVE_KERNEL_USERMACHINEMODEL_H
#define CREATIVE_KERNEL_USERMACHINEMODEL_H
#include <QtCore/QObject>

namespace creative_kernel
{
    class UserMachineModel : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QVariantList shapeOptions READ shapeOptions NOTIFY shapeOptionsChanged)
        Q_PROPERTY(QString shape READ shape WRITE setShape NOTIFY shapeChanged)
        Q_PROPERTY(float width READ width WRITE setWidth NOTIFY  widthChanged)
        Q_PROPERTY(float depth READ depth WRITE setDepth NOTIFY depthChanged)
        Q_PROPERTY(float height READ height WRITE setHeight NOTIFY heightChanged)
        Q_PROPERTY(QVariantList extruderTypeOptions READ extruderTypeOptions NOTIFY extruderTypeOptionsChanged)
        Q_PROPERTY(QString extruderType READ extruderType WRITE setExtruderType NOTIFY extruderTypeChanged)
        Q_PROPERTY(float nozzleSize READ nozzleSize WRITE setNozzleSize NOTIFY nozzleSizeChanged)
        Q_PROPERTY(float materialDiameter READ materialDiameter WRITE setMaterialDiameter NOTIFY materialDiameterChanged)
        Q_PROPERTY(float nozzleOffsetX READ nozzleOffsetX WRITE setNozzleOffsetX NOTIFY nozzleOffsetXChanged)
        Q_PROPERTY(float nozzleOffsetY READ nozzleOffsetY WRITE setNozzleOffsetY NOTIFY nozzleOffsetYChanged)
        Q_PROPERTY(QVariantList gcodeFlavorOptions READ gcodeFlavorOptions NOTIFY gcodeFlavorOptionsChanged)
        Q_PROPERTY(QString gcodeFlavor READ gcodeFlavor WRITE setGcodeFlavor NOTIFY gcodeFlavorChanged)
        Q_PROPERTY(QString startGcode READ startGcode WRITE setStartGcode NOTIFY startGcodeChanged)
        Q_PROPERTY(QString endGcode READ endGcode WRITE setEndGcode NOTIFY endGcodeChanged)
    public:
        explicit UserMachineModel(QObject* parent = nullptr);
        ~UserMachineModel();

        QString name() const;
        void setName(const QString& name);

        QVariantList shapeOptions() const;

        QString shape() const;
        void setShape(const QString& shape);

        float width() const;
        void setWidth(const float& width);

        float depth() const;
        void setDepth(const float& depth);

        float height() const;
        void setHeight(const float& height);

        QVariantList extruderTypeOptions() const;

        QString extruderType() const;
        void setExtruderType(const QString& extruderType);

        float nozzleSize() const;
        void setNozzleSize(const float& nozzleSize);

        float materialDiameter() const;
        void setMaterialDiameter(const float& materialDiameter);

        float nozzleOffsetX() const;
        void setNozzleOffsetX(const float& nozzleOffsetX);

        float nozzleOffsetY() const;
        void setNozzleOffsetY(const float& nozzleOffsetY);

        QVariantList gcodeFlavorOptions() const;

        QString gcodeFlavor() const;
        void setGcodeFlavor(const QString& gcodeFlavor);

        QString startGcode() const;
        void setStartGcode(const QString& startGcode);

        QString endGcode() const;
        void setEndGcode(const QString& endGcode);

    signals:
        void nameChanged();
        void shapeOptionsChanged();
        void shapeChanged();
        void widthChanged();
        void depthChanged();
        void heightChanged();
        void extruderTypeOptionsChanged();
        void extruderTypeChanged();
        void nozzleSizeChanged();
        void materialDiameterChanged();
        void nozzleOffsetXChanged();
        void nozzleOffsetYChanged();
        void gcodeFlavorOptionsChanged();
        void gcodeFlavorChanged();
        void startGcodeChanged();
        void endGcodeChanged();

    private:
        QString name_;
        QVariantList shapeOptions_;
        QString shape_;
        float width_;
        float depth_;
        float height_;
        QVariantList extruderTypeOptions_;
        QString extruderType_;
        float nozzleSize_;
        float materialDiameter_;
        float nozzleOffsetX_;
        float nozzleOffsetY_;
        QVariantList gcodeFlavorOptions_;
        QString gcodeFlavor_;
        QString startGcode_;
        QString endGcode_;
    };
}

#endif //