#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

namespace guinevere::text {

class FreeTypeLibrary {
public:
    FreeTypeLibrary();
    FreeTypeLibrary(const FreeTypeLibrary&) = delete;
    FreeTypeLibrary& operator=(const FreeTypeLibrary&) = delete;

    FreeTypeLibrary(FreeTypeLibrary&& other) noexcept;
    FreeTypeLibrary& operator=(FreeTypeLibrary&& other) noexcept;

    ~FreeTypeLibrary();

    FT_Library get() const;

private:
    FT_Library library_ = nullptr;
};

} // namespace guinevere::text
