#include "colorworker.h"
#include "trimesh2/TriMesh.h"
#include <QElapsedTimer>
#include <QDebug>

ColorWorker::ColorWorker(spread::MeshSpreadWrapper *wrapper, int colorIndex, spread::CursorType colorMethod, QObject* parent) :
    m_meshSpreadWrapper(wrapper), 
    m_colorIndex(colorIndex),
    m_colorMethod(colorMethod),
    QObject(parent)
{}

void ColorWorker::setTriangleParameter(int triangleID)
{
    if (m_colorMethod != spread::CursorType::TRIANGLE || m_colorMethod != spread::CursorType::FILL)
        return;

    m_triangleId = triangleID;
}

void ColorWorker::setCircleParameter(const spread::SceneData& data, bool isFirstCircle)
{
    m_isFirstCircle = isFirstCircle;
    m_faceId = data.faceId;
    m_center = data.center;
    m_cameraPos = data.cameraPos;
    m_radius = data.radius;
    m_planeNormal = data.planeNormal;
	m_planeOffset = data.planeOffset;
}

void ColorWorker::setSphereParameter(const spread::SceneData& data, float size, bool isFirstCircle)
{
    m_isFirstCircle = isFirstCircle;
    m_sphereSize = size;
    m_faceId = data.faceId;
    m_center = data.center;
    m_cameraPos = data.cameraPos;
    m_radius = data.radius;
    m_planeNormal = data.planeNormal;
	m_planeOffset = data.planeOffset;
}

void ColorWorker::setHeightRangeParameter(const spread::SceneData& data, float height)
{
    m_faceId = data.faceId;
    m_center = data.center;
    m_cameraPos = data.cameraPos;
    m_radius = data.radius;
    m_planeNormal = data.planeNormal;
    m_planeOffset = data.planeOffset;
    m_height = height;
}

void ColorWorker::setGapFillParameter(float size)
{
    m_gapFillAreaSize = size;
}

std::vector<int> ColorWorker::execute()
{
    std::vector<int> dirtyChunks;
    if (m_colorMethod == spread::CursorType::TRIANGLE)
    {
        m_meshSpreadWrapper->bucket_fill_select_triangles(m_colorIndex, dirtyChunks);
    }
    else if (m_colorMethod == spread::CursorType::CIRCLE)
    {
        static trimesh::vec3 lastCenter(0, 0, 0);
        if (m_isFirstCircle)
        {
            m_meshSpreadWrapper->circile_factory(m_center, m_cameraPos, m_radius,
                            m_faceId, m_colorIndex, m_planeNormal, m_planeOffset,
                            dirtyChunks);
        }
        else 
        {
            m_meshSpreadWrapper->double_circile_factory(lastCenter, m_center, m_cameraPos,
                                   m_radius, m_faceId, m_colorIndex, m_planeNormal, m_planeOffset,
                                   dirtyChunks);
        }
        lastCenter = m_center;
    }
    else if (m_colorMethod == spread::CursorType::SPHERE)
    {
        static trimesh::vec3 lastCenter(0, 0, 0);
        if (m_isFirstCircle)
        {
            m_meshSpreadWrapper->sphere_factory(m_center, m_cameraPos, m_sphereSize,
                            m_faceId, m_colorIndex, m_planeNormal, m_planeOffset,
                            dirtyChunks);
        }
        else 
        {
            m_meshSpreadWrapper->double_sphere_factory(lastCenter, m_center, m_cameraPos,
                                   m_sphereSize, m_faceId, m_colorIndex, m_planeNormal, m_planeOffset,
                                   dirtyChunks);
        }
        lastCenter = m_center;
    }
    else if (m_colorMethod == spread::CursorType::FILL)
    {
        m_meshSpreadWrapper->bucket_fill_select_triangles(m_colorIndex, dirtyChunks);
    }
	else if (m_colorMethod == spread::CursorType::HEIGHT_RANGE)
	{
        m_meshSpreadWrapper->height_factory(m_center, m_cameraPos, m_height, m_faceId, m_colorIndex, m_planeNormal, m_planeOffset, dirtyChunks);
       // m_meshSpreadWrapper->sphere_factory(m_center, m_cameraPos, m_height, m_faceId, m_colorIndex, m_planeNormal, m_planeOffset, dirtyChunks);
	}
	else if (m_colorMethod == spread::CursorType::GAP_FILL)
	{
        m_meshSpreadWrapper->apply_triangle_state(dirtyChunks);
	}

    return dirtyChunks;
}
