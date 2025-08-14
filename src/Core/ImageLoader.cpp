#include <Core/ImageLoader.h>

#include <System/File.h>
#include <System/Debug.h>
#include <Core/WideString.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>  // ptrdiff_t on osx
#include <fstream>

#define STBI_ASSERT(x) assert(x)
#define STBI_VERSION 1
#define STBIDEF static
#define stbi_inline inline

using namespace Vortex;

namespace {

typedef unsigned char stbi_uc;

#ifdef _MSC_VER
typedef unsigned short stbi__uint16;
typedef signed short stbi__int16;
typedef unsigned int stbi__uint32;
typedef signed int stbi__int32;
#else
#include <stdint.h>
typedef uint16_t stbi__uint16;
typedef int16_t stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t stbi__int32;
#endif

// should produce compiler error if size is wrong
typedef stbi_uc validate_uint32[sizeof(stbi__uint32) == 4 ? 1 : -1];

#ifdef _MSC_VER
#define STBI_NOTUSED(v) (void)(v)
#else
#define STBI_NOTUSED(v) (void)sizeof(v)
#endif

#ifdef _MSC_VER
#define STBI_HAS_LROTL
#endif

#ifdef STBI_HAS_LROTL
#define stbi_lrot(x, y) _lrotl(x, y)
#else
#define stbi_lrot(x, y) (((x) << (y)) | ((x) >> (32 - (y))))
#endif

///////////////////////////////////////////////
//
//  stbi__context struct and start_xxx functions

struct stbi_io_callbacks {
    int read(char *p, int n) {
        file.read(p, n);
        return static_cast<int>(file.gcount());
    }
    void skip(int n) { file.ignore(n); }
    bool eof() const { return file.eof(); }
    std::ifstream file;
};

// stbi__context structure is our basic context used by all images, so it
// contains all the IO context, plus some basic image information
typedef struct {
    stbi__uint32 img_x, img_y;
    int img_n, img_out_n;

    stbi_io_callbacks *io;

    int read_from_callbacks;
    int buflen;
    stbi_uc buffer_start[128];

    stbi_uc *img_buffer, *img_buffer_end;
    stbi_uc *img_buffer_original;
} stbi__context;

static void stbi__refill_buffer(stbi__context *s);

// initialize a callback-based context
static void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c) {
    s->io = c;
    s->buflen = sizeof(s->buffer_start);
    s->read_from_callbacks = 1;
    s->img_buffer_original = s->buffer_start;
    stbi__refill_buffer(s);
}

static void stbi__rewind(stbi__context *s) {
    // conceptually rewind SHOULD rewind to the beginning of the stream,
    // but we just rewind to the beginning of the initial buffer, because
    // we only use it after doing 'test', which only ever looks at at most 92
    // bytes
    s->img_buffer = s->img_buffer_original;
}

static int stbi__jpeg_test(stbi__context *s);
static stbi_uc *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp);
static int stbi__png_test(stbi__context *s);
static stbi_uc *stbi__png_load(stbi__context *s, int *x, int *y, int *comp);

// this is not threadsafe
static const char *stbi__g_failure_reason = nullptr;
static const char *stbi__g_failure_details = nullptr;

static int stbi_errstr(const char *details, const char *reason) {
    stbi__g_failure_reason = reason;
    stbi__g_failure_details = details;
    return 0;
}

static void *stbi__malloc(size_t size) { return malloc(size); }

// stbi__err - error
// stbi__errpf - error returning pointer to float
// stbi__errpuc - error returning pointer to stbi_uc

#ifdef STBI_NO_FAILURE_STRINGS
#define stbi__err(x, y) 0
#else
#define stbi__err(x, y) stbi_errstr(x, y)
#endif

#define stbi__errpf(x, y) ((float *)(stbi__err(x, y) ? nullptr : nullptr))
#define stbi__errpuc(x, y) ((stbi_uc *)(stbi__err(x, y) ? nullptr : nullptr))

STBIDEF void stbi_image_free(void *retval_from_stbi_load) {
    free(retval_from_stbi_load);
}

static stbi_uc *stbi_load_main(stbi__context *s, int *x, int *y, int *comp) {
    if (stbi__jpeg_test(s)) return stbi__jpeg_load(s, x, y, comp);
    if (stbi__png_test(s)) return stbi__png_load(s, x, y, comp);
    return stbi__errpuc("unknown image type",
                        "Image not of any known type, or corrupt");
}

stbi_uc *stbi_load_from_callbacks(const stbi_io_callbacks &clbk, int *x, int *y,
                                  int *comp) {
    stbi__context s;
    stbi__start_callbacks(&s, (stbi_io_callbacks *)(&clbk));
    return stbi_load_main(&s, x, y, comp);
}

//////////////////////////////////////////////////////////////////////////////
//
// Common code used by all image loaders
//

enum { SCAN_load = 0, SCAN_type, SCAN_header };

static void stbi__refill_buffer(stbi__context *s) {
    int n = s->io->read((char *)s->buffer_start, s->buflen);
    if (n == 0) {
        // at end of file, treat same as if from memory, but need to Handle case
        // where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
        s->read_from_callbacks = 0;
        s->img_buffer = s->buffer_start;
        s->img_buffer_end = s->buffer_start + 1;
        *s->img_buffer = 0;
    } else {
        s->img_buffer = s->buffer_start;
        s->img_buffer_end = s->buffer_start + n;
    }
}

stbi_inline static stbi_uc stbi__get8(stbi__context *s) {
    if (s->img_buffer < s->img_buffer_end) return *s->img_buffer++;
    if (s->read_from_callbacks) {
        stbi__refill_buffer(s);
        return *s->img_buffer++;
    }
    return 0;
}

stbi_inline static int stbi__at_eof(stbi__context *s) {
    if (s->io) {
        if (!s->io->eof()) return 0;
        // if feof() is true, check if buffer = end
        // special case: we've only got the special 0 character at the end
        if (s->read_from_callbacks == 0) return 1;
    }

    return s->img_buffer >= s->img_buffer_end;
}

static void stbi__skip(stbi__context *s, int n) {
    if (s->io) {
        int blen = (int)(s->img_buffer_end - s->img_buffer);
        if (blen < n) {
            s->img_buffer = s->img_buffer_end;
            s->io->skip(n - blen);
            return;
        }
    }
    s->img_buffer += n;
}

static int stbi__getn(stbi__context *s, stbi_uc *buffer, int n) {
    if (s->io) {
        int blen = (int)(s->img_buffer_end - s->img_buffer);
        if (blen < n) {
            int res, count;

            memcpy(buffer, s->img_buffer, blen);

            count = s->io->read((char *)buffer + blen, n - blen);
            res = (count == (n - blen));
            s->img_buffer = s->img_buffer_end;
            return res;
        }
    }

    if (s->img_buffer + n <= s->img_buffer_end) {
        memcpy(buffer, s->img_buffer, n);
        s->img_buffer += n;
        return 1;
    } else
        return 0;
}

static int stbi__get16be(stbi__context *s) {
    int z = stbi__get8(s);
    return (z << 8) + stbi__get8(s);
}

static stbi__uint32 stbi__get32be(stbi__context *s) {
    stbi__uint32 z = stbi__get16be(s);
    return (z << 16) + stbi__get16be(s);
}

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder (not actually fully baseline implementation)
//
//    simple implementation
//      - channel subsampling of at most 2 in each dimension
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - uses a lot of intermediate memory, could cache poorly
//      - load http://nothings.org/remote/anemones.jpg 3 times on 2.8Ghz P4
//          stb_jpeg:   1.34 seconds (MSVC6, default release build)
//          stb_jpeg:   1.06 seconds (MSVC6, processor = Pentium Pro)
//          IJL11.dll:  1.08 seconds (compiled by intel)
//          IJG 1998:   0.98 seconds (MSVC6, makefile provided by IJG)
//          IJG 1998:   0.95 seconds (MSVC6, makefile + proc=PPro)

// huffman decoding acceleration
#define FAST_BITS 9  // larger handles more cases; smaller stomps less cache

typedef struct {
    stbi_uc fast[1 << FAST_BITS];
    // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
    stbi__uint16 code[256];
    stbi_uc values[256];
    stbi_uc size[257];
    stbi__uint32 maxcode[18];
    int delta[17];  // old 'firstsymbol' - old 'firstcode'
} stbi__huffman;

typedef struct {
    stbi__context *s;
    stbi__huffman huff_dc[4];
    stbi__huffman huff_ac[4];
    stbi_uc dequant[4][64];

    // sizes for components, interleaved MCUs
    int img_h_max, img_v_max;
    int img_mcu_x, img_mcu_y;
    int img_mcu_w, img_mcu_h;

    // definition of jpeg image component
    struct {
        int id;
        int h, v;
        int tq;
        int hd, ha;
        int dc_pred;

        int x, y, w2, h2;
        stbi_uc *data;
        void *raw_data;
        stbi_uc *linebuf;
    } img_comp[4];

    stbi__uint32 code_buffer;  // jpeg entropy-coded buffer
    int code_bits;             // number of valid bits
    stbi_uc marker;            // marker seen while filling entropy buffer
    int nomore;                // flag if we saw a marker so must stop

    int scan_n, order[4];
    int restart_interval, todo;
} stbi__jpeg;

static int stbi__build_huffman(stbi__huffman *h, int *count) {
    int i, j, k = 0, code;
    // build size list for each symbol (from JPEG spec)
    for (i = 0; i < 16; ++i)
        for (j = 0; j < count[i]; ++j) h->size[k++] = (stbi_uc)(i + 1);
    h->size[k] = 0;

    // compute actual symbols (from jpeg spec)
    code = 0;
    k = 0;
    for (j = 1; j <= 16; ++j) {
        // compute delta to add to code to compute symbol id
        h->delta[j] = k - code;
        if (h->size[k] == j) {
            while (h->size[k] == j) h->code[k++] = (stbi__uint16)(code++);
            if (code - 1 >= (1 << j))
                return stbi__err("bad code lengths", "Corrupt JPEG");
        }
        // compute largest code + 1or this size, preshifted as needed later
        h->maxcode[j] = code << (16 - j);
        code <<= 1;
    }
    h->maxcode[j] = 0xffffffff;

    // build non-spec acceleration table; 255 is flag for not-accelerated
    memset(h->fast, 255, 1 << FAST_BITS);
    for (i = 0; i < k; ++i) {
        int s = h->size[i];
        if (s <= FAST_BITS) {
            int c = h->code[i] << (FAST_BITS - s);
            int m = 1 << (FAST_BITS - s);
            for (j = 0; j < m; ++j) {
                h->fast[c + j] = (stbi_uc)i;
            }
        }
    }
    return 1;
}

