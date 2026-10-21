#include "img.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace spike {

namespace {

constexpr uint32_t kSectorSize   = 2048;
constexpr uint32_t kEntrySize    = 32;
constexpr uint32_t kHeaderSize   = 8;
constexpr uint32_t kMaxEntries   = 200000; // sanity bound for the directory

std::string ToLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

} // namespace

ImgArchive::~ImgArchive()
{
    Close();
}

void ImgArchive::Close()
{
    if (m_file) {
        std::fclose(m_file);
        m_file = nullptr;
    }
    m_entries.clear();
}

bool ImgArchive::Open(const std::string& path, std::string* error)
{
    Close();

    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        if (error) *error = "cannot open " + path;
        return false;
    }

    char     magic[4];
    uint32_t count = 0;
    if (std::fread(magic, 1, 4, f) != 4 || std::memcmp(magic, "VER2", 4) != 0) {
        if (error) *error = path + ": not a VER2 IMG archive";
        std::fclose(f);
        return false;
    }
    if (std::fread(&count, 1, 4, f) != 4 || count == 0 || count > kMaxEntries) {
        if (error) *error = path + ": implausible entry count";
        std::fclose(f);
        return false;
    }

    std::vector<uint8_t> dir(static_cast<size_t>(count) * kEntrySize);
    if (std::fread(dir.data(), 1, dir.size(), f) != dir.size()) {
        if (error) *error = path + ": truncated directory";
        std::fclose(f);
        return false;
    }

    if (std::fseek(f, 0, SEEK_END) != 0) {
        if (error) *error = path + ": seek failed";
        std::fclose(f);
        return false;
    }
    const uint64_t fileSize = static_cast<uint64_t>(std::ftell(f));

    m_entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* raw = &dir[static_cast<size_t>(i) * kEntrySize];
        uint32_t       offsetSectors, sizeSectors;
        std::memcpy(&offsetSectors, raw + 0, 4);
        std::memcpy(&sizeSectors, raw + 4, 4);

        char nameBuf[25] = {};
        std::memcpy(nameBuf, raw + 8, 24);
        if (nameBuf[0] == '\0')
            continue;

        ImgEntry e;
        e.name   = ToLower(nameBuf);
        e.offset = static_cast<uint64_t>(offsetSectors) * kSectorSize;
        e.size   = sizeSectors * kSectorSize;

        if (e.offset + e.size > fileSize) {
            if (error) *error = path + ": entry '" + e.name + "' outside file";
            std::fclose(f);
            m_entries.clear();
            return false;
        }
        m_entries.push_back(std::move(e));
    }

    m_file = f;
    return true;
}

const ImgEntry* ImgArchive::Find(const std::string& name) const
{
    const std::string key = ToLower(name);
    for (const ImgEntry& e : m_entries)
        if (e.name == key)
            return &e;
    return nullptr;
}

bool ImgArchive::Read(const ImgEntry& entry, std::vector<uint8_t>* out, std::string* error) const
{
    if (!m_file) {
        if (error) *error = "archive is not open";
        return false;
    }
    if (std::fseek(m_file, static_cast<long>(entry.offset), SEEK_SET) != 0) {
        if (error) *error = entry.name + ": seek failed";
        return false;
    }

    out->resize(entry.size);
    const size_t got = std::fread(out->data(), 1, entry.size, m_file);
    out->resize(got);

    if (got != entry.size) {
        if (error) *error = entry.name + ": short read";
        return false;
    }
    return true;
}

} // namespace spike
