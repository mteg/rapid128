#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
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
//extern struct r128_codetree_node * code_tree;
extern char code_symbols[];
extern char ** code_tables[];

inline int r128_log_return(struct r128_ctx *ctx, int level, int rc, char *fmt, ...)
{
  va_list ap;
  if(level > ctx->logging_level) return rc;
  
  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  
  return rc;
}

#define r128_log(ctx, level, fmt, ...) r128_log_return(ctx, level, 0, ##fmt)

inline static void r128_fail(struct r128_ctx *ctx, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  
  exit(1);
}

double r128_time(struct r128_ctx *ctx)
{
  struct timespec ts;
  double res;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  res  = (double) (ts.tv_nsec - ctx->startup.tv_nsec);
  res /= 1000000000.0;
  res += (double) (ts.tv_sec - ctx->startup.tv_sec);
  
  return res;
}

void *r128_malloc(struct r128_ctx *ctx, int size)
{
  void *ret;
  assert((ret = malloc(size)));
  return ret;
}

void *r128_realloc(struct r128_ctx *ctx, void *ptr, int size)
{
  void *ret;
  assert((ret = realloc(ptr, size)));
  return ret;
}

void *r128_zalloc(struct r128_ctx *ctx, int size)
{
  void *ret = r128_alloc(ctx, size);
  memset(ret, 0, size);
  return ret;
}


int r128_report_code(struct r128_ctx *ctx, struct r128_image *img, char *code, int len)
{
  
  if(ctx->flags & R128_FL_EREPORT)
  {
    int i;
    printf("%s:%s:%s", img->root ? img->root->filename : img->filename, img->time_spent, r128_strerror(img->best_rc));
    for(i = 0; i<img->bestcode_len; i++)
      printf("%c%d", i ? ' ' : ':', img->bestcode[i]);
    printf(":");
  }
  
  if(code)
  {
    code[len] = 0;
    printf("%s\n", code);
  }
  else
    printf("\n");
    
  return R128_EC_SUCCESS;
}

int r128_cksum(int *symbols, int len)
{
  int i, wsum = symbols[0];

  for(i = 1; i<len; i++)
    wsum += symbols[i] * i;

  return wsum % 103;
}

int r128_parse(struct r128_ctx *ctx, struct r128_image *img, int *symbols, int len)
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
      return r128_report_code(ctx, img, outbuf, ppos);
    else if(c < 0 && c > 105)
      nb = sprintf(outbuf + pos, " - ?? %d ?? - ", c); 
    else
      nb = sprintf(outbuf + pos, "%s", code_tables[table][c]);

    ppos = pos; pos += nb;
  }
  return R128_EC_NOEND;
}



u_int32_t r128_read_bits(struct r128_ctx *ctx, u_int8_t *line, int w, double ppos, 
                            double uwidth, double threshold,
                            u_int32_t pattern, u_int32_t mask, int read_limit, double *curpos)
{
  int i;
  u_int32_t res = 0;
  
  for(i = 0; i<read_limit; i++)
  {
    double accu = 0.0, npos, sppos;
    int pmin, pmax, j;

    npos = ppos + uwidth;
 
    if(lrint(ceil(npos)) >= w) 
    {
      if(curpos) *curpos = npos;
      break;
    }

    sppos = ceil(ppos);
    
    if(sppos < npos)
    {
      /* Add remaining fraction */
      accu += (ceil(ppos) - ppos) * (double) line[lrint(floor(ppos))];
      
      /* Advance to next whole pixel */ 
      ppos = sppos;
      
      /* Number of pixels to read and average */
      pmin = lrint(ppos);
      pmax = lrint(floor(npos));
      for(j = pmin; j<pmax; j++)
        accu += (double) line[j];
        
      /* Add remaining fraction */
      accu += (npos - floor(npos)) * (double) line[lrint(floor(npos))];
    }
    else
      accu += (npos - ppos) * (double) line[lrint(floor(ppos))];    
    
    /* Ready for inputting the next bit */
    res <<= 1;

    /* Threshold! */
    if(accu < threshold)
      res |= 1;

    if(curpos)
      *curpos = npos;

    if(mask)
      if((res & mask) == pattern)
        return res;
        
    ppos = npos;
  }
  
  return res;
}

void r128_update_best_code(struct r128_ctx *ctx, struct r128_image *im, u_int8_t *symbols, int len)
{
  if(im->bestcode_len > len) return;
  if(im->bestcode_alloc < len)
  {
    while(im->bestcode_alloc < len)
      im->bestcode_alloc *= 2;
    im->bestcode = (u_int8_t*) r128_realloc(im->bestcode, im->bestcode_alloc);
  }
  
  memcpy(im->bestcode, symbols, len);
  im->bestcode_len = len;
}


