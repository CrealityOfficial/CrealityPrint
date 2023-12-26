#include "gcodehelper.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtGui/QPainter>

#include "crslice/gcode/thumbnail.h"
#include <fstream>

namespace cxsw
{
    bool cxSaveGCode(const QString& fileName, const QString& previewImageString,
		const QStringList& layerString, const QString& prefixString, const QString& tailCode)
    {
		QFile out;
		out.setFileName(fileName);
		if (!out.open(QIODevice::WriteOnly))
		{
			out.close();
			return false;
		}
        
        ///添加预览图信息
        if (!previewImageString.isEmpty())
        {
            out.write(previewImageString.toLocal8Bit());
			out.write("\r\n", 2); //in << endl;
        }

        //添加打印时间等信息
        if (!prefixString.isEmpty())
        {
			out.write(prefixString.toLocal8Bit());
			out.write("\n", 1);
        }

        for (const QString& layerLine : layerString)
        {
			out.write(layerLine.toLocal8Bit());
			out.write("\n", 1);
        }
        
        if (!tailCode.isEmpty())
        {
			out.write(tailCode.toLocal8Bit());
			out.write("\n", 1);
        }
        out.close();
        return true;
    }

    void image2String(const QImage& image, int eWidth, int eHeight, bool clear, QString& outString)
    {
        quint16 tmpInt16;
        uchar tmpChar[2] = { 0 };
        QString tmpNum = QString::number(eWidth);
        if(clear)
            outString.clear();
        outString += QString(";image%1:").arg(tmpNum);

        int r, g, b, rgb, width, height;
        QImage hendimg = image.scaled(eWidth, eHeight, Qt::KeepAspectRatio);
        width = hendimg.width();
        height = hendimg.height();

        int rgbtest = 0;
        int lineCount = 0;
        for (int h = 0; h < height; h++)
        {
            for (int w = 0; w < width; w++)
            {
                QColor pcolor(hendimg.pixel(w, h));
                r = pcolor.red() >> 3;
                g = pcolor.green() >> 2;
                b = pcolor.blue() >> 3;
                rgb = (r << 11) | (g << 5) | b;
                *(qint16*)tmpChar = (quint16)rgb;
                for (int i = 0; i < 2; i++)
                {
                    quint8 hbit = ((quint8)tmpChar[i]) >> 4;
                    quint8 lbit = ((quint8)tmpChar[i]) & 15;
                    outString += "0123456789ABCDEF"[hbit % 16];
                    outString += "0123456789ABCDEF"[lbit % 16];
                }
            }
        }
        outString += "\r\n";
    }

	QImage* createImageWithOverlay(const QImage& baseImage, const QImage& overlayImage)
	{
		QImage* imageWithOverlay = new QImage(baseImage.size(), baseImage.format());
		QImage mask = imageWithOverlay->createMaskFromColor(QRgb(0), Qt::MaskInColor);
		imageWithOverlay->setAlphaChannel(mask);
		QPainter painter(imageWithOverlay);

		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.fillRect(imageWithOverlay->rect(), Qt::transparent);

		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0, 0, baseImage);

		int offsetX = (baseImage.width() - overlayImage.width()) / 2;
		int offsetY = (baseImage.height() - overlayImage.height()) / 2;
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(offsetX, offsetY, overlayImage);

		painter.end();

