#include <sarcReader.h>
#include <cstdio>
#include <cstring>

u16 SARCReader::read16(const void* ptr) const {
    const u8* bytes = (const u8*)ptr;
    if (is_big_endian)
        return (bytes[0] << 8) | bytes[1];
    else
        return bytes[0] | (bytes[1] << 8);
}

u32 SARCReader::read32(const void* ptr) const {
    const u8* bytes = (const u8*)ptr;
    if (is_big_endian)
        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    else
        return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

u32 SARCReader::hashFilename(const char* name, u32 hash_key) const {
    u32 result = 0;
    for (u32 i = 0; name[i]; i++)
        result = result * hash_key + name[i];
    return result;
}

bool SARCReader::open(const u8* data) {
    archive_data = data;
    
    printf("=== SARC Debug Info ===\n");
    
    const SARCHeader* sarc = (const SARCHeader*)data;
    
    printf("SARC Magic: %c%c%c%c\n", sarc->magic[0], sarc->magic[1], 
           sarc->magic[2], sarc->magic[3]);
    
    if (memcmp(sarc->magic, "SARC", 4) != 0) {
        printf("Error: Not a SARC archive\n");
        return false;
    }
    
    u16 byte_order_raw = (sarc->byte_order_bytes[0] << 8) | sarc->byte_order_bytes[1];
    is_big_endian = (byte_order_raw == 0xFEFF);
    
    printf("Byte order raw: 0x%04X (%s endian)\n", byte_order_raw,
           is_big_endian ? "big" : "little");
    
    u16 header_size = read16(&sarc->header_size);
    u32 file_size = read32(&sarc->file_size);
    u32 data_offset = read32(&sarc->data_offset);
    
    
    const u8* sfat_ptr = data + header_size;
    
    printf("\nBytes at offset 0x%X (SFAT expected):\n", header_size);
    for (int i = 0; i < 16; i++) {
        printf("%02X ", sfat_ptr[i]);
    }
    printf("\n");
    printf("As string: %c%c%c%c\n", sfat_ptr[0], sfat_ptr[1], 
           sfat_ptr[2], sfat_ptr[3]);
    
    const SFATHeader* sfat = (const SFATHeader*)sfat_ptr;
    
    if (memcmp(sfat->magic, "SFAT", 4) != 0) {
        printf("Error: Missing SFAT block\n");
        return false;
    }
    
    u16 sfat_header_size = read16(&sfat->header_size);
    file_count = read16(&sfat->file_count);
    u32 hash_key = read32(&sfat->hash_key);
    
    printf("SFAT header_size: %u (0x%X)\n", sfat_header_size, sfat_header_size);
    printf("SFAT file_count: %u\n", file_count);
    printf("SFAT hash_key: 0x%08X\n", hash_key);
    
    fat_entries = (const SFATEntry*)(sfat_ptr + sfat_header_size);
    
    const u8* sfnt_ptr = sfat_ptr + sfat_header_size + file_count * sizeof(SFATEntry);
    
    printf("\nBytes at SFNT location (offset 0x%lX):\n", 
           (unsigned long)(sfnt_ptr - data));
    for (int i = 0; i < 16; i++) {
        printf("%02X ", sfnt_ptr[i]);
    }
    printf("\n");
    printf("As string: %c%c%c%c\n", sfnt_ptr[0], sfnt_ptr[1], 
           sfnt_ptr[2], sfnt_ptr[3]);
    
    const SFNTHeader* sfnt = (const SFNTHeader*)sfnt_ptr;
    if (memcmp(sfnt->magic, "SFNT", 4) != 0) {
        printf("Error: Missing SFNT block\n");
        return false;
    }
    
    u16 sfnt_header_size = read16(&sfnt->header_size);
    printf("SFNT header_size: %u (0x%X)\n", sfnt_header_size, sfnt_header_size);
    
    name_table = (const char*)(sfnt_ptr + sfnt_header_size);
    data_block = data + data_offset;
    
    printf("\n=== SARC Parsed Successfully ===\n\n");
    
    return true;
}

std::vector<SARCReader::FileEntry> SARCReader::listFiles() const {
    std::vector<FileEntry> files;
    
    for (u16 i = 0; i < file_count; i++) {
        const SFATEntry* entry = &fat_entries[i];
        
        u32 name_offset_raw = read32(&entry->name_offset);
        u32 name_offset = (name_offset_raw & 0x00FFFFFF) * 4;
        
        const char* filename = name_table + name_offset;
        
        u32 data_start = read32(&entry->data_start);
        u32 data_end = read32(&entry->data_end);
        
        FileEntry file;
        file.name = filename;
        file.data = data_block + data_start;
        file.size = data_end - data_start;
        
        files.push_back(file);
    }
    
    return files;
}

const u8* SARCReader::getFile(const char* path, u32* out_size) const {
    for (u16 i = 0; i < file_count; i++) {
        const SFATEntry* entry = &fat_entries[i];
        
        u32 name_offset_raw = read32(&entry->name_offset);
        u32 name_offset = (name_offset_raw & 0x00FFFFFF) * 4;
        
        const char* filename = name_table + name_offset;
        
        if (strcmp(filename, path) == 0) {
            u32 data_start = read32(&entry->data_start);
            u32 data_end = read32(&entry->data_end);
            
            if (out_size)
                *out_size = data_end - data_start;
            
            return data_block + data_start;
        }
    }
    
    return nullptr;
}