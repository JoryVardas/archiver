#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include "common.h"

struct ArchivedDirectory {
    ArchivedDirectory(ID id, const std::string_view name,
                      const std::optional<ID>& parentID);

    ID id() const;
    const std::string& name() const;
    std::optional<ID> parent() const;
    bool isRoot() const;

    static ArchivedDirectory getRootDirectory();

  private:
    ID _id;
    std::optional<ID> _parent;
    std::string _name;
    static constexpr ID RootDirectoryID = 1;
    static constexpr std::string_view RootDirectoryName = "/";
};

_make_exception_(RawDirectoryError);
struct RawDirectory {
    explicit RawDirectory(const std::filesystem::path& path);

    const std::filesystem::path& containingPath() const;
    const std::string& name() const;
    std::filesystem::path fullPath() const;

  private:
    std::string _name;
    std::filesystem::path _containingPath;
};

#endif