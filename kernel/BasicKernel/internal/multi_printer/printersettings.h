#ifndef CREATIVE_KERNEL_PRINTER_SETTINGS_202312131652_H
#define CREATIVE_KERNEL_PRINTER_SETTINGS_202312131652_H

#include <QObject>
// #include "basickernelexport.h"
#include "internal/render/platetexturecomponententity.h"
#include "qtuser3d/math/box3d.h"
#include "crslice2/crscene.h"

namespace creative_kernel
{
    class Printer;
    class PrinterSettings : public QObject
    {
        Q_OBJECT
// QVariantList
		Q_PROPERTY(QList<float> pauseLayers READ pauseLayers NOTIFY pauseLayersChanged)
		Q_PROPERTY(QList<float> customGCodeLayers READ customGCodeLayers NOTIFY customGCodeLayersChanged)
		// Q_PROPERTY(QList<int> specificExtruderLayers READ specificExtruderLayers NOTIFY specificExtruderLayersChanged)
		Q_PROPERTY(QList<float> specificExtruderHeights READ specificExtruderHeights NOTIFY specificExtruderHeightsChanged)
        Q_PROPERTY(QList<float> specificExtruderHeights READ specificExtruderHeights NOTIFY specificExtruderHeightsChanged)

    signals:
        void gcodeConfigChanged();
        void dirtyChanged();
        void sequenceChanged();
        void pauseLayersChanged();
        void customGCodeLayersChanged();
        // void specificExtruderLayersChanged();
        void specificExtruderHeightsChanged();


    public:
        PrinterSettings(Printer* printer);
        ~PrinterSettings();

        Q_INVOKABLE void reload();

        void reloadHeightLayerMap();

        bool findPlateItemByHeight(float z, crslice2::Plate_Item& item);
        bool removePlateItemByHeight(float z);

        void updatePasueLayers();
        Q_INVOKABLE void setLayerPause(int layer);
        Q_INVOKABLE bool isPauseLayer(int layer);

        Q_INVOKABLE void setLayerExtruder(int layer, int extruder);
        Q_INVOKABLE bool hasSpecificExtruder(int layer);
        bool hasSpecificExtruder();

        void updateCustomGCodeLayers();
        Q_INVOKABLE void setLayerGCode(int layer, const QString& gcode);
        Q_INVOKABLE bool useCustomGCode(int layer);
        Q_INVOKABLE QString customGCode(int layer);

        Q_INVOKABLE void removeLayerConfig(int layer);
        Q_INVOKABLE void clearLayerConfig();
        void clearLayerConfig(crslice2::Plate_Type type);

        QList<float> pauseLayers();
        QList<float> customGCodeLayers();
        // QList<int> specificExtruderLayers();
        QList<float> specificExtruderHeights();
        QSet<int> specificExtruderIndexes();

        Q_INVOKABLE QColor defaultColor();
        Q_INVOKABLE QColor layerColor(int layer);

        Q_INVOKABLE void correctHeight(float oldHeight, float newHeight);
        Q_INVOKABLE void correctLayer(float oldHeight, int layer);

        float layerHeight(int layer);
        Q_INVOKABLE int heightLayer(float height);

        crslice2::PlateInfo getPlateInfo();
        void setCustomGCode(const QMap<float, crslice2::Plate_Item>& items);

        void applySettings();

        QString value(const QString& key) const;
        void setValue(const QString& key, const QString& value);

        bool isDirty();
        void resetDirty();
    
        std::map<std::string, std::string> stdSettings();

        void ResetlayersConfig();

        void _updateSpecificExtruderHeights(bool needDirty = true);

    private slots:
        void updateSequence();
        // void updateSpecificExtruderLayers();
        void updateSpecificExtruderHeights();

    private:
        QList<int> parseSequenceString(const QString& str);
        QString generateSequenceString(const QList<int>& list);
             
    private:
		QMap<QString, QString> m_settings;
        bool m_dirty { false };
        Printer* m_printer;
        
        QMap<float, crslice2::Plate_Item> m_layersConfig;
        QList<float> m_pauseLayers;
        QList<float> m_customGCodeLayers;
        // QList<float> m_specificExtruderLayers;
        QList<float> m_specificExtruderHeights;
        QMap<float, int> m_heightLayerMap;

        QVariantList m_layersColors;

    };
}

#endif //CREATIVE_KERNEL_PRINTER_SETTINGS_202312131652_H