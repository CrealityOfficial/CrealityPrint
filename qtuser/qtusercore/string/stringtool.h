#ifndef _QTUSER_CORE_STRINGTOOL_1589422468670_H
#define _QTUSER_CORE_STRINGTOOL_1589422468670_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QString>
#include <string>
#include <vector>

namespace qtuser_core
{
	QTUSER_CORE_API QString timeString();
	QTUSER_CORE_API QString qstringMd5(const QString& s);

	QTUSER_CORE_API float StrToFloat(const std::string& value);

	//���ַ���ת��Ϊ������
	QTUSER_CORE_API int StrToInt(const std::string& strSrc);

	//���ַ���ת��Ϊbool
	QTUSER_CORE_API bool StrToBool(const std::string& strSrc);



	//������ת��Ϊ�ַ���
	QTUSER_CORE_API std::string float2Str(float floateger);

	//intת��Ϊ�ַ���
	QTUSER_CORE_API std::string int2Str(int num);

	//boolת��Ϊ�ַ���
	QTUSER_CORE_API std::string bool2Str(bool bValue);

	//�����ļ��� SourceFile Դ�ļ�  NewFile Ŀ���ļ�
	QTUSER_CORE_API bool ForCopyFile(const char* SourceFile, const char* NewFile);//

	//�ָ��ַ��� Դ��Src��Ŀ�괮Vctdest  �ָ��c
	QTUSER_CORE_API void SplitString(const std::string& Src, std::vector<std::string>& Vctdest, const std::string& c);

	/* C++��ȥ���ַ�����β��˫���ź��� */
	QTUSER_CORE_API int trim_z(std::string& s);
	/* C��ȥ���ַ�����β��˫���ź���  */
	QTUSER_CORE_API int trim_z(char* s);

	/* C��ȥ���ַ�����(��)�ո���  */
	QTUSER_CORE_API int ltrim_z(char* s);
	/* C��ȥ���ַ���β(��)�ո���  */
	QTUSER_CORE_API int rtrim_z(char* s);

	/* ȥ���Ҳ���(һ��)�ַ�(\n)*/
	QTUSER_CORE_API int rtrim_n_z(char* s, char ch);
	/* ȥ�������(һ��)�ַ�(\n)*/
	QTUSER_CORE_API int ltrim_n_z(char* s, char ch);
	/* ȥ������ס��Ҳ���(һ��)�ַ�(\n)*/
	QTUSER_CORE_API int trim_n_z(char* s, char ch);

	/*  �ָ��ַ���
	@pch ��Ҫ�ָ���ַ���
	@sp �ָ���ţ��磺", : _ -"��
	@return vector<char*>
	���ӣ�
	pch="����������"; sp=":"
	vector<char*> vec;
	vec[0] = "����";
	vec[1] = "����";*/
	QTUSER_CORE_API std::vector<char*> split_z(char* pch, char* sp);
}
#endif // _QTUSER_CORE_STRINGTOOL_1589422468670_H