static void stbi__grow_buffer_unsafe(stbi__jpeg *j) {
    do {
        int b = j->nomore ? 0 : stbi__get8(j->s);
        if (b == 0xff) {
            int c = stbi__get8(j->s);
            if (c != 0) {
                j->marker = (stbi_uc)c;
                j->nomore = 1;
                return;
            }
        }
        j->code_buffer |= b << (24 - j->code_bits);
        j->code_bits += 8;
    } while (j->code_bits <= 24);
}

// (1 << n) - 1
static stbi__uint32 stbi__bmask[17] = {0,    1,    3,     7,     15,   31,
                                       63,   127,  255,   511,   1023, 2047,
                                       4095, 8191, 16383, 32767, 65535};

// decode a jpeg huffman value from the bitstream
stbi_inline static int stbi__jpeg_huff_decode(stbi__jpeg *j, stbi__huffman *h) {
    stbi__uint32 temp;
    int c, k;

    if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

    // look at the top FAST_BITS and determine what symbol ID it is,
    // if the code is <= FAST_BITS
    c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
    k = h->fast[c];
    if (k < 255) {
        int s = h->size[k];
        if (s > j->code_bits) return -1;
        j->code_buffer <<= s;
        j->code_bits -= s;
        return h->values[k];
    }

    // naive test is to shift the code_buffer down so k bits are
    // valid, then test against maxcode. To speed this up, we've
    // preshifted maxcode left so that it has (16-k) 0s at the
    // end; in other words, regardless of the number of bits, it
    // wants to be compared against something shifted to have 16;
    // that way we don't need to shift inside the loop.
    temp = j->code_buffer >> 16;
    for (k = FAST_BITS + 1;; ++k)
        if (temp < h->maxcode[k]) break;
    if (k == 17) {
        // error! code not found
        j->code_bits -= 16;
        return -1;
    }

    if (k > j->code_bits) return -1;

    // convert the huffman code to the symbol id
    c = ((j->code_buffer >> (32 - k)) & stbi__bmask[k]) + h->delta[k];
    STBI_ASSERT((((j->code_buffer) >> (32 - h->size[c])) &
                 stbi__bmask[h->size[c]]) == h->code[c]);

    // convert the id to a symbol
    j->code_bits -= k;
    j->code_buffer <<= k;
    return h->values[c];
}

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
stbi_inline static int stbi__extend_receive(stbi__jpeg *j, int n) {
    stbi__uint32 m = 1 << (n - 1);
    stbi__uint32 k;
    if (j->code_bits < n) stbi__grow_buffer_unsafe(j);

#if 1
    k = stbi_lrot(j->code_buffer, n);
    j->code_buffer = k & ~stbi__bmask[n];
    k &= stbi__bmask[n];
    j->code_bits -= n;
#else
    k = (j->code_buffer >> (32 - n)) & stbi__bmask[n];
    j->code_bits -= n;
    j->code_buffer <<= n;
#endif
    // the following test is probably a random branch that won't
    // predict well. I tried to table accelerate it but failed.
    // maybe it's compiling as a conditional move?
    if (k < m)
        return (-1 << n) + k + 1;
    else
        return k;
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static stbi_uc stbi__jpeg_dezigzag[64 + 15] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40,
    48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61,
    54, 47, 55, 62, 63,
    // let corrupt input sample past end
    63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63};

// decode one 64-entry block--
static int stbi__jpeg_decode_block(stbi__jpeg *j, short data[64],
                                   stbi__huffman *hdc, stbi__huffman *hac,
                                   int b) {
    int diff, dc, k;
    int t = stbi__jpeg_huff_decode(j, hdc);
    if (t < 0) return stbi__err("bad huffman code", "Corrupt JPEG");

    // 0 all the ac values now so we can do it 32-bits at a time
    memset(data, 0, 64 * sizeof(data[0]));

    diff = t ? stbi__extend_receive(j, t) : 0;
    dc = j->img_comp[b].dc_pred + diff;
    j->img_comp[b].dc_pred = dc;
    data[0] = (short)dc;

    // decode AC components, see JPEG spec
    k = 1;
    do {
        int r, s;
        int rs = stbi__jpeg_huff_decode(j, hac);
        if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
        s = rs & 15;
        r = rs >> 4;
        if (s == 0) {
            if (rs != 0xf0) break;  // end block
            k += 16;
        } else {
            k += r;
            // decode into unzigzag'd location
            data[stbi__jpeg_dezigzag[k++]] = (short)stbi__extend_receive(j, s);
        }
    } while (k < 64);
    return 1;
}

// take a -128..127 value and stbi__clamp it and convert to 0..255
stbi_inline static stbi_uc stbi__clamp(int x) {
    // trick to use a single test to catch both cases
    if ((stbi__uint32)x > 255) {
        if (x < 0) return 0;
        if (x > 255) return 255;
    }
    return (stbi_uc)x;
}

#define stbi__f2f(x) (int)(((x) * 4096 + 0.5))
#define stbi__fsh(x) ((x) << 12)

// derived from jidctint -- DCT_ISLOW
#define STBI__IDCT_1D(s0, s1, s2, s3, s4, s5, s6, s7)       \
    int t0, t1, t2, t3, p1, p2, p3, p4, p5, x0, x1, x2, x3; \
    p2 = s2;                                                \
    p3 = s6;                                                \
    p1 = (p2 + p3) * stbi__f2f(0.5411961f);                 \
    t2 = p1 + p3 * stbi__f2f(-1.847759065f);                \
    t3 = p1 + p2 * stbi__f2f(0.765366865f);                 \
    p2 = s0;                                                \
    p3 = s4;                                                \
    t0 = stbi__fsh(p2 + p3);                                \
    t1 = stbi__fsh(p2 - p3);                                \
    x0 = t0 + t3;                                           \
    x3 = t0 - t3;                                           \
    x1 = t1 + t2;                                           \
    x2 = t1 - t2;                                           \
    t0 = s7;                                                \
    t1 = s5;                                                \
    t2 = s3;                                                \
    t3 = s1;                                                \
    p3 = t0 + t2;                                           \
    p4 = t1 + t3;                                           \
    p1 = t0 + t3;                                           \
    p2 = t1 + t2;                                           \
    p5 = (p3 + p4) * stbi__f2f(1.175875602f);               \
    t0 = t0 * stbi__f2f(0.298631336f);                      \
    t1 = t1 * stbi__f2f(2.053119869f);                      \
    t2 = t2 * stbi__f2f(3.072711026f);                      \
    t3 = t3 * stbi__f2f(1.50132111);                        \
    p1 = p5 + p1 * stbi__f2f(-0.899976223f);                \
    p2 = p5 + p2 * stbi__f2f(-2.562915447f);                \
    p3 = p3 * stbi__f2f(-1.961570560f);                     \
    p4 = p4 * stbi__f2f(-0.390180644f);                     \
    t3 += p1 + p4;                                          \
    t2 += p2 + p3;                                          \
    t1 += p2 + p4;                                          \
    t0 += p1 + p3;

typedef stbi_uc stbi_dequantize_t;

// .344 seconds on 3*anemones.jpg
static void stbi__idct_block(stbi_uc *out, int out_stride, short data[64],
                             stbi_dequantize_t *dequantize) {
    int i, val[64], *v = val;
    stbi_dequantize_t *dq = dequantize;
    stbi_uc *o;
    short *d = data;

    // columns
    for (i = 0; i < 8; ++i, ++d, ++dq, ++v) {
        // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
        if (d[8] == 0 && d[16] == 0 && d[24] == 0 && d[32] == 0 && d[40] == 0 &&
            d[48] == 0 && d[56] == 0) {
            //    no shortcut                 0     seconds
            //    (1|2|3|4|5|6|7)==0          0     seconds
            //    all separate               -0.047 seconds
            //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
            int dcterm = d[0] * dq[0] << 2;
            v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] =
                dcterm;
        } else {
            STBI__IDCT_1D(d[0] * dq[0], d[8] * dq[8], d[16] * dq[16],
                          d[24] * dq[24], d[32] * dq[32], d[40] * dq[40],
                          d[48] * dq[48], d[56] * dq[56])
            // constants scaled things up by 1<<12; let's bring them back
            // down, but keep 2 extra bits of precision
            x0 += 512;
            x1 += 512;
            x2 += 512;
            x3 += 512;
            v[0] = (x0 + t3) >> 10;
            v[56] = (x0 - t3) >> 10;
            v[8] = (x1 + t2) >> 10;
            v[48] = (x1 - t2) >> 10;
            v[16] = (x2 + t1) >> 10;
            v[40] = (x2 - t1) >> 10;
            v[24] = (x3 + t0) >> 10;
            v[32] = (x3 - t0) >> 10;
        }
    }

    for (i = 0, v = val, o = out; i < 8; ++i, v += 8, o += out_stride) {
        // no fast case since the first 1D IDCT spread components out
        STBI__IDCT_1D(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7])
        // constants scaled things up by 1<<12, plus we had 1<<2 from first
        // loop, plus horizontal and vertical each scale by sqrt(8) so together
        // we've got an extra 1<<3, so 1<<17 total we need to remove.
        // so we want to round that, which means adding 0.5 * 1<<17,
        // aka 65536. Also, we'll end up with -128 to 127 that we want
        // to encode as 0..255 by adding 128, so we'll add that before the shift
        x0 += 65536 + (128 << 17);
        x1 += 65536 + (128 << 17);
        x2 += 65536 + (128 << 17);
        x3 += 65536 + (128 << 17);
        // tried computing the shifts into temps, or'ing the temps to see
        // if any were out of range, but that was slower
        o[0] = stbi__clamp((x0 + t3) >> 17);
        o[7] = stbi__clamp((x0 - t3) >> 17);
        o[1] = stbi__clamp((x1 + t2) >> 17);
        o[6] = stbi__clamp((x1 - t2) >> 17);
        o[2] = stbi__clamp((x2 + t1) >> 17);
        o[5] = stbi__clamp((x2 - t1) >> 17);
        o[3] = stbi__clamp((x3 + t0) >> 17);
        o[4] = stbi__clamp((x3 - t0) >> 17);
    }
}

