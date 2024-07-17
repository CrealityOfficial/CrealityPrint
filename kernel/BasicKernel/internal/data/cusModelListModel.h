#ifndef CUSMODELLISTMODEL_H
#define CUSMODELLISTMODEL_H
#include "qtuser3d/module/pickableselecttracer.h"
#include <QtCore/QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QColor>

namespace creative_kernel
{
    class ModelN;
    class ModelInfo : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool isChosed READ isChosed NOTIFY isChosedChanged)
        Q_PROPERTY(bool isVisible READ isVisible NOTIFY isVisibleChanged)
        Q_PROPERTY(QString fileName READ fileName NOTIFY fileNameChanged)
        Q_PROPERTY(bool triangleNum READ triangleNum WRITE setTriangleNum NOTIFY triangleNumChanged)
        Q_PROPERTY(bool zSeamVisible READ zSeamVisible NOTIFY zSeamVisibleChanged)
        Q_PROPERTY(bool colors2Facets READ colors2Facets NOTIFY colors2FacetsChanged)
        Q_PROPERTY(bool objectSettings READ objectSettings NOTIFY objectSettingsChanged)
        Q_PROPERTY(int supportVisible READ supportVisible NOTIFY supportVisibleChanged)
        Q_PROPERTY(int colorIndex READ colorIndex NOTIFY colorIndexChanged)
        Q_PROPERTY(QColor modelColor READ modelColor NOTIFY modelColorChanged)
        
        Q_PROPERTY(bool modelAdptiveLayer READ modelAdptiveLayer NOTIFY modelAdptiveLayerChanged)
    public:
        ModelInfo(ModelN* model = nullptr, QObject* parent = nullptr);
        virtual ~ModelInfo();

        ModelN* model();

        QString fileName() const;
        void setFileName(const QString& fileName);

        QColor getColor();
        void setColor(const QString& color);

        int plateIndex();
        void setPlateIndex(int index);

        int getColorIndex();
        void setColorIndex(int colorIndex);

        int triangleNum();
        void setTriangleNum(int num);

        bool isChosed() const;
        void setIsChosed_cpp(bool newIsChosed);

        bool isVisible() const;
        void setIsVisible_cpp(bool newIsVisible);

        bool itemVisible();
        void setItemVisible(bool visible);

        bool zSeamVisible();
        void setZSeamVisible(bool visible);

        bool colors2Facets();
        void setColors2Facets(bool visible);

        bool modelAdptiveLayer();
        void setModelAdaptiveLayer(bool visible);

        bool objectSettings();
        void setObjectSettings(bool visible);

        bool supportVisible();
        int colorIndex();

        QColor modelColor();
    public slots:
        void slotSupportTreeAdaptiveLayer();
    signals:
        void isChosedChanged();
        void isVisibleChanged();
        void fileNameChanged();
        void triangleNumChanged();
        void zSeamVisibleChanged();
        void colors2FacetsChanged();
        void objectSettingsChanged();
        void colorIndexChanged();
        void modelColorChanged();
        void supportVisibleChanged();
        void modelAdptiveLayerChanged();

    private:
        int m_PlateIndex;
        QString m_FileName;
        ModelN* m_model = nullptr;
        QColor m_Color;
        bool m_isChosed;
        bool m_isVisible;
        bool m_ItemVisible;
        int m_ColorIndex = 0;
    };
    class CusModelListModelProxy : public QSortFilterProxyModel
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel* sourceModelData READ sourceModel)
    public:
        CusModelListModelProxy(QObject* parent = nullptr);
        ~CusModelListModelProxy();

        Q_INVOKABLE void filterModelByPlate(int plateIndex);
        QAbstractItemModel* sourceModelData();

        Q_INVOKABLE void checkOne(int checkIndex);
        Q_INVOKABLE void checkAppendOne(int checkIndex);
        Q_INVOKABLE void checkRange(int checkBeginIndex, int checkEndIndex);
        Q_INVOKABLE void setItemVisible(int modelIndex, bool visible);
        Q_INVOKABLE void setColor(int modelIndex, const QString color, int colorIndex);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    private:
        int m_PlateIndex;
    };
    class Printer;
    class CusModelListModel : public QAbstractListModel
        , public qtuser_3d::SelectorTracer
    {
        Q_OBJECT
        Q_PROPERTY(bool isCheckedAll READ isCheckedAll NOTIFY isCheckedAllChanged)
    public:
        enum FileInfoRoles {
            File_Name = Qt::UserRole + 1,
            File_Visible,
            File_Checked,
            File_Size,
            File_Data,
            File_Color,
            File_ColorIndex,
            File_PlateIndex,
            File_ItemVisible,
            File_ZSeamVisible, //Z·ì
            File_Colors2Facets //Í¿É«
        };

        CusModelListModel(QObject* parent = nullptr);
        virtual ~CusModelListModel();

        QList<ModelInfo*> modelList();

        void addModel(ModelN* model);
        void addModel(ModelInfo* model);
        void removeModel(ModelN* model);

        int currentCheckedPlate();
        Q_INVOKABLE void addModel();
        Q_INVOKABLE void deleteSelections();
        Q_INVOKABLE void checkAll(bool on);
        Q_INVOKABLE void checkOne(int index);
        Q_INVOKABLE void checkAppendOne(int index);
        Q_INVOKABLE void checkRange(int begin, int index);
        Q_INVOKABLE void setItemVisible(int index, bool visible);
        Q_INVOKABLE bool hasSupport();
        Q_INVOKABLE void setColor(int index, const QString color, int colorIndex);

        bool isCheckedAll();
    signals:
        void plateCountChanged();
        void isCheckedAllChanged();
        void selectionChanged();

    public slots:
        void plateModelListChanged(int plateIndex, const QList<ModelN*>& modelList);
        void onChangeColors();

    protected:
        int rowCount(const QModelIndex& parent = QModelIndex()) const;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
        virtual QHash<int, QByteArray> roleNames() const;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

        void onSelectionsChanged() override;
        void selectChanged(qtuser_3d::Pickable* pickable) override;
        ModelInfo* _index(ModelN* model);

    protected:
        QList<ModelInfo*> m_modelsInfoList;
        bool m_isCheckedAll = false;
    };

    //Q_DECLARE_METATYPE(creative_kernel::CusModelListModelProxy)
}
#endif // CUSMODELLISTMODEL_H
