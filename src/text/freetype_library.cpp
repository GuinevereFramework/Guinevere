#include <stdexcept>

#include <guinevere/text/freetype_library.hpp>

namespace guinevere::text {

FreeTypeLibrary::FreeTypeLibrary()
{
    if(FT_Init_FreeType(&library_) != 0) {
        library_ = nullptr;
        throw std::runtime_error("FT_Init_FreeType failed");
    }
}

FreeTypeLibrary::FreeTypeLibrary(FreeTypeLibrary&& other) noexcept
    : library_(other.library_)
{
    other.library_ = nullptr;
}

FreeTypeLibrary& FreeTypeLibrary::operator=(FreeTypeLibrary&& other) noexcept
{
    if(this == &other) {
        return *this;
    }

    if(library_ != nullptr) {
        FT_Done_FreeType(library_);
    }

    library_ = other.library_;
    other.library_ = nullptr;

    return *this;
}

FreeTypeLibrary::~FreeTypeLibrary()
{
    if(library_ != nullptr) {
        FT_Done_FreeType(library_);
        library_ = nullptr;
    }
}

FT_Library FreeTypeLibrary::get() const
{
    return library_;
}

} // namespace guinevere::text