		return imageWithOverlay;
	}

	QRect getModuleRect(QImage* previewImage)
	{
		int imageW = previewImage->width();
		int imageH = previewImage->height();
		int dataLength = imageW * imageH * 4;
		std::vector<unsigned char> data(dataLength);
		memcpy(data.data(), previewImage->bits(), dataLength);
		int lineDataLength = imageW * 4;
		int minX = imageW, maxX = -1, minY = -1, maxY = -1;
		for (int i = 3; i < dataLength; i += 4)
		{
			if (data[i] != 0)
			{
				int x = (i % lineDataLength) / 4;
				int y = i / lineDataLength;
				if (minY == -1) minY = y;
				maxY = y;
				if (minX > x)
				{
					minX = x;
				}
				if (maxX < x)
				{
					maxX = x;
				}
			}
		}
		QRect moduleRect = QRect(minX, minY, maxX - minX, maxY - minY);
		return moduleRect;
	}

	QImage* resizeModule(QImage* previewImage)
	{
		int imageW = previewImage->width();
		int imageH = previewImage->height();
		double WH_ratio = imageW / (double)imageH;

		QRect moduleRect = getModuleRect(previewImage);
		QImage moduleImg = previewImage->copy(moduleRect);

		int bigImageW = imageW - 20;
		int bigImageH = imageH - 20;
		QImage bigImg = moduleImg.scaled(bigImageW, bigImageH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		QImage baseImageDst = QImage(previewImage->size(), previewImage->format());
		QImage baseImageDst_mask = baseImageDst.createMaskFromColor(QRgb(0), Qt::MaskInColor);
		baseImageDst.setAlphaChannel(baseImageDst_mask);
		QImage* dstImg = createImageWithOverlay(baseImageDst, bigImg);
		return dstImg;
	}

	int getLineStart(QImage* Image)
	{
		int LineStart = -1;
		int imageW = Image->width();
		int imageH = Image->height();
		QImage imageC = Image->alphaChannel();
		unsigned char* imgCData = imageC.bits();

		for (int i = 0; i < imageH; i++)
		{
			for (int j = 0; j < imageW; j++)
			{
				if (imgCData[i * imageW + j] > 0)
				{
					LineStart = i;
					break;
				}
			}
			if (LineStart > -1) break;
		}

		return LineStart;
	}

	int getLineEnd(QImage* Image)
	{
		int LineEnd = -1;
		int imageW = Image->width();
		int imageH = Image->height();
		QImage imageC = Image->alphaChannel();
		unsigned char* imgCData = imageC.bits();

		for (int i = imageH - 1; i >= 0; i--)
		{
			for (int j = 0; j < imageW; j++)
			{
				if (imgCData[i * imageW + j] > 0)
				{
					LineEnd = i;
					break;
				}
			}
			if (LineEnd > -1) break;
		}

		return LineEnd;
	}

	void getImageStr(QString& imageStr, QImage* Image, int layers, QString sPreImgFormat, float layerHeight, bool multiFormat, const QString& path)
	{
		QString imgSavePath = QString("%1/imgPreview.%2").arg(path).arg(sPreImgFormat);
		Image->save(imgSavePath);

		QByteArray cdata = imgSavePath.toLocal8Bit().data();
		std::string imgSaveStdPath(cdata);

		std::ifstream ios(imgSaveStdPath, std::ios::binary | std::ios::in);
		std::string s;
		std::vector<unsigned char> data;

		if (ios.is_open()) 
		{
			char buffer[1024]{ 0 };
			while (ios.good()) 
			{
				ios.read(buffer, sizeof(buffer));
				std::string temp(buffer, ios.gcount());
				int src_size = data.size();
				data.resize(src_size + temp.size());
				copy(temp.begin(), temp.end(), data.begin() + src_size);
			}
			ios.close();
		}

		std::vector<std::string> outStr;
		QString imgSize;
		imgSize.sprintf("%d*%d", Image->width(), Image->height());
		QString imgStartEndLine;
		imgStartEndLine.sprintf("%d %d", getLineStart(Image), getLineEnd(Image));
		gcode::thumbnail_to_gcode(data, imgSize.toStdString(), sPreImgFormat.toStdString(), imgStartEndLine.toStdString(), layers, outStr, layerHeight);
		int encodeSize = 0;
		for (size_t i = 0; i < outStr.size(); i++)
		{
			auto line = outStr[i];
			if (i != 0 && i != outStr.size() - 1)
			{
				encodeSize = encodeSize + line.length() - 2;
			}
			imageStr += QString(line.c_str());
			imageStr += "\n";
		}
		if (multiFormat)
		{
			//imageStr += ";Generated with Cura_SteamEngine 5.2.1";
			//imageStr += "\n";
			imageStr += ";";
			imageStr += "\n";
			QString firstLine;
			firstLine.sprintf("; thumbnail begin %d %d %d", Image->width(), Image->height(), encodeSize);
			imageStr += firstLine;
			imageStr += "\n";
			for (size_t i = 1; i < outStr.size() - 1; i++)
			{
				imageStr += QString(outStr[i].c_str());
				imageStr += "\n";
			}
			imageStr += "; thumbnail end";
			imageStr += "\n";
			imageStr += ";";
			imageStr += "\n";
		}
	}
}
