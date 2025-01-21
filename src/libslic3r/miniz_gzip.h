// miniz_gzip: gzip and gunzip using the miniz library: https://github.com/richgel999/miniz (a lightweight
//  zlib alternative)
// Also implements a "block gzip" format providing block-level random read access and the ability to replace
//  or append blocks w/o rewriting blocks before the first modified block - see below
// The block index is stored in the "extra" field of the gzip header which is supported by gzip and most other
//  compression utlities, but not some other applications with incomplete gzip support.  Storing the index
//  elsewhere, e.g., a separate file, is also possible.

#ifndef MINIZ_GZIP_H
#define MINIZ_GZIP_H

#ifdef __cplusplus
#include <iostream>
#include <stdint.h>

static size_t stream_read(void* dest, size_t len, void* ctx)
{
  static_cast<std::iostream*>(ctx)->read((char*)dest, len);
  return static_cast<std::iostream*>(ctx)->gcount();
}

static size_t stream_write(const void* src, size_t len, void* ctx)
{
  static_cast<std::iostream*>(ctx)->write((const char*)src, len);
  return len;  //static_cast<std::iostream*>(ctx)->gcount();
}

static void stream_seek(long offset, int origin, void* ctx)
{
  auto s = static_cast<std::iostream*>(ctx);
  // seekg clears eofbit, but doesn't set goodbit, so it fails if at end!
  s->clear();
  // fstream has only a single file position and doing both seekg and seekp from curr position will give
  //  twice desired offset, so we must always seek from beginning
  long offsetg = origin == SEEK_SET ? offset : offset + s->tellg();
  long offsetp = origin == SEEK_SET ? offset : offset + s->tellp();
  s->seekg(offsetg, std::ios_base::beg);
  s->seekp(offsetp, std::ios_base::beg);
}
#endif

// we should probably change signatures to match fread, fwrite, fseek exactly
struct minigz_io_t
{
  typedef size_t (*read_fn_t)(void*, size_t, void*);  // dest, bytes, ctx -- read from stream to dest
  typedef size_t (*write_fn_t)(const void*, size_t, void*);  // src, bytes, ctx -- write src to stream
  typedef void (*seek_fn_t)(long, int, void*);  // offset, origin (SEEK_SET or SEEK_CUR), ctx

#ifdef __cplusplus
  minigz_io_t(std::iostream& strm) : ctx(&strm), read(stream_read), write(stream_write), seek(stream_seek) {}
  minigz_io_t(void* _ctx, read_fn_t _read, write_fn_t _write, seek_fn_t _seek)
    : ctx(_ctx), read(_read), write(_write), seek(_seek) {}
  // should be able to drop 'explicit', at least if we do some template magic
  template<typename T>
  explicit minigz_io_t(T& t) : minigz_io_t((void*)&t, T::readfn, T::writefn, T::seekfn) {}
#endif
  void* ctx;
  read_fn_t read;
  write_fn_t write;
  seek_fn_t seek;
};

typedef minigz_io_t minigz_in_t;
typedef minigz_io_t minigz_out_t;


// return value is number of bytes written to output stream (or < 0 if error)
int gzip(minigz_in_t istrm, minigz_out_t ostrm, int level = 6);  //MZ_DEFAULT_LEVEL = 6
int gunzip(minigz_in_t istrm, minigz_out_t ostrm);

// lower level fns
#define MINIZ_GZ_CRC32_INIT (0)
#define MINIZ_GZ_NO_FINISH 0x00010000
int miniz_go(int level, minigz_in_t istrm, minigz_out_t ostrm, uint32_t* crc_32 = NULL, size_t max_read_bytes = SIZE_MAX);
void gzip_header(minigz_out_t ostrm, uint8_t flags = 0);
void gzip_footer(minigz_out_t ostrm, int len, uint32_t crc32);

#include <vector>

struct bgz_block_info_t
{
  uint32_t offset;  // offset of block in compressed data
  uint32_t crc32_cum;
  uint32_t len_cum;  // cumulative length of uncompressed data
  uint32_t reserved;
};

