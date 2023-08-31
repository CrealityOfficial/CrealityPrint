#ifndef _GCODEFILEPREVIEW_1596594780158_H
#define _GCODEFILEPREVIEW_1596594780158_H
#include "slice/previewbase.h"

namespace creative_kernel
{
	class GcodeFilePreview : public PreviewBase
	{
		Q_OBJECT
	public:
		GcodeFilePreview(QObject* parent = nullptr);
		virtual ~GcodeFilePreview();
	
		void work(qtuser_core::Progressor* progressor) override;
	};
}

#endif // _GCODEFILEPREVIEW_1596594780158_H