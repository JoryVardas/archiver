#include "archive_manager.h"

ArchiveManager::ArchiveManager(
    std::shared_ptr<ArchiveDatabase>& archiveDatabase,
    std::shared_ptr<Config>& config)
    : _archiveDatabase(archiveDatabase), _config(config) {
    std::vector<ArchiveInfo>& singleArchive = _archiveMap["<SINGLE>"];
    singleArchive.push_back({getSingleFileArchive(), 0, true});
};
const Archive& ArchiveManager::getArchiveForFile(const RawFile& file) {
    if (file.size() >= _config->archive.single_archive_size) {
        return _archiveMap["<SINGLE>"][0].archive;
    }

    std::vector<ArchiveInfo>& potentialArchives = _archiveMap[file.extension()];
    if (std::size(potentialArchives) == 0) {
        std::optional<std::pair<Archive, Size>> archiveInfo =
            _archiveDatabase->getArchiveForExtension(file.extension());
        if (archiveInfo) {
            Archive archive = archiveInfo->first;
            archive.path = _config->archive.temp_archive_directory /
                           fmt::format("{}", archive.id);
            potentialArchives.push_back({archive, archiveInfo->second, true});
        }
    }

    auto iter =
        std::find_if(std::begin(potentialArchives), std::end(potentialArchives),
                     [&file, this](const auto& cur) {
                         return cur.canAdd && doesFileFitInArchive(file, cur);
                     });
    if (iter == std::end(potentialArchives)) {
        Archive archive =
            _archiveDatabase->addNewArchiveForExtension(file.extension());
        archive.path = _config->archive.temp_archive_directory /
                       fmt::format("{}", archive.id);
        potentialArchives.push_back({archive, file.size(), true});
        return potentialArchives.at(potentialArchives.size() - 1).archive;
    } else {
        iter->size += file.size();
        return (*iter).archive;
    }
};

bool ArchiveManager::doesFileFitInArchive(
    const RawFile& file, const ArchiveInfo& archiveInfo) const {
    if (archiveInfo.archive == getSingleFileArchive())
        return true;
    return (_config->archive.target_size - archiveInfo.size) >= file.size();
};
Archive ArchiveManager::getSingleFileArchive() const {
    return {1, "<SINGLE>", _config->archive.temp_archive_directory / "1"};
};