#define STBI__MARKER_none 0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static stbi_uc stbi__get_marker(stbi__jpeg *j) {
    stbi_uc x;
    if (j->marker != STBI__MARKER_none) {
        x = j->marker;
        j->marker = STBI__MARKER_none;
        return x;
    }
    x = stbi__get8(j->s);
    if (x != 0xff) return STBI__MARKER_none;
    while (x == 0xff) x = stbi__get8(j->s);
    return x;
}

// in each scan, we'll have scan_n components, and the order
// of the components is specified by order[]
#define STBI__RESTART(x) ((x) >= 0xd0 && (x) <= 0xd7)

// after a restart interval, stbi__jpeg_reset the entropy decoder and
// the dc prediction
static void stbi__jpeg_reset(stbi__jpeg *j) {
    j->code_bits = 0;
    j->code_buffer = 0;
    j->nomore = 0;
    j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred =
        0;
    j->marker = STBI__MARKER_none;
    j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
    // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
    // since we don't even allow 1<<30 pixels
}

static int stbi__parse_entropy_coded_data(stbi__jpeg *z) {
    stbi__jpeg_reset(z);
    if (z->scan_n == 1) {
        int i, j;
        short data[64];
        int n = z->order[0];
        // non-interleaved data, we just need to process one block at a time,
        // in trivial scanline order
        // number of blocks to do just depends on how many actual "pixels" this
        // component has, independent of interleaved MCU blocking and such
        int w = (z->img_comp[n].x + 7) >> 3;
        int h = (z->img_comp[n].y + 7) >> 3;
        for (j = 0; j < h; ++j) {
            for (i = 0; i < w; ++i) {
                if (!stbi__jpeg_decode_block(z, data,
                                             z->huff_dc + z->img_comp[n].hd,
                                             z->huff_ac + z->img_comp[n].ha, n))
                    return 0;
                stbi__idct_block(
                    z->img_comp[n].data + z->img_comp[n].w2 * j * 8 + i * 8,
                    z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);
                // every data block is an MCU, so countdown the restart interval
                if (--z->todo <= 0) {
                    if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
                    // if it's NOT a restart, then just bail, so we get corrupt
                    // data rather than no data
                    if (!STBI__RESTART(z->marker)) return 1;
                    stbi__jpeg_reset(z);
                }
            }
        }
    } else {  // interleaved!
        int i, j, k, x, y;
        short data[64];
        for (j = 0; j < z->img_mcu_y; ++j) {
            for (i = 0; i < z->img_mcu_x; ++i) {
                // scan an interleaved mcu... process scan_n components in order
                for (k = 0; k < z->scan_n; ++k) {
                    int n = z->order[k];
                    // scan out an mcu's worth of this component; that's just
                    // determined by the basic H and V specified for the
                    // component
                    for (y = 0; y < z->img_comp[n].v; ++y) {
                        for (x = 0; x < z->img_comp[n].h; ++x) {
                            int x2 = (i * z->img_comp[n].h + x) * 8;
                            int y2 = (j * z->img_comp[n].v + y) * 8;
                            if (!stbi__jpeg_decode_block(
                                    z, data, z->huff_dc + z->img_comp[n].hd,
                                    z->huff_ac + z->img_comp[n].ha, n))
                                return 0;
                            stbi__idct_block(z->img_comp[n].data +
                                                 z->img_comp[n].w2 * y2 + x2,
                                             z->img_comp[n].w2, data,
                                             z->dequant[z->img_comp[n].tq]);
                        }
                    }
                }
                // after all interleaved components, that's an interleaved MCU,
                // so now count down the restart interval
                if (--z->todo <= 0) {
                    if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
                    // if it's NOT a restart, then just bail, so we get corrupt
                    // data rather than no data
                    if (!STBI__RESTART(z->marker)) return 1;
                    stbi__jpeg_reset(z);
                }
            }
        }
    }
    return 1;
}

static int stbi__process_marker(stbi__jpeg *z, int m) {
    int L;
    switch (m) {
        case STBI__MARKER_none:  // no marker found
            return stbi__err("expected marker", "Corrupt JPEG");

        case 0xC2:  // stbi__SOF - progressive
            return stbi__err("progressive jpeg",
                             "JPEG format not supported (progressive)");

        case 0xDD:  // DRI - specify restart interval
            if (stbi__get16be(z->s) != 4)
                return stbi__err("bad DRI len", "Corrupt JPEG");
            z->restart_interval = stbi__get16be(z->s);
            return 1;

        case 0xDB:  // DQT - define quantization table
            L = stbi__get16be(z->s) - 2;
            while (L > 0) {
                int q = stbi__get8(z->s);
                int p = q >> 4;
                int t = q & 15, i;
                if (p != 0) return stbi__err("bad DQT type", "Corrupt JPEG");
                if (t > 3) return stbi__err("bad DQT table", "Corrupt JPEG");
                for (i = 0; i < 64; ++i)
                    z->dequant[t][stbi__jpeg_dezigzag[i]] = stbi__get8(z->s);
                L -= 65;
            }
            return L == 0;

        case 0xC4:  // DHT - define huffman table
            L = stbi__get16be(z->s) - 2;
            while (L > 0) {
                stbi_uc *v;
                int sizes[16], i, n = 0;
                int q = stbi__get8(z->s);
                int tc = q >> 4;
                int th = q & 15;
                if (tc > 1 || th > 3)
                    return stbi__err("bad DHT header", "Corrupt JPEG");
                for (i = 0; i < 16; ++i) {
                    sizes[i] = stbi__get8(z->s);
                    n += sizes[i];
                }
                L -= 17;
                if (tc == 0) {
                    if (!stbi__build_huffman(z->huff_dc + th, sizes)) return 0;
                    v = z->huff_dc[th].values;
                } else {
                    if (!stbi__build_huffman(z->huff_ac + th, sizes)) return 0;
                    v = z->huff_ac[th].values;
                }
                for (i = 0; i < n; ++i) v[i] = stbi__get8(z->s);
                L -= n;
            }
            return L == 0;
    }
    // check for comment block or APP blocks
    if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
        stbi__skip(z->s, stbi__get16be(z->s) - 2);
        return 1;
    }
    return 0;
}

// after we see stbi__SOS
static int stbi__process_scan_header(stbi__jpeg *z) {
    int i;
    int Ls = stbi__get16be(z->s);
    z->scan_n = stbi__get8(z->s);
    if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int)z->s->img_n)
        return stbi__err("bad stbi__SOS component count", "Corrupt JPEG");
    if (Ls != 6 + 2 * z->scan_n)
        return stbi__err("bad stbi__SOS len", "Corrupt JPEG");
    for (i = 0; i < z->scan_n; ++i) {
        int id = stbi__get8(z->s), which;
        int q = stbi__get8(z->s);
        for (which = 0; which < z->s->img_n; ++which)
            if (z->img_comp[which].id == id) break;
        if (which == z->s->img_n) return 0;
        z->img_comp[which].hd = q >> 4;
        if (z->img_comp[which].hd > 3)
            return stbi__err("bad DC huff", "Corrupt JPEG");
        z->img_comp[which].ha = q & 15;
        if (z->img_comp[which].ha > 3)
            return stbi__err("bad AC huff", "Corrupt JPEG");
        z->order[i] = which;
    }
    if (stbi__get8(z->s) != 0)
        return stbi__err("bad stbi__SOS", "Corrupt JPEG");
    stbi__get8(z->s);  // should be 63, but might be 0
    if (stbi__get8(z->s) != 0)
        return stbi__err("bad stbi__SOS", "Corrupt JPEG");

    return 1;
}

