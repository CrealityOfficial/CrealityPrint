#include "c2dlib.h"

#include <QFile>
#include <QSvgGenerator>
#include <QPainter>
#include <QImage>
#include <QDebug>
#include <QCoreApplication>
//#include "fmesh/font/fontoutlinecenter.h"
#include "imageproc.h"
#include <QPainterPath>
#include <cmath>

CX2DLib::CX2DLib()
{
}

CX2DLib::~CX2DLib()
{
}

QImage CX2DLib::CXReversalImgae(QImage& srcImg)
{
    int imgW = srcImg.width();
    int imgH = srcImg.height();
    srcImg = srcImg.convertToFormat(QImage::Format::Format_ARGB32);
    int channels = 4;
    IMG2DLib::Mat src_img = IMG2DLib::Mat(srcImg.bits(), imgW, imgH, channels);
    IMG2DLib::reversalImgae(src_img);

    QImage retImg = QImage(src_img.imgdata, imgW, imgH, srcImg.format());
    QImage resultImg = retImg.copy(QRect(QPoint(), retImg.size()));
    return resultImg;
}

QImage CX2DLib::CXFlipImage(QImage& srcImg, bool bUpDown, bool bRightLeft)
{
    int imgW = srcImg.width();
    int imgH = srcImg.height();
    srcImg = srcImg.convertToFormat(QImage::Format::Format_ARGB32);
    int channels = 4;
    IMG2DLib::Mat src_img = IMG2DLib::Mat(srcImg.bits(), imgW, imgH, channels);
    if (bUpDown) IMG2DLib::flipUpDown(src_img);
    if (bRightLeft) IMG2DLib::flipRightLeft(src_img);

    QImage retImg = QImage(src_img.imgdata, imgW, imgH, srcImg.format());
    QImage resultImg = retImg.copy(QRect(QPoint(), retImg.size()));
    return resultImg;
}

QImage CX2DLib::CXBlackWhitePicture(QImage& srcImg, const int value)
{
    int imgW = srcImg.width();
    int imgH = srcImg.height();
    srcImg = srcImg.convertToFormat(QImage::Format::Format_ARGB32);
    int channels = 4;
    IMG2DLib::Mat src_img = IMG2DLib::Mat(srcImg.bits(), imgW, imgH, channels);
    IMG2DLib::colorToGray(src_img);
    IMG2DLib::GrayToBinary(src_img, value);

    QImage retImg = QImage(src_img.imgdata, imgW, imgH, srcImg.format());
    QImage resultImg = retImg.copy(QRect(QPoint(), retImg.size()));
    return resultImg;
}

//QImage CX2DLib::CXBlackWhitePicture(QImage& srcImg, const int value)
//{
//    QImage retImg = srcImg;
//    QRgb* bin = (QRgb*)retImg.scanLine(0);
//    QRgb* pixels = (QRgb*)srcImg.scanLine(0);
//
//    const QRgb threadw = qRgb(255, 255, 255);
//    const QRgb threadb = qRgb(0, 0, 0);
//
//    int index = 0;
//    int w = srcImg.width();
//    int h = srcImg.height();
//    for (int j = 0; j < h; j++)
//    {
//        for (int i = 0; i < w; i++)
//        {
//            index = j * w + i;
//            int gray = qGray(pixels[index]);
//            if (gray > value)
//                bin[index] = threadw;
//            else
//                bin[index] = threadb;
//        }
//    }
//    return retImg;
//}

QImage CX2DLib::CXGrayScalePicture(QImage& srcImg, int value, int contrastValue, int brightValue, int setWhiteVal)
{
    int imgW = srcImg.width();
    int imgH = srcImg.height();
    srcImg = srcImg.convertToFormat(QImage::Format::Format_ARGB32);
    int channels = 4;
    IMG2DLib::Mat src_img = IMG2DLib::Mat(srcImg.bits(), imgW, imgH, channels);
    IMG2DLib::colorToGray(src_img, contrastValue, brightValue, setWhiteVal);
    
    QImage retImg = QImage(src_img.imgdata, imgW, imgH, srcImg.format());
    QImage resultImg = retImg.copy(QRect(QPoint(), retImg.size()));
    return resultImg;
}

