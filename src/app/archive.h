#ifndef _ARCHIVE_H
#define _ARCHIVE_H

#include "common.h"

struct Archive {
    ID id;
    Extension extension;
    std::filesystem::path path;

    bool operator==(const Archive& ref) const;
};

#endif