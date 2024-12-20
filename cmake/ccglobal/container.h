#ifndef _CONTAINER_1638848103128_H
#define _CONTAINER_1638848103128_H
#include "ccglobal/log.h"
#include <list>
#include <assert.h>
#include<algorithm>

#define LIST_ADD(container, x) \
		{ \
			auto it = std::find(container.begin(), container.end(), x); \
			if(it != container.end() || !x) { \
				LOGE("LIST_ADD dumplicate element or empty element."); }\
			else \
				container.push_front(x); \
		}

#define LIST_ADD_BACK(container, x) \
		{ \
			auto it = std::find(container.begin(), container.end(), x); \
			if(it != container.end() || !x) { \
				LOGE("LIST_ADD dumplicate element or empty element."); }\
			else \
				container.push_back(x); \
		}

#define LIST_REMOVE(container, x) \
		{ \
			auto it = std::find(container.begin(), container.end(), x); \
			if(it == container.end() || !x) {\
				LOGE("LIST_REMOVE try remove no exist element."); }\
			else \
				container.erase(it); \
		}

template<class T>
void container_delete_all(T& t)
{
	for (auto it = t.begin(); it != t.end(); ++it)
	{
		if(*it)
			delete *it;
	}
	t.clear();
}

template<class T>
typename T::value_type container_find_from_id(T& t, int id)   // just for pointer 
{
	typename T::value_type value = nullptr;
	for (auto it = t.begin(); it != t.end(); ++it)
	{
		if (*it && (*it)->ID() == id)
		{
			value = (*it);
			break;
		}
	}
	return value;
}

template<class T>
bool container_test_in(T& t, typename T::value_type v)   // just for pointer 
{
	bool in = false;
	for (auto it = t.begin(); it != t.end(); ++it)
	{
		if (*it == v)
		{
			in = true;
			break;
		}
	}
	return in;
}

template<class T>
class LContainer
{
public:
	typedef typename std::list<T*>::iterator LIterator;
public:
	LContainer() {}
	~LContainer() {}

	void addElement(T* element)
	{
		if (!element)
			return;

		LIterator it = std::find(m_elements.begin(), m_elements.end(), element);
		if (it != m_elements.end())
			return;

		m_elements.push_back(element);
	}

	void removeElement(T* element)
	{
		if (!element)
			return;

		LIterator it = std::find(m_elements.begin(), m_elements.end(), element);
		if (it == m_elements.end())
			return;

		m_elements.erase(it);
	}

	template<class Callback>
	void notify(Callback c)
	{
		for (LIterator it = m_elements.begin(); it != m_elements.end(); ++it)
			c(*it);
	}

protected:
	std::list<T*> m_elements;
};

#endif // _CONTAINER_1638848103128_H