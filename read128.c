#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include "read128.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

int err(char *msg)
{
  fprintf(stderr, "%s\n", msg);
  return 1;
}

extern struct codetree_node * code_tree;
extern char ** code_tables[];

int r128_report_code(struct r128_ctx *ctx, char *code)
{
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
  int pos = 0;

  /* Min-length */
  if(len < 3)
    return R128_EC_SYNTAX;
  
  /* Checksum! */
  if(!(ctx->flags & R128_FL_NOCKSUM))
    if(r128_cksum(symbols, len - 2) != symbols[len - 1])
      return R128_EC_CHECKSUM; 
  
  outbuf[0] = 0;
  
  for(i = 0; i<len; i++)
  {
    int c = symbols[i];
    
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
      return r128_report_code(ctx, outbuf);
    else if(c < 0 && c > 105)
      pos += sprintf(outbuf + pos, " - ?? %d ?? - ", c); 
    else
      pos += sprintf(outbuf + pos, "%s", code_tables[table][c]);
  }
  return R128_EC_NOEND;
}


int r128_decode(struct r128_ctx *ctx, char *code_in)
{
  int code_out[1024];
  int symbol = 0, i;
  struct codetree_node *n = code_tree;
  
  for(i = 0; code_in[i]; i++)
  {
    /* Push four bits */
    u_int8_t nibble = code_in[i] - 'A';
    int bitcount = 0;
    
    for(bitcount = 0; bitcount<4; bitcount++, nibble <<= 1)
    {
      int bit = nibble & 0x08;
      n = bit ? n->one : n->zero;

      if(!n) return R128_EC_SYNTAX; /* Out of code */
      if(n->code == -1) continue; /* Next bit please! */
      
      /* Got a symbol! */
      code_out[symbol++] = n->code;
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
      rc = min(rc, R128_EC_NOEND);

      /* See if there is at least one ending */
      for(j = 0; j<4; j++)
        if(strstr(ptr, ends[j]))
        {
          /* Ending found - decode & eject! */
          rc = min(rc, r128_decode(ctx, ptr));
          break;
        }

      /* Have we succeeded? */
      if(rc == RC128_EC_SUCCESS)
        if(!(ctx->flags & R128_FL_ALLCODES))
          return rc;

      /* Try next occurence of this start code */
      ptr++;
    }
  }
  
  /* No luck or they want us to print absolutely all */
  return rc;
}
  
