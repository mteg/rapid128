#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "read128.h"

static inline int minrc(int a, int b)
{
  if(a < b)
    return a;
  else
    return b;
}

struct r128_image * err(char *msg)
{
  fprintf(stderr, "%s\n", msg);
  assert(NULL);
  return NULL;
}

#warning implement a table instead of a tree
extern struct r128_codetree_node * code_tree;
extern char ** code_tables[];

inline static void r128_log(struct r128_ctx *ctx, int level, char *fmt, ...)
{
  va_list ap;
  if(level > ctx->logging_level) return;
  
  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}


int r128_report_code(struct r128_ctx *ctx, char *code, int len)
{
  code[len] = 0;
  printf("%s\n", code);
  return R128_EC_SUCCESS;
}

int r128_cksum(int *symbols, int len)
{
  int i, wsum = symbols[0];

  for(i = 1; i<len; i++)
    wsum += symbols[i] * i;

  return wsum % 103;
}

int r128_parse(struct r128_ctx *ctx, int *symbols, int len)
{
  int table = 0, i;
  char outbuf[1024];
  int pos = 0, ppos = 0;

  /* Min-length */
  if(len < 3)
    return R128_EC_SYNTAX;
  
  /* Checksum! */
  if(!(ctx->flags & R128_FL_NOCKSUM))
    if(r128_cksum(symbols, len - 2) != symbols[len - 2])
      return R128_EC_CHECKSUM; 
  
  outbuf[0] = 0;
  
  for(i = 0; i<len; i++)
  {
    int c = symbols[i], nb = 0;
   
    if(c == 103)
      table = 0; /* START A */
    else if(c == 104)
      table = 1; /* START B */
    else if(c == 105)
      table = 2; /* START C */
    else if(c == 99 && table != 2)
      table = 2; /* CODE C */
    else if(c == 100 && table != 1)
      table = 1; /* CODE B */
    else if(c == 101 && table != 0)
      table = 0; /* CODE A */
    else if(c == 106)
      return r128_report_code(ctx, outbuf, ppos);
    else if(c < 0 && c > 105)
      nb = sprintf(outbuf + pos, " - ?? %d ?? - ", c); 
    else
      nb = sprintf(outbuf + pos, "%s", code_tables[table][c]);

    ppos = pos; pos += nb;
  }
  return R128_EC_NOEND;
}


int r128_decode(struct r128_ctx *ctx, char *code_in)
{
  int code_out[1024];
  int symbol = 0, i;
  struct r128_codetree_node *n = code_tree;
  
  for(i = 0; code_in[i]; i++)
  {
    /* Push four bits */
    u_int8_t nibble = code_in[i] - 'A';
    int bitcount = 0;
    
    for(bitcount = 0; bitcount<4; bitcount++, nibble <<= 1)
    {
      int bit = nibble & 0x08;
      n = bit ? n->one : n->zero;

      if(!n) { fprintf(stderr, "ERR (%s)\n", code_in); return R128_EC_SYNTAX; } /* Out of code */
      if(n->code == -1) continue; /* Next bit please! */
      
      /* Got a symbol! */
      code_out[symbol++] = n->code;
      fprintf(stderr, "[%d] ", n->code); 
      if(n->code == 106)
        /* Proper STOP! */
        return r128_parse(ctx, code_out, symbol);

      n = code_tree;
    }
  }
  return R128_EC_SYNTAX;
}

int r128_find_code(struct r128_ctx *ctx, char *code)
{
  static char * starts[] = {"NCA", "NCB", "NAJ", "NAI", "NDJ", "NDI"};
  static char * ends[] = {"BNG", "DKM", "HFI", "IOL"};
  int i, j, rc = R128_EC_NOCODE;
  char *ptr;
  
  /* Try all start codes */
  for(i = 0; i<6; i++)
  {
    ptr = code;
    /* Look for all occurences of this start code */
    while((ptr = strstr(ptr, starts[i])))
    {
      /* Wow, we have _something_ */
      rc = minrc(rc, R128_EC_NOEND);

      /* See if there is at least one ending */
      for(j = 0; j<4; j++)
        if(strstr(ptr, ends[j]))
        {
          /* Ending found - decode & eject! */
          rc = minrc(rc, r128_decode(ctx, ptr));
          break;
        }

      /* Have we succeeded? */
      if(R128_ISDONE(ctx, rc)) return rc;

      /* Try next occurence of this start code */
      ptr++;
    }
  }
  
  /* No luck or they want us to print absolutely all */
  return rc;
}
  
