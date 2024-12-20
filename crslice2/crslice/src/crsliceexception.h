#ifndef CR_SLICE_EXCEPTION_202406181125_H
#define CR_SLICE_EXCEPTION_202406181125_H
#include <stdexcept>

namespace crslice2
{
	class CrSliceException : public std::runtime_error
	{
	public:
		CrSliceException(std::string const &msg, size_t objectId) : std::runtime_error(msg), m_sliceObjectId(objectId) {}
		size_t sliceObjectId() const { return m_sliceObjectId; }

	private:
		size_t m_sliceObjectId = 0;

	};
}

#endif // CR_SLICE_EXCEPTION_202406181125_H