#ifndef TRACER_1630734954343_H
#define TRACER_1630734954343_H

#include <stdarg.h>
#include <stdio.h>
#include <map>
#include <string>

namespace ccglobal
{
	/*
	* ׷�ٹ��� ״̬�������
	* progress �ص����̽���
	* failed ���̳������ش��󣬹��̻���������ص��������������ԭ��
	* success ���̳ɹ�ִ���꣬��ص��������
	*/

	class Tracer
	{
	public:
		virtual ~Tracer() {}
		virtual void progress(float r) = 0;
		virtual bool interrupt() = 0;

		virtual void message(const char* msg) = 0;
		virtual void failed(const char* msg) = 0;
		virtual void success() = 0;
		virtual void message(int msg, int ext1, int ext2, bool differentThread) {};
		virtual void variadicFormatMessage(int msg, ...) {}
		virtual void recordExtraMessage(const char* keyMsg, const char* valueMsg, size_t objId) {};
		virtual int extraMessageSize() { return 0; };
		virtual std::map< std::string, std::pair<std::string, size_t> > getExtraRecordMessage() { return std::map<std::string, std::pair<std::string, size_t>>(); };

		void formatMessage(const char* format, ...)
		{
			char buf[1024] = { 0 };
			va_list args;
			va_start(args, format);
			vsprintf(buf, format, args);
			message(buf);
			va_end(args);
		}

		void resetProgressScope(float start = 0.0f, float end = 1.0f)
		{
			m_start = start;
			m_end = end;
			progress(0.0f);
		}

		void resetScope(float end = -1.0f)
		{
			m_start = m_realValue;
			if(end > 0.0f)
				m_end = end;
		}

		void resetPercentScope(float percent)
		{
			m_start = m_realValue;
			m_end = m_start + percent * (m_end - m_start);
		}

		float start() const
		{
			return m_start;
		}

		float end() const
		{
			return m_end;
		}

	protected:
		float m_start = 0.0f;
		float m_end = 1.0f;
		float m_realValue;
	};




#define JUDGE_PROGRESS_MSG_BREAK(x, v, msg) \
if(x != nullptr) { \
	float vv = v; \
	if(vv > 1) { \
		vv = 1; \
	} \
	if(vv > 0) { \
		x->progress(vv); \
	} \
	if (x->interrupt()) { \
		if(msg != "") \
			x->failed("user canceled"); \
		return; \
	} \
}


#define JUDGE_PROGRESS_MSG_RETURN(x, v, r, msg) \
if(x != nullptr) { \
	float vv = v; \
	if(vv > 1) { \
		vv = 1; \
	} \
	if(vv > 0) { \
		x->progress(vv); \
	} \
	if (x->interrupt()) { \
		if(msg != "") \
			x->failed("user canceled"); \
		return r; \
	} \
}



#define SPLIT_PROGRESS_MSG_BREAK(i, seg, total, x, msg) \
if(x != nullptr) { \
	int check = (int)((float)(total) / (float)(seg)); \
	if((i + 1) == check) { \
		float per = (float)(i) / (float)(total); \
	JUDGE_PROGRESS_MSG_BREAK(x, per, msg) \
	} \
}


#define SPLIT_PROGRESS_MSG_RETURN(i, seg, total, x, r, msg) \
if(x != nullptr) { \
	int check = (int)((float)(total) / (float)(seg)); \
	if((i + 1) == check) { \
		float per = (float)(i) / (float)(total); \
	JUDGE_PROGRESS_MSG_RETURN(x, per, r, msg) \
	} \
}


#define RESET_PROGRESS_SCOPE(x, v) \
if(x != nullptr) { x->resetScope(v); }

#define RESET_PROGRESS_PERCENT(x, v) \
if(x != nullptr) { x->resetPercentScope(v); }


#define PROGRESS_BREAK_FINISH_CUR_NEXT(x, v, msg) \
JUDGE_PROGRESS_MSG_BREAK(x, 1, msg); \
RESET_PROGRESS_SCOPE(x, v);

#define PROGRESS_RETURN_FINISH_CUR_NEXT(x, v, r, msg) \
JUDGE_PROGRESS_MSG_RETURN(x, 1, r, msg); \
RESET_PROGRESS_SCOPE(x, v);


}

#define SAFE_TRACER(tracer, ...) \
	if(tracer) tracer->formatMessage(__VA_ARGS__)
#endif // TRACER_1630734954343_H