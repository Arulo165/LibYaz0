#include <szsDecode.h>
#include <cstdio>

u32 SZSDecompressor::getMagic(const void* header)
{
    return readBigEndian32(header);
}

u32 SZSDecompressor::getDecompAlignment(const void* header)
{
    const u8* ptr = static_cast<const u8*>(header);
    return readBigEndian32(ptr + 8);
}

u32 SZSDecompressor::getDecompSize(const void* header)
{
    const u8* ptr = static_cast<const u8*>(header);
    return readBigEndian32(ptr + 4);
}

u8* SZSDecompressor::decompress(const u8* src, u32 src_size, u32* out_size)
{
    if (!src || src_size < 0x10)
        return nullptr;
    
    if (getMagic(src) != 0x59617A30) {
        printf("Error: Not a valid SZS file (missing Yaz0 header)\n");
        return nullptr;
    }
    
    u32 decomp_size = getDecompSize(src);
    if (decomp_size == 0) {
        printf("Error: Invalid decompressed size\n");
        return nullptr;
    }
    
    // Buffer allokieren
    u8* dst = (u8*)malloc(decomp_size);
    if (!dst) {
        printf("Error: Cannot allocate %u bytes\n", decomp_size);
        return nullptr;
    }
    
    // Dekomprimieren
    DecompContext context;
    context.initialize(dst);
    context.forceDestCount = decomp_size;
    
    s32 result = streamDecomp(&context, src, src_size);
    
    if (result < 0) {
        printf("Error: Decompression failed with error %d\n", result);
        free(dst);
        return nullptr;
    }
    
    if (out_size)
        *out_size = decomp_size;
    
    return dst;
}

s32 SZSDecompressor::readHeader_(DecompContext* context, const u8* srcp, u32 src_size)
{
    s32 header_size = 0;

    while (context->headerSize > 0)
    {
        context->headerSize--;

        if (context->headerSize == 0xF)
        {
            if (*srcp != 'Y')
                return -1;
        }
        else if (context->headerSize == 0xE)
        {
            if (*srcp != 'a')
                return -1;
        }
        else if (context->headerSize == 0xD)
        {
            if (*srcp != 'z')
                return -1;
        }
        else if (context->headerSize == 0xC)
        {
            if (*srcp != '0')
                return -1;
        }

        else if (context->headerSize >= 8)
            context->destCount |= static_cast<u32>(*srcp) << ((static_cast<u32>(context->headerSize) - 8) << 3);

        srcp++; header_size++; src_size--;
        if (src_size == 0 && context->headerSize > 0)
            return header_size;
    }

    if (context->forceDestCount > 0 && context->forceDestCount < context->destCount)
        context->destCount = context->forceDestCount;

    return header_size;
}

s32 SZSDecompressor::streamDecomp(DecompContext* context, const void* src, u32 len)
{
    if (!context || !src)
        return -1;

    const u8* srcp = static_cast<const u8*>(src);

    if (context->headerSize > 0)
    {
        s32 header_size = readHeader_(context, srcp, len);
        if (header_size < 0)
            return header_size;

        srcp += header_size; len -= header_size;
        if (len == 0)
        {
            if (context->headerSize == 0)
                return context->destCount;

            return -1;
        }
    }

    while (context->destCount > 0)
    {
        if (context->step == STEP_LONG)
        {
            u32 n = (*srcp++) + 0x12; len--;
            if (s32(n) > context->destCount)
            {
                if (context->forceDestCount == 0)
                    return -2;

                n = static_cast<u16>(context->destCount);
            }

            context->destCount -= n;

            do
            {
                *context->destp = *(context->destp - context->lzOffset);
                context->destp++;
            }
            while (--n != 0);

            context->step = STEP_NORMAL;
        }
        else if (context->step == STEP_SHORT)
        {
            u32 offsetLen = static_cast<u32>(context->packHigh) << 8 | *srcp++; len--;
            context->lzOffset = (offsetLen & 0xFFFu) + 1;

            u32 n = offsetLen >> 12;
            if (n == 0)
                context->step = STEP_LONG;

            else
            {
                n  += 2;
                if (s32(n) > context->destCount)
                {
                    if (context->forceDestCount == 0)
                        return -2;

                    n = static_cast<u16>(context->destCount);
                }

                context->destCount -= n;

                do
                {
                    *context->destp = *(context->destp - context->lzOffset);
                    context->destp++;
                }
                while (--n != 0);

                context->step = STEP_NORMAL;
            }
        }
        else
        {
            if (context->flagMask == 0)
            {
                context->flags = *srcp++; len--;
                context->flagMask = 0x80;
                if (len == 0)
                    break;
            }

            if ((context->flags & context->flagMask) != 0)
            {
                *context->destp++ = *srcp++;
                context->destCount--;
            }
            else
            {
                context->packHigh = *srcp++;
                context->step = STEP_SHORT;
            }

            len--;
            context->flagMask >>= 1;
        }

        if (len == 0)
            break;
    }

    if (context->destCount == 0 && context->forceDestCount == 0 && len > 0x20)
        return -1;

    return context->destCount;
}