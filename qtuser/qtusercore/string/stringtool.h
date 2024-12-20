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

	//将字符串转化为浮点数
	QTUSER_CORE_API int StrToInt(const std::string& strSrc);

	//将字符串转化为bool
	QTUSER_CORE_API bool StrToBool(const std::string& strSrc);



	//浮点数转化为字符串
	QTUSER_CORE_API std::string float2Str(float floateger);

	//int转化为字符串
	QTUSER_CORE_API std::string int2Str(int num);

	//bool转化为字符串
	QTUSER_CORE_API std::string bool2Str(bool bValue);

	//复制文件： SourceFile 源文件  NewFile 目标文件
	QTUSER_CORE_API bool ForCopyFile(const char* SourceFile, const char* NewFile);//

	//分割字符串 源串Src，目标串Vctdest  分割符c
	QTUSER_CORE_API void SplitString(const std::string& Src, std::vector<std::string>& Vctdest, const std::string& c);

	/* C++：去掉字符串首尾的双引号函数 */
	QTUSER_CORE_API int trim_z(std::string& s);
	/* C：去掉字符串首尾的双引号函数  */
	QTUSER_CORE_API int trim_z(char* s);

	/* C：去掉字符串首(左)空格函数  */
	QTUSER_CORE_API int ltrim_z(char* s);
	/* C：去掉字符串尾(右)空格函数  */
	QTUSER_CORE_API int rtrim_z(char* s);

	/* 去掉右侧首(一个)字符(\n)*/
	QTUSER_CORE_API int rtrim_n_z(char* s, char ch);
	/* 去掉左侧首(一个)字符(\n)*/
	QTUSER_CORE_API int ltrim_n_z(char* s, char ch);
	/* 去掉左侧首、右侧首(一个)字符(\n)*/
	QTUSER_CORE_API int trim_n_z(char* s, char ch);

	/*  分割字符串
	@pch 需要分割的字符串
	@sp 分割符号，如：", : _ -"等
	@return vector<char*>
	例子：
	pch="姓名：张三"; sp=":"
	vector<char*> vec;
	vec[0] = "姓名";
	vec[1] = "张三";*/
	QTUSER_CORE_API std::vector<char*> split_z(char* pch, char* sp);
}
#endif // _QTUSER_CORE_STRINGTOOL_1589422468670_H
