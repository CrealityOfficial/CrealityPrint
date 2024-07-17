#ifndef _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
#define _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
#include "qtusercore/plugin/toolcommand.h"
#include "operation/pickop.h"
#include "data/interface.h"
#include "data/undochange.h"

namespace creative_kernel
{
    enum LayoutWay
    {
        QUICK_LAYOUT,
        FINE_LAYOUT
    };

    struct extraLayoutInfo
    {
        QList<ModelN*> needLayoutModels;
        int priorBinIndex;
        bool needPrintersLayout;
        bool needAddToScene;
        float modelSpacing;
    };

    class LayoutToolCommand : public ToolCommand, public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(int type READ type  WRITE setType NOTIFY typeChanged)
        Q_PROPERTY(float modelSpacing READ modelSpacing WRITE setModelSpacing NOTIFY modelSpacingChanged)
        Q_PROPERTY(float platformSpacing READ platformSpacing WRITE setPlatformSpacing NOTIFY platformSpacingChanged)
        Q_PROPERTY(int angleIndex READ angleIndex WRITE setAngleIndex NOTIFY angleIndexChanged)
        Q_PROPERTY(bool allowRotate READ allowRotate WRITE setAllowRotate NOTIFY allowRotateChanged)
    public:
        LayoutToolCommand(QObject* parent = nullptr);
        virtual ~LayoutToolCommand();
        Q_INVOKABLE void execute();
		Q_INVOKABLE bool isSelect();
        Q_INVOKABLE bool checkSelect() override;
		//Q_INVOKABLE void layoutByType(int layoutType);

        void layoutModels(QList<ModelN*> models, int priorBinIndex, bool needPrintersLayout, bool needAddToScene);

        void extendFillModel(ModelN* extendModel, int curBinIndex);

        Q_INVOKABLE void autoLayout(int nlayoutWay = 0);

        Q_INVOKABLE QVariantList angleList() const {
            return m_angleList;
        }

        bool checkPrinterLock();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

        int type()const { return m_layoutType; }
        void setType(const int type) { m_layoutType = type; }
        float modelSpacing() const { return m_modelSpacing; }
        void setModelSpacing(const float val) {
            m_modelSpacing = val;
            emit modelSpacingChanged();
        }
        float platformSpacing() const { return m_platfromSpacing; }
        void setPlatformSpacing(const float val) {
            m_platfromSpacing = val;
            emit platformSpacingChanged();
        }
        int angleIndex() const {
            int xxx = m_angleList.indexOf(QVariant::fromValue(m_angle));
            return m_angleList.indexOf(QVariant::fromValue(m_angle));
        }
        void setAngleIndex(const int index) {
            m_angle = m_angleList[index < m_angleList.size() ? index : 0].toInt();
        }
        bool allowRotate()const { return m_bAllowRotate; }
        void setAllowRotate(const bool val) {
            m_bAllowRotate = val;
        }
		
		void executeJobP(QList<ModelN*> models, int priorBinIndex, bool needPrintersLayout=true, bool needAddToScene=true);

        void executeExtendFillJob(ModelN* extendModel, int curBinIndex);

        void releaseAlwaysRender();
        void doExtraLayout(int estimatePrinterSize, bool bFinal);
        void extraLayoutWork(int estimatePrinterSize);
        void doFinalLayout(const LayoutChangeInfo& changeInfo);
        void onFinalLayoutSuccessed();
		
    Q_SIGNALS:
        void typeChanged();
        void modelSpacingChanged();
        void platformSpacingChanged();
        void angleIndexChanged();
        void allowRotateChanged();
        

	private:
        PickOp* m_pickOp = nullptr;

        int m_layoutType = 0;  //0: layout all models ; 1: layout selectmodel
        float m_modelSpacing;
        float m_platfromSpacing;
        int m_angle;

        bool m_bAllowRotate = false;
        QVariantList m_angleList;

        LayoutWay m_waytype = LayoutWay::QUICK_LAYOUT;

        bool m_bAlignMove;

        // only when layout all models in scene, need to check the printer lock status
        bool m_checkPrinterLock;

        bool m_hasDoneExtraLayout;

        float m_originSpacing = 10.0f;

        extraLayoutInfo  m_extraLayoutInfo;

        LayoutChangeInfo m_currentSavedInfo;

    };

}
#endif // _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
