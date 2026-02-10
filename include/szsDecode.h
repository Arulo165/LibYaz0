#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;

inline u32 bswap32(u32 value) {
#if defined(_MSC_VER)
    return _byteswap_ulong(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#else
    
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
#endif
}


inline bool isLittleEndian() {
    const u32 test = 1;
    return *reinterpret_cast<const u8*>(&test) == 1;
}

inline u32 readBigEndian32(const void* ptr) {
    const u8* bytes = static_cast<const u8*>(ptr);
    return (static_cast<u32>(bytes[0]) << 24) |
           (static_cast<u32>(bytes[1]) << 16) |
           (static_cast<u32>(bytes[2]) << 8)  |
           (static_cast<u32>(bytes[3]));
}

class SZSDecompressor
{
public:
    enum Step
    {
        STEP_NORMAL = 0,
        STEP_SHORT,
        STEP_LONG,
    };

    struct DecompContext
    {
        u8*     destp;
        s32     destCount;
        s32     forceDestCount;
        u8      flagMask;
        u8      flags;
        u8      packHigh;
        Step    step;
        u16     lzOffset;
        u8      headerSize;
        
        DecompContext() 
        {
            initialize(nullptr);
        }
        
        void initialize(void* dst) 
        {
            destp           = static_cast<u8*>(dst);
            destCount       = 0;
            forceDestCount  = 0;
            flagMask        = 0;
            flags           = 0;
            packHigh        = 0;
            step            = STEP_NORMAL;
            lzOffset        = 0;
            headerSize      = 0x10;
        }
    };

    static u8* decompress(const u8* src, u32 src_size, u32* out_size = nullptr);
    
   
    static u32 getMagic(const void* header);
    static u32 getDecompSize(const void* header);
    static u32 getDecompAlignment(const void* header);
    
private:
    static s32 readHeader_(DecompContext* context, const u8* srcp, u32 src_size);
    static s32 streamDecomp(DecompContext* context, const void* src, u32 len);
};