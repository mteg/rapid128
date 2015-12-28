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
#include "rapid128.h"

int r128_log_return(struct r128_ctx *ctx, int level, int rc, char *fmt, ...)
{
  unsigned int t, lrd;
  va_list ap;
  if(level > ctx->logging_level) return rc;
  
  t = r128_time(ctx); lrd = t - ctx->last_report;
  fprintf(stderr, "[%2d.%03d] (+%d.%03d) ", t / 1000, t % 1000, lrd / 1000, lrd % 1000);
  ctx->last_report = t;
  
  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  
  return rc;
}


void r128_fail(struct r128_ctx *ctx, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  
  exit(1);
}

unsigned int r128_time(struct r128_ctx *ctx)
{
  struct timespec ts;
  int res;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  res  = ts.tv_nsec - ctx->startup.tv_nsec;
  res /= 1000000;
  res += (ts.tv_sec - ctx->startup.tv_sec) * 1000;
  return res;
}

void *r128_malloc(struct r128_ctx *ctx, int size)
{
  void *ret;
  assert((ret = malloc(size)));
  return ret;
}

void r128_free(void *p)
{
  free(p);
}

void *r128_realloc(struct r128_ctx *ctx, void *ptr, int size)
{
  void *ret;
  assert((ret = realloc(ptr, size)));
  return ret;
}

void *r128_zalloc(struct r128_ctx *ctx, int size)
{
  void *ret = r128_malloc(ctx, size);
  memset(ret, 0, size);
  return ret;
}

int r128_report_code(struct r128_ctx *ctx, struct r128_image *img, char *code, int len)
{
  if(code) img->best_rc = R128_EC_SUCCESS;
  if(ctx->flags & R128_FL_EREPORT)
  {
    int i;
    printf("%s:%d.%03d:%s:%s", img->root ? img->root->filename : img->filename,
      img->time_spent / 1000, img->time_spent % 1000, code ? ctx->tactics : "", r128_strerror(img->best_rc));
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

