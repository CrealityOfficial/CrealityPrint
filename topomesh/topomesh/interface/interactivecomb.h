#ifndef TOPOMESH_INTERACTIVECOMB_1697094139676_H
#define TOPOMESH_INTERACTIVECOMB_1697094139676_H
#include "topomesh/interface/honeycomb.h"

namespace topomesh
{
	class InteractiveCombDebugger
	{
	public:
		virtual ~InteractiveCombDebugger() {}
	};

	struct InteractiveCombImpl;
	class TOPOMESH_API InteractiveComb
	{
	public:
		InteractiveComb(TopoTriMeshPtr mesh);
		~InteractiveComb();

		void setDebugger(InteractiveCombDebugger* debugger);
		TopoTriMeshPtr generate(const CombParam& honeyparams = CombParam(), ccglobal::Tracer* tracer = nullptr);

		//debug
		std::vector<int> checkLagestPlaner();
	protected:
		InteractiveCombImpl* impl;
	};
}

#endif // TOPOMESH_INTERACTIVECOMB_1697094139676_H