//QImage CX2DLib::CXDitheringPicture(QImage& srcImg, int value)
//{
//    QImage tempImg;
//    tempImg = srcImg;
//    QImage pos_1 = tempImg;                         //拷贝
//    QImage show;
//    if (tempImg.format() > 3)
//    {
//        QRgb* pixels = (QRgb*)tempImg.scanLine(0);
//        QRgb* grays = (QRgb*)pos_1.scanLine(0);
//        int gray = 0;
//        int v = 0;
//        int error = 0;
//        for (int y = 0; y < tempImg.height(); y++)
//        {
//            for (int x = 0; x < tempImg.width(); x++)
//            {
//                //gray scale
//                gray = qGray(pixels[x + y * (tempImg.width())]);
//                if ((x == 1) || (x == 0))
//                    grays[x + y * (tempImg.width())] = qRgb(255, 255, 255);
//                else
//                    grays[x + y * (tempImg.width())] = qRgb(gray, gray, gray);
//            }
//        }
//        for (int y = 0; y < tempImg.height() - 2; y++)
//        {
//            for (int x = 0; x < tempImg.width() - 2; x++)
//            {
//                {
//                    //gray scale
//                    gray = qGray(grays[x + y * (tempImg.width())]);
//                    v = (gray >= value) ? 255 : 0;
//                    error = (gray - v) / 8;
//                    QRgb q_error = qRgb(error, error, error);
//                    QRgb q_gray = qRgb(v, v, v);
//                    grays[x + y * (tempImg.width())] = q_gray;
//                    grays[1 + x + y * (tempImg.width())] = grays[1 + x + y * (tempImg.width())] + q_error;
//                    grays[2 + x + y * (tempImg.width())] = grays[2 + x + y * (tempImg.width())] + q_error;
//                    grays[x - 1 + (y + 1) * (tempImg.width())] = grays[x - 1 + (y + 1) * (tempImg.width())] + q_error;
//                    grays[x + (y + 1) * (tempImg.width())] = grays[x + (y + 1) * (tempImg.width())] + q_error;
//                    grays[1 + x + (y + 1) * (tempImg.width())] = grays[1 + x + (y + 1) * (tempImg.width())] + q_error;
//                    grays[x + (y + 2) * (tempImg.width())] = grays[x + (y + 2) * (tempImg.width())] + q_error;
//                }
//            }
//        }
//        show = pos_1;
//    }
//    else
//    {
//        show = tempImg;
//    }
//    return show;
//}

QImage CX2DLib::CXDitheringPicture(QImage& srcImg, int value)
{
    int imgW = srcImg.width();
    int imgH = srcImg.height();
    srcImg = srcImg.convertToFormat(QImage::Format::Format_ARGB32);
    int channels = 4;
    if (srcImg.format() == QImage::Format_Grayscale8) channels = 1;
    IMG2DLib::Mat src_img = IMG2DLib::Mat(srcImg.bits(), imgW, imgH, channels);
    IMG2DLib::DitheringImage(src_img, value);
    QImage retImg = QImage(src_img.imgdata, imgW, imgH, srcImg.format());
    QImage resultImg = retImg.copy(QRect(QPoint(), retImg.size()));
    return resultImg;
}

