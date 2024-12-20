#ifndef CREATIVE_KERNEL_MESSAGE_OBJECT_202312131652_H
#define CREATIVE_KERNEL_MESSAGE_OBJECT_202312131652_H

#include <QObject>
#include "basickernelexport.h"

#define MESSAGE_NAME(type) #type

namespace creative_kernel
{
	enum MessageType {
		StayOnBorder = 0,
		ModelCollision = 1,
		ModelTooTall = 2,
		WipeTowerCollision = 3,
		AdaptiveLayer = 4,

		SupportGeneratorWarn,

		// parameter system
		LayerHeightExceeded,
		InitialLayerHeightExceeded,
		LineWidthExceeded,
		InitialLineWidthExceeded,
		InnerWallLineWidthExceeded,
		OuterWallLineWidthExceeded,
		TopSurfaceLineWidthExceeded,
		SparseInfillLineWidthExceeded,
		InternalSolidInfillLineWidthExceeded,
		SupportLineWidthExceeded,
		EnablePrimeTowerExceeded,
		TreeSupportTipDiameterExceeded,
		TreeSupportBranchDiameterExceeded,
		IroningSpacingExceeded,
		SupportStyleExceeded,

		ParameterUpdateable,

		// platform parameter
		BedTypeInvalid,

		// preview
		PreviewEmpty,

		ModelGroupHigherThanBottom,

		// slice error
		GCodePathOutofPrinterScope = 1000,
		SliceEngineWarning = 5000,
		SliceEngineFail = 20000,
	};

	enum MessageLevel {
		Tip = 0,
		Warning = 1,
		Error = 2,
	};

	class BASIC_KERNEL_API MessageObject : public QObject
	{
		Q_OBJECT
	public:
		MessageObject(QObject* parent = NULL);
		virtual ~MessageObject() = default;

		Q_INVOKABLE int code();
		Q_INVOKABLE int level();
		Q_INVOKABLE QString message();
		Q_INVOKABLE QString linkString();
		Q_INVOKABLE void handle();

	protected:
		virtual int codeImpl() = 0;
		virtual int levelImpl() = 0;
		virtual QString messageImpl() = 0;
		virtual QString linkStringImpl() = 0;
		virtual void handleImpl() = 0;

	};

};

#endif // CREATIVE_KERNEL_MESSAGE_OBJECT_202312131652_H
