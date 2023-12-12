#ifndef CX3in1EXPORTER_H
#define CX3in1EXPORTER_H

#include<QImage>
#include <QTextStream>
#include <QtCore/QStandardPaths>
#include"QPotrace/include/qxpotrace.h"
#define PI 3.1415926535

static std::string convtInt2mmString(const int32_t coord)
{
	QString outstr;
	QTextStream out(&outstr);
	constexpr size_t buffer_size = 24;
	char buffer[buffer_size];
	int char_count = sprintf(buffer, "%d", coord); // we haven't found any way for the windows compiler to accept formatting of a coord_t, so it has to be int32_t instead
	int end_pos = char_count; // the first character not to write any more
	int trailing_zeros = 1;
	while (trailing_zeros < 4 && buffer[char_count - trailing_zeros] == '0')
	{
		trailing_zeros++;
	}
	trailing_zeros--;
	end_pos = char_count - trailing_zeros;
	if (trailing_zeros == 3)
	{ // no need to write the decimal dot
		buffer[char_count - trailing_zeros] = '\0';
		out << buffer;
		return std::move(outstr.toStdString());
	}
	if (char_count <= 3)
	{
		int start = 0; // where to start writing from the buffer
		if (coord < 0)
		{
			out << '-';
			start = 1;
		}
		out << "0.";
		for (int nulls = char_count - start; nulls < 3; nulls++)
		{ // fill up to 3 decimals with zeros
			out << '0';
		}
		buffer[char_count - trailing_zeros] = '\0';
		out << (static_cast<char*>(buffer) + start);
	}
	else
	{
		char prev = '.';
		int pos;
		for (pos = char_count - 3; pos <= end_pos; pos++)
		{ // shift all characters and insert the decimal dot
			char next_prev = buffer[pos];
			buffer[pos] = prev;
			prev = next_prev;
		}
		buffer[pos] = '\0';
		out << buffer;
	}
	return std::move(outstr.toStdString());
}
static std::string convtDoubleToString(const unsigned int precision, const double coord)
{
	char format[5] = "%.xF"; // write a float with [x] digits after the dot
	format[2] = '0' + precision; // set [x]
	constexpr size_t buffer_size = 400;
	char buffer[buffer_size];
	int char_count = sprintf(buffer, format, coord);
	if (char_count <= 0)
	{
		return "";
	}
	if (buffer[char_count - precision - 1] == '.')
	{
		int non_nul_pos = char_count - 1;
		while (buffer[non_nul_pos] == '0')
		{
			non_nul_pos--;
		}
		if (buffer[non_nul_pos] == '.')
		{
			buffer[non_nul_pos] = '\0';
		}
		else
		{
			buffer[non_nul_pos + 1] = '\0';
		}
	}
	return std::string(buffer);
}
struct tparams  {

	QString laser_on_cmd; //激光打开
	QString laser_of_cmd;  /// 激光关闭
	double plotter_feed_z;  ///厚度
	int travel_speed;   ///空走速度
	int crue_speed; ///激光打印速度
	int feed_speed;
	int y_max;
	int pic_width;
	int pic_height;
	int machine_width;
	int machine_length;
	float xoffset;
	float yoffset;
	float plat_width;
	float plat_height;

	//plotter
	float printZ;
	float travelZ;
	float infillDensity;
	int plot_speed;///绘图仪打印速度

	//////////////gray to gcode pix
	double p_power_time;
	int r_per_pixcel;
	int v_per_pixcel;
	int p_power;
	tparams()
	{
		laser_on_cmd = "m666";
		laser_of_cmd = "m888";
		plotter_feed_z = 0.2;
		travel_speed = 20;
		crue_speed = 20;
		y_max = 200;

		xoffset = 0.0;
		yoffset = 0.0;
		plat_width = 300.00;
		plat_height = 300.00;
	}
} ;

class  CX2DLib
{
public:
	CX2DLib();
	~CX2DLib();

public:


	//转灰度图
	static QImage CXGrayScalePicture(QImage& srcImg, int value, int contrastValue, int brightValue, int setWhiteVal);
	//转黑白图
	static QImage CXBlackWhitePicture(QImage& srcImg, int valu);

	static QImage CXDitheringPicture(QImage& srcImg, int value);

	static QImage CXImage2VectorImage(QImage& srcImg, QPainterPath& path);

	static QImage CXReversalImgae(QImage& srcImg);

	static QImage CXFlipImage(QImage& srcImg, bool bUpDown, bool bRightLeft);
	
	//激光 图片转GCode 灰度或黑白
	std::string CXImage2LaserGcode(const QImage& img, const tparams* picSetting,QPointF pos,QPointF scope , QString picType="gray");
	//绘图仪 图片转GCode 灰度或黑白
	std::string CXImage2PlotterGcode(const QImage& img, const tparams* picSetting, QPointF pos, QPointF scope, QString picType = "vector");
	std::string CXShape2Gcode(const QString& shapeType, const tparams* shapeSetting,QPointF pos, QPointF scope);

	//Image轮廓转SVG
	bool CXImage2Svg(const QImage& src,const QString& outfile, const tparams* svgSetting);
	void setPos(QPointF pos) { m_pos = pos; }
	void setScope(QPointF scope) { m_scope = scope; }

private:

	//灰度图转GCode
	std::string CXGrayPicToGcode(const QImage& src, const tparams* picSetting);
	//激光黑白图转Gcode
	std::string CXLaserBlackPicToGcode(const QImage& src, const tparams* picSetting);

	//绘图仪黑白图转Gcode
	std::string CXPlotterBlackPicToGcode(const QImage& src, const tparams* picSetting);

	QPointF m_pos;
	QPointF m_scope;
	bool preOn = false;
};

#endif