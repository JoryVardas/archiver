#ifndef _ARCHIVE_H
#define _ARCHIVE_H

#include "common.h"
#include <compare>

using ArchiveID = ID;

struct Archive {
  ArchiveID id;
  Extension extension;

  friend auto operator<=>(const Archive&, const Archive&) = default;
};

#endif