void r128_scan_line(struct r128_ctx *ctx, u_int8_t *im, int w, int h, int l, double pps, double offset, double threshold)
{
  char b1[8192];
  char b2[8192];
  char b3[8192];
  char b4[8192];
  char *b[4] = {
    b1, b2, b3, b4
  };
  int i, len, charlen, rc = R128_EC_NOCODE;
  double ppos = offset;
  
  threshold *= 255.0 * pps;
  
  u_int8_t *line = im + l * w;
  
  len = lrint(floor(((double) w) / pps));
  charlen = (len / 4) + 1;
  
  printf("w = %d, pps = %.5f, offs = %.5f, thresh = %.5f, code len = %d\n", w, pps, offset, threshold, len);
  
  for(i = 0; i<4; i++)
    memset(b[i], 0, 8192);
    
  for(i = 0; i<len; i++)
  {
    double accu = 0.0, npos;
    int pmin, pmax, j, digit;

    npos = ppos + pps;
    
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
    
    rc = min(rc, r128_find_code(ctx, b[i]));
    if(RC128_ISDONE(rc, ctx)) return rc;
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

int r128_bfs_scan(struct r128_ctx *ctx, struct r128_image *img,
          double threshold, double offset, double cuwidth, int mindepth, int maxdepth)
{
  int line, rc = R128_EC_NOCODE;
  
  int ctr, ctr_max = 1 << maxdepth;
  
  /* Do a BFS scan of lines */
  memset(ctx->line_done, 0, ctx->height);
  for(ctr = 1 << mindepth; ctr < ctr_max; ctr++)
  {
    /* Determine line number */
    int line_idx = (ctx->height * bfsguidance(ctr)) >> 16;
    
    /* Check if this line was already done in this pass */
    if(ctx->line_done[line_idx]) continue;
    
    /* Mark line as checked */
    ctx->line_done[line_idx] = 1;

    /* Check the line */
    rc = min(rc, r128_scan_line(ctx, im, line, threshold, offset, cuwidth));
    
    /* If we found anything, maybe we can immediately quit? */
    if(R128_ISDONE(ctx, rc))
      return rc;
  }
}

int r128_explore(struct r128_ctx *ctx, char *bounds)
{
  static double offsets[] = {0, 0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875};
  int offs, offs_min, offs_max;
  int x_min, x_max, x;
  int y_min, y_max;
  struct r128_image *img;
  int rc;
  
  img = strstr(bounds, "I") ? ctx->im : ctx->im_blurred;
  
  if(strstr(bounds, "X"))
  {
    x_min = 0;
    x_max = ctx->x_depth1 + 1;
  }
  else
  {
    x_min = ctx->x_depth1;
    x_max = ctx->x_depth2 + 1;
  }
  
  if(strstr(bounds, "Y"))
  {
    y_min = 0;
    y_max = ctx->y_depth1 + 1;
  }
  else
  {
    y_min = ctx->y_depth1;
    y_max = ctx->y_depth2 + 1;
  }
  
  if(strstr(bounds, "O"))
  {
    offs_min = 0;
    offs_max = 4;
  }
  else
  {
    offs_min = 4;
    offs_max = 8;
  }
  for(offs = offs_min; offs < offs_max; offs++)
    for(x = x_min; x < x_max; x++)
    {
      rc = min(rc, r128_bfs_scan(ctx, ));
      if(R128_ISDONE(ctx, rc))
        return rc;
    }
  
  return R128_EC_NOCODE;
  
  

  
  for(o
  
}

int r128_struggle(struct r128_ctx *ctx)
{
  double min_cuw, max_cuw;
  
  /* Minimum codeunit width of document is? */
  min_cuw = ((double) ctx->width) / ctx->max_uwidth;
  max_cuw = ((double) ctx->width) / ctx->min_uwidth;
  
  rd ofs
  0 0
  0 1
  
  "ihro,Ihro,ihrO,IhrO,ihRo,IhRo,ihRO,IhRO,iHro,IHro,iHrO,IHrO,iHRo,IHRo,iHRO,IHRO";
  "IYXO,iYXO,IYXo,iYXo,IYxO,iYxO,IYxo,iYxo,IyXO,iyXO,IyXo,iyXo,IyxO,iyxO,Iyxo,iyxo"
  
  /* Scanning strategy: 
      (1) original, min codeheight, first resolution depth, basic offsets
      (2) motionblurred, min codeheight, first resolution depth, basic offsets
      (3) original, min codeheight, first resolution depth, remaining offsets
      (4) motionblurred, min codeheight, first resolution depth, remaining offsets
      (5) original, min codeheight, remaining resolution depths, basic offsets
      (6) motionblurred, min codeheight, remaining resolution depths, basic offsets
      (7) original, min codeheight, remaining resolution depths, remaining offsets
      (8) motionblurred, min codeheight, remaining resolution depths, remaining offsets
      
      like above, but all codeheights 
   */
  
  
  
int r128_bfs_scan(struct r128_ctx *ctx, struct r128_image *img,
          double threshold, double offset, double cuwidth, int rescan, int minspan)

}

int main(int argc, char ** argv)
{
  FILE *fh;
  char line[128 + 1];
  int w, h, i; 
  u_int8_t *im;
  struct r128_ctx ctx; 
  
  int c, nh = 0;
  
  while((c = getopt(argc, argv, "")))
  {
    switch(c)
    {
      case 'h':
        help(argv[0], nh++);
        break;
    }
  }
  
  
  if(argc < 3)
  {
    fprintf(stderr, "Usage: read128 <file name>.pgm <coeff>\n");
    return 1;
  }
  
  if(!(fh = fopen(argv[1], "rb")))
  {
    perror("fopen");
    return 1;
  }
  
  if(!fgets(line, 128, fh)) return err("Invalid file format (1)");
  if(strncmp(line, "P5", 2)) return err("Invalid file format (2)");

  if(!fgets(line, 128, fh)) return err("Invalid file format (3)");
//  if(!fgets(line, 128, fh)) return err("Invalid file format (3a)");
  if(!fgets(line, 128, fh)) return err("Invalid file format (4)");
  if(sscanf(line, "%d %d", &w, &h) != 2) return err("Invalid file format (5, wrong size)");
  if(!fgets(line, 128, fh)) return err("Invalid file format (6)");
  
  assert(im = (unsigned char*) malloc(w * h));
  if(fread(im, w * h, 1, fh) != 1) return err("Read error (file truncated?)");
  fclose(fh);
  
  
  assert(ctx.bfs_queue = (struct r128_queue_entry*) malloc(sizeof(struct r128_queue_entry) * (ctx.height + 16)));
  ctx.q_len = 1; ctx.q_head = 0;
  ctx.bfs_queue[0].line_from = 0;
  ctx.bfs_queue[0].line_to = ctx.height - 1;
  
  
  
  
  
  for(i = 0; i<h; i++)
  {
    double pps = atof(argv[2]);
    double offset = 0;
    int j;
    
    for(j = 0; j<8; j++, offset += pps / 8.0)
      scan_line(im, w, h, i, pps, offset, 0.5);
  }
  
  
  return 0;
}
