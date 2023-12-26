#include "plateentity.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/geometry/roundedrectanglehelper.h"
#include "qtuser3d/geometry/trianglescreatehelper.h"
#include "qtuser3d/utils/texturecreator.h"

#include "qtuser3d/module/simplepickable.h"

#include "entity/printergrid.h"

#include "platepurecomponententity.h"
#include "platetexturecomponententity.h"

#include "interface/selectorinterface.h"

#include <QtGui/qpainter.h>

#include "internal/tool/layouttoolcommand.h"
#include "external/interface/modelinterface.h"

using namespace qtuser_3d;
namespace creative_kernel {

	PlateEntity::PlateEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_theme(-1)
		, m_highlight(false)
		, m_topLeftLabel(nullptr)
		, m_topRightLabel(nullptr)
	{		
		QMatrix4x4 pose;

		m_groundEntity = new PlatePureComponentEntity(this);
		pose.translate(0.0f, 0.0f, -0.25f);
		m_groundEntity->setPose(pose);

		m_interlayerEntity = new PlatePureComponentEntity(this);
		pose.translate(0.0f, 0.0f, 0.05f);
		m_interlayerEntity->setPose(pose);
		
		
		QVector4D greyColor = QVector4D(0.309804f, 0.313725f, 0.325490f, 1.0f);
		m_printerGrid = new qtuser_3d::PrinterGrid(this);
		m_printerGrid->setLineColor(greyColor);
		pose.translate(0.0f, 0.0f, 0.05f);
		m_printerGrid->setPose(pose);
		
		m_upperEntity = new PlateTextureComponentEntity(this);
		pose.translate(0.0f, 0.0f, 0.05f);
		m_upperEntity->setPose(pose);

		createTopRightEntities();
		createHandleEntities();

		//createTopLeftLabel();
		//createTopRightLabel();

		auto f = [](IconTypes type) {

			qDebug() << "on click" << type;

			switch (type)
			{
			case IconType::Autolayout:
			{
				creative_kernel::layoutByType(0);
			}
			break;

			case IconType::PickBottom:
			{
				creative_kernel::setMaxFaceBottom();
			}
			break;

			default:
				break;
			}
		};
		setClickCallback(f);
	}

	PlateEntity::~PlateEntity()
	{
		untracePickables();
	}

	void PlateEntity::tracePickables()
	{
		//tracePickable(m_closeEntity->pickable());
		tracePickable(m_autoLayoutEntity->pickable());
		tracePickable(m_pickBottomEntity->pickable());
		//tracePickable(m_lockEntity->pickable());
		//tracePickable(m_settingEntity->pickable());

		//tracePickable(m_topLeftLabel->pickable());
	}
	void PlateEntity::untracePickables()
	{
		//unTracePickable(m_closeEntity->pickable());
		unTracePickable(m_autoLayoutEntity->pickable());
		unTracePickable(m_pickBottomEntity->pickable());
		//unTracePickable(m_lockEntity->pickable());
		//unTracePickable(m_settingEntity->pickable());

		//unTracePickable(m_topLeftLabel->pickable());
	}

	void PlateEntity::setGridColor(const QVector4D clr)
	{
		m_printerGrid->setLineColor(clr);
	}

	void PlateEntity::updateBounding(qtuser_3d::Box3D& box)
	{
		m_printerGrid->updateBounding(box, 1);
	}

	void PlateEntity::setHighlight(bool highlight)
	{
		m_highlight = highlight;

		QString url = highlight ? qrcPathOfName("thick_line.png") : qrcPathOfName("thin_line.png");
		Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(QUrl(url));
		m_upperEntity->setTexture(t);
	}

	void PlateEntity::setSize(const QSizeF size)
	{
		m_size = size;
		setupGroundGeometry(size);
		setupInterlayerGeometry(size);
		setupUpperGeometry(size);
		adjustTopRightEntities(size);
		adjustHandleEntities(size);

		//adjustTopLeftLabel(size);
		//adjustTopRightLabel(size);
	}