static int stbi__process_frame_header(stbi__jpeg *z, int scan) {
    stbi__context *s = z->s;
    int Lf, p, i, q, h_max = 1, v_max = 1, c;
    Lf = stbi__get16be(s);
    if (Lf < 11) return stbi__err("bad stbi__SOF len", "Corrupt JPEG");  // JPEG
    p = stbi__get8(s);
    if (p != 8)
        return stbi__err(
            "only 8-bit",
            "JPEG format not supported: 8-bit only");  // JPEG baseline
    s->img_y = stbi__get16be(s);
    if (s->img_y == 0)
        return stbi__err(
            "no header height",
            "JPEG format not supported: delayed height");  // Legal, but we
                                                           // don't Handle
                                                           // it--but neither
                                                           // does IJG
    s->img_x = stbi__get16be(s);
    if (s->img_x == 0)
        return stbi__err("0 width", "Corrupt JPEG");  // JPEG requires
    c = stbi__get8(s);
    if (c != 3 && c != 1)
        return stbi__err("bad component count",
                         "Corrupt JPEG");  // JFIF requires
    s->img_n = c;
    for (i = 0; i < c; ++i) {
        z->img_comp[i].data = nullptr;
        z->img_comp[i].linebuf = nullptr;
    }

    if (Lf != 8 + 3 * s->img_n)
        return stbi__err("bad stbi__SOF len", "Corrupt JPEG");

    for (i = 0; i < s->img_n; ++i) {
        z->img_comp[i].id = stbi__get8(s);
        if (z->img_comp[i].id != i + 1)  // JFIF requires
            if (z->img_comp[i].id != i)  // some version of jpegtran outputs
                                         // non-JFIF-compliant files!
                return stbi__err("bad component ID", "Corrupt JPEG");
        q = stbi__get8(s);
        z->img_comp[i].h = (q >> 4);
        if (!z->img_comp[i].h || z->img_comp[i].h > 4)
            return stbi__err("bad H", "Corrupt JPEG");
        z->img_comp[i].v = q & 15;
        if (!z->img_comp[i].v || z->img_comp[i].v > 4)
            return stbi__err("bad V", "Corrupt JPEG");
        z->img_comp[i].tq = stbi__get8(s);
        if (z->img_comp[i].tq > 3) return stbi__err("bad TQ", "Corrupt JPEG");
    }

    if (scan != SCAN_load) return 1;

    if ((1 << 30) / s->img_x / s->img_n < s->img_y)
        return stbi__err("too large", "Image too large to decode");

    for (i = 0; i < s->img_n; ++i) {
        if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
        if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
    }

    // compute interleaved mcu info
    z->img_h_max = h_max;
    z->img_v_max = v_max;
    z->img_mcu_w = h_max * 8;
    z->img_mcu_h = v_max * 8;
    z->img_mcu_x = (s->img_x + z->img_mcu_w - 1) / z->img_mcu_w;
    z->img_mcu_y = (s->img_y + z->img_mcu_h - 1) / z->img_mcu_h;

    for (i = 0; i < s->img_n; ++i) {
        // number of effective pixels (e.g. for non-interleaved MCU)
        z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max - 1) / h_max;
        z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max - 1) / v_max;
        // to simplify generation, we'll allocate enough memory to decode
        // the bogus oversized data from using interleaved MCUs and their
        // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
        // discard the extra data until colorspace conversion
        z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
        z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
        z->img_comp[i].raw_data =
            stbi__malloc(z->img_comp[i].w2 * z->img_comp[i].h2 + 15);
        if (z->img_comp[i].raw_data == nullptr) {
            for (--i; i >= 0; --i) {
                free(z->img_comp[i].raw_data);
                z->img_comp[i].data = nullptr;
            }
            return stbi__err("outofmem", "Out of memory");
        }
        // align blocks for installable-idct using mmx/sse
        z->img_comp[i].data =
            (stbi_uc *)(((size_t)z->img_comp[i].raw_data + 15) & ~15);
        z->img_comp[i].linebuf = nullptr;
    }

    return 1;
}

// use comparisons since in some cases we Handle more than one case (e.g.
// stbi__SOF)
#define stbi__DNL(x) ((x) == 0xdc)
#define stbi__SOI(x) ((x) == 0xd8)
#define stbi__EOI(x) ((x) == 0xd9)
#define stbi__SOF(x) ((x) == 0xc0 || (x) == 0xc1)
#define stbi__SOS(x) ((x) == 0xda)

static int decode_jpeg_header(stbi__jpeg *z, int scan) {
    int m;
    z->marker = STBI__MARKER_none;  // initialize cached marker to empty
    m = stbi__get_marker(z);
    if (!stbi__SOI(m)) return stbi__err("no stbi__SOI", "Corrupt JPEG");
    if (scan == SCAN_type) return 1;
    m = stbi__get_marker(z);
    while (!stbi__SOF(m)) {
        if (!stbi__process_marker(z, m)) return 0;
        m = stbi__get_marker(z);
        while (m == STBI__MARKER_none) {
            // some files have extra padding after their blocks, so ok, we'll
            // scan
            if (stbi__at_eof(z->s))
                return stbi__err("no stbi__SOF", "Corrupt JPEG");
            m = stbi__get_marker(z);
        }
    }
    if (!stbi__process_frame_header(z, scan)) return 0;
    return 1;
}

static int decode_jpeg_image(stbi__jpeg *j) {
    int m;
    j->restart_interval = 0;
    if (!decode_jpeg_header(j, SCAN_load)) return 0;
    m = stbi__get_marker(j);
    while (!stbi__EOI(m)) {
        if (stbi__SOS(m)) {
            if (!stbi__process_scan_header(j)) return 0;
            if (!stbi__parse_entropy_coded_data(j)) return 0;
            if (j->marker == STBI__MARKER_none) {
                // Handle 0s at the end of image data from IP Kamera 9060
                while (!stbi__at_eof(j->s)) {
                    int x = stbi__get8(j->s);
                    if (x == 255) {
                        j->marker = stbi__get8(j->s);
                        break;
                    } else if (x != 0) {
                        return 0;
                    }
                }
                // if we reach eof without hitting a marker, stbi__get_marker()
                // below will fail and we'll eventually return 0
            }
        } else {
            if (!stbi__process_marker(j, m)) return 0;
        }
        m = stbi__get_marker(j);
    }
    return 1;
}

// static jfif-centered resampling (across block boundaries)

typedef stbi_uc *(*resample_row_func)(stbi_uc *out, stbi_uc *in0, stbi_uc *in1,
                                      int w, int hs);

#define stbi__div4(x) ((stbi_uc)((x) >> 2))

static stbi_uc *resample_row_1(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far,
                               int w, int hs) {
    STBI_NOTUSED(out);
    STBI_NOTUSED(in_far);
    STBI_NOTUSED(w);
    STBI_NOTUSED(hs);
    return in_near;
}

static stbi_uc *stbi__resample_row_v_2(stbi_uc *out, stbi_uc *in_near,
                                       stbi_uc *in_far, int w, int hs) {
    // need to generate two samples vertically for every one in input
    int i;
    STBI_NOTUSED(hs);
    for (i = 0; i < w; ++i) out[i] = stbi__div4(3 * in_near[i] + in_far[i] + 2);
    return out;
}

static stbi_uc *stbi__resample_row_h_2(stbi_uc *out, stbi_uc *in_near,
                                       stbi_uc *in_far, int w, int hs) {
    // need to generate two samples horizontally for every one in input
    int i;
    stbi_uc *input = in_near;

    if (w == 1) {
        // if only one sample, can't do any interpolation
        out[0] = out[1] = input[0];
        return out;
    }

    out[0] = input[0];
    out[1] = stbi__div4(input[0] * 3 + input[1] + 2);
    for (i = 1; i < w - 1; ++i) {
        int n = 3 * input[i] + 2;
        out[i * 2 + 0] = stbi__div4(n + input[i - 1]);
        out[i * 2 + 1] = stbi__div4(n + input[i + 1]);
    }
    out[i * 2 + 0] = stbi__div4(input[w - 2] * 3 + input[w - 1] + 2);
    out[i * 2 + 1] = input[w - 1];

    STBI_NOTUSED(in_far);
    STBI_NOTUSED(hs);

    return out;
}

#define stbi__div16(x) ((stbi_uc)((x) >> 4))

static stbi_uc *stbi__resample_row_hv_2(stbi_uc *out, stbi_uc *in_near,
                                        stbi_uc *in_far, int w, int hs) {
    // need to generate 2x2 samples for every one in input
    int i, t0, t1;
    if (w == 1) {
        out[0] = out[1] = stbi__div4(3 * in_near[0] + in_far[0] + 2);
        return out;
    }

    t1 = 3 * in_near[0] + in_far[0];
    out[0] = stbi__div4(t1 + 2);
    for (i = 1; i < w; ++i) {
        t0 = t1;
        t1 = 3 * in_near[i] + in_far[i];
        out[i * 2 - 1] = stbi__div16(3 * t0 + t1 + 8);
        out[i * 2] = stbi__div16(3 * t1 + t0 + 8);
    }
    out[w * 2 - 1] = stbi__div4(t1 + 2);

    STBI_NOTUSED(hs);

    return out;
}

static stbi_uc *stbi__resample_row_generic(stbi_uc *out, stbi_uc *in_near,
                                           stbi_uc *in_far, int w, int hs) {
    // resample with nearest-neighbor
    int i, j;
    STBI_NOTUSED(in_far);
    for (i = 0; i < w; ++i)
        for (j = 0; j < hs; ++j) out[i * hs + j] = in_near[i];
    return out;
}

#define float2fixed(x) ((int)((x) * 65536 + 0.5))

// 0.38 seconds on 3*anemones.jpg   (0.25 with processor = Pro)
// VC6 without processor=Pro is generating multiple LEAs per multiply!
static void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y,
                                   const stbi_uc *pcb, const stbi_uc *pcr,
                                   int count, int step) {
    int i;
    for (i = 0; i < count; ++i) {
        int y_fixed = (y[i] << 16) + 32768;  // rounding
        int r, g, b;
        int cr = pcr[i] - 128;
        int cb = pcb[i] - 128;
        r = y_fixed + cr * float2fixed(1.40200f);
        g = y_fixed - cr * float2fixed(0.7141) - cb * float2fixed(0.3441);
        b = y_fixed + cb * float2fixed(1.77200f);
        r >>= 16;
        g >>= 16;
        b >>= 16;
        if ((unsigned)r > 255) {
            if (r < 0)
                r = 0;
            else
                r = 255;
        }
        if ((unsigned)g > 255) {
            if (g < 0)
                g = 0;
            else
                g = 255;
        }
        if ((unsigned)b > 255) {
            if (b < 0)
                b = 0;
            else
                b = 255;
        }
        out[0] = (stbi_uc)r;
        out[1] = (stbi_uc)g;
        out[2] = (stbi_uc)b;
        out[3] = 255;
        out += step;
    }
}

// clean up the temporary component buffers
static void stbi__cleanup_jpeg(stbi__jpeg *j) {
    int i;
    for (i = 0; i < j->s->img_n; ++i) {
        if (j->img_comp[i].raw_data) {
            free(j->img_comp[i].raw_data);
            j->img_comp[i].raw_data = nullptr;
            j->img_comp[i].data = nullptr;
        }
        if (j->img_comp[i].linebuf) {
            free(j->img_comp[i].linebuf);
            j->img_comp[i].linebuf = nullptr;
        }
    }
}

typedef struct {
    resample_row_func resample;
    stbi_uc *line0, *line1;
    int hs, vs;   // expansion factor in each axis
    int w_lores;  // horizontal pixels pre-expansion
    int ystep;    // how far through vertical expansion we are
    int ypos;     // which pre-expansion row we're on
} stbi__resample;