int r128_read_code(struct r128_ctx *ctx, struct r128_image *img, u_int8_t *line, int w, double ppos, double uwidth, double threshold)
{
  int rc = R128_EC_NOEND;
  int i_thresh = lrint(threshold / uwidth);
  static double weights[] = {1.0, 1.02, 0.98, 1.01, 0.99};
  double new_ppos;
  
  while(lrint(ceil(ppos + 5*uwidth)) < w) 
  {
    int i, cs, i_ppos, offset;
    u_int32_t code;
    
    /* Przejście między symbolami: Biały/Czarny */

    /* Pod ppos powinien być czarny piksel, a przed - biały */
    
/*    i_ppos = lrint(floor(ppos));
    
    for(offset = 0; offset < 5; offset++)
    {
      if(line[i_ppos + offset] < i_thresh && line[i_ppos + offset - 1] > i_thresh)
      {
        printf("corrected by %d; @%d: %d %d\n", offset, i_ppos + offset - 1, line[i_ppos + offset - 1], line[i_ppos + offset]);
        i_ppos += offset;
        break;
      }
      if(line[i_ppos - offset] < i_thresh && line[i_ppos - offset - 1] > i_thresh)
      {
        printf("corrected by %d; @%d: %d %d\n", offset, i_ppos - offset - 1, line[i_ppos - offset - 1], line[i_ppos - offset]);
        i_ppos -= offset;
        break;
      }
    }  
    ppos = i_ppos;*/
//    ppos = floor(ppos);
    
    for(i = 0; i<5; i++) 
    {
      code = r128_read_bits(ctx, line, w, ppos - uwidth - uwidth / 4.0, uwidth * weights[i], threshold * weights[i], 0, 0, 13, &new_ppos);
      cs = code_symbols[(code >> 2) & 0x1ff];
    
      if(!((code & 2) || (!(code & 1)) || (!(code & 0x800)) || (code & 0x1000) || cs == -1))
        break;
      {
//        int i;
//        fprintf(stderr, " ER");
//        for(i = 12; i>=0; i--)
//          fprintf(stderr, ((code >> i) & 0x1) ? "1" : "0");
      }
    }
    
    if(i == 5)
    {
        fprintf(stderr, "\n");
        r128_update_best_code(ctx, im, ctx->codebuf, ctx->codepos);
        return R128_EC_SYNTAX; 
    }
    
    ctx->codebuf[ctx->codepos++] = cs;
    
    /* Perhaps we need to extend the code */
    if(ctx->codepos >= ctx->codealloc)
    {
      ctx->codealloc *= 2;
      assert((ctx->codebuf = (int*) r128_realloc(ctx, ctx->codebuf, sizeof(int) * ctx->codealloc)));
    }
    
    fprintf(stderr, "[%d] ", cs); 
    if(cs == 106)
    {
      r128_update_best_code(ctx, im, ctx->codebuf, ctx->codepos);
      /* Proper STOP! */
      return r128_parse(ctx, im, ctx->codebuf, ctx->codepos);
    }
    ppos = new_ppos - uwidth;
  }
  
  return rc;
}
  
