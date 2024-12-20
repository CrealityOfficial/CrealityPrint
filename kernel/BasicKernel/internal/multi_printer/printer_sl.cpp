#include "printer.h"
#include "printersettings.h"
#include "interface/printerinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/displaywarninginterface.h"

#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/trimesh2/camera.h"
#include <qtuser3d/framegraph/colorpicker.h>
#include <qtuser3d/framegraph/surface.h>
#include <qtuser3d/framegraph/texturerendertarget.h>
#include "kernel/kernel.h"
#include <Qt3DRender/QCamera>
#include <QQuickImageProvider>

#include "kernel/kernelui.h"
#include "kernel/kernel.h"

#include "cxkernel/interface/uiinterface.h"

#include "external/kernel/reuseablecache.h"
#include "external/slice/sliceattain.h"
#include "external/kernel/visualscene.h"

#include "data/modeln.h"
#include "data/modelgroup.h"
#include "slice/sliceflow.h"

const char* PlateTypeKey = "curr_bed_type";
const char* PrintSequenceKey = "print_sequence";

namespace creative_kernel {

    class ImageProvider : public QQuickImageProvider
    {
    public:
        ImageProvider(const QImage& _image) : QQuickImageProvider(QQuickImageProvider::Image)
        {
            image = _image;
        }

        QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
        {
            Q_UNUSED(id);

            if (size)
                *size = image.size();

            return image;
        }
    private:
        QImage image;
    };


	void Printer::applyLayersColor(const QVariantList& layersColor)
	{
		for(ModelN* model : m_modelsInside)
		{
			model->setLayersColor(layersColor);
		}
	}

	void Printer::reloadLayersConfig()
	{
		m_settings->reload();
	}

	void Printer::clearLayersConfig()
	{
		m_settings->clearLayerConfig();
	}	

	crslice2::PlateInfo Printer::getPlateInfo()
	{
		return m_settings->getPlateInfo();
	}

	void Printer::requestPicture()
	{
		m_requestPictureTimer.start(500);
	}

	void Printer::triggerCapture()
	{
		auto captureCallback = [=](QObject* receiver, const QImage& image)
		{
            Printer* printer = dynamic_cast<Printer*>(receiver);
            if (printer)
			{

				for (ModelGroup* modelGroup : printer->modelGroupsInside())
				{
					for (ModelN* model : modelGroup->models((int)ModelNType::normal_part))
					{
						model->setOffscreenRenderEnabled(false);
					}
				}
                printer->setPicture(image);
				int id = printer->index();
				ImageProvider* provider1 = new ImageProvider(printer->picture());
				QString name1 = QString("active_slice%1").arg(id);
				cxkernel::registerImageProvider(name1, provider1);

				id += 100;
				ImageProvider* provider2 = new ImageProvider(printer->picture());
				QString name2 = QString("active_slice%1").arg(id);
				cxkernel::registerImageProvider(name2, provider2);

				bool isValid = false;
				QImage image = printer->picture();
				for (int y = 0; y < image.height(); ++y) {
					for (int x = 0; x < image.width(); ++x) {
						QColor color = QColor::fromRgba(image.pixel(x, y));
						if (color.isValid() && color != QColor(0, 0, 0, 0)) {
							isValid = true;
							break;
						}
					}
					if (isValid)
						break;
				}
				if (!isValid)
					qWarning() << QString("Capture a invalid picture on plate %1!").arg(printer->index());
				// printer->picture().save(QString("E:/%1.png").arg(name1));

			}

		};

		VisualScene* visScene = getKernel()->visScene();

		//使用外部摄像头
		Qt3DRender::QCamera* pickerCamera = new Qt3DRender::QCamera(visScene);
		
		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::OrthographicProjection);

		pickerCamera->setAspectRatio(1.0f);

		float fovy = 5.0f;

		float minX, minY, minZ, maxX, maxY, maxZ;
		minX = minY = minZ = 100000;
		maxX = maxY = maxZ = -100000;
		QVector3D printerCenter = globalBox().center();
		QVector2D printerOffset = QVector2D(printerCenter.x(), printerCenter.y()) * 2;
		for (ModelN* model : modelsInside())
		{
			if (model->modelNType() == ModelNType::normal_part && 
				model->isVisible())
			{
				model->setOffscreenRenderEnabled(true);

				// 计算预览区域
				auto box = model->globalSpaceBox();
				minX = box.min.x() < minX ? box.min.x() : minX;
				minY = box.min.y() < minY ? box.min.y() : minY;
				minZ = box.min.z() < minZ ? box.min.z() : minZ;
				maxX = box.max.x() > maxX ? box.max.x() : maxX;
				maxY = box.max.y() > maxY ? box.max.y() : maxY;
				maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;

				QMatrix4x4 matrix1 = model->globalMatrix();
				matrix1(0, 3) = 0;
				matrix1(1, 3) = 0;
				matrix1(2, 3) = 0;
				matrix1 = matrix1.inverted();
				QVector3D temp1 = printerOffset;
				temp1 = matrix1.map(temp1);
				model->setOffscreenRenderOffset(temp1);
			}
			else 
			{
				model->setOffscreenRenderEnabled(false);
			}
		}
		minX += printerOffset.x();
		minY += printerOffset.y();
		maxX += printerOffset.x();
		maxY += printerOffset.y();