static stbi_uc *load_jpeg_image(stbi__jpeg *z, int *out_x, int *out_y,
                                int *comp) {
    int n, decode_n;
    z->s->img_n = 0;  // make stbi__cleanup_jpeg safe

    // load a jpeg image from whichever source
    if (!decode_jpeg_image(z)) {
        stbi__cleanup_jpeg(z);
        return nullptr;
    }

    // determine actual number of components to generate
    n = z->s->img_n;

    if (z->s->img_n == 3 && n < 3)
        decode_n = 1;
    else
        decode_n = z->s->img_n;

    // resample and color-convert
    {
        int k;
        stbi__uint32 i, j;
        stbi_uc *output;
        stbi_uc *coutput[4];

        stbi__resample res_comp[4];

        for (k = 0; k < decode_n; ++k) {
            stbi__resample *r = &res_comp[k];

            // allocate line buffer big enough for upsampling off the edges
            // with upsample factor of 4
            z->img_comp[k].linebuf = (stbi_uc *)stbi__malloc(z->s->img_x + 3);
            if (!z->img_comp[k].linebuf) {
                stbi__cleanup_jpeg(z);
                return stbi__errpuc("outofmem", "Out of memory");
            }

            r->hs = z->img_h_max / z->img_comp[k].h;
            r->vs = z->img_v_max / z->img_comp[k].v;
            r->ystep = r->vs >> 1;
            r->w_lores = (z->s->img_x + r->hs - 1) / r->hs;
            r->ypos = 0;
            r->line0 = r->line1 = z->img_comp[k].data;

            if (r->hs == 1 && r->vs == 1)
                r->resample = resample_row_1;
            else if (r->hs == 1 && r->vs == 2)
                r->resample = stbi__resample_row_v_2;
            else if (r->hs == 2 && r->vs == 1)
                r->resample = stbi__resample_row_h_2;
            else if (r->hs == 2 && r->vs == 2)
                r->resample = stbi__resample_row_hv_2;
            else
                r->resample = stbi__resample_row_generic;
        }

        // can't error after this so, this is safe
        output = (stbi_uc *)stbi__malloc(n * z->s->img_x * z->s->img_y + 1);
        if (!output) {
            stbi__cleanup_jpeg(z);
            return stbi__errpuc("outofmem", "Out of memory");
        }

        // now go ahead and resample
        for (j = 0; j < z->s->img_y; ++j) {
            stbi_uc *out = output + n * z->s->img_x * j;
            for (k = 0; k < decode_n; ++k) {
                stbi__resample *r = &res_comp[k];
                int y_bot = r->ystep >= (r->vs >> 1);
                coutput[k] = r->resample(
                    z->img_comp[k].linebuf, y_bot ? r->line1 : r->line0,
                    y_bot ? r->line0 : r->line1, r->w_lores, r->hs);
                if (++r->ystep >= r->vs) {
                    r->ystep = 0;
                    r->line0 = r->line1;
                    if (++r->ypos < z->img_comp[k].y)
                        r->line1 += z->img_comp[k].w2;
                }
            }
            if (n >= 3) {
                stbi_uc *y = coutput[0];
                if (z->s->img_n == 3) {
                    stbi__YCbCr_to_RGB_row(out, y, coutput[1], coutput[2],
                                           z->s->img_x, n);
                } else
                    for (i = 0; i < z->s->img_x; ++i) {
                        out[0] = out[1] = out[2] = y[i];
                        out[3] = 255;  // not used if n==3
                        out += n;
                    }
            } else {
                stbi_uc *y = coutput[0];
                if (n == 1)
                    for (i = 0; i < z->s->img_x; ++i) out[i] = y[i];
                else
                    for (i = 0; i < z->s->img_x; ++i)
                        *out++ = y[i], *out++ = 255;
            }
        }
        stbi__cleanup_jpeg(z);
        *out_x = z->s->img_x;
        *out_y = z->s->img_y;
        if (comp)
            *comp = z->s->img_n;  // report original components, not output
        return output;
    }
}

static stbi_uc *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp) {
    stbi__jpeg j;
    j.s = s;
    return load_jpeg_image(&j, x, y, comp);
}

static int stbi__jpeg_test(stbi__context *s) {
    int r;
    stbi__jpeg j;
    j.s = s;
    r = decode_jpeg_header(&j, SCAN_type);
    stbi__rewind(s);
    return r;
}

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS 9  // accelerate all cases in default tables
#define STBI__ZFAST_MASK ((1 << STBI__ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct {
    stbi__uint16 fast[1 << STBI__ZFAST_BITS];
    stbi__uint16 firstcode[16];
    int maxcode[17];
    stbi__uint16 firstsymbol[16];
    stbi_uc size[288];
    stbi__uint16 value[288];
} stbi__zhuffman;

stbi_inline static int stbi__bitreverse16(int n) {
    n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
    n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
    n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
    return n;
}

stbi_inline static int stbi__bit_reverse(int v, int bits) {
    STBI_ASSERT(bits <= 16);
    // to bit reverse n bits, reverse 16 and shift
    // e.g. 11 bits, bit reverse and shift away 5
    return stbi__bitreverse16(v) >> (16 - bits);
}

