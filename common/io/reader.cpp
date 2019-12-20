#include <common/io/reader.hpp>
#include <common/error.hpp>

Reader::Reader()
{
}

Reader::~Reader()
{
}

bool Reader::IsOpen()
{
}

int64_t Reader::Tellg()
{
}

void Reader::Skip(int64_t size)
{
}

int64_t Reader::Lseek(int64_t offset)
{
}

int64_t Reader::FileSize()
{
}

int Reader::Read(void *buf, size_t size, ssize_t *nread)
{
}