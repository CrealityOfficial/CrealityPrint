#include "modeleffectsetter.h"
#include "interface/reuseableinterface.h"
#include "interface/appsettinginterface.h"

using namespace Qt3DRender;

EffectItem::EffectItem(QObject* parent)
	:QObject(parent)
	, index(-1)
{}

EffectItem::EffectItem(QString t, QString uniform, int idx, QVariant v, QObject* parent)
	: QObject(parent)
	, title(t)
	, uniformName(uniform)
	, index(idx)
	, value(v)
{}

EffectItem::~EffectItem()
{}

ModelEffectSettor::ModelEffectSettor(QNode* parent)
	:Qt3DCore::QNode(parent)
	, m_outputItemIndex(0)
{
	m_effect = creative_kernel::getCachedModelEffect();

	m_stateGains = CONFIG_GLOBAL_VARLIST(modeleffect_statecolors, model_group);

	assert(m_effect != nullptr);
	assert(m_stateGains.size() >= 4);

	const QVariantList& varList = m_stateGains;

	QVector<Qt3DRender::QParameter*> parameters = m_effect->parameters();
	auto f = [parameters](const char* uniformName) -> QVariant {
		for (Qt3DRender::QParameter* param : parameters)
		{
			if (param->name() == uniformName)
			{
				return param->value();
			}
		}
		return QVariant();
	};
	
	m_items.push_back(new EffectItem("1. none state",    "stateGain", 0, varList.at(0)));
	m_items.push_back(new EffectItem("2. hover state",   "stateGain", 1, varList.at(1)));
	m_items.push_back(new EffectItem("3. selected state", "stateGain", 2, varList.at(2)));
	m_items.push_back(new EffectItem("4. preview color",  "stateGain", 3, varList.at(3)));
	m_items.push_back(new EffectItem("5. out of platform", "outPlatformGain", -1, f("outPlatformGain")));
	m_items.push_back(new EffectItem("6. inside of model", "insideGain", -1, f("insideGain")));
}

ModelEffectSettor::~ModelEffectSettor()
{
	for (auto a : m_items)
	{
		delete a;
	}
	m_items.clear();
}

void ModelEffectSettor::setNoneGain(const QVector4D& c)
{
	m_stateGains.replace(0, c);
	updateStateGains();
}

void ModelEffectSettor::setHoverGain(const QVector4D& c)
{
	m_stateGains.replace(1, c);
	updateStateGains();
}

void ModelEffectSettor::setSelectedGain(const QVector4D& c)
{
	m_stateGains.replace(2, c);
	updateStateGains();
}

void ModelEffectSettor::setPreviewGain(const QVector4D& c)
{
	m_stateGains.replace(3, c);
	updateStateGains();
}

void ModelEffectSettor::setOutPlatformGain(const QVector4D& c)
{
	setParameter("outPlatformGain", c);
}

void ModelEffectSettor::setInsideGain(const QVector4D& c)
{
	setParameter("insideGain", c);
}

void ModelEffectSettor::setParameter(const QString& name, const QVariant& value)
{
	m_effect->setParameter(name, value);
}

void ModelEffectSettor::updateStateGains()
{
	setParameter("stateGain[0]", m_stateGains);
}

void ModelEffectSettor::printParameter()
{
	qDebug() << "stateGain" << m_stateGains;

	QVector<Qt3DRender::QParameter*> parameters = m_effect->parameters();
	for (Qt3DRender::QParameter* param : parameters)
	{
		QString name = param->name();
		if (name == "outPlatformGain" || name == "insideGain")
		{
			qDebug() << name << param->value();
		}
	}
}

void ModelEffectSettor::update(EffectItem* item)
{
	if (!item)
	{
		return;
	}

	if (item->uniformName == "stateGain")
	{
		if (0 <= item->index && item->index < m_items.size())
		{
			m_stateGains.replace(item->index, item->value);
			updateStateGains();
		}
	}
	else {
		setParameter(item->uniformName, item->value);
	}

}

EffectItem* ModelEffectSettor::getNextItem()
{
	int k = m_outputItemIndex % (m_items.size() + 1);
	m_outputItemIndex++;

	if (k < m_items.size())
	{
		return m_items.at(k);
	}

	return nullptr;
}
