#ifndef _NULLSPACE_CONVEXHULLCALCULATOR_1591752146634_H
#define _NULLSPACE_CONVEXHULLCALCULATOR_1591752146634_H
#include "basickernelexport.h"
#include <QtCore/QObject>
#include "qtusercore/module/progressor.h"
#include "trimesh2/TriMesh.h"
#include <QtCore/QVector>
#include <QtGui/QVector3D>
#include <QtGui/QVector2D>

class BASIC_KERNEL_API ConvexHullCalculator: public QObject
{
public:
	ConvexHullCalculator(QObject* parent = nullptr);
	virtual ~ConvexHullCalculator();

	static void calculate(trimesh::TriMesh* mesh, qtuser_core::Progressor* progressor);

	static void calculate(const QVector<QVector2D>& inputPoints, QVector<QVector3D>& convexHull2D, float z);
};
#endif // _NULLSPACE_CONVEXHULLCALCULATOR_1591752146634_H