static int stbi__zbuild_huffman(stbi__zhuffman *z, stbi_uc *sizelist, int num) {
    int i, k = 0;
    int code, next_code[16], sizes[17];

    // DEFLATE spec for generating codes
    memset(sizes, 0, sizeof(sizes));
    memset(z->fast, 255, sizeof(z->fast));
    for (i = 0; i < num; ++i) ++sizes[sizelist[i]];
    sizes[0] = 0;
    for (i = 1; i < 16; ++i) STBI_ASSERT(sizes[i] <= (1 << i));
    code = 0;
    for (i = 1; i < 16; ++i) {
        next_code[i] = code;
        z->firstcode[i] = (stbi__uint16)code;
        z->firstsymbol[i] = (stbi__uint16)k;
        code = (code + sizes[i]);
        if (sizes[i])
            if (code - 1 >= (1 << i))
                return stbi__err("bad codelengths", "Corrupt JPEG");
        z->maxcode[i] = code << (16 - i);  // preshift for inner loop
        code <<= 1;
        k += sizes[i];
    }
    z->maxcode[16] = 0x10000;  // sentinel
    for (i = 0; i < num; ++i) {
        int s = sizelist[i];
        if (s) {
            int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
            z->size[c] = (stbi_uc)s;
            z->value[c] = (stbi__uint16)i;
            if (s <= STBI__ZFAST_BITS) {
                int k = stbi__bit_reverse(next_code[s], s);
                while (k < (1 << STBI__ZFAST_BITS)) {
                    z->fast[k] = (stbi__uint16)c;
                    k += (1 << s);
                }
            }
            ++next_code[s];
        }
    }
    return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct {
    stbi_uc *zbuffer, *zbuffer_end;
    int num_bits;
    stbi__uint32 code_buffer;

    char *zout;
    char *zout_start;
    char *zout_end;
    int z_expandable;

    stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

stbi_inline static stbi_uc stbi__zget8(stbi__zbuf *z) {
    if (z->zbuffer >= z->zbuffer_end) return 0;
    return *z->zbuffer++;
}

static void stbi__fill_bits(stbi__zbuf *z) {
    do {
        STBI_ASSERT(z->code_buffer < (1U << z->num_bits));
        z->code_buffer |= stbi__zget8(z) << z->num_bits;
        z->num_bits += 8;
    } while (z->num_bits <= 24);
}

stbi_inline static stbi__uint32 stbi__zreceive(stbi__zbuf *z, int n) {
    stbi__uint32 k;
    if (z->num_bits < n) stbi__fill_bits(z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}

stbi_inline static int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z) {
    int b, s, k;
    if (a->num_bits < 16) stbi__fill_bits(a);
    b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
    if (b < 0xffff) {
        s = z->size[b];
        a->code_buffer >>= s;
        a->num_bits -= s;
        return z->value[b];
    }

    // not resolved by fast table, so compute it the slow way
    // use jpeg approach, which requires MSbits at top
    k = stbi__bit_reverse(a->code_buffer, 16);
    for (s = STBI__ZFAST_BITS + 1;; ++s)
        if (k < z->maxcode[s]) break;
    if (s == 16) return -1;  // invalid code!
    // code size is s, so:
    b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
    STBI_ASSERT(z->size[b] == s);
    a->code_buffer >>= s;
    a->num_bits -= s;
    return z->value[b];
}

static int stbi__zexpand(stbi__zbuf *z, int n)  // need to make room for n bytes
{
    char *q;
    int cur, limit;
    if (!z->z_expandable)
        return stbi__err("output buffer limit", "Corrupt PNG");
    cur = (int)(z->zout - z->zout_start);
    limit = (int)(z->zout_end - z->zout_start);
    while (cur + n > limit) limit *= 2;
    q = (char *)realloc(z->zout_start, limit);
    if (q == nullptr) return stbi__err("outofmem", "Out of memory");
    z->zout_start = q;
    z->zout = q + cur;
    z->zout_end = q + limit;
    return 1;
}

static int stbi__zlength_base[31] = {
    3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};

static int stbi__zlength_extra[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
                                      1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
                                      4, 4, 5, 5, 5, 5, 0, 0, 0};

static int stbi__zdist_base[32] = {
    1,    2,    3,    4,    5,    7,     9,     13,    17,  25,   33,
    49,   65,   97,   129,  193,  257,   385,   513,   769, 1025, 1537,
    2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0};

static int stbi__zdist_extra[32] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                    4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                    9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static int stbi__parse_huffman_block(stbi__zbuf *a) {
    for (;;) {
        int z = stbi__zhuffman_decode(a, &a->z_length);
        if (z < 256) {
            if (z < 0)
                return stbi__err("bad huffman code",
                                 "Corrupt PNG");  // error in huffman codes
            if (a->zout >= a->zout_end)
                if (!stbi__zexpand(a, 1)) return 0;
            *a->zout++ = (char)z;
        } else {
            stbi_uc *p;
            int len, dist;
            if (z == 256) return 1;
            z -= 257;
            len = stbi__zlength_base[z];
            if (stbi__zlength_extra[z])
                len += stbi__zreceive(a, stbi__zlength_extra[z]);
            z = stbi__zhuffman_decode(a, &a->z_distance);
            if (z < 0) return stbi__err("bad huffman code", "Corrupt PNG");
            dist = stbi__zdist_base[z];
            if (stbi__zdist_extra[z])
                dist += stbi__zreceive(a, stbi__zdist_extra[z]);
            if (a->zout - a->zout_start < dist)
                return stbi__err("bad dist", "Corrupt PNG");
            if (a->zout + len > a->zout_end)
                if (!stbi__zexpand(a, len)) return 0;
            p = (stbi_uc *)(a->zout - dist);
            while (len--) *a->zout++ = *p++;
        }
    }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a) {
    static stbi_uc length_dezigzag[19] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                          11, 4,  12, 3, 13, 2, 14, 1, 15};
    stbi__zhuffman z_codelength;
    stbi_uc lencodes[286 + 32 + 137];  // padding for maximum single op
    stbi_uc codelength_sizes[19];
    int i, n;

    int hlit = stbi__zreceive(a, 5) + 257;
    int hdist = stbi__zreceive(a, 5) + 1;
    int hclen = stbi__zreceive(a, 4) + 4;

    memset(codelength_sizes, 0, sizeof(codelength_sizes));
    for (i = 0; i < hclen; ++i) {
        int s = stbi__zreceive(a, 3);
        codelength_sizes[length_dezigzag[i]] = (stbi_uc)s;
    }
    if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

    n = 0;
    while (n < hlit + hdist) {
        int c = stbi__zhuffman_decode(a, &z_codelength);
        STBI_ASSERT(c >= 0 && c < 19);
        if (c < 16)
            lencodes[n++] = (stbi_uc)c;
        else if (c == 16) {
            c = stbi__zreceive(a, 2) + 3;
            memset(lencodes + n, lencodes[n - 1], c);
            n += c;
        } else if (c == 17) {
            c = stbi__zreceive(a, 3) + 3;
            memset(lencodes + n, 0, c);
            n += c;
        } else {
            STBI_ASSERT(c == 18);
            c = stbi__zreceive(a, 7) + 11;
            memset(lencodes + n, 0, c);
            n += c;
        }
    }
    if (n != hlit + hdist) return stbi__err("bad codelengths", "Corrupt PNG");
    if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
    if (!stbi__zbuild_huffman(&a->z_distance, lencodes + hlit, hdist)) return 0;
    return 1;
}

static int stbi__parse_uncomperssed_block(stbi__zbuf *a) {
    stbi_uc header[4];
    int len, nlen, k;
    if (a->num_bits & 7) stbi__zreceive(a, a->num_bits & 7);  // discard
    // drain the bit-packed data into header
    k = 0;
    while (a->num_bits > 0) {
        header[k++] =
            (stbi_uc)(a->code_buffer & 255);  // suppress MSVC run-time check
        a->code_buffer >>= 8;
        a->num_bits -= 8;
    }
    STBI_ASSERT(a->num_bits == 0);
    // now fill header the normal way
    while (k < 4) header[k++] = stbi__zget8(a);
    len = header[1] * 256 + header[0];
    nlen = header[3] * 256 + header[2];
    if (nlen != (len ^ 0xffff)) return stbi__err("zlib corrupt", "Corrupt PNG");
    if (a->zbuffer + len > a->zbuffer_end)
        return stbi__err("read past buffer", "Corrupt PNG");
    if (a->zout + len > a->zout_end)
        if (!stbi__zexpand(a, len)) return 0;
    memcpy(a->zout, a->zbuffer, len);
    a->zbuffer += len;
    a->zout += len;
    return 1;
}

static int stbi__parse_zlib_header(stbi__zbuf *a) {
    int cmf = stbi__zget8(a);
    int cm = cmf & 15;
    /* int cinfo = cmf >> 4; */
    int flg = stbi__zget8(a);
    if ((cmf * 256 + flg) % 31 != 0)
        return stbi__err("bad zlib header", "Corrupt PNG");  // zlib spec
    if (flg & 32)
        return stbi__err(
            "no preset dict",
            "Corrupt PNG");  // preset dictionary not allowed in png
    if (cm != 8)
        return stbi__err("bad compression",
                         "Corrupt PNG");  // DEFLATE required for png
    // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
    return 1;
}

// should statically initialize these for optimal thread safety
static stbi_uc stbi__zdefault_length[288], stbi__zdefault_distance[32];
static void stbi__init_zdefaults(void) {
    int i;  // use <= to match clearly with spec
    for (i = 0; i <= 143; ++i) stbi__zdefault_length[i] = 8;
    for (; i <= 255; ++i) stbi__zdefault_length[i] = 9;
    for (; i <= 279; ++i) stbi__zdefault_length[i] = 7;
    for (; i <= 287; ++i) stbi__zdefault_length[i] = 8;

    for (i = 0; i <= 31; ++i) stbi__zdefault_distance[i] = 5;
}

static int stbi__parse_zlib(stbi__zbuf *a, int parse_header) {
    int final, type;
    if (parse_header)
        if (!stbi__parse_zlib_header(a)) return 0;
    a->num_bits = 0;
    a->code_buffer = 0;
    do {
        final = stbi__zreceive(a, 1);
        type = stbi__zreceive(a, 2);
        if (type == 0) {
            if (!stbi__parse_uncomperssed_block(a)) return 0;
        } else if (type == 3) {
            return 0;
        } else {
            if (type == 1) {
                // use fixed code lengths
                if (!stbi__zdefault_distance[31]) stbi__init_zdefaults();
                if (!stbi__zbuild_huffman(&a->z_length, stbi__zdefault_length,
                                          288))
                    return 0;
                if (!stbi__zbuild_huffman(&a->z_distance,
                                          stbi__zdefault_distance, 32))
                    return 0;
            } else {
                if (!stbi__compute_huffman_codes(a)) return 0;
            }
            if (!stbi__parse_huffman_block(a)) return 0;
        }
    } while (!final);
    return 1;
}

static int stbi__do_zlib(stbi__zbuf *a, char *obuf, int olen, int exp,
                         int parse_header) {
    a->zout_start = obuf;
    a->zout = obuf;
    a->zout_end = obuf + olen;
    a->z_expandable = exp;

    return stbi__parse_zlib(a, parse_header);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len,
                                                int initial_size, int *outlen) {
    stbi__zbuf a;
    char *p = (char *)stbi__malloc(initial_size);
    if (p == nullptr) return nullptr;
    a.zbuffer = (stbi_uc *)buffer;
    a.zbuffer_end = (stbi_uc *)buffer + len;
    if (stbi__do_zlib(&a, p, initial_size, 1, 1)) {
        if (outlen) *outlen = (int)(a.zout - a.zout_start);
        return a.zout_start;
    } else {
        free(a.zout_start);
        return nullptr;
    }
}

STBIDEF char *stbi_zlib_decode_malloc(char const *buffer, int len,
                                      int *outlen) {
    return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer,
                                                           int len,
                                                           int initial_size,
                                                           int *outlen,
                                                           int parse_header) {
    stbi__zbuf a;
    char *p = (char *)stbi__malloc(initial_size);
    if (p == nullptr) return nullptr;
    a.zbuffer = (stbi_uc *)buffer;
    a.zbuffer_end = (stbi_uc *)buffer + len;
    if (stbi__do_zlib(&a, p, initial_size, 1, parse_header)) {
        if (outlen) *outlen = (int)(a.zout - a.zout_start);
        return a.zout_start;
    } else {
        free(a.zout_start);
        return nullptr;
    }
}

// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding

typedef struct {
    stbi__uint32 length;
    stbi__uint32 type;
} stbi__pngchunk;

#define PNG_TYPE(a, b, c, d) (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

static stbi__pngchunk stbi__get_chunk_header(stbi__context *s) {
    stbi__pngchunk c;
    c.length = stbi__get32be(s);
    c.type = stbi__get32be(s);
    return c;
}

static int stbi__check_png_header(stbi__context *s) {
    static stbi_uc png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    int i;
    for (i = 0; i < 8; ++i)
        if (stbi__get8(s) != png_sig[i])
            return stbi__err("bad png sig", "Not a PNG");
    return 1;
}

typedef struct {
    stbi__context *s;
    stbi_uc *idata, *expanded, *out;
} stbi__png;

enum {
    STBI__F_none = 0,
    STBI__F_sub = 1,
    STBI__F_up = 2,
    STBI__F_avg = 3,
    STBI__F_paeth = 4,
    STBI__F_avg_first,
    STBI__F_paeth_first
};

static stbi_uc first_row_filter[5] = {STBI__F_none, STBI__F_sub, STBI__F_none,
                                      STBI__F_avg_first, STBI__F_paeth_first};

static int stbi__paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

#define STBI__BYTECAST(x) \
    ((stbi_uc)((x) & 255))  // truncate int to byte without warnings

// create the png data from post-deflated data
static int stbi__create_png_image_raw(stbi__png *a, stbi_uc *raw,
                                      stbi__uint32 raw_len, int out_n,
                                      stbi__uint32 x, stbi__uint32 y) {
    stbi__context *s = a->s;
    stbi__uint32 i, j, stride = x * out_n;
    int k;
    int img_n = s->img_n;  // copy it into a local for later
    STBI_ASSERT(out_n == s->img_n || out_n == s->img_n + 1);
    a->out = (stbi_uc *)stbi__malloc(x * y * out_n);
    if (!a->out) return stbi__err("outofmem", "Out of memory");
    if (s->img_x == x && s->img_y == y) {
        if (raw_len != (img_n * x + 1) * y)
            return stbi__err("not enough pixels", "Corrupt PNG");
    } else {  // interlaced:
        if (raw_len < (img_n * x + 1) * y)
            return stbi__err("not enough pixels", "Corrupt PNG");
    }
    for (j = 0; j < y; ++j) {
        stbi_uc *cur = a->out + stride * j;
        stbi_uc *prior = cur - stride;
        int filter = *raw++;
        if (filter > 4) return stbi__err("invalid filter", "Corrupt PNG");
        // if first row, use special filter that doesn't sample previous row
        if (j == 0) filter = first_row_filter[filter];
        // Handle first pixel explicitly
        for (k = 0; k < img_n; ++k) {
            switch (filter) {
                case STBI__F_none:
                    cur[k] = raw[k];
                    break;
                case STBI__F_sub:
                    cur[k] = raw[k];
                    break;
                case STBI__F_up:
                    cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
                    break;
                case STBI__F_avg:
                    cur[k] = STBI__BYTECAST(raw[k] + (prior[k] >> 1));
                    break;
                case STBI__F_paeth:
                    cur[k] =
                        STBI__BYTECAST(raw[k] + stbi__paeth(0, prior[k], 0));
                    break;
                case STBI__F_avg_first:
                    cur[k] = raw[k];
                    break;
                case STBI__F_paeth_first:
                    cur[k] = raw[k];
                    break;
            }
        }
        if (img_n != out_n) cur[img_n] = 255;
        raw += img_n;
        cur += out_n;
        prior += out_n;
        // this is a little gross, so that we don't switch per-pixel or
        // per-component
        if (img_n == out_n) {
#define CASE(f)                                               \
    case f:                                                   \
        for (i = x - 1; i >= 1;                               \
             --i, raw += img_n, cur += img_n, prior += img_n) \
            for (k = 0; k < img_n; ++k)
            switch (filter) {
                CASE(STBI__F_none) cur[k] = raw[k];
                break;
                CASE(STBI__F_sub)
                cur[k] = STBI__BYTECAST(raw[k] + cur[k - img_n]);
                break;
                CASE(STBI__F_up) cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
                break;
                CASE(STBI__F_avg)
                cur[k] =
                    STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - img_n]) >> 1));
                break;
                CASE(STBI__F_paeth)
                cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - img_n],
                                                             prior[k],
                                                             prior[k - img_n]));
                break;
                CASE(STBI__F_avg_first)
                cur[k] = STBI__BYTECAST(raw[k] + (cur[k - img_n] >> 1));
                break;
                CASE(STBI__F_paeth_first)
                cur[k] =
                    STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - img_n], 0, 0));
                break;
            }