void bgz_header(minigz_out_t ostrm, uint16_t n);
void bgz_write_index(minigz_out_t ostrm, bgz_block_info_t* data, size_t count);
std::vector<bgz_block_info_t> bgz_get_index(minigz_in_t istrm);
bool bgz_read_block(minigz_in_t istrm, bgz_block_info_t* block_info, minigz_out_t ostrm);

#endif  // MINIZ_GZIP_H

#ifdef MINIZ_GZ_IMPLEMENTATION
#undef MINIZ_GZ_IMPLEMENTATION
#include "miniz/miniz.h"
#include <memory>

//static constexpr size_t STRM_MAX = std::numeric_limits<std::streamsize>::max();
static size_t chunkSize = 1 << 20;

// Ref: https://github.com/strake/gzip/blob/master/gzip.c
// level < 0 to inflate, level >= 0 to deflate
// returns number of uncompressed bytes (written if inflate, read if deflate) or < 0 on error
int miniz_go(int level, minigz_in_t istrm, minigz_out_t ostrm, uint32_t* crc_32, size_t max_read_bytes)
{
  int res = -1;  // -1 indicates error
  int final_flush = level & MINIZ_GZ_NO_FINISH ? MZ_FULL_FLUSH : MZ_FINISH;
  level = level & ~MINIZ_GZ_NO_FINISH;

  mz_stream s;
  memset(&s, 0, sizeof(mz_stream));
  int initerr = level < 0 ?
      inflateInit2(&s, -MZ_DEFAULT_WINDOW_BITS) :
      deflateInit2(&s, level, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 6, MZ_DEFAULT_STRATEGY);
  if(initerr) return -1;

  uint8_t* x = (uint8_t*)malloc(chunkSize);
  uint8_t* y = (uint8_t*)malloc(chunkSize);

  size_t n = 0, nout = 0, nreq = chunkSize;
  for(;;) {
    if(s.avail_in == 0) {
      size_t reqlim = max_read_bytes - s.total_in;
      n = istrm.read(x, reqlim < nreq ? reqlim : nreq, istrm.ctx);
      s.next_in = x;
      s.avail_in = n;
    }
    s.next_out = y;
    s.avail_out = chunkSize;
    const uint8_t* next_in = s.next_in;
    res = level < 0 ?
        mz_inflate(&s, MZ_SYNC_FLUSH) :
        mz_deflate(&s, n < nreq ? final_flush : MZ_NO_FLUSH);
    switch(res) {
    case MZ_STREAM_END:
    case MZ_OK:
      nout = chunkSize - s.avail_out;
      if(ostrm.write(y, nout, ostrm.ctx) != nout)
        goto error;  //return -1;  // abort on write error
      if(level < 0) {  // inflate
        if(crc_32) *crc_32 = mz_crc32(*crc_32, y, chunkSize - s.avail_out);
        if(res == MZ_STREAM_END) {  //|| (n < nreq && s.avail_out > 0)) { -- check moved to BUF_ERROR case
          if(s.avail_in > 0) {
            //istrm.clear();  // seekg clears eofbit, but doesn't set goodbit, so it fails if at end!
            istrm.seek(-int(s.avail_in), SEEK_CUR, istrm.ctx);  // rewind for unused bytes
          }
          goto done;  //return s.total_out;
        }
      }
      else {  // deflate
        if(crc_32) *crc_32 = mz_crc32(*crc_32, next_in, s.next_in - next_in);
        if(res == MZ_STREAM_END || (n < nreq && s.avail_out > 0))  // 2nd case is for SYNC_FLUSH
          goto done;  //return s.total_in;
      }
      break;
    case MZ_BUF_ERROR:
      // check is for bgz inflate - couldn't find way to avoid BUF_ERROR when chunk size == inflated size
      if(n == 0 && s.avail_out > 0)
        goto done;  //return level < 0 ? s.total_out : s.total_in;
      continue;  // applies to for(;;) not switch()
    case MZ_DATA_ERROR:
      goto error;  //return -1;
    case MZ_PARAM_ERROR:
      goto error;  //return -1;
    }
  }
done:
  res = level < 0 ? s.total_out : s.total_in;
error:
  free(y); free(x);
  level < 0 ? inflateEnd(&s) : deflateEnd(&s);
  return res;
}

