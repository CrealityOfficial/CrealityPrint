#ifndef _US_MODELINPUT_1589340410146_H
#define _US_MODELINPUT_1589340410146_H
#include "us/usettings.h"
#include "data/header.h"
#include <QMatrix4x4>

namespace creative_kernel
{
	class ModelInput : public QObject
	{
	public:
		ModelInput(QObject* parent = nullptr);
		virtual ~ModelInput();

		int id() const;
		void setID(int id);

		const QString& name() const;
		void setName(const QString& name);

		void setColors2Facets(const std::vector<std::string>& colors2Facets);
		void setSeam2Facets(const std::vector<std::string>& seam2Facets);
		void setSupport2Facets(const std::vector<std::string>& support2Facets);
		std::vector<std::string>& getColors2Facets();
		std::vector<std::string>& getSeam2Facets();
		std::vector<std::string>& getSupport2Facets();

		virtual int vertexNum() const;
		virtual float* position() const;
		virtual float* normal() const;
		virtual TriMeshPtr ptr();
		void setPtr(TriMeshPtr mesh);
		TriMeshPtr mesh();

		void setModelType(int modelType);
		int modelType();

		virtual int triangleNum() const;
		virtual int* indices() const;

		trimesh::xform getXform() { return m_xform; }

	protected:
		int m_id;
		QString m_name;
		TriMeshPtr m_mesh;
		trimesh::xform m_xform;
		int m_modelType;

		std::vector<std::string> m_colors2Facets; //color
		std::vector<std::string> m_seam2Facets; //zseam
		std::vector<std::string> m_support2Facets; //support
	public:
		us::USettings* settings;
	};

	class ModelGroupInput : public QObject
	{
	public:
		ModelGroupInput(QObject* parent = nullptr);
		virtual ~ModelGroupInput();

		void addModelInput(ModelInput* modelInput);

		QList<ModelInput*> modelInputs;
		us::USettings* settings;

		trimesh::xform groupTransform;
		int64_t groupId;
	};
}

#endif // _US_MODELINPUT_1589340410146_H
