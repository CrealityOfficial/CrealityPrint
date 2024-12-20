#ifndef _GCORE_STYLE_GRBL_H_
#define _GCORE_STYLE_GRBL_H_
struct GImageParser;
struct GCoreConfig;
struct GcoPoint;
struct GcoBlock;

void GRBLStyle_build_head(struct GImageParser *p, const struct GCoreConfig *cfg, unsigned char idx, char* l);
void GRBLStyle_build_tail(const struct GCoreConfig *cfg, unsigned char idx, char* l);
void GRBLStyle_build_ending(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void GRBLStyle_build_newLine(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void GRBLStyle_build_inLine(struct GImageParser *p, const struct GCoreConfig *cfg, char *l);
void GRBLStyle_build_block(struct GcoBlock* blk, char *l);

#endif // _GRBL_STYLE_GRBL_H_