static void storLE32(uint8_t *p, uint32_t n)
{
  p[0] = n >> 0;  p[1] = n >> 8;  p[2] = n >> 16;  p[3] = n >> 24;
}

static void skipString(minigz_in_t istrm)
{
  uint8_t c = '\0';
  while(istrm.read(&c, 1, istrm.ctx) == 1 && c != '\0') {}
}

int gunzip(minigz_in_t istrm, minigz_out_t ostrm)
{
  uint8_t x[10];
  if(istrm.read(x, 10, istrm.ctx) != 10) return -1;  // , "not in gz format");
  if(x[0] != 0x1F || x[1] != 0x8B) return -1;  // (-1, "not in gz format");
  if(x[2] != 8) return -1;  // (-1, "unknown z-algorithm: 0x%0hhX", x[2]);
  if(x[3] & 1 << 2) {
    uint8_t n_[2];  // FEXTRA
    istrm.read(n_, 2, istrm.ctx);  //readn(ifd, n_, 2);
    istrm.seek(n_[0] << 0 | n_[1] << 8, SEEK_CUR, istrm.ctx);  //istrm.ignore(n_[0] << 0 | n_[1] << 8);
  }
  uint8_t c[2];
  if(x[3] & 1 << 3) skipString(istrm);  // FNAME
  if(x[3] & 1 << 4) skipString(istrm);  // FCOMMENT
  if(x[3] & 1 << 1) istrm.read(c, 2, istrm.ctx); // FCRC - header checksum (2 bytes)
  uint32_t crc_32 = MZ_CRC32_INIT;
  int len = miniz_go(-1, istrm, ostrm, &crc_32);  // level < 0 requests decompression
  uint8_t crc_calc[4];
  storLE32(crc_calc, crc_32);
  uint8_t crc_gzip[4];
  istrm.read(crc_gzip, 4, istrm.ctx);
  istrm.seek(4, SEEK_CUR, istrm.ctx);  // skip the 4 byte uncompressed length so stream is past gzip file
  return memcmp(crc_calc, crc_gzip, 4) == 0 ? len : -1;
}

void gzip_header(minigz_out_t ostrm, uint8_t flags)
{
  uint8_t hdr[10] = {
    0x1F, 0x8B,	  // magic
    8,  // method: deflate = 8
    flags,
    0,0,0,0,  // modification time
    0,		/* xfl */
    0xFF,		/* OS */
  };
  ostrm.write(hdr, 10, ostrm.ctx);
}

void gzip_footer(minigz_out_t ostrm, int len, uint32_t crc_32)
{
  uint8_t ftr[8];
  storLE32(ftr + 0, crc_32);
  storLE32(ftr + 4, len);
  ostrm.write(ftr, 8, ostrm.ctx);  //write(ofd, ftr, 8);
}

int gzip(minigz_in_t istrm, minigz_out_t ostrm, int level)  //MZ_BEST_COMPRESSION
{
  gzip_header(ostrm);
  uint32_t crc_32 = MZ_CRC32_INIT;
  int len = miniz_go(level, istrm, ostrm, &crc_32);
  gzip_footer(ostrm, len, crc_32);
  return len;
}

// block gzip - gzip file composed of multiple blocks written with MZ_FULL_FLUSH so as to be independently
//  decompressible for random access; index also stores cumulative CRCs and lengths so that new blocks can be
//  written after any block (continuing to end of file)
// index of blocks is stored in gzip header extra field (w/ subfield id bytes "SL")
// Format for N blocks (N+1 entries):
//  - pos after header, crc after header (== 0), uncompressed len after header (== 0)
//  - pos after 0, crc after 0, len after 0
//  - pos after 1, crc after 1, len after 1
//  ...
//  - pos after N-1, crc after N-1 (== final crc), len after N-1 (== total uncompressed len)

