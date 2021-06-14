#include "file_manager.h"

FileManager::FileManager(std::shared_ptr<FileDatabase>& fileDatabase,
                         std::shared_ptr<Config>& config)
    : _fileDatabase(fileDatabase), _config(config){};

std::optional<FileRevision>
FileManager::getMostRecentRevision(const ArchivedFile& file) const {
    return _fileDatabase->getMostRecentRevision(file);
};

void FileManager::copyFileToArchive(const RawFile& file,
                                    const FileRevision& revision) const {
    std::filesystem::copy_file(file.getFullPath(),
                               revision.containingArchive().path /
                                   revision.getName());
};
void FileManager::copyFileFromArchive(
    const ArchivedFile& file, const FileRevision& revision,
    const RawDirectory& outputDirection) const {
    std::filesystem::copy_file(revision.containingArchive().path /
                                   revision.getName(),
                               outputDirection.fullPath() / file.name());
};
std::optional<FileRevision>
FileManager::getFileRevisionMatchingInfo(const FileInfo& fileInfo) const {
    return _fileDatabase->getFileRevisionMatchingInfo(fileInfo);
};
ArchivedFile
FileManager::addFileIfNew(const RawFile& file,
                          const ArchivedDirectory& parentDirectory) {
    auto archivedFile = _fileDatabase->getFile(parentDirectory, file.name());
    if (!archivedFile) {
        return _fileDatabase->addNewFile(parentDirectory, file.name());
    } else
        return *archivedFile;
};

void FileManager::addDuplicateFileRevision(
    const ArchivedFile& file, const FileRevision& duplicateRevision) {
    _fileDatabase->addDuplicateRevision(file, duplicateRevision);
};
FileRevision FileManager::addNewFileRevision(const ArchivedFile& file,
                                             const FileInfo& fileInfo,
                                             const Archive& archive) {
    return _fileDatabase->addNewRevision(file, fileInfo, archive);
}