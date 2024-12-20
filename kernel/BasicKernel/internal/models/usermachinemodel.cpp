#include "usermachinemodel.h"
#include "internal/parameter/parameterpath.h"
#include "us/usettingwrapper.h"

namespace creative_kernel
{
    UserMachineModel::UserMachineModel(QObject* parent)
    {
        auto settings = createDefaultGlobal();
        name_ = settings->value("machine_name", "");
        shapeOptions_ = settings->enums("machine_shape");
        shape_ = settings->value("machine_shape", "");
        width_ = get_machine_width(settings);
        depth_ = get_machine_depth(settings);
        height_ = get_machine_height(settings);

        extruderTypeOptions_ = settings->enums("machine_extruder_type");
        extruderType_ = settings->value("machine_extruder_type", "");
        nozzleSize_ = settings->value("machine_nozzle_size", "").toFloat();
        materialDiameter_ = settings->value("material_diameter", "").toFloat();
        nozzleOffsetX_ = settings->value("machine_nozzle_offset_x", "").toFloat();
        nozzleOffsetY_ = settings->value("machine_nozzle_offset_y", "").toFloat();
        gcodeFlavorOptions_ = settings->enums("machine_gcode_flavor");
        gcodeFlavor_ = settings->value("machine_gcode_flavor", "klipper");
        startGcode_ = settings->value("machine_start_gcode", "");
        endGcode_ = settings->value("machine_end_gcode", "");
        delete settings;
    }

    UserMachineModel::~UserMachineModel()
    {
    }
    QString UserMachineModel::name() const
    {
        return name_;
    }
    void UserMachineModel::setName(const QString& name)
    {
        if (name_ != name)
        {
            name_ = name;
            emit nameChanged();
        }
    }
    QVariantList UserMachineModel::shapeOptions() const
    {
        return shapeOptions_;
    }
    QString UserMachineModel::shape() const
    {
        return shape_;
    }
    void UserMachineModel::setShape(const QString& shape)
    {
        if (shape_ != shape)
        {
            shape_ = shape;
            emit shapeChanged();
        }
    }
    float UserMachineModel::width() const
    {
        return width_;
    }
    void UserMachineModel::setWidth(const float& width)
    {
        if (!qFuzzyCompare(width_, width))
        {
            width_ = width;
            emit widthChanged();
        }
    }
    float UserMachineModel::depth() const
    {
        return depth_;
    }
    void UserMachineModel::setDepth(const float& depth)
    {
        if (!qFuzzyCompare(depth_, depth))
        {
            depth_ = depth;
            emit depthChanged();
        }
    }
    float UserMachineModel::height() const
    {
        return height_;
    }
    void UserMachineModel::setHeight(const float& height)
    {
        if (!qFuzzyCompare(height_, height))
        {
            height_ = height;
            emit heightChanged();
        }
    }
    QVariantList UserMachineModel::extruderTypeOptions() const
    {
        return extruderTypeOptions_;
    }
    QString UserMachineModel::extruderType() const
    {
        return extruderType_;
    }
    void UserMachineModel::setExtruderType(const QString& extruderType)
    {
        if (extruderType_ != extruderType)
        {
            extruderType_ = extruderType;
            emit extruderTypeChanged();
        }
    }
    float UserMachineModel::nozzleSize() const
    {
        return nozzleSize_;
    }
    void UserMachineModel::setNozzleSize(const float& nozzleSize)
    {
        if (!qFuzzyCompare(nozzleSize_, nozzleSize))
        {
            nozzleSize_ = nozzleSize;
            emit nozzleSizeChanged();
        }
    }
    float UserMachineModel::materialDiameter() const
    {
        return materialDiameter_;
    }
    void UserMachineModel::setMaterialDiameter(const float& materialDiameter)
    {
        if (!qFuzzyCompare(materialDiameter_, materialDiameter))
        {
            materialDiameter_ = materialDiameter;
            emit materialDiameterChanged();
        }
    }
    float UserMachineModel::nozzleOffsetX() const
    {
        return nozzleOffsetX_;
    }
    void UserMachineModel::setNozzleOffsetX(const float& nozzleOffsetX)
    {
        if (!qFuzzyCompare(nozzleOffsetX_, nozzleOffsetX))
        {
            nozzleOffsetX_ = nozzleOffsetX;
            emit nozzleOffsetXChanged();
        }
    }
    float UserMachineModel::nozzleOffsetY() const
    {
        return nozzleOffsetY_;
    }
    void UserMachineModel::setNozzleOffsetY(const float& nozzleOffsetY)
    {
        if (!qFuzzyCompare(nozzleOffsetY_, nozzleOffsetY))
        {
            nozzleOffsetY_ = nozzleOffsetY;
            emit nozzleOffsetYChanged();
        }
    }
    QVariantList UserMachineModel::gcodeFlavorOptions() const
    {
        return gcodeFlavorOptions_;
    }
    QString UserMachineModel::gcodeFlavor() const
    {
        return gcodeFlavor_;
    }
    void UserMachineModel::setGcodeFlavor(const QString& gcodeFlavor)
    {
        if (gcodeFlavor_ != gcodeFlavor)
        {
            gcodeFlavor_ = gcodeFlavor;
            emit gcodeFlavorChanged();
        }
    }
    QString UserMachineModel::startGcode() const
    {
        return startGcode_;
    }
    void UserMachineModel::setStartGcode(const QString& startGcode)
    {
        if (startGcode_ != startGcode)
        {
            startGcode_ = startGcode;
            emit startGcodeChanged();
        }
    }
    QString UserMachineModel::endGcode() const
    {
        return endGcode_;
    }
    void UserMachineModel::setEndGcode(const QString& endGcode)
    {
        if (endGcode_ != endGcode)
        {
            endGcode_ = endGcode;
            emit endGcodeChanged();
        }
    }
}