// create header for block gzip file with n extra bytes
void bgz_header(minigz_out_t ostrm, uint16_t n)
{
  gzip_header(ostrm, 0x4);  // 0x4 = FEXTRA

  uint8_t* extra = (uint8_t*)calloc(n + 2, 1);
  extra[0] = uint8_t(n);
  extra[1] = uint8_t(n >> 8);
  ostrm.write(extra, n+2, ostrm.ctx);
  free(extra);
  //ostrm << uint8_t(n) << uint8_t(n >> 8) << std::string(n, '\0');
}

void bgz_write_index(minigz_out_t ostrm, bgz_block_info_t* data, size_t count)
{
  ostrm.seek(12, SEEK_SET, ostrm.ctx);  // 10 bytes header + 2 bytes FEXTRA total length
  uint16_t n = count * sizeof(bgz_block_info_t);

  uint8_t hdr[4] = { 'S', 'L', uint8_t(n), uint8_t(n >> 8) };
  ostrm.write(hdr, 4, ostrm.ctx);
  //ostrm << 'S' << 'L' << uint8_t(n) << uint8_t(n >> 8);

  uint8_t bytes[16];
  for(size_t ii = 0; ii < count; ++ii) {
    storLE32(bytes, data[ii].offset);
    storLE32(bytes + 4, data[ii].crc32_cum);
    storLE32(bytes + 8, data[ii].len_cum);
    storLE32(bytes + 12, data[ii].reserved);
    ostrm.write(bytes, 16, ostrm.ctx);
  }
}

