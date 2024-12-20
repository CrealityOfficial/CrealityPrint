#ifndef PICKABLESELECTTRACER_1595663113326_H
#define PICKABLESELECTTRACER_1595663113326_H

namespace qtuser_3d
{
	class Pickable;
	class SelectorTracer
	{
	public:
		virtual ~SelectorTracer() {}

		virtual void onSelectionsChanged() = 0;
		virtual void selectChanged(Pickable* pickable) = 0;
	};
}
#endif // PICKABLESELECTTRACER_1595663113326_H