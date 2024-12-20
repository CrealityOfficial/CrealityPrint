#ifndef LAYOUT_MANAGER_202404300925_H
#define LAYOUT_MANAGER_202404300925_H
#include "operation/pickop.h"
#include "data/interface.h"
#include "data/undochange.h"

namespace creative_kernel
{
    typedef std::vector<trimesh::vec3> CONTOUR_PATH;

    enum LayoutWay
    {
        QUICK_LAYOUT,
        FINE_LAYOUT
    };

    struct  LayoutNestResultEx
    {
        trimesh::vec3 rt; // x, y translation  z rotation angle
        int binIndex = 0;
    };

    struct LayoutNestFixedItemInfo
    {
        int fixBinId;
        trimesh::box3 fixBinGlobalBox;
        CONTOUR_PATH  fixOutline;
    };

    struct LayoutParameterInfo
    {
        QList<int> needLayoutGroupIds;
        int priorBinIndex;
        bool needPrintersLayout;
        int layoutType = 0;  //0: layout all models ; 1: layout selectmodels

        float modelSpacing;
        float platfromSpacing;
        int   angle;
        bool alignMove;
        bool outlineConcave;
    };

    struct LayoutPositionResult
    {
        QList<QVector3D> positions;
    };
    
	
    class BASIC_KERNEL_API LayoutManager : public QObject
    {
        Q_OBJECT
    public:
        LayoutManager(QObject* parent = nullptr);
        virtual ~LayoutManager();

        void layoutModelGroups(QList<int> groupIds, int priorBinIndex);
        void extendFillModelGroup(int extendGroupId, int curBinIndex);

        void getPositionBySimpleLayout(const QList<CONTOUR_PATH>& inOutlinePaths,QList<QVector3D>& outPos);

    protected:
        void executeJobP(QList<int> groupIds, int priorBinIndex);

        void executeExtendFillJob(int extendModelGroupId, int curBinIndex);

        void releaseAlwaysRender();
        void doExtraLayout(int estimatePrinterSize);

        void onLayoutSuccessed(const LayoutChangeInfo& changeInfo);

        // from a printer,  get fixed group outline info and the wipe tower outline info  
        void getPrinterFixItemInfo(int printerIdx, std::vector<LayoutNestFixedItemInfo>& outFixInfos);
		
    Q_SIGNALS:

	private:
        LayoutParameterInfo  m_extraLayoutParameterInfo;
    };

}
Q_DECLARE_METATYPE(creative_kernel::LayoutPositionResult)
#endif // LAYOUT_MANAGER_202404300925_H
