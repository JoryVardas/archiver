#include "directory_manager.h"

DirectoryManager::DirectoryManager(
    std::shared_ptr<DirectoryDatabase>& directoryDatabase,
    std::shared_ptr<Config>& config)
    : _directoryDatabase(directoryDatabase), _config(config){};

std::optional<ArchivedDirectory>
DirectoryManager::getDirectory(const RawDirectory& directory) {
    std::filesystem::path fullPath = directory.fullPath();
    std::optional<ArchivedDirectory> cur =
        ArchivedDirectory::getRootDirectory();

    std::for_each(
        std::begin(fullPath), std::end(fullPath),
        [&cur, &_directoryDatabase = _directoryDatabase](const auto& part) {
            if (cur) {
                cur = _directoryDatabase->getDirectory(cur->id(), part);
            }
        });

    return cur;
};

ArchivedDirectory
DirectoryManager::addDirectoryIfNew(const RawDirectory& directory) {
    std::filesystem::path fullPath = directory.fullPath();
    ArchivedDirectory cur = ArchivedDirectory::getRootDirectory();

    std::for_each(
        std::begin(fullPath), std::end(fullPath),
        [&cur, &_directoryDatabase = _directoryDatabase](const auto& part) {
            std::optional<ArchivedDirectory> existingDirectory =
                _directoryDatabase->getDirectory(cur.id(), part);
            if (existingDirectory)
                cur = *existingDirectory;
            else
                cur = _directoryDatabase->addNewDirectory(cur.id(), part);
        });

    return cur;
}