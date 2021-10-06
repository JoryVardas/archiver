#ifndef ARCHIVER_DIRECTORY_DATABASE_HPP
#define ARCHIVER_DIRECTORY_DATABASE_HPP

#include "../app/file.h"
#include <string_view>
_make_exception_(FileDatabaseError);

class FileDatabase {
  public:
    virtual FileRevision
    getMostRecentRevision(const ArchivedFile& file) const abstract;
    virtual ArchivedFile addNewFile(const ArchivedDirectory& parentDirectory,
                                    const std::string& name) abstract;
    virtual FileRevision addNewRevision(const ArchivedFile& file,
                                        const FileInfo& fileInfo,
                                        const Archive& archive) abstract;
    virtual std::optional<ArchivedFile>
    getFile(const ArchivedDirectory& parentDirectory,
            const std::string& name) const abstract;
    virtual std::optional<FileRevision>
    findRevision(const FileInfo& fileInfo) const abstract;
    virtual void
    addDuplicateRevision(const ArchivedFile& file,
                         const FileRevision& duplicateRevision) abstract;
    virtual std::optional<FileRevision>
    getFileRevisionMatchingInfo(const FileInfo& fileInfo) const abstract;
};

#endif
