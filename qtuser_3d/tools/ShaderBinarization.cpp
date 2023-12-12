#include <iostream>
#include <string>

#include <stdio.h>
#include <vector>

#include "buildinfo.h"

#if _WIN32
#include <io.h>

#include <fstream>
#include <sstream>
#include <set>
#include <map>

#include <Windows.h>

std::wstring convert(const char* name)
{
	size_t len = strlen(name) + 1;
	size_t converted = 0;
	wchar_t* wstr = (wchar_t*)malloc(len * sizeof(wchar_t));
	mbstowcs_s(&converted, wstr, len, name, _TRUNCATE);
	std::wstring result(wstr);
	free(wstr);
	return result;
}

std::string convert(const wchar_t* name, char dfault = '?',
	const std::locale& loc = std::locale())
{
	std::string file;

	std::ostringstream stm;

	const wchar_t* s = name;
	while (*s != L'\0') {
		stm << std::use_facet< std::ctype<wchar_t> >(loc).narrow(*s++, dfault);
	}
	return stm.str();
}

struct file_info
{
	std::string name;
	std::wstring path;
	std::string ext;
	int index;
	std::string var;
};

void get_files(const char* path, std::vector<file_info>& files)
{
	char p[1024];
	strcpy(p, path);
	strcat(p, "/*.*");

	intptr_t hfile = 0;
	_finddata64i32_t file_data;

	std::wstring wpath = convert(path);
	if((hfile = _findfirst(p, &file_data)) != -1) 
	{
		do
		{
			if(file_data.attrib & _A_SUBDIR)
			{
			}else
			{
				file_info file;

				std::wstring name = convert(file_data.name);
				size_t pos = name.rfind(L'.');
				if(pos != std::wstring::npos)
				{
					file.ext = std::string(name.begin() + pos + 1, name.end());
					file.name = std::string(name.begin(), name.begin() + pos);
					file.path = wpath + L"/" + name;
				}
				if(file.ext == "vert" || file.ext == "frag" || file.ext == "fragment" || file.ext == "geom"
					|| file.ext == "tes" || file.ext == "tcs")
					files.push_back(file);
			}
		}while(_findnext(hfile, &file_data) == 0);
		_findclose(hfile);
	}
}

