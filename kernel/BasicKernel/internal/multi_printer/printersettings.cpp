#include "printersettings.h"
#include "internal/parameter/parametermanager.h"
#include "kernel/kernel.h"
#include "interface/machineinterface.h"
#include "interface/reuseableinterface.h"

#include "internal/parameter/parametermanager.h"
#include "kernel/kernel.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printprofile.h"
#include "printer.h"
#include "external/slice/sliceattain.h"
#include "external/data/modeln.h"
#include "interface/projectinterface.h"
#include "internal/project_cx3d/autosavejob.h"
#include "interface/spaceinterface.h"

#define BED_TYPE "curr_bed_type"
#define SEQUENCE "print_sequence"
#define FIRST_SEQUENCE "first_layer_print_sequence"
#define CUSTOM "custom"

// "curr_bed_type": "Cool Plate",
// "print_sequence": "by layer", "by object"
namespace creative_kernel
{
    PrinterSettings::PrinterSettings(Printer* printer) : QObject(printer)
    {
        m_printer = printer;

        auto parameterManager = getKernel()->parameterManager();
        connect(parameterManager, &ParameterManager::currentColorsChanged, this, &PrinterSettings::updateSequence);
        connect(parameterManager, &ParameterManager::currentColorsChanged, this, &PrinterSettings::updateSpecificExtruderHeights);
        
        updateSequence();
    }

    PrinterSettings::~PrinterSettings()
    {

    }

    void PrinterSettings::reload()
    {
        reloadHeightLayerMap();
        updatePasueLayers();
        _updateSpecificExtruderHeights(false);
        updateCustomGCodeLayers();
    }

    void PrinterSettings::reloadHeightLayerMap()
    {
        m_heightLayerMap.clear();

        SliceAttain* attain = dynamic_cast<SliceAttain*>(m_printer->sliceCache());
        if (!attain)
            return;

        for (int i = 0, count = attain->layers(); i < count; ++i)
        {
            trimesh::vec3 pos = attain->traitPosition(i + 1, 0);
            float height = pos.z;
            m_heightLayerMap[height] = i;
        }
    }

