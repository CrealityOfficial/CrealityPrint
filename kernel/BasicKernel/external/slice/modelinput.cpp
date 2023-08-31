#include "modelinput.h"

namespace creative_kernel
{
	ModelInput::ModelInput(QObject* parent)
		: QObject(parent)
		, m_id(0)
	{
		settings = new us::USettings(this);
	}

	ModelInput::~ModelInput()
	{
	}

	int ModelInput::id() const
	{
		return m_id;
	}

	void ModelInput::setID(int id)
	{
		m_id = id;
	}

	const QString& ModelInput::name() const
	{
		return m_name;
	}

	void ModelInput::setName(const QString& name)
	{
		m_name = name;
	}

	int ModelInput::vertexNum() const
	{
		return 0;
	}

	float* ModelInput::position() const
	{
		return nullptr;
	}

	float* ModelInput::normal() const
	{
		return nullptr;
	}

	TriMeshPtr ModelInput::ptr()
	{
		return m_mesh;
	}

	TriMeshPtr ModelInput::mesh()
	{
		return m_mesh;
	}

	void ModelInput::setPtr(TriMeshPtr mesh)
	{
		m_mesh = mesh;
	}

	int ModelInput::triangleNum() const
	{
		return 0;
	}

	int* ModelInput::indices() const
	{
		return nullptr;
	}

	ModelGroupInput::ModelGroupInput(QObject* parent)
		:QObject(parent)
	{
		settings = new us::USettings(this);
	}

	ModelGroupInput::~ModelGroupInput()
	{
	}

	void ModelGroupInput::addModelInput(ModelInput* modelInput)
	{
		if (modelInput)
		{
			modelInput->setParent(this);
			modelInputs.push_back(modelInput);
		}
	}
}