static uint32_t loadLE32(uint8_t* p)
{
  return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

// TODO: return bgz_block_info_t* instead of std::vector and make sure this compiles as plain C
std::vector<bgz_block_info_t> bgz_get_index(minigz_in_t istrm)
{
  std::vector<bgz_block_info_t> res;
  uint8_t x[16];
  istrm.seek(0, SEEK_SET, istrm.ctx);
  if(istrm.read(x, 16, istrm.ctx) != 16 || x[0] != 0x1F || x[1] != 0x8B || x[2] != 8) // == 0x8B fails if x is char[]
    return res;  // not a valid gzip file
  if(!(x[3] & 1 << 2) || (x[10] << 0 | x[11] << 8) < 4 || x[12] != 'S' || x[13] != 'L')
    return res;  // bgz header not present

  size_t n = x[14] | (x[15] << 8);
  uint8_t* temp = new uint8_t[n];
  istrm.read(temp, n, istrm.ctx);
  res.reserve(n/sizeof(bgz_block_info_t));
  for(uint8_t* p = temp; p < temp + n; p += sizeof(bgz_block_info_t)) {
    res.push_back({loadLE32(p), loadLE32(p + 4), loadLE32(p + 8), loadLE32(p + 12)});
  }
  delete[] temp;
  return res;
}

bool bgz_read_block(minigz_in_t istrm, bgz_block_info_t* block_info, minigz_out_t ostrm)
{
  size_t n = block_info[1].offset - block_info[0].offset;
  uint32_t crc_32 = block_info[0].crc32_cum;
  istrm.seek(block_info[0].offset, SEEK_SET, istrm.ctx);
  miniz_go(-1, istrm, ostrm, &crc_32, n);
  return crc_32 == block_info[1].crc32_cum;
}

#endif // MINIZ_GZ_IMPLEMENTATION

// g++ -DMINIZ_GZ_UTIL -DMINIZ_GZ_IMPLEMENTATION -isystem .. -o bgunzip -x c++ miniz_gzip.h
//-isystem ../stb
#ifdef MINIZ_GZ_UTIL

#include <fstream>

#include "miniz/miniz.c"
#include "miniz/miniz_tdef.c"
#include "miniz/miniz_tinfl.c"

int main(int argc, char* argv[])
{
  if(argc < 2) {
    printf("Usage: bgunzip <file to extract>\n");
    return -1;
  }
  std::string outname(argv[1]);
  if(outname.substr(outname.size()-3) == ".gz") outname.resize(outname.size()-3);
  else if(outname.back() == 'z') outname.pop_back();
  else outname.append(".infl");

  std::fstream fin(argv[1], std::fstream::in | std::fstream::binary);
  std::vector<bgz_block_info_t> block_info = bgz_get_index(fin);
  if(block_info.empty()) {
    std::fstream fout(outname.c_str(), std::fstream::out | std::fstream::binary);
    fin.seekg(0);
    if(gunzip(fin, fout) > 0) {
      printf("No gzip blocks found, extracted as normal gzip file to %s", outname.c_str());
    }
    else {
      // use, e.g., `tail -c +<offset> infile > outfile` to extract a possible orphan block, then use bgunzip to inflate
      uint32_t crc_32 = 0;
      std::fstream fout2((outname + ".XXX").c_str(), std::fstream::out | std::fstream::binary);
      fin.seekg(0);
      int nread = miniz_go(-1, fin, fout2, &crc_32, 0x7FFFFFFF);
      printf("No gzip blocks found, gunzip failed; inflated %d bytes to %s.XXX\n", nread, outname.c_str());
    }
  }
  else {
    for(size_t ii = 0; ii < block_info.size() - 1; ++ii) {
      bgz_block_info_t* b = &block_info[ii];
      std::string namehack = outname + std::to_string(0.001 * ii + 1e-6).substr(1, 4);
      std::fstream fout(namehack.c_str(), std::fstream::out | std::fstream::binary);
      bool ok = bgz_read_block(fin, b, fout);
      printf("block %03d: offset %d, cum_len: %d  %s\n", ii, b->offset, b->len_cum, ok ? "OK" : "ERROR");
    }
  }
  return 0;
}

#endif //MINIZ_GZ_UTIL

// To build test executable (replace .. with path to directory containing miniz/ as needed)
//   g++ -DMINIZ_GZ_TEST -DMINIZ_GZ_IMPLEMENTATION -isystem .. -o gztest -x c++ miniz_gzip.h
#ifdef MINIZ_GZ_TEST
#include <sstream>
#include <fstream>

#ifndef ASSERT
#include <assert.h>
#define ASSERT assert
#endif

#include "miniz/miniz.c"
#include "miniz/miniz_tdef.c"
#include "miniz/miniz_tinfl.c"

// test string needs to be over ~32KB to provide a good test!

static const char* cons_emes[] = {"b","bb","d","dd","ed","f","ff","ph","gh","lf","ft","g","gg","gh","gu",
"gue","h","wh","j","ge","g","dge","di","gg","k","c","ch","cc","lk","qu","ck","x","l","ll","m","mm","mb","mn",
"lm","n","nn","kn","gn","pn","p","pp","r","rr","wr","rh","s","ss","c","sc","ps","st","ce","se","t","tt","th",
"ed","v","f","ph","ve","w","wh","u","o","z","zz","s","ss","x","ze","se","s","si","z","ch","tch","tu","ti",
"te","sh","ce","s","ci","si","ch","sci","ti","th","th","ng","n","ngue"};

static const char* vowel_emes[] = {"y","i","j","a","ai","au","a","ai","eigh","aigh","ay","er","et","ei","au",
"a_e","ea","ey","e","ea","u","ie","ai","a","eo","ei","ae","e","ee","ea","y","ey","oe","ie","i","ei","eo",
"ay","i","e","o","u","ui","y","ie","i","y","igh","ie","uy","ye","ai","is","eigh","i_e","a","ho","au","aw",
"ough","o","oa","o_e","oe","ow","ough","eau","oo","ew","o","oo","u","ou","u","o","oo","ou","o","oo","ew",
"ue","u_e","oe","ough","ui","oew","ou","oi","oy","uoy","ow","ou","ough","a","er","i","ar","our","ur","air",
"are","ear","ere","eir","ayer","a","ir","er","ur","ear","or","our","yr","aw","a","or","oor","ore","oar",
"our","augh","ar","ough","au","ear","eer","ere","ier","ure","our"};

std::string make_test_str(int len)
{
  constexpr int NUM_CONS = sizeof(cons_emes)/sizeof(cons_emes[0]);
  constexpr int NUM_VOWEL = sizeof(vowel_emes)/sizeof(vowel_emes[0]);
  //constexpr int LEN = 100000;

  //char* test_str = new char[LEN];
  std::string test_str(len, 0);

  bool cons_next = true;
  for(int ii = 0; ii < len;) {
    if(rand() % 3 == 0) {
      test_str[ii++] = ' ';
      cons_next = rand() % 4 == 0;
    }
    else {
      const char* src = cons_next ? cons_emes[rand() % NUM_CONS] : vowel_emes[rand() % NUM_VOWEL];
      cons_next = !cons_next;
      while(*src && ii < len)
        test_str[ii++] = *src++;
    }
  }
  test_str[len-1] = '\0';

  return test_str;
}

void test_bgz_len(int test_len)
{
  // generate test data
  std::stringstream src[] = {
    std::stringstream(make_test_str(test_len)),
    std::stringstream(make_test_str(test_len)),
    std::stringstream(make_test_str(test_len)),
    std::stringstream(make_test_str(test_len)),
    std::stringstream(make_test_str(test_len))
  };
  std::stringstream def_strm;
  //std::stringstream inf_strm;

  // DOC: this shows how to build a block gzip file
  int level = 6;
  {
    std::vector<bgz_block_info_t> block_info;
    bgz_header(def_strm, 1024*sizeof(bgz_block_info_t));
    uint32_t crc_32 = MZ_CRC32_INIT;
    uint32_t len = 0;
    block_info.push_back({uint32_t(def_strm.tellp()), crc_32, len, 0});
    len += miniz_go(level | MINIZ_GZ_NO_FINISH, src[0], def_strm, &crc_32);
    block_info.push_back({uint32_t(def_strm.tellp()), crc_32, len, 0});
    len += miniz_go(level | MINIZ_GZ_NO_FINISH, src[1], def_strm, &crc_32);
    block_info.push_back({uint32_t(def_strm.tellp()), crc_32, len, 0});
    len += miniz_go(level, src[2], def_strm, &crc_32);
    block_info.push_back({uint32_t(def_strm.tellp()), crc_32, len, 0});
    gzip_footer(def_strm, len, crc_32);

    bgz_write_index(def_strm, block_info.data(), block_info.size());  // seek to extra header region and write index
  }

  // test
  {
    std::stringstream inf_strm;
    def_strm.seekg(0);  // read from beginning
    ASSERT(gunzip(def_strm, inf_strm) > 0);  //ASSERT(gunzip(def_strm, inf_strm) == test_str_len);
    ASSERT(inf_strm.str() == src[0].str() + src[1].str() + src[2].str());
  }

  // DOC: this shows how to read block gzip header and blocks
  std::vector<bgz_block_info_t> block_info = bgz_get_index(def_strm);
  ASSERT(block_info.size() == 4);

  // test individual blocks
  for(size_t ii = 0; ii < block_info.size() - 1; ++ii) {
    std::stringstream inf_block;
    ASSERT(bgz_read_block(def_strm, &block_info[ii], inf_block));
    ASSERT(inf_block.str() == src[ii].str());
  }

  // DOC: this shows how to add and replace blocks
  // replace 3rd block (src[2) w/ src[3] and src[4]
  def_strm.seekp(block_info[2].offset);  // position after src[1]
  uint32_t crc_32 = block_info[2].crc32_cum;
  uint32_t len = block_info[2].len_cum;
  block_info.resize(3);
  len += miniz_go(level | MINIZ_GZ_NO_FINISH, src[3], def_strm, &crc_32);
  block_info.push_back({uint32_t(def_strm.tellp()), crc_32, len, 0});
  len += miniz_go(level, src[4], def_strm, &crc_32);
  block_info.push_back({uint32_t(def_strm.tellp()), crc_32, len, 0});
  gzip_footer(def_strm, len, crc_32);
  // NOTE: for a file, we'd need to use ftruncate if new size is less than old size! (after flushing stream!)

  bgz_write_index(def_strm, block_info.data(), block_info.size());  // seek to extra header region and write index

  // test
  {
    std::stringstream inf_strm;
    def_strm.seekg(0);  // read from beginning
    ASSERT(gunzip(def_strm, inf_strm) > 0);  //ASSERT(gunzip(def_strm, inf_strm) == test_str_len);
    ASSERT(inf_strm.str() == src[0].str() + src[1].str() + src[3].str() + src[4].str());
  }
}

// DOC: this shows how to use the high level gzip() and gunzip() functions
void test_chunk_sizes(int test_len)
{
  // test
  std::string test_str = make_test_str(test_len);
  int nsteps = 10;
  int step = 2*(test_len/(2*nsteps)) - 1;

  for(int n = 0; n < nsteps; ++n) {
    chunkSize = (n + 2)*step;
    std::stringstream src_strm(test_str);
    std::stringstream def_strm;
    std::stringstream inf_strm;

    ASSERT(gzip(src_strm, def_strm) == test_len);
    def_strm.seekg(0);  // read from beginning
    ASSERT(gunzip(def_strm, inf_strm) == test_len);
    ASSERT(inf_strm.str() == test_str);
    //assert(n != 1150);
  }

  chunkSize = 1 << 20;
}

// DOC: this shows how to write independent blocks to gzip file w/o saving info needed for block gzip index
void test_level0_block(int test_len)
{
  std::string test_str = make_test_str(test_len);
  int nsteps = 10;
  int step = 2*(test_len/(2*nsteps)) - 1;

  // run tests for including uncompressed block (both at beginning and at end)
  for(int level : {0, 6}) {
    for(int n = 0; n < nsteps; ++n) {
      chunkSize = (n + 2)*step;
      std::stringstream src1_strm(test_str.substr(0, test_len/2));
      std::stringstream src2_strm(test_str.substr(test_len/2));
      std::stringstream def_strm;
      std::stringstream inf_strm;

      gzip_header(def_strm);
      uint32_t crc_32 = MZ_CRC32_INIT;
      int len = miniz_go(level | MINIZ_GZ_NO_FINISH, src1_strm, def_strm, &crc_32);
      len += miniz_go(6 - level, src2_strm, def_strm, &crc_32);
      gzip_footer(def_strm, len, crc_32);
      ASSERT(len == test_len);

      def_strm.seekg(0);  // read from beginning
      ASSERT(gunzip(def_strm, inf_strm) == test_len);
      ASSERT(inf_strm.str() == test_str);
      //assert(n != 1150);
    }
  }
  chunkSize = 1 << 20;
}

int main(int argc, char* argv[])
{
  if(argc > 1) {
    std::fstream f(argv[1], std::fstream::in | std::fstream::binary);
    std::vector<bgz_block_info_t> block_info = bgz_get_index(f);
    if(!block_info.empty()) {
      int ii = 0;
      for(auto& b : block_info)
        printf("block %d: offset %d, cum_len: %d\n", ii++, b.offset, b.len_cum);
    }
    else
      printf("No gzip blocks found!");

    return 0;
  }

  printf("No file specified - running tests...");
  int l = 100000;
  chunkSize = l/2;     test_bgz_len(l);
  chunkSize = l - 10;  test_bgz_len(l);
  chunkSize = l - 1;   test_bgz_len(l);
  chunkSize = l;       test_bgz_len(l);
  chunkSize = l + 1;   test_bgz_len(l);
  chunkSize = l + 10;  test_bgz_len(l);
  chunkSize = 2*l;     test_bgz_len(l);
  chunkSize = 1 << 20; test_bgz_len(l);

  test_chunk_sizes(960);
  test_chunk_sizes(100000);

  test_level0_block(100000);
  return 0;
}

#endif // MINIZ_GZ_TEST
