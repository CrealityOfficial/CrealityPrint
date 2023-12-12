#ifndef COMMON_TESTHELPER_1622639863964_H
#define COMMON_TESTHELPER_1622639863964_H
#include "gtest/gtest.h"
#include "ccglobal/tracer.h"
#include "ccglobal/log.h"

#include <boost/filesystem.hpp>

std::vector<std::string> gtest_get_files_from_environment(const char* envName)
{
	std::vector<std::string> files;
	char* env = getenv(envName);
	if (env)
	{
        std::cout << "envName " << envName << ": " << env << std::endl;
        namespace fs = boost::filesystem;
        fs::path fullpath(env);
        fs::recursive_directory_iterator end_iter;
        for (fs::recursive_directory_iterator iter(fullpath); iter != end_iter; iter++)
        {
            try
            {
                if (fs::is_directory(*iter))
                {
                }
                else
                {
                    std::string file = iter->path().string();
                    files.push_back(iter->path().string());
                }
            }
            catch (const std::exception& ex)
            {
                std::cerr << ex.what() << std::endl;
                continue;
            }
        }
	}
	else
	{
		std::cout << "envName " << envName << " not exist." << std::endl;
	}
	return files;
}

class GTestTracer : public ccglobal::Tracer
{
public:
	GTestTracer() {}
	virtual ~GTestTracer() {}
	void progress(float r) override {}
	bool interrupt() override { return false; }

	void message(const char* msg) override {
		LOGI("	%s", msg);
	}
	void failed(const char* msg) override {
        LOGI("load failed for %s", msg);
    }
	void success() override {
        LOGI("load success.");
    }
};


#endif // COMMON_TESTHELPER_1622639863964_H