//QImage CX2DLib::CXImage2VectorImage(QImage& srcImg, QPainterPath& path)
//{
//    QxPotrace m_potrace;
//    //转化为svg
//    QImage img = srcImg.mirrored(false, false);
//    m_potrace.setBezierPrecision(10);
//    img = img.convertToFormat(QImage::Format::Format_RGB32);
//    if (!m_potrace.trace(img) || m_potrace.polygons().isEmpty()) return QImage("");
//
//    QSvgGenerator generator;
//    QString tmpFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/black_white.svg";
//    QFile::remove(tmpFilePath);
//    generator.setFileName(tmpFilePath);
//    generator.setSize(img.size());
//    generator.setTitle(QObject::tr("Bingo Svg Generate"));
//    generator.setDescription(QObject::tr("An SVG drawing created by the SVG Generator "
//        "Provide by bingo."));
//    qDebug() << "relution is :" << generator.resolution();
//    QPainter painter;
//    painter.setPen(QPen(Qt::black, 2));
//    painter.begin(&generator);
//    painter.setBrush(Qt::NoBrush);
//    foreach(const QxPotrace::Polygon & polygon, m_potrace.polygons())
//    {
//        path.addPolygon(polygon.boundary);
//        foreach(const QPolygonF & hole, polygon.holes)
//        {
//            path.addPolygon(hole);
//        }
//    }
//    painter.fillPath(path, Qt::black);
//    painter.end();
//    QRect moduleRect = QRect(QPoint(), img.size());
//    QImage result_img = QImage(tmpFilePath).copy(moduleRect);
//    return result_img;
//}

QImage CX2DLib::CXImage2VectorImage(QImage& srcImg, QPainterPath& path)
{
    QxPotrace m_potrace;
    //转化为svg
    QImage img = srcImg.mirrored(false, false);
    m_potrace.setBezierPrecision(10);
    img = img.convertToFormat(QImage::Format::Format_RGB32);
    if (!m_potrace.trace(img) || m_potrace.polygons().isEmpty()) return QImage("");

    foreach(const QxPotrace::Polygon & polygon, m_potrace.polygons())
    {
        path.addPolygon(polygon.boundary);
        if(!polygon.boundary.isEmpty()) path.lineTo(polygon.boundary[0]);
        foreach(const QPolygonF & hole, polygon.holes)
        {
            path.addPolygon(hole);
            if (!hole.isEmpty()) path.lineTo(hole[0]);
        }
    }

    QImage result_img = QImage(srcImg.size(), QImage::Format_ARGB32);
    QImage baseImageDst_mask = result_img.createMaskFromColor(QRgb(0), Qt::MaskInColor);
    result_img.setAlphaChannel(baseImageDst_mask);
    QPainter painter;
    painter.begin(&result_img);
    int lineW = (result_img.height() > result_img.width() ? result_img.width() : result_img.height()) / 500 + 1;
    painter.strokePath(path, QPen(Qt::black, lineW));
    painter.end();
    return result_img;
}

