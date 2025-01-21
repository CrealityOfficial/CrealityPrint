#ifndef COMPARE_BASE_H
#define COMPARE_BASE_H
#include "CommonData.h"
#include <string>

class CompareBase
{
public:
    CompareBase() = default;
    CompareBase(CompareOptions compare_option);
    virtual ~CompareBase() = default;

public:
    void set_compare_option(CompareOptions compare_option)
    {
        if (compare_option_ == compare_option_)
            return;
        compare_option_ = compare_option;
    }
    CompareOptions get_compare_option() const { return compare_option_; }

    virtual void compare_files(const std::string& file1, const std::string& file2) = 0;
    virtual void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) = 0;
    bool         get_compare_result() const { return result_; }
    std::string  get_compare_result_str() const { return result_str_; }

protected:
    CompareOptions compare_option_{CompareOptions::COMPARE_OPTION_FULLTEXT};
    void           set_compare_result(bool result)
    {
        if (result == result_)
            return;
        result_ = result;
    }
    void set_compare_result_str(const std::string& result_str) { result_str_ = result_str; }

private:
    bool        result_{false};
    std::string result_str_{};
};
#endif // !COMPARE_BASE_H
