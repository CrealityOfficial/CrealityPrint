#ifndef CREATIVE_KERNEL_MODELEFFECT_SETTER_202312081655_H
#define CREATIVE_KERNEL_MODELEFFECT_SETTER_202312081655_H

#include "qtuser3d/refactor/xeffect.h"

class EffectItem : public QObject
{
public:
	EffectItem(QObject* parent = nullptr);
	EffectItem(QString title, QString uniformName, int idx, QVariant value, QObject* parent = nullptr);
	~EffectItem();

	QString title;
	QString uniformName;
	int index;
	QVariant value;
};

class ModelEffectSettor : public Qt3DCore::QNode
{
public:
	ModelEffectSettor(QNode* parent = nullptr);
	virtual ~ModelEffectSettor();

	void update(EffectItem *item);

	EffectItem* getNextItem();

	void printParameter();

protected:
	void setParameter(const QString& name, const QVariant& value);

	void updateStateGains();

	//state
	void setNoneGain(const QVector4D& c);
	void setHoverGain(const QVector4D& c);
	void setSelectedGain(const QVector4D& c);
	void setPreviewGain(const QVector4D& c);

	//except branch
	void setOutPlatformGain(const QVector4D& c);
	void setInsideGain(const QVector4D& c);

private:
	qtuser_3d::XEffect* m_effect;
	QVariantList m_stateGains;

	QList<EffectItem*> m_items;

	int m_outputItemIndex;
};

#endif //CREATIVE_KERNEL_MODELEFFECT_SETTER_202312081655_H