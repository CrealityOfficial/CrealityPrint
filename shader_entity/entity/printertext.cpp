#include "printertext.h"
#include <QtCore/qmath.h>
#include <QtGui/QVector4D>
#include "entity/textmeshentity.h"

namespace qtuser_3d
{
	PrinterText::PrinterText(Qt3DCore::QNode* parent)
		:QEntity(parent)
	{
	}

	PrinterText::~PrinterText()
	{
	}

	void PrinterText::updateLen(Box3D& box, float gap, int major)
	{
		qDeleteAll(m_majors);
		qDeleteAll(m_minors);

		int maxLen = box.size().x();

		m_majors.clear();
		m_minors.clear();

		float minX = box.min.x();
		float maxX = box.max.x();

		float skirtGap = 14.0f / 220.0 * (maxX - minX);
		int startX = 0;
		int endX = qFloor(maxLen / gap);

		float majorSize = 4.0f;
		float minorSize = 2.0f;
		
		QVector4D color = QVector4D(0.180f, 0.541f, 0.968f, 1.0f);
		for (int i = startX; i <= endX; ++i)
		{
			if (i % 4 == 0)
			{
				QMatrix4x4 majorMatrix;
				TextMeshEntity* textEntity = new TextMeshEntity(this);
				textEntity->setColor(color);
				QString text = QString::number(i * (int)gap);
				int textLen = text.length();
				textEntity->setText(text);
				majorMatrix.translate(QVector3D((float)i * gap - textLen * majorSize / 3.0f, -(skirtGap + majorSize), 0.0f));
				majorMatrix.scale(majorSize);
				textEntity->setPose(majorMatrix);
				m_majors.push_back(textEntity);
			}
		}
		for (int i = startX; i <= endX; ++i)
		{
			if (i % 4 != 0)
			{
				QMatrix4x4 matrix;
				TextMeshEntity* textEntity = new TextMeshEntity(this);
				textEntity->setColor(color);
				QString text = QString::number(i * (int)gap);
				int textLen = text.length();
				textEntity->setText(text);
				matrix.translate(QVector3D((float)i * gap - textLen * minorSize / 3.0f, -(skirtGap + minorSize), 0.0f));
				matrix.scale(minorSize);
				textEntity->setPose(matrix);
				m_minors.push_back(textEntity);
			}
		}
	}

	void PrinterText::updateScaleMark(Box3D& box, float gap, float fontSize)
	{
		qDeleteAll(m_majors);
		m_majors.clear();
		QString Qstrunit = "mm";
		float minX = box.min.x();
		float maxX = box.max.x();
		//int maxLen = box.size().x();
		QVector4D color = QVector4D(0.5f, 0.5f, 0.5f, 1.0f);

		QMatrix4x4 Matrix;
		TextMeshEntity* textEntity = new TextMeshEntity(this);
		textEntity->setColor(color);
		QString text = QString::number(minX) + Qstrunit;
		textEntity->setText(text);
		Matrix.translate(QVector3D(0, gap, 0.0f));
		Matrix.scale(10);
		textEntity->setPose(Matrix);
		m_majors.push_back(textEntity);

		QMatrix4x4 Matrix2;
		TextMeshEntity* TextMiddle = new TextMeshEntity(this);
		TextMiddle->setColor(color);
		QString textmiddle = QString::number(maxX/2.0) + Qstrunit;
		TextMiddle->setText(textmiddle);
		Matrix2.translate(QVector3D(maxX/2.0- text.length()/2.0*fontSize, gap, 0.0f));
		Matrix2.scale(10);
		TextMiddle->setPose(Matrix2);
		m_majors.push_back(TextMiddle);

		QMatrix4x4 Matrix3;
		TextMeshEntity* textEnd = new TextMeshEntity(this);
		textEnd->setColor(color);
		QString textend = QString::number(maxX) + Qstrunit;
		textEnd->setText(textend);
		Matrix3.translate(QVector3D(maxX - text.length()*fontSize, gap, 0.0f));
		Matrix3.scale(10);
		textEnd->setPose(Matrix3);
		m_majors.push_back(textEnd);
	}

}