		float cameraDir[3] = {-1.0f, -1.0f, 1.15f};
		auto rbox = trimesh::box3(trimesh::point3(minX, minY, minZ), trimesh::point3(maxX, maxY, maxZ));
		qtuser_3d::ScreenCameraMeta meta = qtuser_3d::viewAllByProjection(rbox, trimesh::vec3(cameraDir[0], cameraDir[1], cameraDir[2]), fovy);

		pickerCamera->setPosition(qtuser_3d::vec2qvector(meta.position));
		pickerCamera->setViewCenter(qtuser_3d::vec2qvector(meta.viewCenter));
		pickerCamera->setUpVector(qtuser_3d::vec2qvector(meta.upVector));

		pickerCamera->setFarPlane(10000.0f);
		pickerCamera->setNearPlane(1.0f);

		QVector3D box(maxX - minX, maxY - minY, maxZ - minZ);
		QMatrix4x4 view;
        view.lookAt(QVector3D(cameraDir[0], cameraDir[1], cameraDir[2]), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)); // 相机位置和方向

		float zFactor = 2.0; 

		QVector3D p1(box.x() / 2, box.y() / 2, box.z() / zFactor);
		p1 = view.map(p1);

		QVector3D p2(-box.x() / 2, -box.y() / 2, -box.z() / zFactor);
		p2 = view.map(p2);

		QVector3D p3(box.x() / 2, -box.y() / 2, -box.z() / zFactor);
		p3 = view.map(p3);

		QVector3D p4(-box.x() / 2, box.y() / 2, -box.z() / zFactor);
		p4 = view.map(p4);

		float height = p2.distanceToPoint(p1);
		float width = p3.distanceToPoint(p4);
		float borderLength = width > height ? width : height;

		pickerCamera->setBottom(-borderLength / 2.0);
		pickerCamera->setTop(borderLength / 2.0);
		pickerCamera->setLeft(-borderLength / 2.0);
		pickerCamera->setRight(borderLength / 2.0);

		requestCapture(pickerCamera, this, captureCallback);
	}

	void Printer::applySettings()
	{
		m_settings->applySettings();
		settingsApplyed();
	}

	void Printer::setParameter(const QString& key, const QString& value)
	{
		m_settings->setValue(key, value);

		if (key == PlateTypeKey)
		{
			onLocalPlateTypeChanged(value);
		}
		else if (key == PrintSequenceKey)
		{
			if (m_tracer)
			{
				m_tracer->localPrintSequenceChanged(this, value);
			}
		}
		parameterChanged(key, value);
	}

	QString Printer::parameter(const QString& key)
	{
		return m_settings->value(key);
	}
	
	QObject* Printer::settingsObject()
	{
		return (QObject*)m_settings;
	}

	std::map<std::string, std::string> Printer::stdSettings()
	{
		return m_settings->stdSettings();
	}

	void Printer::setSliceError(const QString& error)
	{
		m_sliceInfo.sliceError = error;
		checkSliceError();
	}

	void Printer::clearSliceError()
	{
		m_sliceInfo.sliceError.clear();
		checkSliceError();
	}

	void Printer::checkSliceError()
	{
		checkSliceEngineError(m_sliceInfo.sliceError);
	}

	QString Printer::pictureUrl() 
	{
		int id = m_index + m_pictureIndex * 100;
		m_pictureIndex = !m_pictureIndex;
		QString url = QString("image://active_slice%1").arg(id);
		return url;
	}
	
	QImage Printer::picture() const
	{
		return m_sliceInfo.picture;
	}

	void Printer::setPicture(const QImage& p)
	{
		m_sliceInfo.picture = p.copy();
	}

	bool Printer::isSliced()
	{
		return m_sliceInfo.isSliced;
	}

	void Printer::setIsSliced(bool isSliced)
	{
		if (m_sliceInfo.isSliced == isSliced)
			return;

		m_sliceInfo.isSliced = isSliced;
		emit signalSlicedStateChanged();
	}
	
	void Printer::reserveSliceCache()
	{
		m_reserveSliceCacheFlag = true;
	}

	void Printer::clearSliceCache()
	{
		if (m_sliceInfo.sliceCache)
		{
			delete m_sliceInfo.sliceCache;
			m_sliceInfo.sliceCache = NULL;
			// emit signalAttainChanged();
		}
	}

	QObject* Printer::sliceCache()
	{
		if (!m_sliceInfo.sliceCache)
			return m_sliceInfo.attain;
		else 
			return m_sliceInfo.sliceCache;
	}

	QObject* Printer::attain()
	{
		return m_sliceInfo.attain;
	}

	void Printer::setAttain(SliceAttain* attain)
	{
		if (attain == m_sliceInfo.attain)
			return;

		if (!m_reserveSliceCacheFlag)
			clearSliceCache();

		if (m_sliceInfo.attain && m_reserveSliceCacheFlag)
		{
			clearSliceCache();
			m_sliceInfo.sliceCache = m_sliceInfo.attain;
			m_sliceInfo.attain = NULL;
		}

		m_reserveSliceCacheFlag = false;
        m_sliceInfo.attain = (QObject*)attain;
		emit signalAttainChanged();
	}

	QString Printer::getLocalPrintSequence()
	{
		return parameter(PrintSequenceKey);
	}

	QString Printer::getPrintSequence()
	{
		QString seq = getLocalPrintSequence();
		if (seq.isEmpty())
		{
			seq = m_tracer->globalPrintSequence();
		}
		return seq;
	}

	QString Printer::getLocalPlateType()
	{
		return parameter(PlateTypeKey);
	}
}
