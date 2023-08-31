#include "fdmsupport.h"
#include "data/shapefiller.h"

#include "qtuser3d/module/chunkbuffer.h"

using namespace qtuser_3d;
namespace creative_kernel
{
	FDMSupport::FDMSupport(QObject* parent)
		:QObject(parent)
	{
	}
	
	FDMSupport::~FDMSupport()
	{
	}

	void FDMSupport::setParam(FDMSupportParam& param)
	{
		m_param = param;
	}

	FDMSupportParam& FDMSupport::param()
	{
		return m_param;
	}

	int FDMSupport::updatePosition(QByteArray& data)
	{
		trimesh::vec3* position = (trimesh::vec3*)data.data();

		int validSize = 0;
		validSize += ShapeFiller::fillCylinder(position, m_param.radius, m_param.bottom,
			m_param.radius, m_param.top, 4, 45.0f);

		return validSize;
	}

	void FDMSupport::updateBuffer(float* buffer)
	{
		trimesh::vec3* position = (trimesh::vec3*)buffer;

		ShapeFiller::fillCylinder(position, m_param.radius, m_param.bottom,
			m_param.radius, m_param.top, 4, 45.0f);
	}
}