    bool PrinterSettings::findPlateItemByHeight(float z, crslice2::Plate_Item& item)
    {
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it)
        {
            float dval = z - it.key();
            if (dval < 0.001 && dval > -0.001)
            {
                item = it.value();
                return true;
            }
        }
        return false;
    }

    bool PrinterSettings::removePlateItemByHeight(float z)
    {
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it)
        {
            float dval = z - it.key();
            if (dval < 0.001 && dval > -0.001)
            {
                m_layersConfig.remove(it.key());
                return true;
            }
        }
        return false;
    }

    void PrinterSettings::updatePasueLayers()
    {
        m_pauseLayers.clear();
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it)
        {
            crslice2::Plate_Item item = it.value();
            if (item.type == crslice2::Plate_Type::PausePrint)
            {
                m_pauseLayers << it.key();
            }
        }

        emit pauseLayersChanged();
    }

    void PrinterSettings::setLayerPause(int layer)
    {
        crslice2::Plate_Item item;
        // item.print_z = 0.23999999463558197;
        item.type = crslice2::Plate_Type::PausePrint;
        // item.extruder = 1;
        // item.color = "#000000";
        // item.extra = "";
        float z = layerHeight(layer);
        m_layersConfig[z] = item;

        m_printer->reserveSliceCache();
        updatePasueLayers();
        emit gcodeConfigChanged();
        emit dirtyChanged();
    }   

    bool PrinterSettings::isPauseLayer(int layer)
    {
        float z = layerHeight(layer);
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(z, item))
        {
            return item.type == crslice2::Plate_Type::PausePrint;
        }
        else 
        {
            return false;
        }
    }

    void PrinterSettings::_updateSpecificExtruderHeights(bool needDirty)
    {
        m_specificExtruderHeights.clear();

        QVariantList shaderDatas;
        std::vector<trimesh::vec> colors = currentColors();
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it)
        {
            crslice2::Plate_Item item = it.value();
            if (item.type == crslice2::Plate_Type::ToolChange)
            {
                float z = it.key();
                int extruder = item.extruder - 1;
                
                if (extruder >= colors.size())
                {
                    extruder = 0;
                    crslice2::Plate_Item newItem;
                    newItem.type = crslice2::Plate_Type::ToolChange;
                    newItem.extruder = 1;

                    m_layersConfig[z] = newItem;
                }

                m_specificExtruderHeights << z;
                trimesh::vec c = colors[extruder];
                shaderDatas << QVector4D(c[0], c[1], c[2], z);
            }
        }

        if (!shaderDatas.isEmpty())
        {
            QColor firstColor = defaultColor();
            shaderDatas.insert(0, QVector4D(firstColor.red() / 255.0, 
                                            firstColor.green() / 255.0, 
                                            firstColor.blue() / 255.0, 0));
        }

        m_printer->applyLayersColor(shaderDatas);

        if (m_layersColors != shaderDatas)
        {
           m_layersColors = shaderDatas;
           m_printer->reserveSliceCache();
           if (needDirty)
           {
                m_printer->gCodeSettingDirty();
                m_printer->dirty();
           }
        }

        emit specificExtruderHeightsChanged();
    }

    void PrinterSettings::updateSpecificExtruderHeights()
    {
        _updateSpecificExtruderHeights(true);
    }

    void PrinterSettings::setLayerExtruder(int layer, int extruder)
    {
        crslice2::Plate_Item item;
        item.type = crslice2::Plate_Type::ToolChange;
        item.extruder = extruder;
        float z = layerHeight(layer);
        m_layersConfig[z] = item;

        updateSpecificExtruderHeights();
        m_printer->reserveSliceCache();
        emit gcodeConfigChanged();
        emit dirtyChanged();
    }

    bool PrinterSettings::hasSpecificExtruder(int layer)
    {
        float z = layerHeight(layer);
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(z, item))
        {
            return item.type == crslice2::Plate_Type::ToolChange;
        }
        else 
        {
            return false;
        }
    }

    bool PrinterSettings::hasSpecificExtruder()
    {
        return m_specificExtruderHeights.length() > 0;
    }

    void PrinterSettings::updateCustomGCodeLayers()
    {
        m_customGCodeLayers.clear();
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it)
        {
            crslice2::Plate_Item item = it.value();
            if (item.type == crslice2::Plate_Type::Custom)
            {
                m_customGCodeLayers << it.key();
            }
        }
        emit customGCodeLayersChanged();
    }

    void PrinterSettings::setLayerGCode(int layer, const QString& gcode)
    {
        crslice2::Plate_Item item;
        item.type = crslice2::Plate_Type::Custom;
        item.extra = gcode.toLatin1().data();
        float z = layerHeight(layer);
        m_layersConfig[z] = item;

        m_printer->reserveSliceCache();
        updateCustomGCodeLayers();
        emit gcodeConfigChanged();
        emit dirtyChanged();
    }
    
    bool PrinterSettings::useCustomGCode(int layer)
    {
        float z = layerHeight(layer);
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(z, item))
        {
            return item.type == crslice2::Plate_Type::Custom;
        }
        else 
        {
            return false;
        }
    }
    
    QString PrinterSettings::customGCode(int layer)
    {
        float z = layerHeight(layer);
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(z, item))
        {
            return QString(item.extra.data());
        }
        else 
        {
            return "";
        }
    }

    void PrinterSettings::removeLayerConfig(int layer)
    {
        float z = layerHeight(layer);
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(z, item))
            return;

        removePlateItemByHeight(z);

        if (item.type == crslice2::Plate_Type::PausePrint) 
        {
            updatePasueLayers();
        } 
        else if (item.type == crslice2::Plate_Type::Custom)
        {
            updateCustomGCodeLayers();
        }
        else if (item.type == crslice2::Plate_Type::ToolChange)
        {
            updateSpecificExtruderHeights();
        }

        m_printer->reserveSliceCache();
        emit gcodeConfigChanged();
        emit dirtyChanged();
    }

    void PrinterSettings::clearLayerConfig()
    {
        m_layersConfig.clear();

        m_pauseLayers.clear();
        m_customGCodeLayers.clear();
        m_specificExtruderHeights.clear();

        emit pauseLayersChanged();
        emit customGCodeLayersChanged();
        emit specificExtruderHeightsChanged();

        m_printer->reserveSliceCache();
        emit gcodeConfigChanged();
        emit dirtyChanged();
    }

    void PrinterSettings::clearLayerConfig(crslice2::Plate_Type type)
    {
        bool changed = false;
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; )
        {
            float key = it.key();
            crslice2::Plate_Item item = it.value();
            ++it;

            if (item.type == type)
            {
                changed = true;
                m_layersConfig.remove(key);
            }
        }
        
        if (type == crslice2::Plate_Type::PausePrint) 
        {
            updatePasueLayers();
            emit pauseLayersChanged();
        } 
        else if (type == crslice2::Plate_Type::Custom)
        {
            updateCustomGCodeLayers();
            emit customGCodeLayersChanged();
        }
        else if (type == crslice2::Plate_Type::ToolChange)
        {
            updateSpecificExtruderHeights();
            emit specificExtruderHeightsChanged();
        }
        
        if (changed)
            emit gcodeConfigChanged();
    }

    QList<float> PrinterSettings::pauseLayers()
    {
        return m_pauseLayers;
    }

    QList<float> PrinterSettings::customGCodeLayers()
    {
        return m_customGCodeLayers;
    }

    // QList<int> PrinterSettings::specificExtruderLayers()
    // {
    //     return m_specificExtruderLayers;
    // }

    QList<float> PrinterSettings::specificExtruderHeights()
    {
        return m_specificExtruderHeights;
    }

    QSet<int> PrinterSettings::specificExtruderIndexes()
    {
        QSet<int> idxSet;
        std::vector<trimesh::vec> colors = currentColors();
        trimesh::dbox3 box = sceneBoundingBox();
        float maxZ = box.max.z;
        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it)
        {
            crslice2::Plate_Item item = it.value();
            if (item.type == crslice2::Plate_Type::ToolChange)
            {
                float z = it.key();
                if (maxZ < z)
                    continue;

                int extruder = item.extruder - 1;

                if (extruder >= colors.size())
                {
                    extruder = 0;
                }
                idxSet.insert( extruder );
            }
        }
        return idxSet;
    }

    QColor PrinterSettings::defaultColor()
    {
        auto models = m_printer->modelsInsideVisible();
        if (models.size() <= 0)
        {
            return QColor(0, 0, 0);
        }

        std::vector<trimesh::vec> colors = currentColors();
        if (colors.size() <= 0)
        {
            return QColor(0, 0, 0);
        }

        int defaultIndex = models[0]->defaultColorIndex();
        if (defaultIndex < 0)
        {
            return QColor(0, 0, 0);
        }

        if (defaultIndex >= colors.size())
        {
            defaultIndex = 0;
        }

        trimesh::vec c = colors[defaultIndex];
        return QColor(c[0] * 255, c[1] * 255, c[2] * 255);
    }

    QColor PrinterSettings::layerColor(int layer)
    {
        float z = layerHeight(layer);
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(z, item))
        {
            return QColor(0, 0, 0);
        }

        std::vector<trimesh::vec> colors = currentColors();

        int index = m_layersConfig[z].extruder - 1;
        if (index < 0 || index >= colors.size())
        {
            index = 0;
        }
        trimesh::vec c = colors[index];
        return QColor(c[0] * 255, c[1] * 255, c[2] * 255);
    }

    void PrinterSettings::correctHeight(float oldHeight, float newHeight)
    {
        crslice2::Plate_Item item;
        if (!findPlateItemByHeight(oldHeight, item))
            return;

        removePlateItemByHeight(oldHeight);
        m_layersConfig[newHeight] = item;

        if (item.type == crslice2::Plate_Type::PausePrint) 
        {
            updatePasueLayers();
        } 
        else if (item.type == crslice2::Plate_Type::Custom)
        {
            updateCustomGCodeLayers();
        }
        else if (item.type == crslice2::Plate_Type::ToolChange)
        {
            updateSpecificExtruderHeights();
        }
    }

    void PrinterSettings::correctLayer(float oldHeight, int newlLayer)
    {
        float newHeight = layerHeight(newlLayer);
        if (newHeight == -1)
            return;

        correctHeight(oldHeight, newHeight);
    }

    float PrinterSettings::layerHeight(int layer)
    {
        SliceAttain* attain = dynamic_cast<SliceAttain*>(m_printer->sliceCache());
        if (!attain)
            return -1;

        if (attain->layers() < layer)
            return -1;

         trimesh::vec3 pos = attain->traitPosition(layer + 1, 0);
        //trimesh::vec3 pos = attain->traitPosition(layer, 0);
        return pos.z; 
    }
    
    int PrinterSettings::heightLayer(float height)
    {
        if (m_heightLayerMap.isEmpty())
            return -1;

        auto it1 = m_heightLayerMap.begin(), end = m_heightLayerMap.end();
        if (it1 != end)
        {
            int h1 = it1.key();
            float dval1 = qAbs(height - h1);
            if (dval1 < 0.001)
                return it1.value();
        }
        else
        {
            return -1;
        }
       
        auto it2 = it1;
        it2++;
        for (; it2 != end; ++it1, ++it2)
        {
            float h1 = it1.key(), h2 = it2.key() + 0.001;
            if (h1 < height && height <= h2)
            {
                return it2.value();
            }

       
        }
        return -1;
    }

    void PrinterSettings::setCustomGCode(const QMap<float, crslice2::Plate_Item>& items)
    {
        m_layersConfig.clear();
        m_layersConfig = items;

        updateCustomGCodeLayers();
        updatePasueLayers();
        updateSpecificExtruderHeights();

        m_printer->reserveSliceCache();
        emit gcodeConfigChanged();
    }

    crslice2::PlateInfo PrinterSettings::getPlateInfo()
    {
        crslice2::PlateInfo info;
        info.mode = crslice2::Plate_Mode::MultiAsSingle;
        trimesh::dbox3 box = sceneBoundingBox();
        float maxZ = box.max.z;

        auto it = m_layersConfig.begin(), end = m_layersConfig.end();
        for (; it != end; ++it) 
        {
            float z = it.key();
            crslice2::Plate_Item item = it.value();

            if (maxZ < z)
                continue;

            item.print_z = z;
            if (item.print_z < 0)
                continue;

            

            if (item.type == crslice2::Plate_Type::PausePrint)
            {

            }
            else if (item.type == crslice2::Plate_Type::Custom)
            {

            }
            else if (item.type == crslice2::Plate_Type::ToolChange)
            {
                std::vector<trimesh::vec> colors = currentColors();
                if (colors.size() > item.extruder)
                {
                    trimesh::vec usedColor = colors[item.extruder];
                    QColor qcolor(usedColor[0] * 255, usedColor[1] * 255, usedColor[2] * 255);
                    item.color = qcolor.name().toLatin1().constData();
                }
                // item.extruder += 1;

            }
            info.gcodes.push_back(item);
        }
        return info;
    }
    void PrinterSettings::applySettings()
    {
        ParameterManager* parameterManager = getKernel()->parameterManager();
        parameterManager->resetPlateSettings();
        {
            QString value = m_settings[BED_TYPE];
            if (!value.isEmpty())
                parameterManager->setCurrBedType(value);
        }

        {
            QString value = m_settings[SEQUENCE];
            if (!value.isEmpty())
                parameterManager->setPrintSequence(value);
        }

        if (m_settings[CUSTOM] == "true")
        {

            QString value = m_settings[FIRST_SEQUENCE];
            if (!value.isEmpty())
                parameterManager->setFirstLayerPrintSequence(value);
            else
                parameterManager->setFirstLayerPrintSequence("0");
        }
        else 
        {
            parameterManager->setFirstLayerPrintSequence("0"); 
        }

        parameterManager->setCurrentPlateIndex(m_printer->index());

    }

    QString PrinterSettings::value(const QString& key) const
    {
        return m_settings[key];
    }

    void PrinterSettings::setValue(const QString& key, const QString& value)
    {
        if (m_settings[key] != value)
        {
            m_settings[key] = value;
            if (m_dirty == false)
            {
                m_dirty = true;
                dirtyChanged();
            }
        }
        triggerAutoSave(QList<ModelGroup*>(),AutoSaveJob::PLATE);
    }

    bool PrinterSettings::isDirty()
    {
        return m_dirty;
    }

    void PrinterSettings::resetDirty()
    {
        m_dirty = false;
    }

    void PrinterSettings::updateSequence()
    {
        auto colors = currentColors();
        if (m_settings[FIRST_SEQUENCE].isNull())
        {
            QString sequence;
            for (int i = 1, count = colors.size(); i <= count; ++i)
                sequence += QString("%1,").arg(i);

            if (sequence.endsWith(","))
                sequence.remove(sequence.length() - 1, 1);

            m_settings[FIRST_SEQUENCE] = sequence;
        }
        else
        {
            QList<int> sequenceList = parseSequenceString(m_settings[FIRST_SEQUENCE]);
            QList<int> colorsList;
            for (int i = 1, count = colors.size(); i <= count; ++i)
                colorsList << i;
            for (int number : sequenceList)
            {
                if (!colorsList.contains(number))
                    sequenceList.removeOne(number);
            }
            while (sequenceList.count() < colorsList.count())
                sequenceList << sequenceList.count() + 1;

            m_settings[FIRST_SEQUENCE] = generateSequenceString(sequenceList);
        }
        emit sequenceChanged();
    }

    QList<int> PrinterSettings::parseSequenceString(const QString& str)
    {
        QList<int> result;
        QStringList strList = str.split(",");
        for (QString str : strList)
            result << str.toInt();
        return result;
    }

    QString PrinterSettings::generateSequenceString(const QList<int>& list)
    {
        QString sequence;
        for (int number : list)
            sequence += QString("%1,").arg(number);

        if (sequence.endsWith(","))
            sequence.remove(sequence.length() - 1, 1);

        return sequence;
    }

    std::map<std::string, std::string> PrinterSettings::stdSettings()
    {
        std::map<std::string, std::string> stdMap;
        for (QMap<QString, QString>::iterator iter = m_settings.begin(); iter != m_settings.end(); ++iter)
        {
            stdMap.insert({ iter.key().toLocal8Bit().data(), iter.value().toLocal8Bit().data() });
        }
        return stdMap;
    }

    void PrinterSettings::ResetlayersConfig()
    {
        m_layersConfig.clear();
    }

}
