#ifndef _CXKERNEL_ExtendMeshGenJOB_1595582868614_H
#define _CXKERNEL_ExtendMeshGenJOB_1595582868614_H
#include "cxkernel/data/modelndata.h"
#include "ccglobal/tracer.h"

namespace cxkernel
{
	class CXKERNEL_API PhotoMeshModel : public QObject
	{
		Q_OBJECT
	public:
		PhotoMeshModel(QObject* parent = nullptr);
		virtual ~PhotoMeshModel();

		Q_INVOKABLE void setBaseHeight(float baseHeight);
		Q_INVOKABLE void setMaxHeight(float maxHeight);
		Q_INVOKABLE void setLighterOrDarker(int index);
		Q_INVOKABLE void setMeshWidth(float meshX);
		Q_INVOKABLE void setBlur(int blur);

		TriMeshPtr build(const QString& imageName, ccglobal::Tracer* tracer);
	private:
		struct PhotoMeshParam
		{
			QString fileName;
			int blur;
			int colorSeg;
			int edgeType;
			int convertType;

			float baseHeight;
			float maxHeight;
			int index;

			bool invert;
			float meshXWidth;
			int pointXNum;
			int pointYNum;

			PhotoMeshParam()
			{
				blur = 9;
				edgeType = 0;
				colorSeg = -1;
				convertType = 0;
				baseHeight = 0.35f;
				maxHeight = 2.2f;
				index = 0;
				invert = true;
				meshXWidth = 140.0f;
				pointXNum = 1000;
				pointYNum = 1000;
			}
		};

		PhotoMeshParam m_param;
	};
}

#endif // _CXKERNEL_ExtendMeshGenJOB_1595582868614_H