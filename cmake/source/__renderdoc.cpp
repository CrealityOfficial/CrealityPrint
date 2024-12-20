//auto link, don't edit it

#ifdef _WIN32
#include <Windows.h>
#include "RenderDoc.h"
#include "ccglobal/log.h"

namespace render_doc
{
	class initializer
	{                    
	public:              
		initializer()
		{     
			HINSTANCE renderDoc = LoadLibrary(RENDERDOC_DLL);
			if(!renderDoc)
				LOGE("load renderDoc error.");
		};             
		~initializer() {};
	};
}                    
render_doc::initializer initializerInit;

#endif