#undef CASE
        } else {
            STBI_ASSERT(img_n + 1 == out_n);
#define CASE(f)                                                      \
    case f:                                                          \
        for (i = x - 1; i >= 1; --i, cur[img_n] = 255, raw += img_n, \
            cur += out_n, prior += out_n)                            \
            for (k = 0; k < img_n; ++k)
            switch (filter) {
                CASE(STBI__F_none) cur[k] = raw[k];
                break;
                CASE(STBI__F_sub)
                cur[k] = STBI__BYTECAST(raw[k] + cur[k - out_n]);
                break;
                CASE(STBI__F_up) cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
                break;
                CASE(STBI__F_avg)
                cur[k] =
                    STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - out_n]) >> 1));
                break;
                CASE(STBI__F_paeth)
                cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - out_n],
                                                             prior[k],
                                                             prior[k - out_n]));
                break;
                CASE(STBI__F_avg_first)
                cur[k] = STBI__BYTECAST(raw[k] + (cur[k - out_n] >> 1));
                break;
                CASE(STBI__F_paeth_first)
                cur[k] =
                    STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - out_n], 0, 0));
                break;
            }
#undef CASE
        }
    }
    return 1;
}

static int stbi__create_png_image(stbi__png *a, stbi_uc *raw,
                                  stbi__uint32 raw_len, int out_n,
                                  int interlaced) {
    stbi_uc *final;
    int p;
    if (!interlaced)
        return stbi__create_png_image_raw(a, raw, raw_len, out_n, a->s->img_x,
                                          a->s->img_y);

    // de-interlacing
    final = (stbi_uc *)stbi__malloc(a->s->img_x * a->s->img_y * out_n);
    for (p = 0; p < 7; ++p) {
        int xorig[] = {0, 4, 0, 2, 0, 1, 0};
        int yorig[] = {0, 0, 4, 0, 2, 0, 1};
        int xspc[] = {8, 8, 4, 4, 2, 2, 1};
        int yspc[] = {8, 8, 8, 4, 4, 2, 2};
        int i, j, x, y;
        // pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
        x = (a->s->img_x - xorig[p] + xspc[p] - 1) / xspc[p];
        y = (a->s->img_y - yorig[p] + yspc[p] - 1) / yspc[p];
        if (x && y) {
            if (!stbi__create_png_image_raw(a, raw, raw_len, out_n, x, y)) {
                free(final);
                return 0;
            }
            for (j = 0; j < y; ++j)
                for (i = 0; i < x; ++i)
                    memcpy(final +
                               (j * yspc[p] + yorig[p]) * a->s->img_x * out_n +
                               (i * xspc[p] + xorig[p]) * out_n,
                           a->out + (j * x + i) * out_n, out_n);
            free(a->out);
            raw += (x * out_n + 1) * y;
            raw_len -= (x * out_n + 1) * y;
        }
    }
    a->out = final;

    return 1;
}

static int stbi__compute_transparency(stbi__png *z, stbi_uc tc[3], int out_n) {
    stbi__context *s = z->s;
    stbi__uint32 i, pixel_count = s->img_x * s->img_y;
    stbi_uc *p = z->out;

    // compute color-based transparency, assuming we've
    // already got 255 as the alpha value in the output
    STBI_ASSERT(out_n == 2 || out_n == 4);

    if (out_n == 2) {
        for (i = 0; i < pixel_count; ++i) {
            p[1] = (p[0] == tc[0] ? 0 : 255);
            p += 2;
        }
    } else {
        for (i = 0; i < pixel_count; ++i) {
            if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2]) p[3] = 0;
            p += 4;
        }
    }
    return 1;
}

static int stbi__expand_png_palette(stbi__png *a, stbi_uc *palette, int len,
                                    int pal_img_n) {
    stbi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
    stbi_uc *p, *temp_out, *orig = a->out;

    p = (stbi_uc *)stbi__malloc(pixel_count * pal_img_n);
    if (p == nullptr) return stbi__err("outofmem", "Out of memory");

    // between here and free(out) below, exitting would leak
    temp_out = p;

    if (pal_img_n == 3) {
        for (i = 0; i < pixel_count; ++i) {
            int n = orig[i] * 4;
            p[0] = palette[n];
            p[1] = palette[n + 1];
            p[2] = palette[n + 2];
            p += 3;
        }
    } else {
        for (i = 0; i < pixel_count; ++i) {
            int n = orig[i] * 4;
            p[0] = palette[n];
            p[1] = palette[n + 1];
            p[2] = palette[n + 2];
            p[3] = palette[n + 3];
            p += 4;
        }
    }
    free(a->out);
    a->out = temp_out;

    STBI_NOTUSED(len);

    return 1;
}

