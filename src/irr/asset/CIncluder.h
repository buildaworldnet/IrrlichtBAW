#ifndef __IRR_C_INCLUDER_H_INCLUDED__
#define __IRR_C_INCLUDER_H_INCLUDED__

#include "irr/asset/IIncluder.h"
#include "IFileSystem.h"

namespace irr { namespace asset
{

class CIncluder : public IIncluder
{
public:
    CIncluder(io::IFileSystem* _fs) : m_filesystem{_fs}
    {
    }

    void addSearchDirectory(const std::string& _searchDir) override
    {
        io::path absPath = m_filesystem->getAbsolutePath(_searchDir.c_str());
        IIncluder::addSearchDirectory(absPath.c_str());
    }

    std::string getInclude_internal(const std::string& _path) const override
    {
        auto f = core::make_smart_refctd_ptr<io::IReadFile>(m_filesystem->createAndOpenFile(_path.c_str()), core::dont_grab);
        if (!f)
            return {};
        std::string contents(f->getSize(), '\0');
        f->read(&contents.front(), f->getSize());

        return contents;
    }

private:
    io::IFileSystem* m_filesystem;
};

}}

#endif//__IRR_C_INCLUDER_H_INCLUDED__