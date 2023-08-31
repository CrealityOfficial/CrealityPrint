#include "convexhullcalculator.h"
#include "qhullWrapper/hull/meshconvex.h"
#include "qtusercore/module/progressortracer.h"
#include <QtCore/QDebug>

ConvexHullCalculator::ConvexHullCalculator(QObject* parent)
	:QObject(parent)
{
}

ConvexHullCalculator::~ConvexHullCalculator()
{
}

void ConvexHullCalculator::calculate(trimesh::TriMesh* mesh, qtuser_core::Progressor* progressor)
{
    qtuser_core::ProgressorTracer tracer(progressor);
    qhullWrapper::calculateConvex(mesh, &tracer);
}

void ConvexHullCalculator::calculate(const QVector<QVector2D>& inputPoints, QVector<QVector3D>& convexHull2D, float z)
{
    convexHull2D.clear();
    if (inputPoints.size() == 0)
        return;

    trimesh::TriMesh* mesh = qhullWrapper::convex2DPolygon((const float*)inputPoints.data(), (int)inputPoints.size());
    if (!mesh)
        return;

    size_t size = mesh->vertices.size();
    for (size_t i = 0; i < size; ++i)
    {
        const trimesh::vec3& p = mesh->vertices.at(i);
        convexHull2D.push_back(QVector3D(p.x, p.y, z));
    }
    delete mesh;
}
