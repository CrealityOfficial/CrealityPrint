#ifndef _GCORE_STYLE_CV01_H_
#define _GCORE_STYLE_CV01_H_

struct GImageParser;
struct GCoreConfig;
struct GcoPoint;
struct GcoBlock;
void CV01Style_build_head(struct GImageParser *p, const struct GCoreConfig *cfg, unsigned char idx, char* l);
void CV01Style_build_tail(const struct GCoreConfig *cfg, unsigned char idx, char* l);
void CV01Style_build_ending(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void CV01Style_build_newLine(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void CV01Style_build_inLine(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void CV01Style_build_block(struct GcoBlock* blk, char *l);

#endif // _GCORE_STYLE_CV01_H_