int r128_scan_line(struct r128_ctx *ctx, struct r128_line *li, double uwidth, double offset, double threshold)
{
  char b1[8192];
  char b2[8192];
  char b3[8192];
  char b4[8192];
  char *b[4] = {
    b1, b2, b3, b4
  };
  int i, len, charlen, rc = R128_EC_NOCODE;
  double ppos = offset * uwidth, w = li->linesize;
  u_int8_t *line = li->gray_data;
  
  if(!w) return R128_EC_NOLINE;
  
  threshold *= 255.0 * uwidth;
  
  len = lrint(floor(((double) w) / uwidth));
  charlen = (len / 4) + 1;
  
//  printf("w = %d, uwidth = %.5f, offs = %.5f, thresh = %.5f, code len = %d\n", w, uwidth, offset, threshold, len);

#warning no need to memset that much
  for(i = 0; i<4; i++)
    memset(b[i], 0, 8192);
    
  for(i = 0; i<len; i++)
  {
    double accu = 0.0, npos;
    int pmin, pmax, j, digit;

    npos = ppos + uwidth;
    
    /* Add remaining fraction */
    accu += (ceil(ppos) - ppos) * (double) line[lrint(floor(ppos))];
    
    /* Advance to next whole pixel */ 
    ppos = ceil(ppos);
    
    /* Number of pixels to read and average */
    pmin = lrint(ppos);
    pmax = lrint(floor(npos));
    for(j = pmin; j<pmax; j++)
      accu += (double) line[j];
      
    /* Add remaining fraction */
    accu += (npos - floor(npos)) * (double) line[lrint(floor(npos))];
    
    /* Threshold! */
    if(accu > threshold)
      digit = 0;
    else
      digit = 1;
    
    /* Write to buffers */
    for(j = 0; j<4; j++)
    {
      b[j][(i + j) >> 2] <<= 1;
      b[j][(i + j) >> 2] |= digit;
    }
    
    ppos = npos;
  }
  
  for(i = 0; i<4; i++)
  {
    int j;
    for(j = 0; j<charlen; j++)
      b[i][j] += 'A';

    b[i][j] = 0;
    
    rc = minrc(rc, r128_find_code(ctx, b[i]));
    if(R128_ISDONE(ctx, rc))
      return rc;
  }
  
  return rc;
}

void help(char *progname, int verbosity)
{
  if(!verbosity)
  {
    printf(
      "rapid128 barcode scanner, (c) 2015 Mateusz 'mteg' Golicz\n"
      "Usage: %s [options] <file1> [<file2> [<file3> ...]]\n"
      "\n"
      "Available options:\n"
      " -w <pixels> Minimum unit width to consider\n"
      " -W <pixels> Maximum unit width to consider\n"
      " -H <pixels> Minimum expected barcode height\n",
      progname
    );
  }
  else
  {
  }
}

#define R128_MARGIN_CLASS(c, val) ((val) >= (c)->margin_high_threshold ? 1 : ((val) <= (c)->margin_low_threshold ? -1 : 0))

struct r128_line *r128_get_line(struct r128_ctx *ctx, struct r128_image *im, int line)
{
  int min_len = ctx->min_len, max_gap = ctx->max_gap;
  u_int8_t *data;
  int len, start = 0;
  
  if(im->lines[line].gray_data) return &im->lines[line];
  
  data = im->gray_data + line * ctx->width;

  start = 0;
  len = ctx->width;
  
