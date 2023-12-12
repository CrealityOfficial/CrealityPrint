#include"lasergcodework.h"
#include"drawobjectmodel.h"
#include"drawobject.h"
#include"lasersettings.h"
#include"plottersettings.h"
#include"c2dlib.h"
#include <math.h>
#include <QQmlProperty>
#include <QPainterPath>
#include "kernel/kernelui.h"
#include<string>
#if 0
#include "api.h"
#include"conf.h"
#include"../../buildinfo.h"
#endif
#include <QPainter>
using namespace creative_kernel;

extern "C"
{
	#include "include/gen.h"
	#include "./parser.h"
	#include "include/type.h"
}


#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#elif __GNUC__
#include <dirent.h>
#endif

#include "qtusercore/string/resourcesfinder.h"
#include "interface/machineinterface.h"

void pathScalse(QPainterPath& path, double ratioX, double ratioY);
void pathRotation(QPainterPath& path, double angle, int imgCenterX, int imgCenterY);
QPainterPath getPathsFromImage(QString imagePath, int value);
void checkIdxRecord(std::vector<int>& index_record);
int getIdx(std::vector<int>& index_record, int idxNo);
void runOverPointMark(QImage& srcImg, int density, float& extra_l, float& extra_r);

LaserGcodeWorker::LaserGcodeWorker(QObject* parent)
:Job(parent)
{
	
};
void LaserGcodeWorker::successed(qtuser_core::Progressor* progressor)
{
	emit genGcodeSuccess(m_gcodeBuf);

}


