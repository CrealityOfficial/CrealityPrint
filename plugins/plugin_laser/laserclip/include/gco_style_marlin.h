#ifndef _GCORE_STYLE_MARLIN_H_
#define _GCORE_STYLE_MARLIN_H_
struct GImageParser;
struct GCoreConfig;
struct GcoPoint;
struct GcoBlock;

void MarlinStyle_build_head(struct GImageParser *p, const struct GCoreConfig *cfg, unsigned char idx, char* l);
void MarlinStyle_build_tail(const struct GCoreConfig *cfg, unsigned char idx, char* l);
void MarlinStyle_build_ending(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void MarlinStyle_build_newLine(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void MarlinStyle_build_inLine(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void MarlinStyle_build_block(struct GcoBlock* blk, char *l);

#endif // _GCORE_STYLE_MARLIN_H_