  if((min_len >= max_gap) && 0)
  {
    int prev_so_far = R128_MARGIN_CLASS(ctx, data[min_len - max_gap]);
    int so_far, gap;
    
    /* Left margin */
    for(gap = 1; prev_so_far && (gap + min_len - max_gap)<len; prev_so_far = so_far, gap++)
      if((so_far = R128_MARGIN_CLASS(ctx, data[gap + min_len - max_gap])) != prev_so_far)
        break;
    
    /* Indeed a gap! */
    if(gap > max_gap)
    {
      start = gap + min_len - max_gap;
      len -= start;
    }
    
    /* Right margin */
    if(len > min_len)
    {
      prev_so_far = R128_MARGIN_CLASS(ctx, data[start + len - min_len + max_gap]);

      for(gap = 1; prev_so_far && (start + len - gap - min_len + max_gap) >= 0; prev_so_far = so_far, gap++)
        if((so_far = R128_MARGIN_CLASS(ctx, data[start + len - gap - min_len + max_gap])) != prev_so_far)
          break;
      
      /* Indeed a gap! */
      if(gap > max_gap)
        len -= gap;
    }
  }
  
  if(len < min_len) len = 0;
  r128_log(ctx, R128_DEBUG2, "Prepared line %d of image %p: start = %d, len = %d\n", line, im, start, len);

  im->lines[line].gray_data = data + start;
  im->lines[line].linesize = len;
  
  return &im->lines[line];
  
  
}

static inline int bfsguidance(int ctr)
{
    int b, res = 0;
    
    /* Reverse bits of the counter */
    for(b = 0; b<16; b++)
    {
      res <<= 1;
      res |= ctr & 1;
      ctr >>= 1;
    }
    return res;
}

static inline int log2_ceil(int val)
{
  int s = 1;
  while(s <= val)
    s <<= 1;
  return s;
}

int r128_page_scan(struct r128_ctx *ctx, struct r128_image *img,
          double offset, double uwidth, int minheight, int maxheight)
{
  int rc = R128_EC_NOTHING;
  int min_ctr, max_ctr, ctr, lines_scanned = 0;
  
  min_ctr = log2_ceil(ctx->height / maxheight);
  max_ctr = log2_ceil(ctx->height / minheight);
  
  ctx->page_scan_id++;
  r128_log(ctx, R128_DEBUG1, "Page scan # %d starts: offs = %.3f, th = %.2f, heights = %d ~ %d, uwidth = %.2f\n", 
        ctx->page_scan_id, offset, ctx->threshold, minheight, maxheight, uwidth);
  
  /* Do a BFS scan of page lines @ particular offset and unit width */
  for(ctr = min_ctr; ctr < max_ctr; ctr++)
  {
    /* Determine line number */
    int line_idx = (ctx->height * bfsguidance(ctr)) >> 16;
    
    /* Check if this line was already done in this pass */
    if(ctx->line_scan_status[line_idx] == ctx->page_scan_id) continue;
    
    /* Mark line as checked */
    ctx->line_scan_status[line_idx] = ctx->page_scan_id;

    /* Check the line */
    rc = minrc(rc, r128_scan_line(ctx, r128_get_line(ctx, img, line_idx), uwidth, offset, ctx->threshold));
    
    /* Increment scanned lines counter */
    lines_scanned ++;
    
    /* If we found anything, maybe we can immediately quit? */
    if(R128_ISDONE(ctx, rc))
    {
      r128_log(ctx, R128_NOTICE, "Code was found in page scan # %d, offs = %.3f, th = %.2f, heights = %d ~ %d, uwidth = %.2f, line = %d\n", 
            ctx->page_scan_id, offset, ctx->threshold, minheight, maxheight, uwidth, line_idx);
      return rc;
    }
  }
  r128_log(ctx, R128_DEBUG1, "Page scan %d finished, scanned %d lines, best rc = %d\n", ctx->page_scan_id, lines_scanned, rc);

  return rc;
}

void r128_alloc_lines(struct r128_ctx *ctx, struct r128_image *i)
{
  assert((i->lines = (struct r128_line*) malloc(sizeof(struct r128_line) * ctx->height)));
  memset(i->lines, 0, sizeof(struct r128_line) * ctx->height);
}

struct r128_image * r128_blur_image(struct r128_ctx * ctx)
{
  struct r128_image *i;
  int *accu;
  int x, y, bh = ctx->blurring_height;
  u_int8_t *minus_line = ctx->im->gray_data, *plus_line = ctx->im->gray_data;
  u_int8_t *result;
  
  if(ctx->im_blurred) return ctx->im_blurred;