std::string CX2DLib::CXGrayPicToGcode(const QImage& src, const tparams* picSetting)
{
    QImage op(src);
    QString outstr;
    QTextStream out(&outstr);
    //        QImage temp_d = Image2Halftone(src);
    QImage image = op.scaled(m_scope.x(), m_scope.y(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QString uint = "mm";
    double scale = 0.1;
    //     m_pos = m_pos/10;
    if (uint == "mm")
    {
        m_pos = m_pos / 10;
        scale = 0.1;
    }

    out << "G0 X1 Y1 F6000 \n";
    out << "M4 P0 \n";

    int time = picSetting->p_power_time;
    int r_pmp = picSetting->r_per_pixcel;
    int v_pmp = picSetting->v_per_pixcel;
    int p_power = picSetting->p_power;
    if (p_power >= 100)
        p_power = 100;
    p_power = p_power * 2.10;
    double depth_scale = p_power / 255.0;
    for (int ii = 0; ii < image.height(); ii += r_pmp)
    {
        uchar* scan = image.scanLine(ii);
        QString p1;
        QString p2;
        QString p3;
        QString p4;
        if (ii % (2 * r_pmp) == 0)
        {
            int depth = 4;
            for (int jj = 0; jj < image.width(); jj += v_pmp)
            {

                QRgb* rgbpixel = reinterpret_cast<QRgb*>(scan + jj * depth);
                int gray = qGray(*rgbpixel);

                //power off
                if (gray >= 240)
                {
                    if (preOn)
                    {
                        //                        p1 = "M106 S0 \n";
                        p2 = QObject::tr("G0 X%1 Y%2 F3500 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        out << p1 << p2;
                        preOn = false; //关闭
                    }
                }
                else
                {
                    if (!preOn)
                    {
                        preOn = true;
                        out << "G1 F3000 \n";
                    }

                    p1 = QObject::tr("M106 S%1 \n").arg(40 + (255 - gray) * depth_scale);
                    //                    p1 = tr("M4 P%1 \n").arg((255-gray));
                    p2 = QObject::tr("G1 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                    p3 = QObject::tr("G4 P%1 \n").arg(time);
                    out << p2 << p1 << p3 << p4;
                }
            }
        }
        else
        {
            int depth = 4;
            for (int jj = image.width(); jj > 0; jj -= v_pmp)
            {

                QRgb* rgbpixel = reinterpret_cast<QRgb*>(scan + jj * depth);
                int gray = qGray(*rgbpixel);

                //power off
                if (gray >= 240)
                {
                    if (preOn)
                    {

                        p2 = QObject::tr("G0 X%1 Y%2 F6000 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        out << p1 << p2;
                        preOn = false; //关闭
                    }
                }
                else
                {
                    if (!preOn)
                    {
                        preOn = true;
                        out << "G1 F3000 \n";
                    }

                    p1 = QObject::tr("M106 S%1 \n").arg(40 + (255 - gray) * depth_scale);
                    //                    p1 = tr("M4 P%1 \n").arg((255-gray));
                    p2 = QObject::tr("G1 X%1 Y%2  \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                    p3 = QObject::tr("G4 P%1 \n").arg(time);
                    out << p2 << p1 << p3 << p4;
                    preOn = true;
                }
            }
        }

    }
   return std::move(outstr.toStdString());
}

/// <summary>
/// 绘图仪黑白图形，，
/// </summary>
/// <param name="src"></param>
/// <param name="picSetting"></param>
/// <returns></returns>
std::string CX2DLib::CXPlotterBlackPicToGcode(const QImage& src, const tparams* picSetting)
{
    QString outstr;
    QTextStream out(&outstr);
    double scale = (double)(0.3333333);
    double printZ = picSetting->printZ;
    double travelZ = picSetting->travelZ;
    double infillDens = picSetting->infillDensity;

    QString power_on = picSetting->laser_on_cmd + "\n";
    QString power_off = picSetting->laser_of_cmd + "\n";
    double c_feed = picSetting->plotter_feed_z;
    int travel_speed = 60 * picSetting->travel_speed;
    int crue_speed = 60 * picSetting->crue_speed;
    int machine_max_y = picSetting->y_max;
    bool zIsUp = false;

    int stepLine = (int)(1/ infillDens);
    QImage image = src.mirrored(false, true);
    QRgb* rgbpixel = (QRgb*)image.scanLine(0);
    int gray = 0;
    out<< QObject::tr("G0 Z%1 F3000\n").arg(c_feed);
    for (int i = 0; i < image.height(); i+=stepLine)
    {
        QString p0;
        QString p1;
        QString p2;
        QString ps;
        for (int j = 0; j < image.width(); j++)
        {
            QRgb grays = rgbpixel[j + i * image.width()];
            gray = qGray(grays);
            if (gray > 127) //白色像素
            {
                ///抬升Z轴
                if (!zIsUp)
                {  
                    //G1在此结束，写G1
                    out << QObject::tr("G1 X%1 Y%2 \n").arg(j * scale + m_pos.x()).arg(i * scale + m_pos.y());
                    out << QObject::tr("G0 F%1 \n").arg(travel_speed);
                    out << QObject::tr("G0 X%1 Y%2 \n").arg(j * scale + m_pos.x()).arg(i * scale + m_pos.y());
                    //抬升Z轴，
                    out << QObject::tr("G0 Z%1 F3000\n").arg(travelZ);
                    zIsUp = true;
                }
                else
                {
                    continue;
                }
            }
            else //黑色像素
            {
                if (zIsUp)//之前抬升了，之前是白色,白色到此为止
                {
                    out << QObject::tr("G0 X%1 Y%2 \n").arg(j * scale + m_pos.x()).arg(i * scale + m_pos.y());
                    out << QObject::tr("G0 Z%1 F3000\n").arg(printZ);
                    out << QObject::tr("G1 F%1 \n").arg(crue_speed);
                    out << QObject::tr("G1 X%1 Y%2  \n").arg(j * scale + m_pos.x()).arg(i * scale + m_pos.y());
                    zIsUp = false;
                }
                else
                {
                    continue;
                }
            }
        }
    }
    return std::move(outstr.toStdString());
}
std::string CX2DLib::CXLaserBlackPicToGcode(const QImage& src,const tparams* picSetting)
{
    QString outstr;
    QTextStream out(&outstr);
    double scale = (double)(0.333333333);
    QString power_on = picSetting->laser_on_cmd+ "\n";
    QString power_off = picSetting->laser_of_cmd + "\n";
    double c_feed = picSetting->plotter_feed_z;
    int travel_speed = 60 * picSetting->travel_speed;
    int crue_speed = 60 * picSetting->crue_speed;
    int machine_max_y = picSetting->y_max;
    QImage image = src.mirrored(false, true);
    preOn = false;  //上一个像素，激光是否开启的标志
    QRgb* rgbpixel = (QRgb*)image.scanLine(0);
    int gray = 0;
    out << QObject::tr("G0 Z%1 F3000\n").arg(c_feed);
    int depth = 1;
    int h = image.height();
    int w = image.width();
    for (int ii = 0; ii < image.height(); ii += 1)
    {
        uchar* scan = image.scanLine(ii);
        QString p0;
        QString p1;
        QString p2;
        QString ps;
        if (ii % 2 == 0)
        {
            for (int jj = 0; jj < image.width(); jj += 1)
            {
                QRgb grays = rgbpixel[jj + ii * image.width()];
                gray = qGray(grays);
                 //白色像素
                if (gray >= 128)
                {
                    //如果之前开了激光.由黑-->白，需要开启激光
                    //判断是不是第一个白色像素
                    if (preOn)
                    {
                        //G1在此结束，写G1
                        p0 = QObject::tr("G1 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        p1 = power_off;
                        ps = QObject::tr("G0 F%1 \n").arg(travel_speed);
                        p2 = QObject::tr("G0 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        out << p0 << p1 << ps << p2;
                        preOn = false; //关闭
                    }
                    else
                    {
                        continue;
                    }
                }
                //黑色像素
                else
                {
                    //如果之前preOn=false，那么说明前面是黑色的，由白色-->黑色
                    //且上一个像素激光没有开启
                    if (!preOn)
                    {
                        p0 = QObject::tr("G0 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        p1 = power_on;
                        ps = QObject::tr("G1 F%1 \n").arg(crue_speed);
                        p2 = QObject::tr("G1 X%1 Y%2  \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        preOn = true;
                        out << p0 << p1 << ps << p2;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
        else
        {
            for (int jj = image.width(); jj > 0; jj -= 1)
            {
                QRgb grays = rgbpixel[jj + ii * image.width()];
                gray = qGray(grays);
                //power off
                if (gray >= 128)
                {
                    if (preOn)
                    {
                        //G1在此结束，写G1
                        p0 = QObject::tr("G1 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        p1 = power_off;
                        ps = QObject::tr("G0 F%1 \n").arg(travel_speed);
                        p2 = QObject::tr("G0 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        out << p0 << p1 << ps << p2;
                        preOn = false; //关闭
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    if (!preOn)
                    {
                        p0 = QObject::tr("G0 X%1 Y%2 \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        p1 = power_on;
                        ps = QObject::tr("G1 F%1 \n").arg(crue_speed);
                        p2 = QObject::tr("G1 X%1 Y%2  \n").arg(jj * scale + m_pos.x()).arg(ii * scale + m_pos.y());
                        preOn = true;
                        out << p0 << p1 << ps << p2;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
    }
    return std::move(outstr.toStdString());
}

bool CX2DLib::CXImage2Svg(const QImage& src, const QString& outfile, const tparams* svgSetting)
{
    return false;
}

std::string  CX2DLib::CXImage2LaserGcode(const QImage& img,const tparams* picSetting, QPointF pos, QPointF scope, QString picType)
{
    m_pos = pos;
    m_scope = scope;
    if ("gray" == picType)
    {
        return std::move(CXGrayPicToGcode(img, picSetting));
    }
    else if ("black" == picType)
    {
        return std::move(CXLaserBlackPicToGcode(img, picSetting));
    }
    else
    {

    }
    return std::string("");
}
std::string CX2DLib::CXImage2PlotterGcode(const QImage& img, const tparams* picSetting, QPointF pos, QPointF scope, QString picType)
{
    m_pos = pos;
    m_scope = scope;
   return std::move(CXPlotterBlackPicToGcode(img, picSetting));

}

std::string CX2DLib::CXShape2Gcode(const QString& shapeType, const tparams* shapeSetting, QPointF pos, QPointF scope)
{
    QString outstr;
    QTextStream out(&outstr);
    double printZ = shapeSetting->printZ;
    double travelZ = shapeSetting->travelZ;
    double travelSpeed = shapeSetting->travel_speed;
    double printSpeed = shapeSetting->feed_speed;
    double recWidth = shapeSetting->pic_width;
    double recHeight = shapeSetting->pic_height;
    if ("rect" == shapeType)
    {
        double x1 = pos.x();
        double y1 = pos.y();
        double x2 = pos.x() + recWidth / 3;
        double y2 = pos.y() + recHeight / 3;
        out << QObject::tr("G0 Z%1 F3000\n").arg(travelZ);
        //移动到正方形起始点
        out << QObject::tr("G0 F%1 \n").arg(travelSpeed);
        out << QObject::tr("G0 X%1 Y%2 \n").arg(x1).arg(y1);
         //画线
        out << QObject::tr("G1 F%1 \n").arg(printSpeed);
        out << QObject::tr("G1 X%1 Y%2  \n").arg(x2).arg(y1);
        out << QObject::tr("G1 X%1 Y%2  \n").arg(x2).arg(y2);
        out << QObject::tr("G1 X%1 Y%2  \n").arg(x1).arg(y2);
        out << QObject::tr("G1 X%1 Y%2  \n").arg(x1).arg(y1);
    }
    else if ("ellipse" == shapeType) ///pos 为原点
    {
        double a = recWidth / 2;
        double b = recHeight / 2;
        double x0 = pos.x(); 
        double y0 = pos.y();
        out << QObject::tr("G0 Z%1 F3000\n").arg(travelZ);
        //移动到正方形起始点
        out << QObject::tr("G0 F%1 \n").arg(travelSpeed);
        out << QObject::tr("G0 X%1 Y%2 \n").arg(x0).arg(y0);
        double xn = 0.0, yn = 0.0;
        for (double theta = 0; theta < 360; theta+=0.1)
        {
            xn = a * std::cos(theta*PI/180) + x0;
            yn = b * std::sin(theta * PI / 180) + y0;
            out << QObject::tr("G1 X%1 Y%2  \n").arg(xn).arg(yn);
        }
    }
    else
    {

    }
    return std::move(outstr.toStdString());
}