#ifndef OBJECTSDATAMANAGER_H
#define OBJECTSDATAMANAGER_H
#include <QObject>
#include "internal/multi_printer/printer.h"
#include "data/interface.h"

namespace creative_kernel
{
    class Printer;
    class ObjectsTreeModel;
    class ModelN;
    class ObjectTreeItem;
    class ModelSpace;
    class ModelNSelector;
    class PrinterMangager;
    class ObjectsDataManager : public QObject,  
        public SpaceTracer,
        public PrinterMangagerTracer,
        public ModelNSelectorTracer
    {
        Q_OBJECT
    public: 
        ObjectsDataManager(QObject* parent = nullptr);
        ObjectsDataManager(const ObjectsDataManager& other);
        ~ObjectsDataManager();

        void setModelSpace(ModelSpace* model_space);
        void setModelSelector(ModelNSelector* model_selector);
        void setPrinterManager(PrinterMangager* printer_manager);

        Q_INVOKABLE QVariant objectsTreeModel();

    public:  // for cpp
        QList<ModelGroup*> generlatePrinterModelGroupList(Printer* printer) const;

    protected:
        //print
        void nameChanged(Printer* p, QString newName) override;
        void didAddPrinter(Printer* p) override;
        void didDeletePrinter(Printer* p) override;
        void didSelectPrinter(Printer* p) override;
        void printerNameChanged(Printer* p, QString newName) override;
        void printerModelGroupsInsideChange(Printer* pt, const QList<ModelGroup*>& list) override;
        void printerIndexChanged(Printer* p, int newIndex) override;
        void printersCountChanged(int count) override;

        //modelGroup/model
        void onModelGroupAdded(ModelGroup* _model_group) override;
        void onModelGroupRemoved(ModelGroup* _model_group) override;
        void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds) override;
        void onModelGroupNameChanged(ModelGroup* _model_group) override;
        void onModelNameChanged(ModelN* model) override;
        void onModelTypeChanged(ModelN* model) override;
        void modelGroupsOutOfPrinterChange(const QList<ModelGroup*>& list) override;
        void appendChild(const QVector<QVariant>& datas);

        //selection
        void onSelectionsChanged() override;
        void onSelectionsObjectsChanged(const QList<Printer*>& printersOn, const QList<Printer*>& printersOff, const QList<ModelGroup*>& modelGroupsOn, const QList<ModelGroup*>& modelGroupsOff,
            const QList<ModelN*>& modelsOn, const QList<ModelN*>& modelsOff) override;
    private:
        bool isMoved(const QModelIndex& curParent, const QModelIndex& preParent);
        void insertPlate(Printer* p, int row = 0);
        void addPlate(Printer* p);
        void deletePlate(Printer* p);
        void moveModelGroups(const QModelIndex& curPrinterIndex, ModelGroup* list);

        void addGroup(ModelGroup* mg, const QModelIndex& parent);
        void deleteGroup(ModelGroup* mg, const QModelIndex& parent);

        void addModel(ModelN* mn, const QModelIndex& parent);
        void deleteModel(ModelN* mn, const QModelIndex& parent);

    signals:
        void sigExpandIndex(QModelIndex expandIndex);

    private:
        ModelSpace* m_ModelSpace = nullptr;
        ModelNSelector* m_ModelSelector = nullptr;
        PrinterMangager* m_PrinterManager = nullptr;
        ObjectsTreeModel* m_ObjectsTreeModel = nullptr;
    };
}
#endif // PLATESDATAMANAGER_H
