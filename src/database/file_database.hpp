#ifndef ARCHIVER_DIRECTORY_DATABASE_HPP
#define ARCHIVER_DIRECTORY_DATABASE_HPP

#include "../app/file.h"
#include <string_view>

class FileDatabase {
  public:
    virtual auto getMostRecentRevision(const ArchivedFile& file) const
        -> FileRevision abstract;
    virtual auto addNewFile(const ArchivedDirectory& parentDirectory,
                            const std::string_view name)
        -> ArchivedFile abstract;
    virtual auto addNewRevision(const ArchivedFile& file,
                                const FileInfo& fileInfo,
                                const Archive& archive)
        -> FileRevision abstract;
    virtual auto getFile(const ArchivedDirectory& parentDirectory,
                         const std::string_view& name) const
        -> std::optional<ArchivedFile> abstract;
    virtual auto findRevision(const FileInfo& fileInfo) const
        -> std::optional<FileRevision> abstract;
    virtual void
    addDuplicateRevision(const ArchivedFile& file,
                         const FileRevision& duplicateRevision) abstract;
    virtual auto getFileRevisionMatchingInfo(const FileInfo& fileInfo) const
        -> std::optional<FileRevision> abstract;
};

#endif
