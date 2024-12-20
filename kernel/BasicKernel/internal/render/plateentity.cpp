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
#include "interface/visualsceneinterface.h"

#include <QtGui/qpainter.h>
#include <QGuiApplication>
#include <QFont>
#include "interface/reuseableinterface.h"
#include "qtuser3d/utils/cachetexture.h"
#include "platepickable.h"

using namespace qtuser_3d;
namespace creative_kernel {

	PlateEntity::PlateEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_theme(-1)
		, m_topLeftLabel(nullptr)
		, m_topRightLabel(nullptr)
		, m_selected(false)
		, m_lock(false)
		, m_nameContainSize(QSizeF(60.0f, 60.0f))
		, m_mode(PlateViewMode::EDIT)
		, m_closeEnable(true)
		, m_style(PlateStyle::Custom_Plate)
		, m_order(-1)
	{
		setObjectName("PlateEntity");

		QMatrix4x4 pose;

		m_groundEntity = new PlatePureComponentEntity(this);
		m_groundEntity->setObjectName("ground");
		pose.translate(0.0f, 0.0f, -0.6f);
		m_groundEntity->setPose(pose);

		m_interlayerEntity = new PlateIconComponentEntity(this);
		m_interlayerEntity->setObjectName("interlayer");
		m_interlayerEntity->setIconType(IconType::Platform);
		pose.translate(0.0f, 0.0f, 0.2f);
		m_interlayerEntity->setPose(pose);


		m_printerGrid = new qtuser_3d::PrinterGrid(this);
		m_printerGrid->setObjectName("grid_line");
		pose.translate(0.0f, 0.0f, 0.1f);
		m_printerGrid->setPose(pose);

		m_upperEntity = new PlateTextureComponentEntity(this);
		m_upperEntity->setObjectName("upper");
		m_upperEntity->xeffect()->setPassNoDepthMask(0);
		m_upperEntity->xeffect()->removePassFilter(0, "view", 0);
		m_upperEntity->xeffect()->addPassFilter(0, "alpha2nd");
		pose.translate(0.0f, 0.0f, 0.1f);
		m_upperEntity->setPose(pose);

		createTopLeftLabel();
		createTopRightLabel();

		createTopRightEntities();
		createHandleEntities();
	}

	PlateEntity::~PlateEntity()
	{
		untracePickables();

		if (m_groundEntity) {delete m_groundEntity; m_groundEntity = nullptr;}
		if (m_interlayerEntity) {delete m_interlayerEntity; m_interlayerEntity = nullptr;}
		if (m_printerGrid) {delete m_printerGrid; m_printerGrid = nullptr;}
		if (m_upperEntity) { delete m_upperEntity; m_upperEntity = nullptr; }
		if (m_closeEntity) { delete m_closeEntity; m_closeEntity = nullptr; }
		if (m_autoLayoutEntity) { delete m_autoLayoutEntity; m_autoLayoutEntity = nullptr; }
		if (m_pickBottomEntity) { delete m_pickBottomEntity; m_pickBottomEntity = nullptr; }
		if (m_lockEntity) { delete m_lockEntity; m_lockEntity = nullptr; }
		if (m_settingEntity) { delete m_settingEntity; m_settingEntity = nullptr; }
		if (m_numberEntity) { delete m_numberEntity; m_numberEntity = nullptr; }
		if (m_leftHandleEntity) { delete m_leftHandleEntity; m_leftHandleEntity = nullptr; }
		if (m_rightHandleEntity) { delete m_rightHandleEntity; m_rightHandleEntity = nullptr; }
		if (m_topLeftLabel) { delete m_topLeftLabel; m_topLeftLabel = nullptr; }
		if (m_topRightLabel) { delete m_topRightLabel; m_topRightLabel = nullptr; }
	}

	void PlateEntity::setPrinter(Printer* printer)
	{
		unTracePickable(m_interlayerEntity->pickable());
		m_interlayerEntity->bindPickable(new PlatePickable(printer, m_interlayerEntity));
		tracePickable(m_interlayerEntity->pickable());
	}

	void PlateEntity::tracePickables()
	{
		tracePickable(m_closeEntity->pickable());
		tracePickable(m_autoLayoutEntity->pickable());
		tracePickable(m_pickBottomEntity->pickable());
		tracePickable(m_lockEntity->pickable());
		tracePickable(m_settingEntity->pickable());

		tracePickable(m_topLeftLabel->pickable());
		tracePickable(m_interlayerEntity->pickable());
	}

