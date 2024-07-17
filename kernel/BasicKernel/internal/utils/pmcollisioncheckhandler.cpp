#include "pmcollisioncheckhandler.h"

namespace creative_kernel {
	
	PMCollisionCheckHandler::PMCollisionCheckHandler(QObject* parent)
	{
	}
	PMCollisionCheckHandler::~PMCollisionCheckHandler()
	{
		clearCollisionFlags();
	}

}