  r128_log(ctx, R128_DEBUG1, "Preparing a blurred image\n");
  
  assert((i = (struct r128_image*) malloc(sizeof(struct r128_image))));
  memset(i, 0, sizeof(struct r128_image));
  
  assert((result = i->gray_data = (u_int8_t*) malloc(ctx->width * ctx->height)));
  r128_alloc_lines(ctx, i);

  assert((accu = (int*) malloc(sizeof(int) * ctx->width)));
  memset(accu, 0, sizeof(int) * ctx->width);
  
  for(y = 0; y<ctx->height; y++)
  {
    for(x = 0; x<ctx->width; x++)
    {
      if(y > bh) accu[x] -= minus_line[x];
      accu[x] += plus_line[x];
      *(result++) = accu[x] / bh;
    }
    if(y > bh)
      minus_line += ctx->width; 
    plus_line += ctx->width;
  }
  
  free(accu);
  r128_log(ctx, R128_DEBUG2, "Blurred image prepared\n");
  return i;  
}

int r128_try_tactics(struct r128_ctx *ctx, char *tactics)
{
  static double offsets[] = {0, 0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875};
  int w_ctr_min = 1, w_ctr_max = ctx->wctrmax_stage1, w_ctr;
  int offs, offs_min = 0, offs_max = 4;
  int h_min = ctx->expected_min_height, h_max = ctx->height;
  char *thresh_input;

  struct r128_image *img = ctx->im; 
  int rc = R128_EC_NOTHING;
  
  if(strstr(tactics, "i"))
    img = r128_blur_image(ctx);
  
  if(strstr(tactics, "o")) { offs_min = 4; offs_max = 8; }
  if(strstr(tactics, "h")) { h_min = 1; h_max = ctx->expected_min_height; }
  if(strstr(tactics, "w")) { w_ctr_min = w_ctr_max; w_ctr_max = ctx->wctrmax_stage2; }

  if((thresh_input = strstr(tactics, "@")))
    if(sscanf(thresh_input + 1, "%lf", &ctx->threshold) != 1)
      r128_log(ctx, R128_NOTICE, "Invalid threshold in tactics: '%s', ignoring.\n", tactics);

  r128_log(ctx, R128_DEBUG1, "Now assuming tactics '%s'\n", tactics);
  for(offs = offs_min; offs < offs_max; offs++)
    for(w_ctr = w_ctr_min; w_ctr < w_ctr_max; w_ctr++)
    {
      /* First, determine document width in code units for this try */
      double cuw = ctx->min_doc_cuw + ctx->doc_cuw_span * bfsguidance(w_ctr) / 65536.0;

      /* Determine unit width to act with for this w_ctr */
      double uwidth = ((double) ctx->width) / cuw;
      
      rc = minrc(rc, r128_page_scan(ctx, img, offsets[offs], uwidth, h_min, h_max));
      if(R128_ISDONE(ctx, rc))
        return rc;
    }
  
  r128_log(ctx, R128_DEBUG2, "Best result for tactics '%s': %d\n", tactics, rc);
  return rc;
}

int r128_try_strategy(struct r128_ctx *ctx, char *strategy)
{
  int rc = R128_EC_NOTHING;
  
  assert((strategy = strdup(strategy)));

  /* Precompute data for cropping lines */
  
  /* A meaningful barcode is at least 4 * 13 bits * unit_width pixels wide */
  /* (START-X) 1 symbol checksum (END) */
  
  /* This means, that, taking a minimal assumed unit width,
     if at position 4*13*min_uwidth there was still no barcode, it means
     all there was is a garbage 
     
     On the other hand, there cannot be a gap of more than
     max_uwidth * 4 white/black pixels. This follows from how
     the code is constructed (Never more than 4 ones or zeros in row)
     
     We take an extra margin and assume 6.
  */
  ctx->min_len = lrint(floor(4.0 * 13.0 * ctx->min_uwidth));
  ctx->max_gap = lrint(ceil(6.0 * ctx->max_uwidth));
  
  /* Precompute data for unit width search */
  
  /* Minimum codeunit width of document is? */
  ctx->min_doc_cuw = ((double) ctx->width) / ctx->max_uwidth;
  ctx->max_doc_cuw = ((double) ctx->width) / ctx->min_uwidth;
  
  /* Therefore, we span a range of...? */
  ctx->doc_cuw_span = ctx->max_doc_cuw - ctx->min_doc_cuw;
  
  /* Number of subdivisions required to reach a difference less than cuw_delta1? */
  ctx->wctrmax_stage1 = log2_ceil(lrint(ctx->doc_cuw_span / ctx->min_cuw_delta1));
  ctx->wctrmax_stage2 = log2_ceil(lrint(ctx->doc_cuw_span / ctx->min_cuw_delta2));
  
  while(strategy)
  {
    char *next_strategy = index(strategy, ',');
    if(next_strategy) *(next_strategy++) = 0;
    rc = minrc(rc, r128_try_tactics(ctx, strategy));
    if(R128_ISDONE(ctx, rc))
      return rc;
    strategy = next_strategy;
  }
  r128_log(ctx, R128_DEBUG2, "Giving up. Best result is %d\n", rc);
  return rc;
}