int r128_scan_line(struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, double uwidth, double offset, double threshold)
{
  
/*
  00 1101 0000 1001 = 0x0d09
  00 1101 0010 0001 = 0x0d21
  00 1101 0011 1001 = 0x0d39  
  00 1101 00XX X001 = 0x0d01 = pattern
                      0x3fc1 = mask
 */ 
  int rc = R128_EC_NOCODE;
  double ppos = offset * uwidth, w = li->linesize;
  u_int8_t *line = im->gray_data + li->offset;
  
  if(!w) return R128_EC_NOLINE;
    
  threshold *= 255.0 * uwidth;

  while(1)
  {
    u_int32_t start_symbol = r128_read_bits(ctx, line, w, ppos, uwidth, threshold, 
                      0x0d01, 0x3fc1, 65536, &ppos);
    
    if(lrint(ceil(ppos)) >= w) break; /* Nothing interesting found */
    
    start_symbol &= 0x3fff;

    if(start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
    {
      ctx->codepos = 0;
      ctx->codebuf[ctx->codepos++] = code_symbols[(start_symbol >> 2) & 0x1ff];

      /* Perhaps we found a code! */
      rc = minrc(rc, r128_read_code(ctx, im, line, w, ppos - uwidth, uwidth, threshold));

      /* Have we succeeded? */
      if(R128_ISDONE(ctx, rc)) 
        return rc;
    }
    
    /* Go back a little and try again */
    ppos -= uwidth * 6;
  }
  
  return rc;
}


#define R128_MARGIN_CLASS(c, val) ((val) >= (c)->margin_high_threshold ? 1 : ((val) <= (c)->margin_low_threshold ? -1 : 0))

struct r128_line *r128_get_line(struct r128_ctx *ctx, struct r128_image *im, int line)
{
  int min_len = ctx->min_len, max_gap = ctx->max_gap;
  u_int8_t *data;
  int len, start = 0;
  
  if(im->lines[line].offset != 0xffffffff) return &im->lines[line];
  
  data = im->gray_data;

  start = line * ctx->width;
  len = ctx->width;
  
  if((min_len >= max_gap) && (!(ctx->flags & R128_FL_NOCROP)))
  {
    int prev_so_far = R128_MARGIN_CLASS(ctx, data[min_len - max_gap]);
    int so_far, gap;
    
    /* Left margin */
    for(gap = 1; prev_so_far && (gap + min_len - max_gap)<len; prev_so_far = so_far, gap++)
      if((so_far = R128_MARGIN_CLASS(ctx, data[start + gap + min_len - max_gap])) != prev_so_far)
        break;
    
    /* Indeed a gap! */
    if(gap > max_gap)
    {
      gap += min_len - max_gap;
      start += gap;
      len -= gap;
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

  im->lines[line].offset = start;
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
  int min_ctr, max_ctr, ctr, lines_scanned = 0;
  double t_start = r128_time(ctx);
  
  if(maxheight > im->height) maxheight = im->height;
  
  min_ctr = log2_ceil(im->height / maxheight);
  max_ctr = log2_ceil(im->height / minheight);
  
  ctx->page_scan_id++;
  r128_log(ctx, R128_DEBUG1, "Page scan # %d of img %s starts: offs = %.3f, th = %.2f, heights = %d ~ %d, uwidth = %.2f\n", 
        ctx->page_scan_id, img->filename, offset, ctx->threshold, minheight, maxheight, uwidth);
  
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
    im->best_rc = minrc(im->best_rc, r128_scan_line(ctx, img, r128_get_line(ctx, img, line_idx), uwidth, offset, ctx->threshold));
    
    /* Increment scanned lines counter */
    lines_scanned ++;
    
    /* If we found anything, maybe we can immediately quit? */
    if(R128_ISDONE(ctx, im->best_rc))
    {
      r128_log(ctx, R128_NOTICE, "Code was found in page scan # %d, offs = %.3f, th = %.2f, heights = %d ~ %d, uwidth = %.2f, line = %d\n", 
            ctx->page_scan_id, offset, ctx->threshold, minheight, maxheight, uwidth, line_idx);
      img->time_spent += r128_time(ctx) - t_start;
      return rc;
    }
  }
  r128_log(ctx, R128_DEBUG1, "Page scan %d finished, scanned %d lines, best rc = %d\n", ctx->page_scan_id, lines_scanned, rc);
  img->time_spent += r128_time(ctx) - t_start;

  return rc;
}

void r128_alloc_lines(struct r128_ctx *ctx, struct r128_image *i)
{
  assert((i->lines = (struct r128_line*) malloc(sizeof(struct r128_line) * i->height)));
  memset(i->lines, 0xff, sizeof(struct r128_line) * i->height);
}

struct r128_image * r128_blur_image(struct r128_ctx * ctx, int n)
{
  struct r128_image *i;
  struct r128_image *src = &ctx->im[n];
  
  int *accu;
  int x, y, bh = ctx->blurring_height;
  u_int8_t *minus_line = ctx->im->gray_data, *plus_line = ctx->im->gray_data;
  u_int8_t *result;
  
  if(ctx->im_blurred[n].gray_data) return &ctx->im_blurred[n];

#warning todo tempnam this as well
  r128_log(ctx, R128_DEBUG1, "Preparing a blurred image for %s\n", src->filename);
  
  assert((result = i->gray_data = (u_int8_t*) malloc(ctx->width * ctx->height)));
  i->width = src->width;
  i->height = src->height;
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

int r128_try_tactics(struct r128_ctx *ctx, char *tactics, int start, int len, int codes_to_find)
{
  static double offsets[] = {0, 0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875};
  int w_ctr_min = 1, w_ctr_max = ctx->wctrmax_stage1, w_ctr;
  int offs, offs_min = 0, offs_max = 4;
  int h_min = ctx->expected_min_height, h_max = ctx->max_height;
  char *thresh_input;

  int codes_found = 0;
  
  if(strstr(tactics, "o")) { offs_min = 4; offs_max = 8; }
  if(strstr(tactics, "h")) { h_min = 1; h_max = ctx->expected_min_height; }
  if(strstr(tactics, "w")) { w_ctr_min = w_ctr_max; w_ctr_max = ctx->wctrmax_stage2; }

  if((thresh_input = strstr(tactics, "@")))
    if(sscanf(thresh_input + 1, "%lf", &ctx->threshold) != 1)
      r128_log(ctx, R128_NOTICE, "Invalid threshold in tactics: '%s', keeping to old threshold, equal %.2f.\n", tactics, ctx->threshold);

  r128_log(ctx, R128_DEBUG1, "Now assuming tactics '%s'\n", tactics);
  
  for(offs = offs_min; offs < offs_max && codes_found < codes_to_find; offs++)
    for(w_ctr = w_ctr_min; w_ctr < w_ctr_max; w_ctr++)
    {
      /* First, determine document width in code units for this try */
      double cuw = ctx->min_doc_cuw + ctx->doc_cuw_span * bfsguidance(w_ctr) / 65536.0;
      int i;
      
      /* For all images */
      for(i = 0; i<len; i++)
      {
        /* Get image */
        struct r128_image *img = &ctx->im_blurred[start + i];

        /* Determine unit width to act with for this w_ctr */
        double uwidth = ((double) im->width) / cuw;
      
        /* Check if we can do anything */
        if(!im->gray_data) continue;
        if(uwidth < ctx->min_uwidth || uwidth > ctx->max_uwidth) continue;
        
        if(strstr(tactics, "i"))
          img = r128_blur_image(ctx, start + i);
      
        if(r128_page_scan(ctx, img, offsets[offs], uwidth, h_min, h_max) == R128_EC_SUCCESS)
        {
          codes_found++;
          if(!(ctx->flags & R128_FL_READALL))
          {
            r128_free_image(&ctx->im[start + i]);
            r128_free_image(&ctx->im_blurred[start + i]);            
          }
        }
      }
      if(codes_found >= codes_to_find) break;
    }
  
  r128_log(ctx, R128_DEBUG2, "Finishing tactics '%s': %d of %d codes found\n", tactics, codes_found, codes_to_find);
  return rc;
}

int r128_run_strategy(struct r128_ctx *ctx, char *strategy, int start, int len)
{
  int codes_found = 0, codes_to_find = 0, i; 
  
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

#warning think a while!
  ctx->min_doc_cuw = ((double) ctx->min_width) / ctx->max_uwidth;
  ctx->max_doc_cuw = ((double) ctx->max_width) / ctx->min_uwidth;
  
  /* Therefore, we span a range of...? */
  ctx->doc_cuw_span = ctx->max_doc_cuw - ctx->min_doc_cuw;
  
  /* Number of subdivisions required to reach a difference less than cuw_delta1? */
  ctx->wctrmax_stage1 = log2_ceil(lrint(ctx->doc_cuw_span / ctx->min_cuw_delta1));
  ctx->wctrmax_stage2 = log2_ceil(lrint(ctx->doc_cuw_span / ctx->min_cuw_delta2));
  
  /* Compute # of codes to find */
  for(i = 0; i<len; i++)
    if(ctx->im[start + i].gray_data)
      codes_to_find++;
  
  while(strategy && codes_found < codes_to_find)
  {
    char *next_strategy = index(strategy, ',');
    
    if(next_strategy) *(next_strategy++) = 0;
    codes_found += r128_try_tactics(ctx, strategy, start, len, codes_to_find - codes_found);
    strategy = next_strategy;
  }
  if(ctx->flags & R128_FL_EREPORT)
    for(i = 0; i<len; i++)
      if(ctx->im[start + i].best_rc != R128_RC_SUCCESS)
        r128_report_code(c, &ctx->im[start + i], NULL, 0); 

  r128_log(ctx, R128_DEBUG2, "Giving up a batch strategy run, found %d of %d codes.\n", codes_found, codes_to_find);
  return codes_found;
}


void r128_realloc_buffers(struct r128_ctx *c)
{
  if(c->line_scan_alloc < c->max_height)
  {
    c->line_scan_status = r128_realloc(c->line_scan_status, sizeof(int) * c->max_height);
    memset(c->line_scan_status + c->line_scan_alloc, 0, (c->max_height - line_scan_alloc) * sizeof(int));
    c->line_scan_alloc = c->max_height;
  }
}

const char * r128_strerror(int rc)
{
  switch(rc)
  {
    case R128_EC_SUCCESS: return "R128_EC_SUCCESS";
    case R128_EC_CHECKSUM: return "R128_EC_CHECKSUM";
    case R128_EC_SYNTAX: return "R128_EC_SYNTAX";
    case R128_EC_NOEND: return "R128_EC_NOEND";
    case R128_EC_NOCODE: return "R128_EC_NOCODE";
    case R128_EC_NOLINE: return "R128_EC_NOLINE";
    case R128_EC_NOIMAGE: return "R128_EC_NOIMAGE";
    case R128_EC_NOFILE: return "R128_EC_NOFILE";
    case R128_EC_NOLOADER: return "R128_EC_NOLOADER";
    default: return "(Unknown)";
  }
}


void r128_free_image(struct r128_ctx *c, struct r128_image *im)
{
  if(im->fd != -1) close(im->fd);
  r128_free(im->lines);
  
  if(im->mmaped)
    munmap(im->file, im->file_size);
  else
    r128_free(im->file);
  
  im->fd = -1;
  im->lines = NULL;
  im->file = NULL;
}


char * r128_tempnam(struct r128_ctx *c)
{
  char *res;
  int maxname = 16, l = 0;
  if(c->temp_prefix)
    maxname += (l = strlen(c->temp_prefix));
    
  res = (char*) r128_alloc(c, maxname);
  if(c->temp_prefix)
    memcpy(res, c->temp_prefix, l);
  
  snprintf(c->temp_prefix, maxname - l, "%08d.pgm", c->last_temp++);
  
  return res;
}

char * skip_to_newline_a(char *ptr, int *len, char *copy_to, int copy_len)
{
  while((*len) > 0)
  {
    if(iswhite(*ptr)) break;
    if(copy_len > 0)
    {
      *(copy_to++) = *ptr;
      copy_len--;
    }
    ptr++; (*len)--;
  }
  
  if(copy_len) *copy_to = 0;
  
  if((*len) > 0) { ptr++; (*len)--; }
  return ptr;
}

#define skip_to_newline(ptr, len) skip_to_newline_a(ptr, len, NULL, 0)

char * skip_comments(char *ptr, int *len)
{
  while((*len) > 0)
  {
    if((*ptr) == '#')
    {
      while((*len) > 0)
      {
        if(*ptr == '\n') return ptr;

        ptr++;
        *(len)-- ;        
      }
    }
    else
      return ptr;
  }
  
  return ptr;
}


void r128_parse_pgm(struct r128_ctx *c, struct r128_image *im, char *filename)
{
#define PGM_MAX_LINE 256
#define PGM_MIN_HEADER 8
/*
P1 (3 bytes)
1 1 (4 bytes)
. (1 byte)
*/

  int type = 0, maxval = 255;
  char line[PGM_MAX_HEADER];
  char *b = im->file;
  int len = im->file_size, rc = R128_ERROR_NOIMAGE, explen, as;
  char *nl;
  
  if(!filename) filename = im->filename;
  
  b = im->file;
  len = im->file_size;

  if(len < PGM_MIN_HEADER)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: file too short.\n", filename);
  
  if(b[0] != 'P' && !iswhite(b[2]))
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: file too short.\n", filename);
  
  type = b[1] - '0';
  if(type < 4 || type > 6)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: image type P%c is not supported.\n", filename, b[1]);
  
  b = skip_to_newline(b, &len);
  b = skip_comments(b, &len);
  b = skip_to_newline_a(b, &len, line, PGM_MAX_HEADER - 1);
  if(!len) 
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header.\n", filename);
  
  line[PGM_MAX_HEADER - 1] = 0;

  if(sscanf(line, "%d", &im->width) != 1)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header (unreadable width x height).\n", filename);

  b = skip_comments(b, &len);
  b = skip_to_newline_a(b, &len, line, PGM_MAX_HEADER - 1);

  if(sscanf(line, "%d", &im->height) != 1)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header (unreadable width x height).\n", filename);
  
  if(im->width < 1 || im->height < 1 || im->width > 65535 || im->height > 65536)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: unsupported image size %d x %d.\n", filename, im->width, im->height);
  
  b = skip_comments(b, &len);
  b = skip_to_newline_a(b, &len, line, PGM_MAX_HEADER - 1);

  if(!len) 
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header.\n", filename);

  if(sscanf(line, "%d", &maxval) != 1)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid range.\n", filename);
  
  if(maxval < 1 || maxval > 255)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: unsupported pixel value range %d.\n", filename, maxval);
  
  explen = im->width * im->height;
  if(type == 4) explen = explen / 8;
  if(type == 6) explen *= 3;
  
  if(len < explen)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: file is truncated (remaining %d, we need %d).\n", filename, len, explen);
  
  
  if(type == 5 || type == 6 || maxval != 255) /* Needs processing */
  {
    int pix, pixcnt = im->width * im->height;
    u_int8_t *data = NULL;
    u_int8_t *line = NULL;
    u_int8_t *res = NULL;
    int fd, x = 0, cr = 0;
    char *tempnam;
    
    if(c->flags & R128_FL_MMAPALL)
    {
      char pgmhdr[256];
      int l;

      /* Mmap mode */
      tempnam = r128_tempnam(c);
      
      if((fd = open(tempnam, O_WRONLY | O_TRUNC)) < 0)
      {
        free(tempnam);
        return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: Cannot open temporary file %s to store conversion result, open(): %s.\n", filename, tempnam, strerror(errno));
      }

      
      line = r128_malloc(im->width);
      l = snprintf(pgmhdr, 256, "P5\n%d %d\n255\n", im->width, im->heigth);
      if(write(fd, pgmhdr, l) != l);
      {
        free(line); free(tempnam); close(fd);
        return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: Cannot write PGM header to temporary file %s to store conversion result, write(): %s.\n", filename, tempnam, strerror(errno));
      }
    }
    else
      /* RAM mode */
      data = line = r128_malloc(im->width * im->height);

    for(pix = 0; ; pix++)
    {
      if(x >= im->width)
      {
        /* Line is finished */
        if(c->flags & R128_FL_MMAPALL)
        {
          /* Save this line */
          if(write(fd, line, im->width) != im->width)
          {
            free(line); free(tempnam); close(fd);
            return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: Cannot write to temporary file %s to store conversion result, write(): %s.\n", filename, tempnam, strerror(errno));
          }
        }
        else
          /* Continue writing to RAM */
          line += im->width;

        x = 0;
      }
      if(pix > pixcnt) break;
      switch(type)
      {
        case 4:
          if((x & 7) == 0)
            cr = *(b++);
          line[x++] = (cr & 0x80) ? 255 : 0;
          cr >>= 1;
          break;
        case 5: line[x++] = *(b++) * 255 / maxval; break;
        case 6: line[x++] = (*(b++) + *(b++) + *(b++)) / 3; break;
      }
    }
    
    r128_free_image(c, im);
    
    if(c->flags & R128_FL_MMAPALL)
    {
      int rc;
      /* Mmap mode */
      close(fd);
      free(line);
      
      rc = r128_load_pgm(c, im, tempnam);
      free(tempnam);
      
      return rc;
    }
    else
    {
      free(im->file);
      im->file = data;
      im->gray_data = data;
    }
  }
  else
    im->gray_data = b;

  im->lines = (struct r128_line*) r128_alloc(as = sizeof(struct r128_line) * im->height);
  memset(im->lines, 0xff, as);
  
  return r128_log_return(c, R128_DEBUG1, R128_RC_NOIMAGE - 1, "Successfuly parsed PGM file %s (%d x %d, P5, 255)\n", filename, im->width, im->height);
}

int r128_load_pgm(struct r128_ctx *c, struct r128_image *im, char *filename)
{
  int rc;
  
  r128_log(c, R128_DEBUG1, "Reading file %s.\n", filename);
  if((im->fd = open(filename, O_RDONLY)) == -1)
    return r128_log_return(c, R128_ERROR, R128_EC_NOFILE, "Cannot read file %s: open(): %s\n", filename, strerror(errno));
    
  if(c->flags & R128_FL_RAMALL)
  {
    int alloc_len = R128_ALLOC_STEP;
    im->file = (char*) r128_malloc(alloc_len);
    
    while((rc = read(im->fd, im->file + im->file_size, alloc_len - im->file_size)) > 0)
    {
      im->file_size += rc;
      if(im->file_size == alloc_len)
      {
        alloc_len += R128_ALLOC_STEP;
        im->file = (char*) r128_realloc(im->file, alloc_len);
      }
    }
    
    if(rc == -1)
    {
      close(im->fd);
      r128_free(im->file);
      r128_log(c, R128_ERROR, "Cannot load file %s: read(): %s\n", filename, strerror(errno));
      return R128_EC_NOFILE;
    }
    r128_log(c, R128_DEBUG1, "Successfully loaded file %s into RAM.\n", filename);
  }
  else
  {
    struct stat *s;
    if(fstat(im->fd, &s) == -1)
    {
      close(im->fd);
      r128_log(c, R128_ERROR, "Cannot mmap file %s: stat(): %s\n", filename, strerror(errno));
      return R128_EC_NOFILE;
    }
    
    if((im->file = mmap(NULL, im->file_size = s.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
      im->file = NULL;
      close(im->fd);
      r128_log(c, R128_ERROR, "Cannot mmap file %s: mmap(): %s\n", filename, strerror(errno));
      return R128_EC_NOFILE;
    }
    im->mmaped = 1;
    r128_log(c, R128_DEBUG1, "Successfully mmap()ed file %s.\n", filename); 
  }
  
  if((rc = r128_parse_pgm(c, im, filename)) >= R128_RC_NOIMAGE)
    r128_free_image(c, im);

  return rc;
}

int r128_load_file(struct r128_ctx *c, struct r128_image *im)
{
  double t_start;
  int childpid, readerror = 0, maxfd;
  int sp_stdout[2];
  int sp_stderr[2];
  char *data;
  int alloc_size = R128_READ_STEP;
  static char stderr_buf[1024];
  char *tempnam;
  int fd;
  
  if(!c->loader)
    return r128_load_pgm(c, im, im->filename);

  /* assume it'll be okay! */
  data = (char*) r128_malloc(alloc_size);
  if(c->flags & R128_FL_MMAPALL)
  {
    tempnam = r128_tempnam(c);

    if((fd = open(tempnam, O_WRONLY | O_TRUNC)) < 0)
    {
      free(tempnamp);
      return r128_log_return(c, R128_ERROR, rc, "Cannot load file %s: Cannot open temporary file %s to store loader result, open(): %s.\n", im->filename, tempnam, strerror(errno));
    }
  }
  
  if(socketpair(AF_LOCAL, 0, 0, &sp_stdout) == -1)
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot start loader for file %s: socketpair(): %s\n", im->filename, strerror(errno));

  if(socketpair(AF_LOCAL, 0, 0, &sp_stderr) == -1)
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot start loader for file %s: socketpair(): %s\n", im->filename, strerror(errno));
  
  t_start = r128_time(c);
  switch((childpid = fork()))
  {
    case -1:
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot start loader for file %s: fork(): %s\n", im->filename, strerror(errno));

    case 0:
    {
      char *args[4] = {"/bin/sh", "-c", NULL, NULL};
      char *cmdstr;
      int i, l, pos;
      
      close(sp_stdout[0]);
      close(sp_stderr[0]);
      dup2(1, sp_stdout[1]);
      dup2(2, sp_stderr[2]);
      
      cmdstr = r128_alloc(c, (pos = strlen(c->loader)) + (l = strlen(im->filename)) * 5 + 16);
      memcpy(cmdstr, c->loader, pos);
      pos += sprintf(cmdstr + pos, " '");
      for(i = 0; i<l; i++)
        if(im->filename[i] == '\'')
        {
          cmdstr[pos++] = '\'';
          cmdstr[pos++] = '"';
          cmdstr[pos++] = '\'';
          cmdstr[pos++] = '"';
          cmdstr[pos++] = '\'';
        }
        else	
          cmdstr[pos++] = im->filename[i];
      
      cmdstr[pos++] = '\'';
      cmdstr[pos] = 0;
      args[2] = cmdstr;
      
      execv(args[0], args); 
      r128_fail(c, "Cannot start loader for file %s: execv(): %s\n", im->filename, strerror(errno));
      break;
    }  

    default:
      close(sp_stdout[1]);
      close(sp_stderr[1]);
      break;
  }
  

  /* Read data */
  maxfd = sp_stderr[1];
  if(sp_stdout[1] > maxfd) maxfd = sp_stdout[1];
  
  while(1)
  {
    fd_set fds;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    
    FD_ZERO(&fds);

    FD_SET(sp_stderr[1], &fds);
    FD_SET(sp_stdout[1], &fds);

    if(select(maxfd + 1, &fds, NULL, NULL, &tv) < 0)
    {
      if(errno != EINTR)
      {
        r128_log(c, LOG_ERROR, "Cannot proceed with loading file %s from loader: select(): %s\n", im->filename, strerror(errno));
        break;
      }
    }
    
    if(FD_ISSET(sp_stdout[1], &fds))
    {
      int l = read(sp_stdout[1], data + im->file_size, alloc_size - im->file_size);
      if(l < 0)
      {
        r128_log(c, LOG_ERROR, "Cannot proceed with loading file %s from loader: read(stdout): %s\n", im->filename, strerror(errno));
        break;
      }
      else if(l > 0)
      {
        im->file_size += l;
        if(alloc_size == im->file_size)
        {
          if(c->flags & R128_FL_MMAPALL)
          {
            if((write(fd, data, alloc_size)) != alloc_size)
            {
              r128_log(c, LOG_ERROR, "Cannot proceed with loading file %s from loader: cannot write to temporary file %s, write(): %s: %s\n", im->filename, tempnam, strerror(errno));
              break;
            }
            im->file_size = 0;
          }
          else
          {
            alloc_size += R128_READ_STEP;
            data = (char*) r128_realloc(data, alloc_size);
          }          
        }
      }
      else
        break; /* EOF! Great! */
    }
    
    if(FD_ISSET(sp_stderr[1], &fds))
    {
      int l = read(sp_stderr[1], stderr_buf, 1023);
      if(l < 0)
      {
        r128_log(c, LOG_ERROR, "Cannot proceed with loading file %s from loader: read(stderr): %s\n", im->filename, strerror(errno));
        break;
      }
      else if(l > 0)
      {
        stderr_buf[l] = 0;
        r128_log(c, LOG_NOTICE, "Loader for file %s says: %s\n",  im->filename, stderr_buf);
      }
      else
        break; /* EOF! */
    }
    
    if(c->loader_limit != 0.0)
      if((r128_time(c) - t_start) > c->loader_limit)
      {
        r128_log(c, LOG_ERROR, "Terminating loader for file %s: time limit exceeded.\n", im->filename);
        break;
      }
  }

  close(sp_stderr[1]);
  close(sp_stdout[1]);

  /* Get rid of the loader process */
  kill(childpid, SIGKILL);
  while(waitpid(childpid, NULL, WNOHANG) <= 0)
    usleep(100000);

  if(c->flags & R128_FL_MMAPALL)
  {
    int rc;
    if((write(fd, data, im->file_size)) != im->file_size)
    {
      r128_log(c, LOG_ERROR, "Cannot proceed with loading file %s from loader: cannot write to temporary file %s, write(): %s: %s\n", im->filename, tempnam, strerror(errno));
      break;
    }
    im->file_size = 0;
    
    free(data);
    rc = r128_load_pgm(c, im, tempnam);
    free(tempnam);
    return rc;
  }
  else
  {
    im->file = data;
    return r128_parse_pgm(c, im, NULL);
  }
}

inline static double floatopt(struct r128_ctx *ctx, char *opt, char *val)
{
  double res;
  char *eptr;
  res = strtod(val, &eptr);
  if(*eptr)
    r128_fail(ctx, "Invalid value for %s: %s\n", opt, val);
  return res;
}

inline static int intopt(struct r128_ctx *ctx, char *opt, char *val)
{
  int res;
  char *eptr;
  res = strtol(val, &eptr, 0);
  if(*eptr)
    r128_fail(ctx, "Invalid value for %s: %s\n", opt, val);
  return res;
}

void r128_defaults(struct r128_ctx *c)
{
  memset(c, 0, sizeof(struct r128_ctx));
  c->strategy = "IHWO@0.6,iHWO,IHWo,iHWo,IHwO,iHwO,IHwo,iHwo,IhWO,ihWO,IhWo,ihWo,IhwO,ihwO,Ihwo,ihwo";
  c->min_uwidth = 0.9;
  c->max_uwidth = 4;
  c->threshold = 0.5;
  c->margin_low_threshold = 20;
  c->margin_high_threshold = 235;
  c->min_cuw_delta2 = 2;
  c->min_cuw_delta1 = 10;
  c->expected_min_height = 8;
  c->temp_prefix = "r128temp";
  c->batch_size = 1;

  c->code_alloc = 128;
  asset((c->codebuf = (int*) r128_malloc(c->code_alloc * sizeof(int))));
}

int help(char *progname, int verbosity)
{
  printf("rapid128 barcode scanner, (c) 2015 Mateusz 'mteg' Golicz\n");
  printf("Usage: %s [options] <file1> [<file2> [<file3> ...]]\n", progname);
  printf("\n");
  printf("Available options:\n");
  printf(" -h         Print help and quit\n");
  printf(" -hh        Print extended help and quit\n");
  printf("Available options:\n");
  printf("BASIC SCANNER PARAMETERS\n");
  printf(" -wa <pix>  Minimum expected width of thinnest bar in pixels\n");
  printf(" -wb <pix>  Maximum expected width of thinnest bar in pixels\n");
  printf(" -mh <lvl>  High (white) threshold for cutting margins\n");
  printf(" -ml <lvl>  Low (black) threshold for cutting margins\n");
  printf("\n");
  printf("PERFORMANCE TUNING\n");
  printf(" -ch <pix>  Expected minimal code height\n");
  printf(" -bh <pix>  Blurring height\n");
  printf(" -s  <strg> Configure scanning strategy\n"); 
  printf("\n");
  printf("BATCH PROCESSING\n");
  printf(" -bs <cnt>  Batch size - number of files processed at once\n");
  printf(" -bt <secs> Time limit for each batch\n");
  printf("\n");
  printf("FILE INPUT\n");
  printf(" -lc <cmd>  Set loader command (spacebar + file name is appended at the end)\n");
  printf(" -lt <secs> Time limit for the loader command\n");
  printf("\n");
  printf("MEMORY MANAGEMENT\n");
  printf(" -mm        Save RAM: write to temporary files and mmap() everything\n");
  printf(" -mp <pfx>  Prefix for temporary files (NNNNNN.pgm is appended)\n"); 
  printf(" -ma        Never mmap() anything, load all into RAM\n");
  printf("\n");
  printf("REPORTING AND DEBUGGING\n");
  printf(" -a         Continue scanning the document even after a valid code is found.\n");
  printf(" -e         Extended reporting: print file name, time spent, best result, code symbols.\n");
  printf(" -v         Increase out-of-band verbosity level\n");
  printf(" -q         Decrease out-of-band verbosity level\n");
  printf(" -nd        Do not delete temporary files (use with -mm)\n");
  printf(" -nm        Do not crop lines\n");
  return 0;
}


int main(int argc, char ** argv)
{
  FILE *fh;
  struct r128_ctx ctx;
  int c, nh = 0, i;
  u_int8_t *bestcodes;

  clock_gettime(CLOCK_MONOTONIC, &ctx.startup);
  r128_defaults(&ctx);
  
  while((c = getopt(argc, argv, "abcehlmnqvs:w")) != EOF)
  {
    switch(c)
    {
      case 'a': ctx.flags |= R128_FL_READALL; break;
      case 'b':
        switch((c = getopt(argc, argv, "h:s:t:")))
        {
          case 'h': ctx.blurring_height = intopt(&ctx, "-bh", optarg); break;
          case 's': ctx.batch_size = intopt(&ctx, "-bs", optarg); break;
          case 't': ctx.batch_limit = floatopt(&ctx, "-bt", optarg); break;
          default:  r128_fail(ctx, "Unknown option: -b%c\n", c); break;
        }
        break;
      case 'c':
        switch((c = getopt(argc, argv, "h:")))
        {
          case 'h': ctx.expected_min_height = intopt(&ctx, "-ch", optarg); break;
          default:  r128_fail(ctx, "Unknown option: -c%c\n", c); break;
        }
        break;
      
      case 'e': ctx.flags |= R128_FL_EREPORT; break;
      case 'h': nh++; break;
      case 'l':
        switch((c = getopt(argc, argv, "c:t:")))
        {
          case 'c': asset((ctx.loader = strdup(optarg))); break;
          case 't': ctx.loader_limit = floatopt(&ctx, "-lt", optarg); break;
          default:  r128_fail(ctx, "Unknown option: -l%c\n", c); break;
        }
        break;
      case 'm':
        switch((c = getopt(argc, argv, "map:h:l:")))
        {
          case 'm': ctx.flags |= R128_FL_MMAPALL; break;
          case 'a': ctx.flags |= R128_FL_RAMALL; break;
          case 'p': assert((ctx.temp_prefix = strdup(optarg)); break;
          case 'l': ctx.margin_low_threshold = floatarg(&ctx, "-ml", optarg); break;
          case 'h': ctx.margin_high_threshold = floatopt(&ctx, "-mh", optarg); break;
          default:  r128_fail(ctx, "Unknown option: -m%c\n", c); break;
        }
        break;
      case 'n':
        switch((c = getopt(argc, argv, "dm")))
        {
          case 'd': ctx.flags |= R128_FL_KEEPTEMPS; break;
          case 'm': ctx.flags |= R128_FL_NOCROP; break;
        }
        break;
      case 'q': ctx.logging_level--; break;
      case 'v': ctx.logging_level++; break;        
      case 's': assert((ctx.strategy = strdup(optarg))); break;
      case 'w':
        switch((c = getopt(argc, argv, "a:b:")))
        {
          case 'a': ctx.min_uwidth = floatopt(&ctx, "-wa", optarg); break;
          case 'b': ctx.max_uwidth = floatopt(&ctx, "-wb", optarg); break;
          default:  r128_fail(ctx, "Unknown option: -w%c\n", c); break;
        }
        break;
      default:
        r128_fail(ctx, "Unknown option: %c\n", c);
        break;
    }
  }
  
  if(nh)
    return help(argv[0], nh);
  
  if(!argv[optind])
  {
    fprintf(stderr, "Usage: %s [options] <file name>.pgm\n", argv[0]);
    fprintf(stderr, "Run %s -h to get help\n", argv[0]);
    return 1;
  }
  
  if(ctx.batch_size < 1) ctx.batch_size = 1;
  if(ctx.blurring_height == 0)
    ctx.blurring_height = ctx.expected_min_height / 2;
  
  ctx.n_images = argc - optind;
  ctx.im = (struct r128_image*) r128_zalloc(sizeof(struct r128_image) * ctx.n_images);
  ctx.im_blurred = (struct r128_image*) r128_zalloc(sizeof(struct r128_image) * ctx.n_images);
  bestcodes = (u_int8_t*) r128_malloc(R128_EXP_CODE_MAX * ctx.n_images);
  
  for(i = 0; i<ctx.n_images; i++)
  {
    assert((ctx.im[i].filename = strdup(argv[optind + 1])));

    ctx.im[i].best_rc = R128_EC_NOTHING;
    ctx.im[i].fd = -1;

    ctx.im[i].bestcodes = bestcodes;
    bestcodes += R128_EXP_CODE_MAX;
  }
  
  r128_log(&ctx, LOG_DEBUG1, "Command line arguments parsed. Processing starts.\n");
  for(i = 0; i<ctx.n_images; i += ctx.batch_size)
  {
    int j, batch_size = ctx.batch_size;
    
    if((i + batch_size) < ctx.n_images)
      batch_size = ctx.n_images - i;

    r128_log(&ctx, LOG_DEBUG1, "Starting processing of batch %d.\n", i / ctx.batch_size);
    ctx.batch_start = r128_time(&ctx);
    
    ctx.min_width = 65536;
    ctx.max_width = 0;
    ctx.max_height = 0;
    
    /* Load all images for this batch */
    for(j = 0; j<batch_size; j++)
    {
      struct r128_image * im = &ctx.im[i + j];
      im->best_rc = min(r128_load_file(&ctx, im), im->best_rc);

      if(!im->gray_data) continue;
      
      if(im->height > ctx.max_height) ctx.max_height = im->height;
      if(im->width > ctx.max_width) ctx.max_width = im->width;
      if(im->width < ctx.min_width) ctx.min_width = im->width;
    }
    
    /* Reallocate global buffers to match potentially new image sizes */
    r128_realloc_buffers(&ctx);
    
    /* Find the CODES! */
    r128_run_strategy(&ctx, ctx.strategy, i, batch_size);

    /* Free all images of this batch */
    for(j = 0; j<batch_size; j++)
    {
      r128_free_image(&ctx, &ctx.im[i + j]);
      r128_free_image(&ctx, &ctx.im_blurred[i + j]);
    }
    
    /* Delete all temporary files */
    if(!(ctx.flags & R128_FL_KEEPTEMPS))
    {
      for(j = ctx.first_temp; j<ctx.last_temp; j++)
      {
        char tempname[PATH_MAX + 1];
        snprintf(tempname, PATH_MAX, "%s%08d.pgm", c->temp_prefix, j);
        if(unlink(tempname) == -1)
          r128_log(&ctx, LOG_ERROR, "Cannot delete temporary file %s. unlink(): %s\n", tempname, strerror(errno));
      }
      ctx.first_temp = ctx.last_temp;  
    }

    /* Report batch finished */
    r128_log(&ctx, LOG_DEBUG1, "Finished processing of batch %d. So far, %d images processed, %d codes found.\n",
      i / ctx.batch_size, i + batch_size - 1, ctx.n_codes_found);
  }
  r128_log(&ctx, LOG_DEBUG1, "All batches processed. Terminating.\n");
  
  return 0;
}
