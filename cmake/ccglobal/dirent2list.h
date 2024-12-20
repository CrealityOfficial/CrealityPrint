#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <dirent.h>
#else
#include <io.h>
#endif // __linux__
#include <vector>

/**************************************************************
 * 功能：遍历文件夹下所有文件列表，目前只支持一级目录
 * 输入参数：
 *		strFileFolder：文件夹
 * 输出参数：
 *		vtFilePaths：文件列表
 * ************************************************************/
void getFilePaths(const std::string& strFileFolder, std::vector<std::string>& vtFilePaths)
{
#ifdef __linux__
	DIR* pDir = nullptr;

	if (!(pDir = opendir(strFileFolder.data())))
	{
		std::cout << "file folder doesn't exist!" << std::endl;
		return;
	}

	struct dirent* ptr = nullptr;
	while (0 != (ptr = readdir(pDir)))
	{
		if (0 != strcmp(ptr->d_name, ".") && 0 != strcmp(ptr->d_name, ".."))
		{
			vtFilePaths.emplace_back(strFileFolder + "/" + ptr->d_name);
		}
	}

	closedir(pDir);
#else
	struct _finddata_t fileInfo;
	//std::string strFileInfo = strFileFolder + "/*.jpg"; // ok
	std::string strFileInfo = strFileFolder + "/*.*"; // ok
	//std::string strFileInfo = strFileFolder + "\\*"; // ok
	long long hFd = _findfirst(strFileInfo.data(), &fileInfo);
	if (-1 == hFd)
	{
		_findclose(hFd);
		std::cout << "file folder _findfirst error!" << std::endl;
		return;
	}

	std::string strFilePath = "";
	do
	{
		strFilePath = strFileFolder + "/" + fileInfo.name;
		if (_A_ARCH == fileInfo.attrib)
		{
			vtFilePaths.emplace_back(strFilePath);
		}
	} while (0 == _findnext(hFd, &fileInfo));

	_findclose(hFd);
#endif

	return;
}