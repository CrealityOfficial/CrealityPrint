#ifndef _BASIC_SHAPE_CREATE_HELPER_H__
#define _BASIC_SHAPE_CREATE_HELPER_H__
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include <vector>

namespace qtuser_3d
{
	class QTUSER_3D_API BasicShapeCreateHelper : public GeometryCreateHelper
	{
		Q_OBJECT
	public:
		BasicShapeCreateHelper(QObject* parent = nullptr);
		virtual ~BasicShapeCreateHelper();

		static Qt3DRender::QGeometry* createCylinder(Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createRectangle(float w, float h, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createPen(Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createBall(QVector3D center, float r, float angleSpan, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createInstructionsDLP(float cylinderR, float cylinderLen, float axisR, float axisLen, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createScaleIndicatorDLP(float cylinderR, float cylinderLen, int seg, float squarelen, Qt3DCore::QNode* parent = nullptr);
		
		static Qt3DRender::QGeometry* createInstructions(float cylinderR, float cylinderLen, float axisR, float axisLen, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createScaleIndicator(float cylinderR, float cylinderLen, int seg, float squarelen, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createGeometry(Qt3DCore::QNode* parent, std::vector<float> *vertexDatas, std::vector<float>* normalDatas = nullptr, QVector<unsigned>* indices = nullptr);

		static Qt3DRender::QGeometry* createGeometryEx(Qt3DCore::QNode* parent, std::vector<float>* vertexDatas, std::vector<float>* normalDatas = nullptr, QVector<unsigned>* indices = nullptr);

	public:
		static int createCylinderData(float r, float h, int seg, std::vector<float> &datas);

		static int createVerticalCylinderData(float r, float h, int seg, float offset_per, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);

		static int createPenData(float r, float headh, float bodyh, int seg, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);

		static int createBallData(QVector3D center, float r, float angleSpan, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);

		static int createInstructionsDataDLP(float cylinderR, float cylinderLen, float axisR, float axisLen, int seg, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);

		static int createScaleIndicatorDataDLP(float cylinderR, float cylinderLen, int seg, float squarelen, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);
		
		static int createInstructionsData(float cylinderR, float cylinderLen, float axisR, float axisLen, int seg, std::vector<float>& vertexDatas, std::vector<float>& normals);

		static int createScaleIndicatorData(float cylinderR, float cylinderLen, int seg, float squarelen, std::vector<float>& vertexDatas, std::vector<float>& normals);

	private:
		static int addFaceDataWithQVector3D(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, const QVector3D& n, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);
		
		static int addFaceDataWithQVector3D(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, const QVector3D& n, std::vector<float>& vertexDatas);

		static int addFaceDataWithQVector3DEx(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, const QVector3D& n, std::vector<float>& vertexDatas, std::vector<float>& normalDatas);

	};
}
#endif // QTUSER_3D_TRIANGLESCREATEHELPER_1594889735714_H