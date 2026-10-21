#pragma once

// Minimal reader for GTA San Andreas "/VER2" IMG archives.
//
// Format (little endian):
//   char     magic[4];          "VER2"
//   uint32_t entryCount;
//   entry[entryCount] {         // 32 bytes each, at offset 8
//       uint32_t offset;        // in 2048-byte sectors
//       uint32_t size;          // sector-aligned size, in sectors
//       char     name[24];      // NUL padded, not necessarily terminated
//   }
//
// The directory is read once. File contents are read on demand.

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace spike {

struct ImgEntry
{
    std::string name;   // lower-cased
    uint64_t    offset; // byte offset of the payload
    uint32_t    size;   // payload bytes (sector count * 2048)
};

class ImgArchive
{
public:
    ImgArchive() = default;
    ~ImgArchive();
    ImgArchive(const ImgArchive&) = delete;
    ImgArchive& operator=(const ImgArchive&) = delete;

    bool Open(const std::string& path, std::string* error);
    void Close();

    // Case-insensitive lookup; returns nullptr when absent.
    const ImgEntry* Find(const std::string& name) const;

    // Reads the whole (sector-aligned) payload; `out` is resized to what was read.
    bool Read(const ImgEntry& entry, std::vector<uint8_t>* out, std::string* error) const;

    size_t Count() const { return m_entries.size(); }

private:
    mutable std::FILE*   m_file = nullptr;
    std::vector<ImgEntry> m_entries;
};

} // namespace spike