	void PlateEntity::setTheme(int theme)
	{
		if (m_theme == theme)
			return;
		m_theme = theme;

		setHighlight(m_highlight);

		/*
		Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("close_none.png")));
		m_closeEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("close_hover.png")));
		m_closeEntity->setTexture(t, qtuser_3d::ControlState::hover);

		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("unlock.png")));
		m_lockEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("lock.png")));
		m_lockEntity->setTexture(t, qtuser_3d::ControlState::hover);

		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("setting_none.png")));
		m_settingEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("setting_hover.png")));
		m_settingEntity->setTexture(t, qtuser_3d::ControlState::hover);
			*/

		Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("autolayout_none.png")));
		m_autoLayoutEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("autolayout_hover.png")));
		m_autoLayoutEntity->setTexture(t, qtuser_3d::ControlState::hover);

		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("pickbottom_none.png")));
		m_pickBottomEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("pickbottom_hover.png")));
		m_pickBottomEntity->setTexture(t, qtuser_3d::ControlState::hover);

		
		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("left_hand.png")));
		m_leftHandleEntity->setTexture(t);

		t = qtuser_3d::createFromSource(QUrl(qrcPathOfName("right_hand.png")));
		m_rightHandleEntity->setTexture(t);

		bool dark = (m_theme == 0);
		if (dark)
		{
			m_interlayerEntity->setColor(QVector4D(0.271, 0.271, 0.282, 1.0));
			m_groundEntity->setColor(QVector4D(0.230, 0.230, 0.242, 1.0));
		}
		else {
			m_interlayerEntity->setColor(QVector4D(0.8039, 0.8039, 0.8274, 1.0));
			m_groundEntity->setColor(QVector4D(0.749, 0.749, 0.7725, 1.0));
		}
	}

	void PlateEntity::setupGroundGeometry(const QSizeF size)
	{
		std::vector<trimesh::vec3> vertexs;

		float margin =  8.0f;
		float radius = margin;
		
		trimesh::vec3 origin = trimesh::vec3(-8.0f, -8.0f, 0.0f);
		
		{
			qtuser_3d::RoundedRectangle::RoundedRectangleCorners corners = qtuser_3d::RoundedRectangle::TopLeft + qtuser_3d::RoundedRectangle::BottomLeft | qtuser_3d::RoundedRectangle::BottomRight;
			qtuser_3d::RoundedRectangle::makeRoundedRectangle(origin, size.width() + 2.0 * margin, size.height() + 2.0 * margin, corners, radius, &vertexs, nullptr);
		}
		
		/*{
			qtuser_3d::RoundedRectangle::RoundedRectangleCorners partCorners = qtuser_3d::RoundedRectangle::TopLeft | qtuser_3d::RoundedRectangle::TopRight;
			trimesh::vec3 partOrigin = trimesh::vec3(origin.x, origin.y + size.height() + margin, origin.z);
			qtuser_3d::RoundedRectangle::makeRoundedRectangle(partOrigin, 33.0f, 25.0f, partCorners, radius, &vertexs, nullptr);
		}*/

		{
			qtuser_3d::RoundedRectangle::RoundedRectangleCorners partCorners = qtuser_3d::RoundedRectangle::TopRight | qtuser_3d::RoundedRectangle::BottomRight;
			trimesh::vec3 partOrigin = trimesh::vec3(origin.x + size.width() + margin, origin.y + size.height() + margin * 2.0 - 53.0f, origin.z);
			qtuser_3d::RoundedRectangle::makeRoundedRectangle(partOrigin, 25.0f, 53.0f, partCorners, radius, &vertexs, nullptr);
		}

		Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createFromVertex(vertexs.size(), &vertexs.front().front());

		m_groundEntity->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Triangles);
	}


	void PlateEntity::setupInterlayerGeometry(const QSizeF size)
	{
		std::vector<trimesh::vec3> vertexs;
		makeQuad(trimesh::vec3(0.0, 0.0, 0.0), trimesh::vec3(size.width(), size.height(), 0.0), &vertexs);
		Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createFromVertex(vertexs.size(), &vertexs.front().front());
		m_interlayerEntity->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Triangles);
	}

	void PlateEntity::setupUpperGeometry(const QSizeF size)
	{
		std::vector<trimesh::vec3> vertexs;
		std::vector<trimesh::vec2> uvs;
		makeQuad(trimesh::vec3(0.0, 0.0, 0.0f), trimesh::vec3(size.width(), size.height(), 0.0f), &vertexs);

		uvs.push_back(trimesh::vec2(0.0, 0.0));
		uvs.push_back(trimesh::vec2(1.0, 0.0));
		uvs.push_back(trimesh::vec2(1.0, 1.0));
		uvs.push_back(trimesh::vec2(0.0, 0.0));
		uvs.push_back(trimesh::vec2(1.0, 1.0));
		uvs.push_back(trimesh::vec2(0.0, 1.0));

		Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr);
		m_upperEntity->setGeometry(geometry);
	}


	void PlateEntity::createTopRightEntities()
	{
		std::vector<trimesh::vec3> vertexs;
		std::vector<trimesh::vec2> uvs;

		trimesh::vec3 origin(0.0);
		float radius = 8;
		makeCircle(origin, radius, &vertexs);
		makeTextureCoordinate(&vertexs,
			origin - trimesh::vec3(radius, radius, 0.0),
			trimesh::vec2(radius * 2.0), &uvs);
		Qt3DRender::QGeometry* shareGeometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, this);
		
		/*{
			m_closeEntity = new PlateIconComponentEntity(this);
			m_closeEntity->setObjectName("close");
			m_closeEntity->setGeometry(shareGeometry);
		}*/

		
		{
			m_autoLayoutEntity = new PlateIconComponentEntity(this);
			m_autoLayoutEntity->setObjectName("auto_layout");
			m_autoLayoutEntity->setGeometry(shareGeometry);
			m_autoLayoutEntity->setIconType(IconType::Autolayout);
			
		}

		{
			m_pickBottomEntity = new PlateIconComponentEntity(this);
			m_pickBottomEntity->setObjectName("pick_bottom");
			m_pickBottomEntity->setGeometry(shareGeometry);
			m_pickBottomEntity->setIconType(IconType::PickBottom);
			
		}


		/*{
			m_lockEntity = new PlateIconComponentEntity(this);
			m_lockEntity->setObjectName("lock");
			m_lockEntity->setGeometry(shareGeometry);
		}

		{
			m_settingEntity = new PlateIconComponentEntity(this);
			m_settingEntity->setObjectName("setting");
			m_settingEntity->setGeometry(shareGeometry);
		}*/

		//编号，未完待续


		tracePickables();
	}

	void PlateEntity::adjustTopRightEntities(const QSizeF size)
	{
		/*QMatrix4x4 m;
		m.translate(size.width()+4+8, size.height()-8, 0.0);
		m_closeEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_autoLayoutEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_pickBottomEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_lockEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_settingEntity->setPose(m);*/

		QMatrix4x4 m;
		m.translate(size.width() + 4 + 8, size.height() - 8, 0.0);
		m_autoLayoutEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_pickBottomEntity->setPose(m);

	}

	void PlateEntity::createHandleEntities()
	{
		std::vector<trimesh::vec3> vertexs;
		std::vector<trimesh::vec2> uvs;

		trimesh::vec3 origin(0.0);
		trimesh::vec3 dest(33.0, 20.0, 0.0);
		
		makeQuad(origin, dest, &vertexs);
		makeTextureCoordinate(&vertexs,
			origin,
			trimesh::vec2(33.0, 20.0), &uvs);
		Qt3DRender::QGeometry* shareGeometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, this);

		m_leftHandleEntity = new PlateTextureComponentEntity(this);
		m_leftHandleEntity->setGeometry(shareGeometry);
		
		m_rightHandleEntity = new PlateTextureComponentEntity(this);
		m_rightHandleEntity->setGeometry(shareGeometry);
	}

	void PlateEntity::adjustHandleEntities(const QSizeF size)
	{
		QMatrix4x4 matrix;
		matrix.translate(0.0, -18.0, -0.2);
		m_leftHandleEntity->setPose(matrix);

		matrix.setToIdentity();
		matrix.translate(size.width()-33.0, -18.0, -0.20);
		m_rightHandleEntity->setPose(matrix);
	}

	void PlateEntity::setName(const std::string& str)
	{
		//左上角
		QImage icon(":/kernel/printer/edit_none.png");
		QImage icon2(":/kernel/printer/edit_hover.png");
		int iwidth = icon.width();
		int iheight = icon.height();

		QFont font("Arial", 16);
		QFontMetrics metrics(font);
		QString text(str.c_str());
		int twidth = metrics.horizontalAdvance(text); // 计算文本的水平宽度
		int theight = metrics.ascent(); // 计算文本的垂直高度


		int width = iwidth + twidth + 10;
		int height = std::max(iheight, theight) + 2;

		QRect textRect = QRect(2, (height - theight) / 2, twidth, theight);
		QRect iconRect = QRect(2 + twidth + 7, (height - iheight) / 2, iwidth, iheight); // 根据widget大小计算居中对齐位置
		
		QPixmap panel = QPixmap(width, height);
		panel.fill(QColor(0, 0, 0, 0));

		QPainter painter(&panel);
		
		QPen pen(QColor(203, 203, 203, 255));
		painter.setFont(font);
		painter.setPen(pen);
		painter.drawText(textRect, Qt::AlignCenter, text);

		painter.drawImage(iconRect, icon);
		QImage noneImage = panel.toImage().mirrored();

		painter.drawImage(iconRect, icon2);
		QImage hoverImage = panel.toImage().mirrored();

		//hoverImage.save("output_hover.png");
		{
			float h = 16.0f;
			float w = width * h / height;

			std::vector<trimesh::vec3> vertexs;
			std::vector<trimesh::vec2> uvs;

			trimesh::vec3 origin(0.0);
			trimesh::vec3 dest(w, h, 0.0);

			makeQuad(origin, dest, &vertexs);
			makeTextureCoordinate(&vertexs,
				origin,
				trimesh::vec2(w, h), &uvs);
			Qt3DRender::QGeometry* shareGeometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, this);
			if (m_topLeftLabel)
			{
				m_topLeftLabel->setGeometry(shareGeometry);
				Qt3DRender::QTexture2D* t = qtuser_3d::createFromImage(noneImage);
				m_topLeftLabel->setTexture(t, qtuser_3d::ControlState::none);
				t = qtuser_3d::createFromImage(hoverImage);
				m_topLeftLabel->setTexture(t, qtuser_3d::ControlState::hover);

				adjustTopLeftLabel(m_size);
			}
		}
	}
	
	void PlateEntity::setModelNumber(const std::string& str)
	{
		//右上角
		QImage icon(":/kernel/printer/link.png");
		int iwidth = icon.width();
		int iheight = icon.height();

		QFont font("Arial", 16);
		QFontMetrics metrics(font);
		QString text(str.c_str());
		int twidth = metrics.horizontalAdvance(text); // 计算文本的水平宽度
		int theight = metrics.ascent(); // 计算文本的垂直高度
		printf("width = %d, height = %d\n", twidth, theight);

		int width = iwidth + twidth + 6;
		int height = std::max(iheight, theight) + 2;

		QPixmap panel = QPixmap(width, height);
		panel.fill(QColor(0, 0, 0, 0));

		QPainter painter(&panel);
		
		QRect iconRect = QRect(2, (height-iheight)/2, iwidth, iheight);
		QRect textRect = QRect(2+iwidth+4, (height-theight)/2, twidth, theight); // 根据widget大小计算居中对齐位置

		painter.drawImage(iconRect, icon);

		QPen pen(QColor(203, 203, 203, 255));
		painter.setFont(font);
		painter.setPen(pen); 
		painter.drawText(textRect, Qt::AlignCenter, text); 
		
		QImage result = panel.toImage().mirrored();
		//result.save("output.png");

		{
			float h = 9.0f;
			float w = width * h / height;

			std::vector<trimesh::vec3> vertexs;
			std::vector<trimesh::vec2> uvs;

			trimesh::vec3 origin(0.0);
			trimesh::vec3 dest(w, h, 0.0);

			makeQuad(origin, dest, &vertexs);
			makeTextureCoordinate(&vertexs,
				origin,
				trimesh::vec2(w, h), &uvs);
			Qt3DRender::QGeometry* shareGeometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, this);
			if (m_topRightLabel)
			{
				m_topRightLabelSize = QSizeF(w, h);
				m_topRightLabel->setGeometry(shareGeometry);
				Qt3DRender::QTexture2D* t = qtuser_3d::createFromImage(result);
				m_topRightLabel->setTexture(t);

				adjustTopRightLabel(m_size);
			}
		}
	}

	void PlateEntity::createTopLeftLabel()
	{
		m_topLeftLabel = new PlateIconComponentEntity(this);
		tracePickable(m_topLeftLabel->pickable());
	}

	void PlateEntity::adjustTopLeftLabel(const QSizeF size)
	{
		if (m_topLeftLabel && size.isValid())
		{
			QMatrix4x4 m;
			m.translate(0.0, size.height() + 5.0, 0.0f);
			m_topLeftLabel->setPose(m);
		}
	}

	void PlateEntity::createTopRightLabel()
	{
		m_topRightLabel = new PlateTextureComponentEntity(this);
	}
	
	void PlateEntity::adjustTopRightLabel(const QSizeF size)
	{
		if (m_topRightLabel && size.isValid())
		{
			QMatrix4x4 m;
			m.translate(size.width() - m_topRightLabelSize.width(), size.height() + 10.0f, 0.0f);
			m_topRightLabel->setPose(m);
		}
	}

	QString PlateEntity::qrcPathOfName(const std::string& name)
	{
		bool dark = (m_theme == 0);
		bool light = (m_theme == 1);
		if (dark)
		{
			return QString(("qrc:/kernel/printer/dark/"+name).c_str());
		}
		else if (light) {
			return QString(("qrc:/kernel/printer/light/"+name).c_str());
		}
		return QString();
	}

	void PlateEntity::setClickCallback(OnClickFunc call)
	{
		m_autoLayoutEntity->setClickCallback(call);
		m_pickBottomEntity->setClickCallback(call);

		//m_closeEntity->setClickCallback(call);
		//m_lockEntity->setClickCallback(call);
		//m_settingEntity->setClickCallback(call);
		//m_topLeftLabel->setClickCallback(call);
	}
}