void LaserGcodeWorker::work(qtuser_core::Progressor* progressor)
{
	QString dir = qtuser_core::getOrCreateAppDataLocation("");
	QString tmpImg = dir + "/tmpImage.bmp";
	QString tmpGcode = dir + "/tmpGcode.gcode";
	if (QFile::exists(tmpImg))
	{
		QFile::remove(tmpImg);
	}
	if (QFile::exists(tmpGcode))
	{
		QFile::remove(tmpGcode);
	}

	//tparams params;

	int x = 0, y = 0, width = 0, height = 0;

	int tx = 0, ty = 0;
	int tShapeW = 0, tShapeH = 0;
	int tRotate = 0;

	QString Machine = currentMachineName();
	int nObjCount = m_pDrawObjectModel->rowCount();
	DrawObject* obj = nullptr;
	if (nObjCount == 0) return;
	float v2_offsetX = 60.0;
	int density = 0;
	std::vector<int> index_record;
	for (int i = 0; i < nObjCount; i++)//traversing Objects
	{
		obj = m_pDrawObjectModel->getData(i);
		QString dragType = QQmlProperty::read(obj->qmlObject(), "dragType").toString();
		int z = QQmlProperty::read(obj->qmlObject(), "z").toInt();
		index_record.push_back(z - 1);
		if("image" == dragType)
			density = QQmlProperty::read(obj->qmlObject(), "densityValue").toInt();
	}
	checkIdxRecord(index_record);
	if (density == 0 || density > 9) density = 9;
	double ratio = density / 3.0;
	QSharedPointer<us::USettings> globalsetting(createCurrentGlobalSettings());
	int machine_width = globalsetting->settings().value("machine_width")->toInt() * 3;
	int machine_height = globalsetting->settings().value("machine_depth")->toInt() * 3;

	int platformW = machine_width * ratio;
	int platformH = machine_height * ratio;

	QImage bkg(platformW, platformH, QImage::Format_ARGB32);
	for (int i = 0; i < platformW; i++) {
		for (int j = 0; j < platformH; j++) {
			bkg.setPixel(QPoint(i, j), qRgba(255, 255, 255, 255));//127, 156, 30
		}
	}
	//绘图开始
	QPainter p;
	p.begin(&bkg);
	p.setRenderHint(QPainter::Antialiasing, true);

	bool bNotImageExit = false;
	QPainterPath path_all;
	for (int i = 0; i < nObjCount; i++)//traversing Objects
	{	
		int index = getIdx(index_record, i);
		if (index < 0 || index >= nObjCount) continue;
		obj = m_pDrawObjectModel->getData(index);
		QString dragType = QQmlProperty::read(obj->qmlObject(), "dragType").toString();

		//QPointF pos, scope;
		x = QQmlProperty::read(obj->qmlObject(), "x").toInt();
		tx = ratio * (x - m_origin.x());
		y = QQmlProperty::read(obj->qmlObject(), "y").toInt();
		ty = ratio * ((y - m_origin.y()) + machine_height);
		width = QQmlProperty::read(obj->qmlObject(), "width").toInt(); 
		tShapeW = ratio * width;
		height = QQmlProperty::read(obj->qmlObject(), "height").toInt();
		tShapeH = ratio * height;
		tRotate = QQmlProperty::read(obj->qmlObject(), "rotation").toInt();

		if ("laser" == m_genType)/////����
		{
			if (Machine == "Ender-3 V2 Laser")
			{
				tx += v2_offsetX * density;
			}
			if ("image" == dragType)  ///ͼ���ദ��
			{
				QString imgType = obj->type();
				QImage srcImg = obj->dstImageData();

				double scaleX = tShapeW / (double)srcImg.width() ;
				double scaleY = tShapeH / (double)srcImg.height();

				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);

				if ("gray" == imgType) //����ĻҶ�����
				{
					srcImg = srcImg.scaled(tShapeW, tShapeH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
					p.drawImage(tx, ty, srcImg);
				}
				else if ("black" == imgType)//����ڰ�ͼƬ
				{
					srcImg = srcImg.scaled(tShapeW, tShapeH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
					p.drawImage(tx, ty, srcImg);
				}
				else if ("Dithering" == imgType)//����ڰ�ͼƬ
				{
					srcImg = srcImg.scaled(tShapeW, tShapeH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
					p.drawImage(tx, ty, srcImg);
				}
				else if ("vector" == imgType) 
				{
					QPainterPath path = obj->path();
					pathScalse(path, scaleX, scaleY);
					path.translate(tx, ty);
					//p.fillPath(path, Qt::black);
					QRectF rect = path.boundingRect();
					p.drawPoint(rect.topLeft());
					p.drawPoint(rect.bottomRight());

					srcImg = srcImg.scaled(tShapeW, tShapeH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
					pathRotation(path, tRotate, srcImg.width() / 2, srcImg.height() / 2);//绘图是画板旋转，路径旋转应在绘图之后
					path_all.addPath(path);
				}

				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
			}
			else if ("text" == dragType) //���ִ���
			{
				QString text = QQmlProperty::read(obj->qmlObject(), "text").toString();
				QString fontfamily = QQmlProperty::read(obj->qmlObject(), "fontfamily").toString();
				QString fontstyle = QQmlProperty::read(obj->qmlObject(), "fontstyle").toString();			
				int fontsize = QQmlProperty::read(obj->qmlObject(), "fontsize").toInt();

				QFont font;
				font.setFamily(fontfamily);
				font.setStyleName(fontstyle);
				font.setPointSize(fontsize* ratio);
				p.setFont(font);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				const QRect rectangle = QRect(tx, ty, tShapeW, tShapeH);
				QRect boundingRect;
				p.drawText(rectangle, Qt::AlignLeft | Qt::AlignTop, text, &boundingRect);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
			}
			else if ("ellipse" == dragType)//��Բ·��
			{
				p.setPen(QPen(Qt::black, 2));
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				p.drawEllipse(tx, ty, tShapeW, tShapeH);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
			}
			else if ("rect" == dragType) /// ��������
			{
				p.setPen(QPen(Qt::black, 2));
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				p.drawRect(tx, ty, tShapeW, tShapeH);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
			}
			else {}
		}
		else 
		{
			if ("image" == dragType)  ///ͼ���ദ��
			{
				QString imgType = obj->type();
				QImage srcImg = obj->dstImageData();

				double scaleX = tShapeW / (double)srcImg.width();
				double scaleY = tShapeH / (double)srcImg.height();
				srcImg = srcImg.scaled(tShapeW, tShapeH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				QPainterPath path = obj->path();
				pathScalse(path, scaleX, scaleY);
				path.translate(tx, ty);
				//p.fillPath(path, Qt::black);
				QRectF rect = path.boundingRect();
				p.drawPoint(rect.topLeft());
				p.drawPoint(rect.bottomRight());

				pathRotation(path, tRotate, srcImg.width() / 2, srcImg.height() / 2);//绘图是画板旋转，路径旋转应在绘图之后
				path_all.addPath(path);

				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
			}
			else if ("text" == dragType) //���ִ���
			{
				QString text = QQmlProperty::read(obj->qmlObject(), "text").toString();
				QString fontfamily = QQmlProperty::read(obj->qmlObject(), "fontfamily").toString();
				QString fontstyle = QQmlProperty::read(obj->qmlObject(), "fontstyle").toString();
				int fontsize = QQmlProperty::read(obj->qmlObject(), "fontsize").toInt();

				QFont font;
				font.setFamily(fontfamily);
				font.setStyleName(fontstyle);
				font.setPointSize(fontsize);
				p.setFont(font);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				const QRect rectangle = QRect(tx, ty, tShapeW, tShapeH);
				QRect boundingRect;
				p.drawText(rectangle, Qt::AlignLeft | Qt::AlignTop, text, &boundingRect);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				bNotImageExit = true;
			}
			else if ("ellipse" == dragType)//��Բ·��
			{
				p.setPen(QPen(Qt::black, 2));
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				p.drawEllipse(tx, ty, tShapeW, tShapeH);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				bNotImageExit = true;
			}
			else if ("rect" == dragType) /// ��������
			{
				p.setPen(QPen(Qt::black, 2));
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(tRotate);//旋转
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				p.drawRect(tx, ty, tShapeW, tShapeH);
				p.translate(tx, ty);
				p.translate(tShapeW / 2, tShapeH / 2);
				p.rotate(-tRotate);//还原
				p.translate(-tShapeW / 2, -tShapeH / 2);
				p.translate(-tx, -ty);
				bNotImageExit = true;
			}
		}
	}
	//结束绘图
	p.end();
	float extra_l = 0;
	float extra_r = 0;
	if ("laser" == m_genType && obj->type() != "vector")
	{
		runOverPointMark(bkg, density, extra_l, extra_r);
	}

	int bkg_h = bkg.height();
	int bkg_w = bkg.width();
	 QImage bkg_mid = bkg.scaled(GCORE_MAX_SUPPORTED_IMG_W, GCORE_MAX_SUPPORTED_IMG_H, Qt::KeepAspectRatio, Qt::FastTransformation);
	int bkg_h_scalsed = bkg_mid.height();
	double scalse = bkg_h_scalsed / (double)bkg_h;
	scalse = int(scalse * 10) / 10.0;
	bkg_h_scalsed = bkg_h * scalse + 0.5;
	bkg = bkg.scaled(bkg_w * scalse + 0.5, bkg_h_scalsed, Qt::KeepAspectRatio, (obj->type() == "vector" || obj->type() == "gray") ? Qt::SmoothTransformation : Qt::FastTransformation);
	bkg.save(tmpImg);

	QByteArray cdata = tmpImg.toLocal8Bit();
	const std::string imgPath(cdata);
	//const std::string imgPath = tmpImg.toStdString(); 

	cdata = (dir + "/tmpImage.gcore").toLocal8Bit();
	const std::string gcorePath(cdata);
	//const std::string gcorePath = (dir + "/tmpImage.gcore").toStdString();

	cdata = tmpGcode.toLocal8Bit();
	const std::string gcodePath(cdata);
	//const std::string gcodePath = tmpGcode.toStdString();

	//if ("plotter" == m_genType && bNotImageExit)//文字、图形生成矢量数据
	//{
	//	int value = QQmlProperty::read(obj, "threshold").toInt();
	//	QPainterPath sharp_path = getPathsFromImage(tmpImg, value);
	//	path_all.addPath(sharp_path);
	//}

#if 1
	GCoreConfig config;
	config.model = Ender3_S;
	config.start = BottomLeft;
	config.dire = StraightLeft;

	config.gco_style = MarlinStyle;
	/*if (m_pLaserSettings->laserGcoStyle() == 1)
	{
		config.gco_style = GRBLStyle;
	}
	else if (m_pLaserSettings->laserGcoStyle() == 2)
	{
		config.gco_style = MarlinStyle;
	}*/
	
	config.offset.x = 0 * 10000; // 0mm
	config.offset.y = 0 * 10000; // 0mm
	config.density = density * 10 * scalse + 0.5; // 9 pixel//mm
	config.platHeight = bkg_h_scalsed;
	config.vector_factor = 10;
	config.vector_path.ptNum = path_all.elementCount();
	config.vector_path.curr_idx = 0;
	config.vector_path.pt = new PixelData[config.vector_path.ptNum];
	config.extra_travel_distance_x_left = extra_l * 10;
	config.extra_travel_distance_x_right = extra_r * 10;
	QString laser_open_type = globalsetting->settings().value("laser_open_type")->str();
	QString laser_close_type = globalsetting->settings().value("laser_close_type")->str();
	if (laser_open_type.isEmpty() || laser_close_type.isEmpty())
	{
		config.laser_start_command = NULL;
		config.laser_stop_command = NULL;
		config.cur_command = NULL;
	}
	else
	{
		config.laser_start_command = new char[255];
		config.laser_stop_command = new char[255];
		strcpy(config.laser_start_command, laser_open_type.toStdString().c_str());
		strcpy(config.laser_stop_command, laser_close_type.toStdString().c_str());
		config.cur_command = 0;
	}


	for (int i = 0; i < config.vector_path.ptNum; i++)
	{
		QPainterPath::Element ele = path_all.elementAt(i);
		config.vector_path.pt[i].pos.x = ele.x * config.vector_factor * scalse + 0.5;
		config.vector_path.pt[i].pos.y = ele.y * config.vector_factor * scalse + 0.5;
		config.vector_path.pt[i].grayval = ele.type == QPainterPath::ElementType::MoveToElement ? 0 : 255;
	}
	if ("laser" == m_genType)
	{
		int laser_work_speed = m_pLaserSettings->laserWorkSpeed();
		config.carving_cnt = m_pLaserSettings->laserTotalNum(); //1;
		config.power_rate = m_pLaserSettings->laserPowerRate() * 10; //80 * 10; // 80%
		config.speed_rate = m_pLaserSettings->laserSpeedRate() * 10; //100 * 10; // 100%
		config.jog_speed = m_pLaserSettings->laserJogSpeed(); //3000;
		
		QString imgType = obj->type();
		if (imgType == "vector" && Machine == "CP-01")
		{
			config.work_speed = 0.2 * laser_work_speed; 
		}
		else
		{
			config.work_speed = laser_work_speed; //1800;
		}
		if (Machine == "Ender-3 S1 Laser")
		{
			config.model = Ender3_S;
			if (imgType == "black" && QQmlProperty::read(obj->qmlObject(), "m4mode").toBool())
			{
				config.laser_start_command = new char[255];
				config.laser_stop_command = new char[255];
				std::string laser_start = "M4 S" + std::to_string(config.power_rate);
				strcpy(config.laser_start_command, laser_start.c_str());
				strcpy(config.laser_stop_command, "M4 S0");
			}
		}
		else if (Machine == "Ender-3 V2 Laser")
		{
			config.v2_offsetX = v2_offsetX * 10000;
			config.model = Ender3_V2_Laser;
			config.laser_start_command = new char[255];
			config.laser_stop_command = new char[255];
			std::string laser_start = "M106 S" + std::to_string(int(config.power_rate / 1000. * 255));
			strcpy(config.laser_start_command, laser_start.c_str());
			strcpy(config.laser_stop_command, "M106 S0");
			config.cur_command = 0;
		}
		else if (Machine == "CP-01")
			config.model = CP_01;		
	}
	else
	{
		config.carving_cnt = m_pPlotterSettings->plotterTotalNum(); //1;
		config.power_rate = m_pPlotterSettings->plotterPowerRate() * 10; 
		config.speed_rate = m_pPlotterSettings->plotterSpeedRate() * 10; 
		config.jog_speed = m_pPlotterSettings->plotterJogSpeed(); 
		config.work_speed = m_pPlotterSettings->plotterWorkSpeed(); 
		config.cnc_work_deep = - m_pPlotterSettings->plotterWorkDepth() * 10;
		config.cnc_jog_height = m_pPlotterSettings->plotterTravelHeight() * 10;
	}

	/*
	GCoreConfig config;
	config.model = CV_01_Pro;
	config.start = BottomLeft;
	config.dire = StraightLeft;
	config.gco_style = MarlinStyle;
	config.total_num = 1;
	config.offset.x = 10 * 10000;
	config.offset.y = 10 * 10000;
	config.density = 10 * 10;
	config.power_rate = 80 * 10; // 80%
	config.speed_rate = 100 * 10; // 100%
	config.jog_speed = 5000;
	config.work_speed = 2000;
	*/



	int ret = gcore_generate(imgPath.c_str(), gcorePath.c_str(), &config);
	if(ret == 0)
	{
		GCoreParser gparser;
		memset(&gparser, 0, sizeof(gparser));
		ret = gcore_parser_init(&gparser, gcorePath.c_str());
		if (ret == 0)
		{
			if ("laser" == m_genType)
			{
				gcore_parser_save(&gparser, gcodePath.c_str());
			}
			else
			{
				gcore_parser_save_cnc_gcode(&gparser, gcodePath.c_str());
			}			
			printf("save gcode success...-----");
		}
		gcore_parser_deinit(&gparser);
	}
#else
	Clip  conf;
	conf.power_max = m_pLaserSettings->laserPowerMax();
	conf.work_speed = m_pLaserSettings->laserWorkSpeed();
	conf.jog_speed = m_pLaserSettings->laserJogSpeed();
	conf.density = m_pLaserSettings->laserDensity();
	conf.offset_x = platformW / 2;/*图片中心位置 X*/
	conf.offset_y = platformH / 2;/*图片中心位置 Y*/

	line_to_line_gcode(imgPath.c_str(), gcodePath.c_str(), &conf);//bmp to gcode.
#endif
}
void LaserGcodeWorker::setLaserSetting(LaserSettings* setting)
{ 
	m_pLaserSettings = setting; 
}
void LaserGcodeWorker::setPlotterSetting(PlotterSettings* setting)
{
	m_pPlotterSettings = setting;
}

void pathScalse(QPainterPath& path, double ratioX, double ratioY)
{
	QPainterPath result_path;
	int ele_num = path.elementCount();
	for (int i = 0; i < ele_num; i++)
	{
		QPainterPath::Element ele = path.elementAt(i);
		ele.x = ele.x * ratioX;
		ele.y = ele.y * ratioY;
		if (ele.type == QPainterPath::ElementType::MoveToElement)
			result_path.moveTo(ele.x, ele.y);
		else if (ele.type == QPainterPath::ElementType::LineToElement)
			result_path.lineTo(ele.x, ele.y);
	}
	path.swap(result_path);
}

void pathRotation(QPainterPath& path, double angle, int imgCenterX, int imgCenterY)
{
	QPainterPath result_path;
	int ele_num = path.elementCount();
	double sinAng = std::sin(angle * PI / 180);
	double cosAng = std::cos(angle * PI / 180);
	for (int i = 0; i < ele_num; i++)
	{
		double x, y;
		QPainterPath::Element ele = path.elementAt(i);
		ele.x = ele.x - imgCenterX;
		ele.y = ele.y - imgCenterY;
		x = ele.x * cosAng - ele.y * sinAng;
		y = ele.x * sinAng + ele.y * cosAng;
		ele.x = x + imgCenterX;
		ele.y = y + imgCenterY;
		if (ele.type == QPainterPath::ElementType::MoveToElement)
			result_path.moveTo(ele.x, ele.y);
		else if (ele.type == QPainterPath::ElementType::LineToElement)
			result_path.lineTo(ele.x, ele.y);
	}
	path.swap(result_path);
}

QPainterPath getPathsFromImage(QString imagePath, int value)
{
	QPainterPath result_path;
	QImage* grad_image = new QImage;
	QImage* black_image = new QImage;
	QImage inputImg = QImage(imagePath);
	*black_image = CX2DLib::CXBlackWhitePicture(inputImg, value);
	*grad_image = CX2DLib::CXImage2VectorImage(*black_image, result_path);
	delete black_image;
	delete grad_image;
	return result_path;
}

void checkIdxRecord(std::vector<int>& index_record)
{
	int count = index_record.size();
	for (int i = 0; i < count; i++)
	{
		bool found = false;
		for (int j = 0; j < count; j++)
		{
			if (i == index_record[j])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			for (int j = 0; j < count; j++)
			{
				if (index_record[j] > i)
				{
					index_record[j] --;
				}
			}
			i--;
		}
	}
}

int getIdx(std::vector<int>& index_record, int idxNo)
{
	int count = index_record.size();
	for (int i = 0; i < count; i++)
	{
		if (idxNo == index_record[i])
			return i;
	}
	return -1;
}

void runOverPointMark(QImage& srcImg,int density, float& extra_l, float& extra_r)
{
	int delta = 10 * density;
	extra_l = delta;
	extra_r = delta;
	int h = srcImg.height();
	int w = srcImg.width();
	QRgb* rgbpixel = (QRgb*)srcImg.scanLine(0);
	for (int ii = 0; ii < h; ii ++)
	{
		int find_idx = -1;
		for (int jj = 0; jj < w; jj++)
		{
			QRgb grays = rgbpixel[jj + ii * (srcImg.width())];
			if (qGray(grays) < 255)
			{
				find_idx = jj;
				break;
			}
		}
		if (find_idx != -1)
		{
			int idx = find_idx - delta;
			if (idx < 0)
			{
				if (find_idx < extra_l) extra_l = find_idx;
				idx = 0;
			}
			rgbpixel[idx + ii * (srcImg.width())] = qRgb(254, 254, 254);
		}


		find_idx = -1;
		for (int jj = w - 1; jj >= 0; jj--)
		{
			QRgb grays = rgbpixel[jj + ii * (srcImg.width())];
			if (qGray(grays) < 255)
			{
				find_idx = jj;
				break;
			}
		}
		if (find_idx != -1)
		{
			int idx = find_idx + delta;
			if (idx > w - 1)
			{
				if (w - 1 - find_idx < extra_r) extra_r = w - 1 - find_idx;
				idx = w - 1;
			}
			rgbpixel[idx + ii * (srcImg.width())] = qRgb(254, 254, 254);
		}
	}
	extra_l /= density;
	extra_r /= density;
}