bool write_shader_string(const std::string& shader_string_file, std::vector<file_info>& files)
{
	std::ofstream out;
	out.open(shader_string_file, std::ios::out | std::ios::trunc);
	if(!out.is_open())
	{
		out.close();
		return false;
	}

	int size = (int)files.size();
	out << "//------------ code\n";
	for(size_t i = 0; i < size; ++i)
	{
		file_info& info = files[i];
		info.index = (int)i;
		std::ifstream in;
		in.open(info.path, std::ios::in);
		if(in.is_open())
		{
			out <<"const char* "
				<<info.name.c_str()
				<<"_"
				<<info.ext.c_str()
				<<" = ""\n";

			info.var = info.name + "_" + info.ext;

			char line[256];
			while(in.getline(line, 256))
			{
				out<<"                             \""
					<<line;

				out<<"\\n"
					<<"\"\n";
			}
			out<<""";\n";
		}
		in.close();
	}

	out << "//-----------code array.\n";
	out << "const char* shader_code_array[" << size << "] = {\n";
	for (size_t i = 0; i < size; ++i)
	{
		file_info& info = files[i];
		out << "	" << info.var;
		if (i < size - 1) out << ",";
		out << "\n";
	}
	out << "};\n";

	out << "//-------------programs!\n";
	struct program_def
	{
		file_info* vert;
		file_info* tcs;
		file_info* tes;
		file_info* geom;
		file_info* frag;
	};

	auto f = [](program_def& def, file_info& info){
		if(info.ext == "vert") def.vert = &info;
		if (info.ext == "tcs") def.tcs = &info;
		if (info.ext == "tes") def.tes = &info;
		if(info.ext == "geom") def.geom = &info;
		if(info.ext == "frag" || info.ext == "fragment") def.frag = &info;
	};
	std::map<std::string, program_def> programs;
	for(size_t i = 0; i < files.size(); ++i)
	{
		file_info& info = files[i];
		std::map<std::string, program_def>::iterator it = programs.find(info.name);
		if(it != programs.end())
		{
			f((*it).second, info);
		}else
		{
			program_def def;
			def.frag = 0;
			def.vert = 0;
			def.geom = 0;
			def.tcs = 0;
			def.tes = 0;
			std::pair<std::map<std::string, program_def>::iterator, bool> result = programs.insert(std::pair<std::string, program_def>(info.name, def));
			if(result.second)
				f((*(result.first)).second, info);
		}
	}

	auto g = [&out](file_info* info){
		if(info)
		{
			out << info->index;
		}else
			out << " -1 ";
	};

	out << "struct ProgramDef\n";
	out << "{\n";
	out << "	const char* name;\n";
	out << "	int vIndex;\n";
	out << "    int tcsIndex;\n";
	out << "    int tesIndex;\n";
	out << "	int gIndex;\n";
	out << "	int fIndex;\n";
	out << "};\n";
	out << "int programs_meta_size = " << programs.size() << ";\n";
	out << "ProgramDef programs_meta[" << programs.size() << "] = {\n";
	for(std::map<std::string, program_def>::iterator it = programs.begin(); it != programs.end(); ++it)
	{
		program_def& def = (*it).second;
		out <<"  { \""<<(*it).first.c_str()<<"\" , ";
		g(def.vert);
		out <<"  , ";
		g(def.tcs);
		out << "  , ";
		g(def.tes);
		out << "  , ";
		g(def.geom);
		out <<"  , ";
		g(def.frag);
		out<<"  },\n ";
	}
	out << "};\n";
	out.close();
	return true;
}
/// <summary>
/// 
/// </summary>
/// <param name="shader_string_file">д��Դ</param>
/// <param name="gles_files">gles����</param>
/// <param name="gl_files">gl����</param>
/// <returns></returns>
bool write_shader_string_gl_gles(const std::string& shader_string_file, std::vector<file_info>& gles_files, std::vector<file_info>& gl_files)
{
	std::ofstream out;
	out.open(shader_string_file, std::ios::out | std::ios::trunc);
	if (!out.is_open())
	{
		out.close();
		return false;
	}

	

	/// <summary>
	/// ---- start gles code
	/// </summary>
	/// 
	int gles_size = (int)gles_files.size();
	out << "//------------ gles code\n";
	for (size_t i = 0; i < gles_size; ++i)
	{
		file_info& info = gles_files[i];
		info.index = (int)i;
		std::ifstream in;
		in.open(info.path, std::ios::in);
		if (in.is_open())
		{
			out << "const char* "
				<< "gles_"
				<< info.name.c_str()
				<< "_"
				<< info.ext.c_str()
				<< " = ""\n";

			info.var = info.name + "_" + info.ext;

			char line[256];
			while (in.getline(line, 256))
			{
				out << "                             \""
					<< line;

				out << "\\n"
					<< "\"\n";
			}
			out << """;\n";
		}
		in.close();
	}

	out << "//-----------gles code array.\n";
	out << "const char* " << "gles_" << "shader_code_array[" << gles_size << "] = {\n";
	for (size_t i = 0; i < gles_size; ++i)
	{
		file_info& info = gles_files[i];
		out << "	" << "gles_" << info.var;
		if (i < gles_size - 1) out << ",";
		out << "\n";
	}
	out << "};\n";

	/// ---end gles code

	/// -- start gl code
	int size = (int)gl_files.size();
	out << "//------------ gl3.0 code\n";
	for (size_t i = 0; i < size; ++i)
	{
		file_info& info = gl_files[i];
		info.index = (int)i;
		std::ifstream in;
		in.open(info.path, std::ios::in);
		if (in.is_open())
		{
			out << "const char* "
				<< info.name.c_str()
				<< "_"
				<< info.ext.c_str()
				<< " = ""\n";

			info.var = info.name + "_" + info.ext;

			char line[256];
			while (in.getline(line, 256))
			{
				out << "                             \""
					<< line;

				out << "\\n"
					<< "\"\n";
			}
			out << """;\n";
		}
		in.close();
	}

	out << "//-----------gl3.0 code array.\n";
	out << "const char* shader_code_array[" << size << "] = {\n";
	for (size_t i = 0; i < size; ++i)
	{
		file_info& info = gl_files[i];
		out << "	"  << info.var;
		if (i < size - 1) out << ",";
		out << "\n";
	}
	out << "};\n";
	/// ---end gl code
	/// --start common struct
	out << "//-------------programs!\n";
	struct program_def
	{
		file_info* vert;
		file_info* tcs;
		file_info* tes;
		file_info* geom;
		file_info* frag;
	};

	auto f = [](program_def& def, file_info& info) {
		if (info.ext == "vert") def.vert = &info;
		if (info.ext == "tcs") def.tcs = &info;
		if (info.ext == "tes") def.tes = &info;
		if (info.ext == "geom") def.geom = &info;
		if (info.ext == "frag" || info.ext == "fragment") def.frag = &info;
	};
	std::map<std::string, program_def> programs;
	for (size_t i = 0; i < gl_files.size(); ++i)
	{
		file_info& info = gl_files[i];
		std::map<std::string, program_def>::iterator it = programs.find(info.name);
		if (it != programs.end())
		{
			f((*it).second, info);
		}
		else
		{
			program_def def;
			def.frag = 0;
			def.vert = 0;
			def.geom = 0;
			def.tcs = 0;
			def.tes = 0;
			std::pair<std::map<std::string, program_def>::iterator, bool> result = programs.insert(std::pair<std::string, program_def>(info.name, def));
			if (result.second)
				f((*(result.first)).second, info);
		}
	}

	auto g = [&out](file_info* info) {
		if (info)
		{
			out << info->index;
		}
		else
			out << " -1 ";
	};

	out << "struct ProgramDef\n";
	out << "{\n";
	out << "	const char* name;\n";
	out << "	int vIndex;\n";
	out << "    int tcsIndex;\n";
	out << "    int tesIndex;\n";
	out << "	int gIndex;\n";
	out << "	int fIndex;\n";
	out << "};\n";
	out << "int programs_meta_size = " << programs.size() << ";\n";
	out << "ProgramDef programs_meta[" << programs.size() << "] = {\n";
	for (std::map<std::string, program_def>::iterator it = programs.begin(); it != programs.end(); ++it)
	{
		program_def& def = (*it).second;
		out << "  { \"" << (*it).first.c_str() << "\" , ";
		g(def.vert);
		out << "  , ";
		g(def.tcs);
		out << "  , ";
		g(def.tes);
		out << "  , ";
		g(def.geom);
		out << "  , ";
		g(def.frag);
		out << "  },\n ";
	}
	out << "};\n";
	/// --end common struct
	out.close();
	return true;
}

bool Check(FILETIME t1, FILETIME t2)
{
	ULARGE_INTEGER uli1;
	ULARGE_INTEGER uli2;

	uli1.LowPart = t1.dwLowDateTime;
	uli1.HighPart = t1.dwHighDateTime;
	uli2.LowPart = t2.dwLowDateTime;
	uli2.HighPart = t2.dwHighDateTime;

	return uli1.QuadPart > uli2.QuadPart;
}

#endif // _WIN32

#ifdef INVOKE_BINARY
int invoke_shader_binary(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	std::string inputs[2] = {
		"gl/3.3/",
		"gles/2/"
	};

	std::string outputs[2] = {
		"GL3.code",
		"GLES2.code"
	};
		 
	std::string inputDirRoot = std::string(CMAKE_MODULE) + "/../shader_entity/shaders/";
	std::string outputDirRoot = std::string(CMAKE_MODULE) + "/../shader_entity/shaders/";
	if (argc >= 2)
		inputDirRoot = argv[1];
	if (argc >= 3)
		outputDirRoot = argv[2];

	//for (int i = 0; i < 2; ++i)
	//{
	//	std::string inputDir = inputDirRoot + inputs[i];
	//	std::string outputFile = outputDirRoot + outputs[i];

	//	std::vector<file_info> files;
	//	get_files(inputDir.c_str(), files);

	//	write_shader_string(outputFile, files);
	//}
	std::string inputDirGL = inputDirRoot + inputs[0];
	std::string inputDirGLES = inputDirRoot + inputs[1];

#if _WIN32
	std::vector<file_info> gl_files;
	get_files(inputDirGL.c_str(), gl_files);

	std::vector<file_info> gles_files;
	get_files(inputDirGLES.c_str(), gles_files);

	std::string outputFile = outputDirRoot + "GL.code";
	write_shader_string_gl_gles(outputFile, gles_files, gl_files);
	return EXIT_SUCCESS;

#else
	return 0;
#endif
}
