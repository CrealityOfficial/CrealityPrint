#ifndef CORE_TEXTIMGGENERATOR_H
#define CORE_TEXTIMGGENERATOR_H

#include "qtusercore/qtusercoreexport.h"

#include <QFont>
#include <QColor>


namespace qtuser_core
{
    class QTUSER_CORE_API TextImgGenerator : public QObject
    {
    public:
        TextImgGenerator(QObject* parent = nullptr);
        ~TextImgGenerator();

        void setText(QString text);
        void setFont(QFont font);
        void setAlignFlag(int flag);
        void setTextWrap(bool wrap);
        void setTextVerticalFlag(bool flag);

        // ��Ϊ -1�����Զ��������С�����ֳ���ȥ���а�������
        void setImgWidth(int width = -1);
        void setImgHeight(int height = -1);

        void setForeColor(QColor color);
        void setBackColor(QColor color);
        
        // ÿ�ζ�����������ͼƬ��������������ͼƬ���ڴ����
        QImage generateImage();

    private:
        bool m_wrapFlag;
        bool m_verticalFlag;
        int m_alignFlag;
        int m_imgWidth;
        int m_imgHeight;

        QColor m_foreColor;
        QColor m_backColor;

        QString m_text;
        QFont m_font;
    };
}

#endif // CORE_TEXTIMGGENERATOR_H

