#include "fdmsupporthandler.h"
#include "qtuser3d/module/facepicker.h"

#include "data/fdmsupportgroup.h"
#include "data/fdmsupport.h"

#include "interface/spaceinterface.h"

using namespace creative_kernel;
using namespace qtuser_3d;
FDMSupportHandler::FDMSupportHandler(QObject* parent)
	:QObject(parent)
	, m_pickSource(nullptr)
	, m_hoverSupport(nullptr)
{
}

FDMSupportHandler::~FDMSupportHandler()
{
}

void FDMSupportHandler::setPickSource(FacePicker* picker)
{
	m_pickSource = picker;
}

void FDMSupportHandler::clear()
{
	m_hoverSupport = nullptr;
}

void FDMSupportHandler::reset()
{
	m_hoverSupport = nullptr;
}

FDMSupport* FDMSupportHandler::check(const QPoint& point, QVector3D& position)
{
	int faceID = -1;
	FDMSupport* hoverSupport = nullptr;
	if (m_pickSource->pick(point, &faceID))
	{
		FDMSupportGroup* supportGroup = face2SupportGroup(faceID);

		if (supportGroup)
		{
			hoverSupport = supportGroup->face2Support(faceID, &position);
		}
	}
	return hoverSupport;
}

FDMSupport* FDMSupportHandler::hover(const QPoint& point, QVector3D& position)
{
	FDMSupport* hoverSupport = check(point, position);

	if (m_hoverSupport == hoverSupport)
	{
		return m_hoverSupport;
	}

	if (m_hoverSupport)
	{
		FDMSupportGroup* supportGroup = qobject_cast<FDMSupportGroup*>(m_hoverSupport->parent());
		supportGroup->setSupportState(m_hoverSupport, 1);
	}

	m_hoverSupport = hoverSupport;
	if (m_hoverSupport)
	{
		FDMSupportGroup* supportGroup = qobject_cast<FDMSupportGroup*>(m_hoverSupport->parent());
		supportGroup->setSupportState(m_hoverSupport, 2);
	}

	return m_hoverSupport;
}

FDMSupportGroup* FDMSupportHandler::face2SupportGroup(int faceID)
{
	FDMSupportGroup* result = nullptr;
	QList<FDMSupportGroup*> fdmSupportGroups = fdmSuppportGroups();
	for (FDMSupportGroup* supportGroup : fdmSupportGroups)
	{
		int startIndex = supportGroup->faceBase();
		int endIndex = supportGroup->primitiveNum() + startIndex;
		if (faceID >= startIndex && faceID < endIndex)
		{
			result = supportGroup;
			break;
		}
	}
	return result;
}