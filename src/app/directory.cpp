#include "directory.h"

ArchivedDirectory::ArchivedDirectory(ID id, const std::string_view name,
                                     const std::optional<ID>& parentID)
    : _id(id), _parent(parentID), _name(name){};

ID ArchivedDirectory::id() const { return _id; };
const std::string& ArchivedDirectory::name() const { return _name; };
std::optional<ID> ArchivedDirectory::parent() const {
    return isRoot() ? std::nullopt : _parent;
};
bool ArchivedDirectory::isRoot() const { return _id == RootDirectoryID; };
ArchivedDirectory ArchivedDirectory::getRootDirectory() {
    return ArchivedDirectory(RootDirectoryID, RootDirectoryName, std::nullopt);
};

RawDirectory::RawDirectory(const std::filesystem::path& path)
    : _name(path.filename()), _containingPath(path.parent_path()) {
    if (!pathExists(path))
        throw RawDirectoryError(fmt::format(
            "Attempt to open the path \"{}\" which does not exist.", path));
    if (!isDirectory(path))
        throw RawDirectoryError(fmt::format(
            "Attempt to open the path \"{}\" as a raw directory.", path));
};
const std::filesystem::path& RawDirectory::containingPath() const {
    return _containingPath;
};
const std::string& RawDirectory::name() const { return _name; };
std::filesystem::path RawDirectory::fullPath() const {
    return _containingPath / _name;
};