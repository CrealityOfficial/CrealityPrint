#pragma once

#include <memory>
#include <qstringliteral.h>
#include <QPointer>
#include <QQmlComponent>

#include <qtuser3d/refactor/xentity.h>
#include <qtuser3d/scene/sceneoperatemode.h>
#include "qtuser3d/module/manipulatecallback.h"
#include "operation/moveoperatemode.h"
#include "internal/alg/new_letter.h"

#include <data/interface.h>
#include <QTimer>

#include <QUndoStack>
#include "reliefundocommand.h"
#include "operation/fontmesh/fontmeshwrapper.h"

namespace creative_kernel
{
	class ModelN;
    class ModelGroup;
};

class OutlineGenerator;
class FontMeshWrapper;
class ReliefMoveUndoCommand;

namespace topomesh {
    class FontMesh;
    struct FontConfig;
}

namespace qtuser_3d
{
    class AlonePointEntity;
    class Rotate3DHelperEntity;
}

class ReliefOperateMode : public creative_kernel::MoveOperateMode
                        , public creative_kernel::ModelNSelectorTracer 
                        , public qtuser_3d::RotateCallback
                        , public creative_kernel::SpaceTracer
{
	Q_OBJECT;

public:
    enum class ReliefType : int 
    {
        Text = 0,
        Shape = 1,
    };

    enum class Shape : int 
    {
        Rectangle = 0,
        Circle = 1,
        Triangle = 2,
    };
public:
    explicit ReliefOperateMode(QObject* parent = nullptr);
    virtual ~ReliefOperateMode();

public:
    ReliefType getType() const;
    void setType(ReliefType type);
    Q_SIGNAL void typeChanged();

    Shape getShape() const;
    void setShape(Shape shape);
    Q_SIGNAL void shapeChanged();

    QString getText() const;
    void setText(const QString& text);
    Q_SIGNAL void textChanged();

    QString getFont() const;
    void setFont(const QString& font);
    Q_SIGNAL void fontChanged();

    int fontSize() const;
    void setFontSize(int size);
    Q_SIGNAL void fontSizeChanged();

    int wordSpace() const;
    void setWordSpace(int space);
    Q_SIGNAL void wordSpaceChanged();

    int lineSpace() const;
    void setLineSpace(int space);
    Q_SIGNAL void lineSpaceChanged();

    float height() const;
    void setHeight(float height);
    Q_SIGNAL void heightChanged();

    float distance() const;
    void setDistance(float distance);
    Q_SIGNAL void distanceChanged();

    int embossType() const;
    void setEmbossType(int type);
    Q_SIGNAL void embossTypeChanged();

    int reliefModelType();
    void setReliefModelType(int type);
    Q_SIGNAL void reliefModelTypeChanged();

    bool bold() const;
	void setBold(bool bold);
    Q_SIGNAL void boldChanged();

    bool italic() const;
	void setItalic(bool italic);
    Q_SIGNAL void italicChanged();

    QPoint pos() const;
    Q_SIGNAL void posChanged();

    bool needIndicate() const;
    Q_SIGNAL void needIndicateChanged();

    bool hoverOnModel() const;
    Q_SIGNAL void hoverOnModelChanged();

    bool canEmboss() const;
    Q_SIGNAL void canEmbossChanged();

    bool distanceEnabled() const;
    Q_SIGNAL void distanceEnabledChanged();

    Q_SIGNAL void fontConfigChanged();

    void applyFontConfigToRelief(creative_kernel::ModelN* relief, ReliefUndoConfig config);
    void rotateRelief(creative_kernel::ModelN* relief, float angle);
    void moveRelief(creative_kernel::ModelN* relief, creative_kernel::ModelN* target, int faceId, QVector3D location, int embossType);
    void initRelief(creative_kernel::ModelN* relief);

	virtual void onModelTypeChanged(creative_kernel::ModelN* model);

private:
    void joinReliefOperate(creative_kernel::ModelN* relief);

protected:
    virtual void onAttach() override;
    virtual void onDettach() override;
    virtual bool shouldMultipleSelect() override; 

    virtual void onHoverMove(QHoverEvent* event) override;
    virtual void onLeftMouseButtonMove(QMouseEvent* event) override;
    virtual void onLeftMouseButtonClick(QMouseEvent* event) override;
    virtual void onLeftMouseButtonPress(QMouseEvent* event) override;
    virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;

	virtual void onSelectionsChanged();

    virtual void onStartRotate() override;
    virtual void onRotate(QQuaternion q) override;
    virtual void onEndRotate(QQuaternion q) override;
    virtual void setRotateAngle(QVector3D axis, float angle) override;

	virtual void onSceneChanged(const trimesh::dbox3& box) override;

private:
    void setFontMeshWrapper(FontMeshWrapper* wrapper);

    void onFontConfigChanged();
    bool holdWrapper();
    void showRotateEntity();
    void hideRotateEntity();
    void emboss(const QPoint& pos, bool reversible = false);
    void rotate(const QQuaternion& q, bool reversible = false);
    void rotate(float angle);

    creative_kernel::ModelGroup* getValidSelectedGroup();
    ReliefUndoConfig generateUndoConfig();


    void recordStep(ReliefUndoConfig oldConfig, ReliefUndoConfig newConfig);          // used in global
    void recordStep(float oldAngle, float newAngle);                                // used in global
    void beginEmboss();
    void endEmboss();

    void recordStep(topomesh::FontMoveConfig  oldConfig, topomesh::FontMoveConfig newConfig);          // used in global
    void recordSubStep();   //  used in relief operate mode

    /* qml tips */
    void setTipObjectPos(const QPoint& point);
    void setTipObjectVisible(bool visible);
    bool getTipObjectVisible();

private:
    QStringList m_angleLogs;

    ReliefType m_reliefType { ReliefType::Text };
    Shape m_shape { Shape::Rectangle };

    ReliefMoveUndoCommand* m_embossCommand { NULL };

    QPoint m_pos;
    bool m_needIndicate { false };
    bool m_hoverOnModel { false };

    bool m_movingRelief { false };
    bool m_rotatingRelief { false };

    bool m_isReliefObject { false };

    bool m_canEmboss { false };

    float m_angle { 0 };

    bool m_generating { false };
    bool m_initing { false };
    
    QPointer<QQmlComponent> tip_component_;
    QPointer<QObject> tip_object_;


	creative_kernel::ModelN* m_operateModel;

    /* generate temp data */
    // topomesh::FontMesh* m_fm;

    FontConfig m_defaultFontConfig;
    FontConfig* m_fontConfig { NULL };
    FontMeshWrapper* m_wrapper{NULL};

	qtuser_3d::XEffect *m_reliefEffect;
	qtuser_3d::XEffect *m_reliefPartEffect;

    qtuser_3d::AlonePointEntity* m_pointEntity;

    qtuser_3d::Rotate3DHelperEntity* m_rotateEntity;

    QTimer m_delayTimer;

    QUndoStack m_subStepStack;
    bool m_applyingConfig{ false };

};