	void PlateEntity::untracePickables()
	{
		unTracePickable(m_closeEntity->pickable());
		unTracePickable(m_autoLayoutEntity->pickable());
		unTracePickable(m_pickBottomEntity->pickable());
		unTracePickable(m_lockEntity->pickable());
		unTracePickable(m_settingEntity->pickable());

		unTracePickable(m_topLeftLabel->pickable());
		unTracePickable(m_interlayerEntity->pickable());
	}

	/*void PlateEntity::setGridColor(const QVector4D clr)
	{
		m_printerGrid->setLineColor(clr);
	}*/

	void PlateEntity::updateBounding(qtuser_3d::Box3D& box)
	{
		QVector3D min = box.min;
		m_origin = trimesh::vec3(min.x(), min.y(), min.z());
		setSize(QSizeF(box.size().x(), box.size().y()));
		m_printerGrid->updateBounding(box, 1);
	}

	void PlateEntity::setSize(const QSizeF size)
	{
		m_size = size;
		setupGroundGeometry(size);
		setupInterlayerGeometry(size);
		setupUpperGeometry(size);
		adjustTopRightEntities(size);
		adjustHandleEntities(size);

		adjustTopLeftLabel(size);
		adjustTopRightLabel(size);
	}

	int PlateEntity::theme()
	{
		return m_theme;
	}

	void PlateEntity::setTheme(int theme)
	{
		if (m_theme == theme)
			return;
		m_theme = theme;

		Qt3DRender::QTexture2D* t = getTexture(qrcPathOfName("left_hand.png"));
		m_leftHandleEntity->setTexture(t);

		t = getTexture(qrcPathOfName("right_hand.png"));
		m_rightHandleEntity->setTexture(t);


		setSelected(m_selected, true);

	}