void r128_defaults(struct r128_ctx *c)
{
  memset(c, 0, sizeof(struct r128_ctx));
  c->strategy = "IHWO@0.5,iHWO,IHWo,iHWo,IHwO,iHwO,IHwo,iHwo,IhWO,ihWO,IhWo,ihWo,IhwO,ihwO,Ihwo,ihwo";
  c->min_uwidth = 0.9;
  c->max_uwidth = 4;
  c->threshold = 0.5;
  c->margin_low_threshold = 20;
  c->margin_high_threshold = 235;
  c->min_cuw_delta2 = 2;
  c->min_cuw_delta1 = 10;
  c->expected_min_height = 8;
  c->blurring_height = 4;
}

void r128_alloc_buffers(struct r128_ctx *c)
{
  assert((c->line_scan_status = (int*) malloc(sizeof(int) * c->height)));
  memset(c->line_scan_status, 0, sizeof(int) * c->height);
}

struct r128_image * r128_read_pgm(struct r128_ctx *c, FILE *fh)
{
  struct r128_image *r;

  char line[128 + 1];
  int w, h; 
  u_int8_t *im;

  r128_log(c, R128_DEBUG1, "Loading image.");

  assert((r = (struct r128_image*) malloc(sizeof(struct r128_image))));
  memset(r, 0, sizeof(struct r128_image));

  if(!fgets(line, 128, fh)) return err("Invalid file format (1)");
  if(strncmp(line, "P5", 2)) return err("Invalid file format (2)");

  if(!fgets(line, 128, fh)) return err("Invalid file format (3)");
//  if(!fgets(line, 128, fh)) return err("Invalid file format (3a)");
  if(!fgets(line, 128, fh)) return err("Invalid file format (4)");
  if(sscanf(line, "%d %d", &w, &h) != 2) return err("Invalid file format (5, wrong size)");
  if(!fgets(line, 128, fh)) return err("Invalid file format (6)");
  
  
  
  assert(im = (unsigned char*) malloc(w * h));
  if(fread(im, w * h, 1, fh) != 1) return err("Read error (file truncated?)");
  
  r->gray_data = im;
  c->width = w;
  c->height = h;
  

  r128_alloc_lines(c, r);
  r128_log(c, R128_DEBUG2, "Image loaded.");
  return r;
}

int main(int argc, char ** argv)
{
  FILE *fh;
  struct r128_ctx ctx;
  int c, nh = 0;

  r128_defaults(&ctx);
  
  while((c = getopt(argc, argv, "hv")) != EOF)
  {
    switch(c)
    {
      case 'h':
        help(argv[0], nh++);
        break;
      case 'v':
        ctx.logging_level++;
        break;        
    }
  }
   
  if(!argv[optind])
  {
    fprintf(stderr, "Usage: read128 <file name>.pgm\n");
    return 1;
  }
  
  if(!(fh = fopen(argv[optind], "rb")))
  {
    perror("fopen");
    return 1;
  }
  ctx.im = r128_read_pgm(&ctx, fh);
  
  fclose(fh);
  
  r128_alloc_buffers(&ctx);
  r128_try_strategy(&ctx, ctx.strategy);
  
  
  return 0;
}
