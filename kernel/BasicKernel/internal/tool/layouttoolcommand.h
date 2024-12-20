#ifndef _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
#define _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
#include "qtusercore/plugin/toolcommand.h"
#include "qtusercore/util/settings.h"
#include "operation/pickop.h"
#include "data/interface.h"

namespace creative_kernel
{
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

        float modelSpacing();
        float platformSpacing() const { 
            
            qtuser_core::VersionSettings setting;
            setting.beginGroup("profile_setting");
            float platformSpacing = setting.value("platform_spacing", 1).toFloat();
            setting.endGroup();

            //m_platfromSpacing = platformSpacing;
            return platformSpacing;
        }
        int type()const { return m_layoutType; }
        int angle() const { return m_angle; }
        float getModelSpacingByPrintSequence();

        Q_INVOKABLE void autoLayout(int nlayoutWay = 0);

        Q_INVOKABLE QVariantList angleList() const {
            return m_angleList;
        }

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

        void setType(const int type) { m_layoutType = type; }
        
        void setModelSpacing(const float val) {
            m_modelSpacing = val;

            qtuser_core::VersionSettings setting;
            setting.beginGroup("profile_setting");
            setting.setValue("model_spacing", val);

            emit modelSpacingChanged();
        }
        
        void setPlatformSpacing(const float val) {
            m_platfromSpacing = val;

            qtuser_core::VersionSettings setting;
            setting.beginGroup("profile_setting");
            setting.setValue("platform_spacing", val);

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
		
    Q_SIGNALS:
        void typeChanged();
        void modelSpacingChanged();
        void platformSpacingChanged();
        void angleIndexChanged();
        void allowRotateChanged();
        

	private:
        //PickOp* m_pickOp = nullptr;

        int m_layoutType = 0;  //0: layout all models ; 1: layout selectmodel
        float m_modelSpacing;
        float m_platfromSpacing;
        int m_angle;

        bool m_bAllowRotate = false;
        QVariantList m_angleList;

        //LayoutWay m_waytype = LayoutWay::QUICK_LAYOUT;
        int m_waytype = 0;

        bool m_bAlignMove;

        float m_originSpacing = 10.0f;
    };

}
#endif // _NULLSPACE_LAYOUTTOOLCOMMAND_1589427153657_H