static int stbi__parse_png_file(stbi__png *z, int scan) {
    stbi_uc palette[1024], pal_img_n = 0;
    stbi_uc has_trans = 0, tc[3];
    stbi__uint32 ioff = 0, idata_limit = 0, i, pal_len = 0;
    int first = 1, k, interlace = 0, is_iphone = 0;
    stbi__context *s = z->s;

    z->expanded = nullptr;
    z->idata = nullptr;
    z->out = nullptr;

    if (!stbi__check_png_header(s)) return 0;

    if (scan == SCAN_type) return 1;

    for (;;) {
        stbi__pngchunk c = stbi__get_chunk_header(s);
        switch (c.type) {
            case PNG_TYPE('C', 'g', 'B', 'I'):
                is_iphone = 1;
                stbi__skip(s, c.length);
                break;
            case PNG_TYPE('I', 'H', 'D', 'R'): {
                int depth, color, comp, filter;
                if (!first) return stbi__err("multiple IHDR", "Corrupt PNG");
                first = 0;
                if (c.length != 13)
                    return stbi__err("bad IHDR len", "Corrupt PNG");
                s->img_x = stbi__get32be(s);
                if (s->img_x > (1 << 24))
                    return stbi__err("too large",
                                     "Very large image (corrupt?)");
                s->img_y = stbi__get32be(s);
                if (s->img_y > (1 << 24))
                    return stbi__err("too large",
                                     "Very large image (corrupt?)");
                depth = stbi__get8(s);
                if (depth != 8)
                    return stbi__err("8bit only",
                                     "PNG not supported: 8-bit only");
                color = stbi__get8(s);
                if (color > 6) return stbi__err("bad ctype", "Corrupt PNG");
                if (color == 3)
                    pal_img_n = 3;
                else if (color & 1)
                    return stbi__err("bad ctype", "Corrupt PNG");
                comp = stbi__get8(s);
                if (comp) return stbi__err("bad comp method", "Corrupt PNG");
                filter = stbi__get8(s);
                if (filter)
                    return stbi__err("bad filter method", "Corrupt PNG");
                interlace = stbi__get8(s);
                if (interlace > 1)
                    return stbi__err("bad interlace method", "Corrupt PNG");
                if (!s->img_x || !s->img_y)
                    return stbi__err("0-pixel image", "Corrupt PNG");
                if (!pal_img_n) {
                    s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
                    if ((1 << 30) / s->img_x / s->img_n < s->img_y)
                        return stbi__err("too large",
                                         "Image too large to decode");
                    if (scan == SCAN_header) return 1;
                } else {
                    // if paletted, then pal_n is our final components, and
                    // img_n is # components to decompress/filter.
                    s->img_n = 1;
                    if ((1 << 30) / s->img_x / 4 < s->img_y)
                        return stbi__err("too large", "Corrupt PNG");
                    // if SCAN_header, have to scan to see if we have a tRNS
                }
                break;
            }

            case PNG_TYPE('P', 'L', 'T', 'E'): {
                if (first) return stbi__err("first not IHDR", "Corrupt PNG");
                if (c.length > 256 * 3)
                    return stbi__err("invalid PLTE", "Corrupt PNG");
                pal_len = c.length / 3;
                if (pal_len * 3 != c.length)
                    return stbi__err("invalid PLTE", "Corrupt PNG");
                for (i = 0; i < pal_len; ++i) {
                    palette[i * 4 + 0] = stbi__get8(s);
                    palette[i * 4 + 1] = stbi__get8(s);
                    palette[i * 4 + 2] = stbi__get8(s);
                    palette[i * 4 + 3] = 255;
                }
                break;
            }

            case PNG_TYPE('t', 'R', 'N', 'S'): {
                if (first) return stbi__err("first not IHDR", "Corrupt PNG");
                if (z->idata)
                    return stbi__err("tRNS after IDAT", "Corrupt PNG");
                if (pal_img_n) {
                    if (scan == SCAN_header) {
                        s->img_n = 4;
                        return 1;
                    }
                    if (pal_len == 0)
                        return stbi__err("tRNS before PLTE", "Corrupt PNG");
                    if (c.length > pal_len)
                        return stbi__err("bad tRNS len", "Corrupt PNG");
                    pal_img_n = 4;
                    for (i = 0; i < c.length; ++i)
                        palette[i * 4 + 3] = stbi__get8(s);
                } else {
                    if (!(s->img_n & 1))
                        return stbi__err("tRNS with alpha", "Corrupt PNG");
                    if (c.length != (stbi__uint32)s->img_n * 2)
                        return stbi__err("bad tRNS len", "Corrupt PNG");
                    has_trans = 1;
                    for (k = 0; k < s->img_n; ++k)
                        tc[k] =
                            (stbi_uc)(stbi__get16be(s) &
                                      255);  // non 8-bit images will be larger
                }
                break;
            }

            case PNG_TYPE('I', 'D', 'A', 'T'): {
                if (first) return stbi__err("first not IHDR", "Corrupt PNG");
                if (pal_img_n && !pal_len)
                    return stbi__err("no PLTE", "Corrupt PNG");
                if (scan == SCAN_header) {
                    s->img_n = pal_img_n;
                    return 1;
                }
                if (ioff + c.length > idata_limit) {
                    stbi_uc *p;
                    if (idata_limit == 0)
                        idata_limit = c.length > 4096 ? c.length : 4096;
                    while (ioff + c.length > idata_limit) idata_limit *= 2;
                    p = (stbi_uc *)realloc(z->idata, idata_limit);
                    if (p == nullptr)
                        return stbi__err("outofmem", "Out of memory");
                    z->idata = p;
                }
                if (!stbi__getn(s, z->idata + ioff, c.length))
                    return stbi__err("outofdata", "Corrupt PNG");
                ioff += c.length;
                break;
            }

            case PNG_TYPE('I', 'E', 'N', 'D'): {
                stbi__uint32 raw_len;
                if (first) return stbi__err("first not IHDR", "Corrupt PNG");
                if (scan != SCAN_load) return 1;
                if (z->idata == nullptr)
                    return stbi__err("no IDAT", "Corrupt PNG");
                z->expanded =
                    (stbi_uc *)stbi_zlib_decode_malloc_guesssize_headerflag(
                        (char *)z->idata, ioff, 16384, (int *)&raw_len,
                        !is_iphone);
                if (z->expanded == nullptr) return 0;  // zlib should set error
                free(z->idata);
                z->idata = nullptr;
                if (has_trans)
                    s->img_out_n = s->img_n + 1;
                else
                    s->img_out_n = s->img_n;
                if (!stbi__create_png_image(z, z->expanded, raw_len,
                                            s->img_out_n, interlace))
                    return 0;
                if (has_trans)
                    if (!stbi__compute_transparency(z, tc, s->img_out_n))
                        return 0;
                if (pal_img_n) {
                    // pal_img_n == 3 or 4
                    s->img_n = pal_img_n;  // record the actual colors we had
                    s->img_out_n = pal_img_n;
                    if (!stbi__expand_png_palette(z, palette, pal_len,
                                                  s->img_out_n))
                        return 0;
                }
                free(z->expanded);
                z->expanded = nullptr;
                return 1;
            }

            default:
                // if critical, fail
                if (first) return stbi__err("first not IHDR", "Corrupt PNG");
                if ((c.type & (1 << 29)) == 0) {
#ifndef STBI_NO_FAILURE_STRINGS
                    // not threadsafe
                    static char invalid_chunk[] = "XXXX PNG chunk not known";
                    invalid_chunk[0] = STBI__BYTECAST(c.type >> 24);
                    invalid_chunk[1] = STBI__BYTECAST(c.type >> 16);
                    invalid_chunk[2] = STBI__BYTECAST(c.type >> 8);
                    invalid_chunk[3] = STBI__BYTECAST(c.type >> 0);
#endif
                    return stbi__err(
                        invalid_chunk,
                        "PNG not supported: unknown PNG chunk type");
                }
                stbi__skip(s, c.length);
                break;
        }
        // end of PNG chunk, read and skip CRC
        stbi__get32be(s);
    }
}

static stbi_uc *stbi__do_png(stbi__png *p, int *x, int *y, int *n) {
    stbi_uc *result = nullptr;
    if (stbi__parse_png_file(p, SCAN_load)) {
        result = p->out;
        p->out = nullptr;
        *x = p->s->img_x;
        *y = p->s->img_y;
        *n = p->s->img_n;
    }
    free(p->out);
    p->out = nullptr;
    free(p->expanded);
    p->expanded = nullptr;
    free(p->idata);
    p->idata = nullptr;

    return result;
}

static stbi_uc *stbi__png_load(stbi__context *s, int *x, int *y, int *comp) {
    stbi__png p;
    p.s = s;
    return stbi__do_png(&p, x, y, comp);
}

static int stbi__png_test(stbi__context *s) {
    int r;
    r = stbi__check_png_header(s);
    stbi__rewind(s);
    return r;
}

// ================================================================================================
// Converts the loaded image to the requested format.

#define COMBO(a, b) ((a) * 8 + (b))

#define CASE(a, b)    \
    case COMBO(a, b): \
        for (int x = w - 1; x >= 0; --x, s += inChannels, d += outChannels)

static stbi_uc ComputeY(int r, int g, int b) {
    return (stbi_uc)(((r * 77) + (g * 150) + (29 * b)) >> 8);
}

static stbi_uc *ConvertImage(stbi_uc *data, int inChannels,
                             ImageLoader::Format fmt, int w, int h) {
    static const int sFmtChannels[] = {4, 3, 2, 1, 1};
    int outChannels = sFmtChannels[fmt];
    if (outChannels == inChannels) return data;

    stbi_uc *out = (stbi_uc *)stbi__malloc(outChannels * w * h);
    if (!out) {
        free(data);
        return stbi__errpuc("outofmem", "Out of memory");
    }

    for (int y = 0; y < h; ++y) {
        stbi_uc *s = data + y * w * inChannels;
        stbi_uc *d = out + y * w * outChannels;
        switch (COMBO(fmt, inChannels)) {
            CASE(ImageLoader::RGBA, 1)
            d[0] = s[0], d[1] = s[0], d[2] = s[0], d[3] = 255;
            break;
            CASE(ImageLoader::RGBA, 2)
            d[0] = s[0], d[1] = s[0], d[2] = s[0], d[3] = s[1];
            break;
            CASE(ImageLoader::RGBA, 3)
            d[0] = s[0], d[1] = s[1], d[2] = s[2], d[3] = 255;
            break;

            CASE(ImageLoader::RGB, 1) d[0] = s[0], d[1] = s[0], d[2] = s[0];
            break;
            CASE(ImageLoader::RGB, 2) d[0] = s[0], d[1] = s[0], d[2] = s[0];
            break;
            CASE(ImageLoader::RGB, 4) d[0] = s[0], d[1] = s[1], d[2] = s[2];
            break;

            CASE(ImageLoader::LUMA, 1) d[0] = s[0], d[1] = 255;
            break;
            CASE(ImageLoader::LUMA, 3)
            d[0] = ComputeY(s[0], s[1], s[2]), d[1] = 255;
            break;
            CASE(ImageLoader::LUMA, 4)
            d[0] = ComputeY(s[0], s[1], s[2]), d[1] = s[3];
            break;

            CASE(ImageLoader::LUM, 2) d[0] = s[0];
            break;
            CASE(ImageLoader::LUM, 3) d[0] = ComputeY(s[0], s[1], s[2]);
            break;
            CASE(ImageLoader::LUM, 4) d[0] = ComputeY(s[0], s[1], s[2]);
            break;

            CASE(ImageLoader::ALPHA, 2) d[0] = s[1];
            break;
            CASE(ImageLoader::ALPHA, 3) d[0] = ComputeY(s[0], s[1], s[2]);
            break;
            CASE(ImageLoader::ALPHA, 4) d[0] = s[3];
            break;

            default:
                STBI_ASSERT(0);
        }
    }

    free(data);
    return out;
}

#undef COMBO
#undef CASE

};  // anonymous namespace.
namespace Vortex {

// ================================================================================================
// ImageLoader :: API functions.

ImageLoader::Data ImageLoader::load(const char *path, ImageLoader::Format fmt) {
    int cmp = 0, w = 0, h = 0;
    ImageLoader::Data out = {nullptr, 0, 0};
    stbi_io_callbacks callback;
    callback.file.open(Widen(path).str(), std::ios::in | std::ios::binary);
    if (callback.file.good()) {
        stbi_uc *pixels = stbi_load_from_callbacks(callback, &w, &h, &cmp);
        if (pixels && w > 0 && h > 0) {
            out.pixels = ConvertImage(pixels, cmp, fmt, w, h);
            out.width = w, out.height = h;
        } else {
            Debug::blockBegin(Debug::WARNING, "could not load image");
            Debug::log("file: %s\n", path);
            if (stbi__g_failure_reason && stbi__g_failure_details) {
                Debug::log("reason: %s\n", stbi__g_failure_reason);
                Debug::log("details: %s\n", stbi__g_failure_details);
            }
            Debug::blockEnd();
        }
    }
    return out;
}

void ImageLoader::release(ImageLoader::Data &data) {
    stbi_image_free(data.pixels);
}

Zlib::Data Zlib::deflate(const stbi_uc *data, int inSize) {
    int numBytes = 0;
    Zlib::Data out = {nullptr, 0};
    out.data = (stbi_uc *)stbi_zlib_decode_malloc((const char *)data, inSize,
                                                  &numBytes);
    if (out.data) out.numBytes = numBytes;
    return out;
}

void Zlib::release(Zlib::Data &data) { free(data.data); }

};  // namespace Vortex
