#include <stdio.h>
#include <errno.h>
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
#include <sys/mman.h>
#include <fcntl.h>
#include "rapid128.h"

#define R128_MARGIN_CLASS(c, val) ((val) >= (c)->margin_high_threshold ? 1 : ((val) <= (c)->margin_low_threshold ? -1 : 0))

struct r128_line *r128_get_line(struct r128_ctx *ctx, struct r128_image *im, int line)
{
  int min_len = ctx->min_len, max_gap = ctx->max_gap;
  u_int8_t *data;
  int len, start = 0;
  
  if(im->lines[line].offset != 0xffffffff) return &im->lines[line];
  
  data = im->gray_data;

  start = line * im->width;
  len = im->width;
  
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
  r128_log(ctx, R128_DEBUG2, "Prepared line %d of %simage %s: start = %d, len = %d\n", line, im->root ? "blurred " : "", 
        im->root ? im->root->filename : im->filename, start, len);

  im->lines[line].offset = start;
  im->lines[line].linesize = len;
  
  return &im->lines[line];
  
  
}



void r128_alloc_lines(struct r128_ctx *ctx, struct r128_image *i)
{
  assert((i->lines = (struct r128_line*) malloc(sizeof(struct r128_line) * i->height)));
  memset(i->lines, 0xff, sizeof(struct r128_line) * i->height);
}



int r128_mmap_converted(struct r128_ctx *c, struct r128_image *im, int fd, char *tempnam)
{
  int rc;
  
  /* Mmap mode */
  close(fd);
  free(im->gray_data);
  im->gray_data = NULL;
  
  rc = r128_load_pgm(c, im, tempnam);
  free(tempnam);
  
  return rc;
}

u_int8_t * r128_alloc_for_conversion(struct r128_ctx *c, struct r128_image *im, char *filename, char *operation, int *fd_store, char **tempnam_store)
{
  int fd;
  u_int8_t *line;
  char *tempnam;
  
  if(c->flags & R128_FL_MMAPALL)
  {
    char pgmhdr[256];
    int l;

    /* Mmap mode */
    tempnam = r128_tempnam(c);
    
    if((fd = open(tempnam, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
    {
      r128_log(c, R128_ERROR, "Cannot parse PGM file %s: Cannot open temporary file %s to store %s result, open(): %s.\n", filename, tempnam, operation, strerror(errno));
      free(tempnam);
      return NULL;
    }

    
    line = r128_malloc(c, im->width);
    l = snprintf(pgmhdr, 256, "P5\n%d %d\n255\n", im->width, im->height);
    if(write(fd, pgmhdr, l) != l)
    {
      r128_log(c, R128_ERROR, "Cannot parse PGM file %s: Cannot write %d bytes of PGM header to temporary file %s to store %s result, write(): %s.\n", filename, l, tempnam, operation, strerror(errno));
      free(line); free(tempnam); close(fd);
      return NULL;
    }
    
    *tempnam_store = tempnam;
    *fd_store = fd;
  }
  else
    line = im->gray_data = (u_int8_t*) r128_malloc(c, im->width * im->height);

  return line;
  
}


struct r128_image * r128_blur_image(struct r128_ctx * ctx, int n)
{
  struct r128_image *i = &ctx->im_blurred[n];
  struct r128_image *src = &ctx->im[n];
  
  int *accu;
  int x, y, bh = ctx->blurring_height, pix = 0;
  u_int8_t *minus_line = src->gray_data, *plus_line = src->gray_data;
  u_int8_t *result;
  char *tempnam;
  int fd = -1;
  
  if(i->root) return i;

  i->root = src;
  i->best_rc = R128_EC_NOIMAGE;
  i->bestcode = r128_malloc(ctx, R128_EXP_CODE_MAX);
  i->bestcode_alloc = R128_EXP_CODE_MAX;
  i->fd = -1;
  
  if(ctx->flags & R128_FL_NOBLUR) return i;
  r128_log(ctx, R128_DEBUG1, "Preparing a blurred image for %s\n", src->filename);

  i->width = src->width;
  i->height = src->height;
  if(!(result = r128_alloc_for_conversion(ctx, i, src->filename, "blurring", &fd, &tempnam)))
    return i;

  r128_alloc_lines(ctx, i);

  assert((accu = (int*) malloc(sizeof(int) * src->width)));
  memset(accu, 0, sizeof(int) * src->width);
  
  for(y = 0; y<src->height; y++)
  {
    for(x = 0; x<src->width; x++)
    {
      if(y >= bh) accu[x] -= minus_line[x];
      accu[x] += plus_line[x];
      result[pix++] = accu[x] / bh;          
    }

    if(ctx->flags & R128_FL_MMAPALL)
    {
      if(write(fd, result, i->width) != i->width)
      {
        free(result); free(tempnam); close(fd);
        r128_log(ctx, R128_ERROR, "Cannot parse PGM file %s: Cannot write to temporary file %s to store blurring result, write(): %s.\n", src->filename, tempnam, strerror(errno));
        return i;
      }
      pix = 0;
    }

    if(y >= bh)
      minus_line += src->width; 
    plus_line += src->width;
  }


  free(accu);

  if(ctx->flags & R128_FL_MMAPALL)
    r128_mmap_converted(ctx, i, fd, tempnam);
  
  r128_log(ctx, R128_DEBUG2, "Blurred image prepared\n");
  return i;  
}


void r128_free_image(struct r128_ctx *c, struct r128_image *im)
{
  if(im->fd != -1) close(im->fd);
  r128_free(im->lines);
  
  if(im->mmaped && im->file)
    munmap(im->file, im->file_size);
  else
    r128_free(im->file);

  im->fd = -1;
  im->lines = NULL;
  im->file = NULL;
  im->gray_data = NULL;
}


char * r128_tempnam(struct r128_ctx *c)
{
  char *res;
  int maxname = 16, l = 0;

  if(c->temp_prefix)
    maxname += (l = strlen(c->temp_prefix));

  res = (char*) r128_malloc(c, maxname);
  if(c->temp_prefix)
    memcpy(res, c->temp_prefix, l);
  
  snprintf(res + l, maxname - l, "%08d.pgm", c->last_temp++);
  return res;
}