	void PlateEntity::setupGroundGeometry(const QSizeF size)
	{
		std::vector<trimesh::vec3> vertexs;

		float margin =  8.0f;
		float radius = margin;

		trimesh::vec3 origin = m_origin + trimesh::vec3(-8.0f, -8.0f, 0.0f);

		if (m_mode == PlateViewMode::EDIT)
		{
			{
				qtuser_3d::RoundedRectangle::RoundedRectangleCorners corners = qtuser_3d::RoundedRectangle::BottomLeft | qtuser_3d::RoundedRectangle::BottomRight;
				qtuser_3d::RoundedRectangle::makeRoundedRectangle(origin, size.width() + 2.0 * margin, size.height() + 2.0 * margin, corners, radius, &vertexs, nullptr);
			}

			//平台左上角
			{
				float radio = m_nameContainSize.width() / m_nameContainSize.height();
				float containH = 16.0f;
				float containW = containH * radio;
				float vMargin = 5.0f;

				qtuser_3d::RoundedRectangle::RoundedRectangleCorners partCorners = qtuser_3d::RoundedRectangle::TopLeft | qtuser_3d::RoundedRectangle::TopRight;
				trimesh::vec3 partOrigin = trimesh::vec3(origin.x, origin.y + size.height() + margin, origin.z);
				qtuser_3d::RoundedRectangle::makeRoundedRectangle(partOrigin, containW + margin * 2.0, containH + vMargin * 2.0, partCorners, radius, &vertexs, nullptr);
			}

			//平台右上角
			{
				qtuser_3d::RoundedRectangle::RoundedRectangleCorners partCorners = qtuser_3d::RoundedRectangle::TopRight | qtuser_3d::RoundedRectangle::BottomRight;
				trimesh::vec3 partOrigin = trimesh::vec3(origin.x + size.width() + margin, origin.y + size.height() + margin * 2.0 - 110.0f, origin.z);
				qtuser_3d::RoundedRectangle::makeRoundedRectangle(partOrigin, 25.0f, 110.0f, partCorners, radius, &vertexs, nullptr);
			}
		}
		else if (m_mode == PlateViewMode::PREVIEW)
		{
			if (exitTopLeftLabel())
			{
				qtuser_3d::RoundedRectangle::RoundedRectangleCorners corners = \
					qtuser_3d::RoundedRectangle::BottomLeft | \
					qtuser_3d::RoundedRectangle::BottomRight | \
					qtuser_3d::RoundedRectangle::TopRight;
				qtuser_3d::RoundedRectangle::makeRoundedRectangle(origin, size.width() + 2.0 * margin, size.height() + 2.0 * margin, corners, radius, &vertexs, nullptr);

				//平台左上角
				{
					float radio = m_nameContainSize.width() / m_nameContainSize.height();
					float containH = 16.0f;
					float containW = containH * radio;
					float vMargin = 5.0f;

					qtuser_3d::RoundedRectangle::RoundedRectangleCorners partCorners = qtuser_3d::RoundedRectangle::TopLeft | qtuser_3d::RoundedRectangle::TopRight;
					trimesh::vec3 partOrigin = trimesh::vec3(origin.x, origin.y + size.height() + margin, origin.z);
					qtuser_3d::RoundedRectangle::makeRoundedRectangle(partOrigin, containW + margin * 2.0, containH + vMargin * 2.0, partCorners, radius, &vertexs, nullptr);
				}
			}
			else {

				qtuser_3d::RoundedRectangle::RoundedRectangleCorners corners = \
					qtuser_3d::RoundedRectangle::BottomLeft | \
					qtuser_3d::RoundedRectangle::BottomRight | \
					qtuser_3d::RoundedRectangle::TopLeft | \
					qtuser_3d::RoundedRectangle::TopRight;
				qtuser_3d::RoundedRectangle::makeRoundedRectangle(origin, size.width() + 2.0 * margin, size.height() + 2.0 * margin, corners, radius, &vertexs, nullptr);
			}

		}

		Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createFromVertex(vertexs.size(), &vertexs.front().front());

		requestRender(this);
		m_groundEntity->setEnabled(false);
		m_groundEntity->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Triangles);
		m_groundEntity->setEnabled(true);
		endRequestRender(this);

	}


	void PlateEntity::resetGroundGeometry()
	{
		setupGroundGeometry(m_size);
	}

	void PlateEntity::setupInterlayerGeometry(const QSizeF size)
	{
		std::vector<trimesh::vec3> vertexs;
		trimesh::vec3 origin = m_origin;
		makeQuad(origin, trimesh::vec3(size.width(), size.height(), 0.0) + origin, &vertexs);
		Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createFromVertex(vertexs.size(), &vertexs.front().front());
		m_interlayerEntity->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Triangles);
	}

	void PlateEntity::setupUpperGeometry(const QSizeF size)
	{
		std::vector<trimesh::vec3> vertexs;
		std::vector<trimesh::vec2> uvs;
		trimesh::vec3 origin = m_origin;
		makeQuad(origin, trimesh::vec3(size.width(), size.height(), 0.0f) + origin, &vertexs);

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

		{
			m_closeEntity = new PlateIconComponentEntity(this);
			m_closeEntity->setObjectName("close");
			m_closeEntity->setGeometry(shareGeometry);
			m_closeEntity->setIconType(IconType::Close);
			m_closeEntity->setTips("Close");
		}


		{
			m_autoLayoutEntity = new PlateIconComponentEntity(this);
			m_autoLayoutEntity->setObjectName("auto_layout");
			m_autoLayoutEntity->setGeometry(shareGeometry);
			m_autoLayoutEntity->setIconType(IconType::Autolayout);
			m_autoLayoutEntity->setTips("Auto Layout");
		}

		{
			m_pickBottomEntity = new PlateIconComponentEntity(this);
			m_pickBottomEntity->setObjectName("pick_bottom");
			m_pickBottomEntity->setGeometry(shareGeometry);
			m_pickBottomEntity->setIconType(IconType::PickBottom);
			m_pickBottomEntity->setTips("Pick Bottom");
		}

		{
			m_lockEntity = new PlateIconComponentEntity(this);
			m_lockEntity->setObjectName("lock");
			m_lockEntity->setGeometry(shareGeometry);
			m_lockEntity->setIconType(IconType::Lock);
			m_lockEntity->setTips("Lock");
		}

		{
			m_settingEntity = new PlateIconComponentEntity(this);
			m_settingEntity->setObjectName("setting");
			m_settingEntity->setGeometry(shareGeometry);
			m_settingEntity->setIconType(IconType::Setting);
			m_settingEntity->setTips("Settings");
		}

		tracePickables();
	}

	void PlateEntity::adjustTopRightEntities(const QSizeF size)
	{
		QMatrix4x4 m;
		m.translate(m_origin.x + size.width() + 4 + 8,
			m_origin.y + size.height() - 8,
			m_origin.z);
		m_closeEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_autoLayoutEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_pickBottomEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_lockEntity->setPose(m);

		m.translate(0.0, -20, 0.0);
		m_settingEntity->setPose(m);
	}

	void PlateEntity::createHandleEntities()
	{
		std::vector<trimesh::vec3> vertexs;
		std::vector<trimesh::vec2> uvs;

		trimesh::vec3 origin(0.0);
		trimesh::vec3 dest(33.0, 16.5, 0.0);

		makeQuad(origin, dest, &vertexs);
		makeTextureCoordinate(&vertexs,
			origin,
			trimesh::vec2(33.0, 16.5), &uvs);
		Qt3DRender::QGeometry* shareGeometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, this);

		m_leftHandleEntity = new PlateTextureComponentEntity(this);
		m_leftHandleEntity->setObjectName("left_handler");
		m_leftHandleEntity->xeffect()->removePassFilter(0, "view", 0);
		m_leftHandleEntity->xeffect()->addPassFilter(0, "alpha");
		m_leftHandleEntity->setGeometry(shareGeometry);

		m_rightHandleEntity = new PlateTextureComponentEntity(this);
		m_rightHandleEntity->setObjectName("right_handler");
		m_rightHandleEntity->xeffect()->removePassFilter(0, "view", 0);
		m_rightHandleEntity->xeffect()->addPassFilter(0, "alpha");
		m_rightHandleEntity->setGeometry(shareGeometry);
	}

	void PlateEntity::adjustHandleEntities(const QSizeF size)
	{

		QVector3D platOriginOffset = QVector3D(m_origin.x, m_origin.y, m_origin.z);
		{
			QVector3D tr(0.0, -15.5, -0.5f);
			tr += platOriginOffset;
			QMatrix4x4 matrix;
			matrix.translate(tr);
			m_leftHandleEntity->setPose(matrix);
		}

		{
			QVector3D tr(size.width() - 33.0, -15.5, -0.5f);
			tr += platOriginOffset;
			QMatrix4x4 matrix;
			matrix.translate(tr);
			m_rightHandleEntity->setPose(matrix);
		}
	}

	const QString& PlateEntity::name()
	{
		return m_name;
	}

	//affected by selected, view mode
	void PlateEntity::setName(const QString& text)
	{
		m_name = text;
		setupNameTextureAndGeometry();
	}

	bool PlateEntity::exitTopLeftLabel()
	{
		bool nameExit = !m_name.isEmpty();
		bool iconExit = (m_mode == PlateViewMode::EDIT);
		return nameExit || iconExit;
	}

	void PlateEntity::setupNameTextureAndGeometry()
	{
		QString drawText = m_name;
		QString text = m_name;

		bool nameExit = !m_name.isEmpty();
		bool iconExit = (m_mode == PlateViewMode::EDIT);

		if (!nameExit && !iconExit)
		{
			m_nameContainSize = QSizeF(0, 0);
			return;
		}

		QFont font = useFont();

		float maxTextGeometryWidth = 160.0f;
		float maxOfTextOrIconGeometryHeight = 16.0f;
		//���Ͻ�
		QString edit_none_path = qImagePathOfName(pngNameOfIcon("edit", m_selected, false));
		QImage icon(edit_none_path);
		QString edit_hover_path = qImagePathOfName(pngNameOfIcon("edit", m_selected, true));
		QImage icon2(edit_hover_path);

		int iwidth = icon.width();
		int iheight = icon.height();

		float scale = iheight / maxOfTextOrIconGeometryHeight;
		float maxTextWidth = maxTextGeometryWidth * scale;

		int twidth = 0;
		int theight = 0;
		if (nameExit)
		{
			font.setPointSize(28);
			font.setBold(true);
			// font.setWeight(QFont::Weight::ExtraBold);
#ifdef _WINDOWS
			font.setFamily("Microsoft Yahei");
			font.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
#endif

			QFontMetrics metrics(font);
			int tmpheight = metrics.ascent(); // 文字高度
			int tmpwidth = metrics.horizontalAdvance(drawText); // �����ı���ˮƽ����
			int i = 1;
			while (tmpwidth > maxTextWidth)
			{
				QString testing = text;
				testing.truncate(text.length() - i);
				testing.append("...");
				tmpwidth = metrics.horizontalAdvance(testing);
				drawText = testing;
				i++;
			}
			twidth = tmpwidth;
			theight = tmpheight;
		}

		int width = 0;
		int height = 0;

		if (nameExit)
		{
			width = twidth;
			height = std::max(iheight, theight);
		}

		if (iconExit)
		{
			width += iwidth;
			height = std::max(iheight, theight);
		}

		if (nameExit && iconExit)
		{
			width += 20.0f;
		}

		/////////////////////////////////////////////
		QRect textRect;
		QRect iconRect;

		if (nameExit)
		{
			textRect = QRect(0, (height - theight) / 2, twidth, theight);
		}
		if (iconExit)
		{
			iconRect = QRect(0, 0, iwidth, iheight);
		}
		if (nameExit && iconExit)
		{
			iconRect = QRect(textRect.right() + 20, (height - iheight) / 2, iwidth, iheight);
		}

		QPixmap panel = QPixmap(width, height);
		panel.fill(QColor(0, 0, 0, 0));
		QPainter painter(&panel);

		if (nameExit)
		{
			QPen pen(m_theme == 0 ? QColor(203, 203, 203, 255) : QColor(0x7D7D82));
			painter.setFont(font);
			painter.setPen(pen);
			painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, drawText);
		}

		if (iconExit)
		{
			painter.drawImage(iconRect, icon);
		}
		QImage noneImage = panel.toImage().mirrored();
		if (iconExit)
		{
			painter.drawImage(iconRect, icon2);
		}
		QImage hoverImage = panel.toImage().mirrored();

		//hoverImage.save("output_hover.png");
		{
			float h = maxOfTextOrIconGeometryHeight;
			float w = width * h / height;
			m_topLeftLabel->setTipsOffset(QVector3D(w + 5, 20, 0));

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
				t->setParent(m_topLeftLabel);
				m_topLeftLabel->setTexture(t, qtuser_3d::ControlState::none);

				t = qtuser_3d::createFromImage(hoverImage);
				t->setParent(m_topLeftLabel);
				m_topLeftLabel->setTexture(t, qtuser_3d::ControlState::hover);

				adjustTopLeftLabel(m_size);
			}
		}

		m_nameContainSize = QSizeF(width, height);
	}

	void PlateEntity::setModelNumber(const QString& str)
	{
		m_modelNumber = str;
		//���Ͻ�
		QString link = qImagePathOfName("link.png");
		QImage icon(link);
		int iwidth = icon.width();
		int iheight = icon.height();

		QFont font = useFont();
		font.setBold(true);
		font.setPointSizeF(16.0);

		QFontMetrics metrics(font);
		QString text(str);
		int twidth = metrics.horizontalAdvance(text); // �����ı���ˮƽ����
		int theight = metrics.ascent(); // �����ı��Ĵ�ֱ�߶�
		printf("width = %d, height = %d\n", twidth, theight);

		int width = iwidth + twidth + 6;
		int height = std::max(iheight, theight) + 2;

		QPixmap panel = QPixmap(width, height);
		panel.fill(QColor(0, 0, 0, 0));

		QPainter painter(&panel);

		QRect iconRect = QRect(2, (height-iheight)/2, iwidth, iheight);
		QRect textRect = QRect(2+iwidth+4, (height-theight)/2, twidth, theight); // ����widget��С������ж���λ��

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

			trimesh::vec3 origin = m_origin;
			trimesh::vec3 dest(w, h, 0.0);

			makeQuad(origin, dest, &vertexs);
			makeTextureCoordinate(&vertexs,
				origin,
				trimesh::vec2(w, h), &uvs);
			Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, nullptr);
			if (m_topRightLabel)
			{
				m_topRightLabelSize = QSizeF(w, h);
				m_topRightLabel->setGeometry(geometry);
				m_topRightLabel->setTextureFromImage(result, true);

				adjustTopRightLabel(m_size);
			}
		}
	}

	void PlateEntity::createTopLeftLabel()
	{
		m_topLeftLabel = new PlateIconComponentEntity(this);
		m_topLeftLabel->setObjectName("edit");
		m_topLeftLabel->xeffect()->setPassDepthTest(0, Qt3DRender::QDepthTest::Less);
		m_topLeftLabel->xeffect()->removePassFilter(0, "view", 0);
		m_topLeftLabel->xeffect()->addPassFilter(0, "alpha");

		m_topLeftLabel->setIconType(IconType::EditName);
		m_topLeftLabel->setTips("Edit Name");
		m_topLeftLabel->setTipsOffset(QVector3D(20, 20, 0));
	}

	void PlateEntity::adjustTopLeftLabel(const QSizeF size)
	{
		if (m_topLeftLabel && size.isValid())
		{
			QMatrix4x4 m;
			m.translate(m_origin.x, m_origin.y + size.height() + 5.0, m_origin.z);
			m_topLeftLabel->setPose(m);
		}
	}

	void PlateEntity::createTopRightLabel()
	{
		//m_topRightLabel = new PlateTextureComponentEntity(this);

		m_numberEntity = new PlateTextureComponentEntity(this);
		m_numberEntity->setObjectName("number");
		m_numberEntity->xeffect()->setPassNoDepthMask(0);
		m_numberEntity->xeffect()->removePassFilter(0, "view", 0);
		m_numberEntity->xeffect()->addPassFilter(0, "alpha");

	}

	void PlateEntity::adjustTopRightLabel(const QSizeF size)
	{
		if (m_topRightLabel && size.isValid())
		{
			QMatrix4x4 m;
			m.translate(m_origin.x + size.width() - m_topRightLabelSize.width(),
				m_origin.y + size.height() + 10.0f,
				m_origin.z);
			m_topRightLabel->setPose(m);
		}

		QMatrix4x4 m;
		m.translate(m_origin.x + size.width() - 16.0,
			m_origin.y + size.height() + 10.0f,
			m_origin.z);
		m_numberEntity->setPose(m);
	}

	std::string PlateEntity::pngNameOfIcon(const std::string& icon, bool select, bool hover)
	{
		std::string selStr = select ? "select_" : "unselect_";
		std::string hovStr = hover ? "hover_" : "none_";
		std::string name = selStr + hovStr + icon + ".png";
		return name;
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


	QString PlateEntity::qImagePathOfName(const std::string& name)
	{
		bool dark = (m_theme == 0);
		bool light = (m_theme == 1);
		if (dark)
		{
			return QString((":/kernel/printer/dark/" + name).c_str());
		}
		else if (light) {
			return QString((":/kernel/printer/light/" + name).c_str());
		}
		return QString();
	}

	void PlateEntity::setClickCallback(OnClickFunc call)
	{
		m_callback = call;
		m_autoLayoutEntity->setClickCallback(call);
		m_pickBottomEntity->setClickCallback(call);

		m_closeEntity->setClickCallback(call);
		m_lockEntity->setClickCallback(call);
		m_settingEntity->setClickCallback(call);
		m_topLeftLabel->setClickCallback(call);
		m_interlayerEntity->setClickCallback(call);
	}

	void PlateEntity::setOrder(int order)
	{
		if (order == m_order)
			return;

		m_order = order;

		int bits = bitsOfOrderToShow(order);

		QList<QImage> numberIconList;
		for (size_t i = 0; i < bits; i++)
		{
			int unit = (order) % 10;
			order /= 10;
			std::string unitRes = ":/kernel/printer/number/number_" + std::to_string(unit) + ".png";
			QImage uniticon(unitRes.c_str());
			numberIconList.push_front(uniticon);
		}

		int width = 0;
		int height = 0;
		{
			for (size_t i = 0; i < numberIconList.size(); i++)
			{
				const QImage& icon = numberIconList.at(i);
				width += icon.width() + (i > 0 ? 10 : 0);
				height = std::max(height, icon.height());
			}
			height += 20;
		}

		QList<QRect> rectList;
		{
			int xOffset = 5;
			for (size_t i = 0; i < numberIconList.size(); i++)
			{
				const QImage& icon = numberIconList.at(i);
				QSize iconSize = icon.size();
				rectList.push_back(QRect(xOffset,
									(height - iconSize.height()) / 2.0,
									iconSize.width(),
									iconSize.height()));
				xOffset += iconSize.width();
			}
		}

		QPixmap panel = QPixmap(width, height);
		panel.fill(QColor(0, 0, 0, 0));

		QPainter painter(&panel);

		for (size_t i = 0; i < numberIconList.size(); i++)
		{
			const QImage& icon = numberIconList.at(i);
			const QRect& rect = rectList.at(i);
			painter.drawImage(rect, icon);
		}

		QImage image = panel.toImage().mirrored();

		m_numberEntity->setTextureFromImage(image, true);

		//set number Entity geometry
		{
			std::vector<trimesh::vec3> vertexs;
			std::vector<trimesh::vec2> uvs;

			trimesh::vec3 origin(0.0);
			float width = 8.0 * bits;
			trimesh::vec3 dest(width, 16.0, 0.0);

			makeQuad(origin, dest, &vertexs);
			makeTextureCoordinate(&vertexs,
				origin,
				trimesh::vec2(dest.x, dest.y), &uvs);
			Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 0, nullptr, nullptr);
			m_numberEntity->setGeometry(geometry);
		}
	}

	int PlateEntity::order()
	{
		return m_order;
	}

	int PlateEntity::bitsOfOrderToShow(int order)
	{
		int n = 1;
		while (order / 10 > 0)
		{
			order /= 10;
			n++;
		}

		n = std::max(2, n);
		return n;
	}

	bool PlateEntity::selected()
	{
		return m_selected;
	}

	void PlateEntity::setSelected(bool selected)
	{
		if (m_selected != selected)
		{
			m_selected = selected;
			setSelected(selected, true);
		}
	}

	void PlateEntity::setSelected(bool selected, bool force)
	{
		m_selected = selected;

		setLock(m_lock);
		setName(m_name);
		setPlateStyle(m_style);

		if (m_mode == PlateViewMode::EDIT)
		{
			m_leftHandleEntity->setEnabled(selected);
			m_rightHandleEntity->setEnabled(selected);
		}
		else if (m_mode == PlateViewMode::PREVIEW)
		{
			m_leftHandleEntity->setEnabled(false);
			m_rightHandleEntity->setEnabled(false);
		}

		bool dark = (m_theme == 0);
		if (dark)
		{
			if (m_selected)
			{
				QImage img(1, 1, QImage::Format_RGBA8888);
				img.setPixelColor(0, 0, QColor(0x636367));
				m_interlayerEntity->PlateTextureComponentEntity::setTextureFromImage(img, true);

				m_printerGrid->setLineColor(QVector4D(0.494f, 0.494f, 0.5175f, 1.0f));
				m_groundEntity->setColor(QVector4D(0.2901f, 0.2901f, 0.3098, 1.0f));

			}
			else {

				QImage img(1, 1, QImage::Format_RGBA8888);
				img.setPixelColor(0, 0, QColor(0x3e3e40));
				m_interlayerEntity->PlateTextureComponentEntity::setTextureFromImage(img, true);

				m_printerGrid->setLineColor(QVector4D(0.2627f, 0.2627f, 0.2745f, 1.0f));
				m_groundEntity->setColor(QVector4D(0.2235f, 0.2235f, 0.2314f, 1.0f));

			}
		}
		else {

			if (m_selected)
			{
				QImage img(1, 1, QImage::Format_RGBA8888);
				img.setPixelColor(0, 0, QColor(0x77777D));
				m_interlayerEntity->PlateTextureComponentEntity::setTextureFromImage(img, true);

				m_printerGrid->setLineColor(QVector4D(0.8039f, 0.8039f, 0.8274f, 1.0f));
				m_groundEntity->setColor(QVector4D(0.607f, 0.607f, 0.6313f, 1.0f));
			}
			else {

				QImage img(1, 1, QImage::Format_RGBA8888);
				img.setPixelColor(0, 0, QColor(0xEAEAEE));
				m_interlayerEntity->PlateTextureComponentEntity::setTextureFromImage(img, true);

				m_printerGrid->setLineColor(QVector4D(0.8588f, 0.8588, 0.8941f, 1.0f));
				m_groundEntity->setColor(QVector4D(0.8823f, 0.8823f, 0.9019f, 1.0f));
			}
		}

		bool selState = m_selected;

		Qt3DRender::QTexture2D* t = getTexture(qrcPathOfName(pngNameOfIcon("close", selState, false)));
		m_closeEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = getTexture(qrcPathOfName(pngNameOfIcon("close", selState, true)));
		m_closeEntity->setTexture(t, qtuser_3d::ControlState::hover);

		t = getTexture(qrcPathOfName(pngNameOfIcon("setting", selState, false)));
		m_settingEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = getTexture(qrcPathOfName(pngNameOfIcon("setting", selState, true)));
		m_settingEntity->setTexture(t, qtuser_3d::ControlState::hover);

		t = getTexture(qrcPathOfName(pngNameOfIcon("autolayout", selState, false)));
		m_autoLayoutEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = getTexture(qrcPathOfName(pngNameOfIcon("autolayout", selState, true)));
		m_autoLayoutEntity->setTexture(t, qtuser_3d::ControlState::hover);

		t = getTexture(qrcPathOfName(pngNameOfIcon("pickbottom", selState, false)));
		m_pickBottomEntity->setTexture(t, qtuser_3d::ControlState::none);
		t = getTexture(qrcPathOfName(pngNameOfIcon("pickbottom", selState, true)));
		m_pickBottomEntity->setTexture(t, qtuser_3d::ControlState::hover);
	}

	bool PlateEntity::lock()
	{
		return m_lock;
	}

	//affected by selected
	void PlateEntity::setLock(bool lock)
	{
		m_lock = lock;

		if (m_lock)
		{
			Qt3DRender::QTexture2D* t = getTexture(qrcPathOfName(pngNameOfIcon("lock", m_selected, false)));
			m_lockEntity->setTexture(t, qtuser_3d::ControlState::none);
			t = getTexture(qrcPathOfName(pngNameOfIcon("lock", m_selected, true)));
			m_lockEntity->setTexture(t, qtuser_3d::ControlState::hover);
		}
		else {
			Qt3DRender::QTexture2D* t = getTexture(qrcPathOfName(pngNameOfIcon("unlock", m_selected, false)));
			m_lockEntity->setTexture(t, qtuser_3d::ControlState::none);
			t = getTexture(qrcPathOfName(pngNameOfIcon("unlock", m_selected, true)));
			m_lockEntity->setTexture(t, qtuser_3d::ControlState::hover);
		}
	}

	QFont PlateEntity::useFont()
	{
		return qApp->font();
	}

	PlateViewMode PlateEntity::viewModel()
	{
		return m_mode;
	}

	void PlateEntity::setViewMode(PlateViewMode mode)
	{
		if (m_mode != mode)
		{
			m_mode = mode;

			bool showIcon = (mode == PlateViewMode::EDIT);

			m_closeEntity->setEnabled(showIcon);
			m_autoLayoutEntity->setEnabled(showIcon);
			m_pickBottomEntity->setEnabled(showIcon);
			m_lockEntity->setEnabled(showIcon);
			m_settingEntity->setEnabled(showIcon);

			if (m_mode == PlateViewMode::EDIT)
			{
				m_leftHandleEntity->setEnabled(m_selected);
				m_rightHandleEntity->setEnabled(m_selected);
			}
			else if (m_mode == PlateViewMode::PREVIEW)
			{
				m_leftHandleEntity->setEnabled(false);
				m_rightHandleEntity->setEnabled(false);
			}


			if (m_topRightLabel)
			{
				m_topRightLabel->setEnabled(showIcon);
			}

			if (exitTopLeftLabel())
			{
				setupNameTextureAndGeometry();
				m_topLeftLabel->setEnabled(true);
			}
			else {
				m_topLeftLabel->setEnabled(false);
			}


			resetGroundGeometry();
		}
	}

	void PlateEntity::setCloseEntityEnable(bool enable)
	{
		if (m_closeEnable != enable)
		{
			m_closeEnable = enable;
			m_closeEntity->enablePick(enable);

			std::string closePath;
			if (enable)
			{
				closePath = pngNameOfIcon("close", m_selected, false);
			}
			else {

				closePath = "disable_close.png";
			}

			Qt3DRender::QTexture2D* t = getTexture(qrcPathOfName(closePath));
			m_closeEntity->setTexture(t, qtuser_3d::ControlState::none);
		}
	}

	void PlateEntity::setPlateStyle(PlateStyle style)
	{
		m_style = style;

		QString url = qrcPathOfName("none.png");

		if (m_selected)
		{
			switch (style)
			{
			case PlateStyle::Custom_Plate:
			{
				url = qrcPathOfName("select_custom.png");
			}
			break;
			case PlateStyle::Textured_PEI_Plate:
			{
				url = qrcPathOfName("select_texture.png");
			}
			break;
			case PlateStyle::Smooth_PEI_Plate:
			{
				url = qrcPathOfName("select_smooth.png");
			}
			break;
			case PlateStyle::Epoxy_Resin_Plate:
			{
				url = qrcPathOfName("select_epoxy.png");
			}
			break;
			default:
				break;
			}
		}

		Qt3DRender::QTexture2D* t = getTexture(url);
		m_upperEntity->setTexture(t, false);
	}

	Qt3DRender::QTexture2D* PlateEntity::getTexture(const QString& key)
	{
		return getCacheTexture()->getTexture(key);
	}
}
