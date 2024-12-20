#ifndef _QTUSER_CORE_SINGLETON_1590845087811_H
#define _QTUSER_CORE_SINGLETON_1590845087811_H
#include "qtusercore/qtusercoreexport.h"

template<class T>
class Singleton
{
public:
	Singleton()
	{

	}

	virtual ~Singleton()
	{
		release();
	}

	T* instance()
	{
		if (!single)
		{
			single = new T();
		}

		return single;
	}

	void release()
	{
		if (single)
		{
			#ifdef __APPLE__
			#else
				delete single;
				single = nullptr;
			#endif
		}
	}
protected:
	static T* single;
};

template<class T>
T* Singleton<T>::single = nullptr;

#define SINGLETON_DECLARE(x) protected:\
								friend class Singleton<x>;\
								x();

#define SINGLETON_IMPL(x) x* get##x()\
							{ \
								static Singleton<x> s;\
								return s.instance();\
							}

#define SINGLETON_EXPORT(api, x) api x* get##x();

template<class T>
class Singleton2
{
public:
	Singleton2()
	{
		single = new T();
	}

	virtual ~Singleton2()
	{
		if (single)
		{
			delete single;
			single = nullptr;
		}
	}

	T* single;
};

#define SINGLETON_DECLARE2(x) protected:\
								friend class Singleton2<x>;\
								x();

#define SINGLETON_IMPL2(x)  Singleton2<x>* static##x = nullptr;\
							x* get##x()\
							{ \
								if(!static##x)\
								{\
									static##x = new Singleton2<x>();\
								}\
								return static##x->single;\
							}\
							void release##x()\
							{\
								if(static##x)\
								{\
									delete static##x;\
									static##x = nullptr;\
								}\
							}
						   

#define SINGLETON_EXPORT2(api, x) api x* get##x(); \
								  api void release##x();
#endif // _QTUSER_CORE_SINGLETON_1590845087811_H
