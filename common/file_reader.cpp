#include <common/file_reader.hpp>
#include <common/error.hpp>

FileReader::FileReader() : fd_(-1)
{
}

FileReader::~FileReader()
{
    Close();
}

int32_t FileReader::Open(const std::string &p)
{
    int32_t ret = ERROR_SUCCESS;

    if (fd_ > 0)
    {
        return ERROR_SYSTEM_FILE_ALREADY_OPENED;
    }

    return ret;
}