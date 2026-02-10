#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;

class SARCReader
{
public:
    struct FileEntry {
        std::string name;
        const u8* data;
        u32 size;
    };

private:
    struct SARCHeader {
        char magic[4];      // "SARC"
        u16 header_size;
        u8 byte_order_bytes[2];  // [0xFE, 0xFF] = Big-Endian, [0xFF, 0xFE] = Little-Endian
        u32 file_size;
        u32 data_offset;
        u16 version;
        u16 reserved;
    } __attribute__((packed));

    struct SFATHeader {
        char magic[4];      // "SFAT"
        u16 header_size;
        u16 file_count;
        u32 hash_key;
    } __attribute__((packed));

    struct SFATEntry {
        u32 hash;
        u32 name_offset;    // Top 8 bits = collision, rest = offset/4
        u32 data_start;
        u32 data_end;
    } __attribute__((packed));

    struct SFNTHeader {
        char magic[4];      // "SFNT"
        u16 header_size;
        u16 reserved;
    } __attribute__((packed));

    const u8* archive_data;
    bool is_big_endian;
    u16 file_count;
    const SFATEntry* fat_entries;
    const char* name_table;
    const u8* data_block;

public:
    SARCReader() : archive_data(nullptr), file_count(0) {}

    bool open(const u8* data);
    std::vector<FileEntry> listFiles() const;
    const u8* getFile(const char* path, u32* out_size = nullptr) const;

private:
    u16 read16(const void* ptr) const;
    u32 read32(const void* ptr) const;
    u32 hashFilename(const char* name, u32 hash_key) const;
};