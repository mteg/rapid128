struct r128_line
{
  u_int32_t offset;
  u_int32_t linesize;
};

struct r128_image 
{
  struct r128_image *root;
  char *filename;
  int width, height, best_rc, fd;

  u_int8_t *bestcode;
#define R128_EXP_CODE_MAX 64
  u_int16_t bestcode_len, bestcode_alloc, mmaped;
  
  double time_spent;

#define R128_READ_STEP 262144
  char *file;
  int file_size;
  
  u_int8_t *gray_data;
  struct r128_line *lines;
};

struct r128_ctx
{
  char *strategy;
  int flags;
#define R128_FL_READALL 1
#define R128_FL_NOCKSUM 2
#define R128_FL_EREPORT 4
#define R128_FL_MMAPALL 8
#define R128_FL_RAMALL 16
#define R128_FL_NOCROP 32
#define R128_FL_KEEPTEMPS 64

  char *temp_prefix;

  struct timespec startup;
  
  int logging_level;
  int page_scan_id;

  int first_temp, last_temp;
  int n_images, n_codes_found;

  /* maximal height */
  int max_height, max_width, min_width;
  
  u_int8_t *codebuf;
  int codepos, codealloc;
  
  /* min/max code unit width to assume */
  double min_uwidth, max_uwidth;
  double min_doc_cuw, max_doc_cuw, doc_cuw_span;

  double threshold;
  int margin_low_threshold, margin_high_threshold;
  
  int min_len, max_gap;
  
  /* how far to go with resolution tests */
  double min_cuw_delta1, min_cuw_delta2;
  int wctrmax_stage1, wctrmax_stage2;
  
  /* how far to go with line distance in BFS scan */
  int expected_min_height, blurring_height;

  /* images */
  struct r128_image *im;
  struct r128_image *im_blurred;
  
  /* current scan info */
  int *line_scan_status;
  int line_scan_alloc;
  
  double batch_limit;
  int batch_size;
  double batch_start;
  
  char *loader;
  double loader_limit;
  
  double last_report;
};


#define R128_EC_SUCCESS 0
#define R128_EC_CHECKSUM 1 
#define R128_EC_SYNTAX 2
#define R128_EC_NOEND 3
#define R128_EC_NOCODE 4
#define R128_EC_NOLINE 5
#define R128_EC_NOIMAGE 6
#define R128_EC_NOFILE 7
#define R128_EC_NOLOADER 8
#define R128_EC_NOTHING 100

#define R128_ISDONE(ctx, rc) ((rc) == R128_EC_SUCCESS && (!((ctx)->flags & R128_FL_READALL)))

#define R128_ERROR 0
#define R128_WARNING 1
#define R128_NOTICE 2
#define R128_DEBUG1 3
#define R128_DEBUG2 4

/*       
000  000 w   0 
001  100 w 0.5
010  010 w 0.25
011  110 w 0.75
100  001 w 0.125
101  101 w 0.625
110  011 w 0.375
111  111 w 0.875
*/

static inline int minrc(int a, int b)
{
  if(a < b)
    return a;
  else
    return b;
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

#define r128_log(ctx, level, fmt...) r128_log_return(ctx, level, 0, ## fmt)

int r128_log_return(struct r128_ctx *ctx, int level, int rc, char *fmt, ...);
void r128_fail(struct r128_ctx *ctx, char *fmt, ...);
double r128_time(struct r128_ctx *ctx);
void *r128_malloc(struct r128_ctx *ctx, int size);
void r128_free(void *p);
void *r128_realloc(struct r128_ctx *ctx, void *ptr, int size);
void *r128_zalloc(struct r128_ctx *ctx, int size);
int r128_report_code(struct r128_ctx *ctx, struct r128_image *img, char *code, int len);
const char * r128_strerror(int rc);
int r128_cksum(u_int8_t *symbols, int len);

int r128_parse(struct r128_ctx *ctx, struct r128_image *img, u_int8_t *symbols, int len);
void r128_update_best_code(struct r128_ctx *ctx, struct r128_image *im, u_int8_t *symbols, int len);
int r128_read_code(struct r128_ctx *ctx, struct r128_image *img, u_int8_t *line, int w, double ppos, double uwidth, double threshold);
int r128_scan_line(struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, double uwidth, double offset, double threshold);

int r128_parse_pgm(struct r128_ctx *c, struct r128_image *im, char *filename);
int r128_load_pgm(struct r128_ctx *c, struct r128_image *im, char *filename);
int r128_load_file(struct r128_ctx *c, struct r128_image *im);

void r128_realloc_buffers(struct r128_ctx *c);
int r128_page_scan(struct r128_ctx *ctx, struct r128_image *img, double offset, double uwidth, int minheight, int maxheight);
int r128_try_tactics(struct r128_ctx *ctx, char *tactics, int start, int len, int codes_to_find);
int r128_run_strategy(struct r128_ctx *ctx, char *strategy, int start, int len);

struct r128_line *r128_get_line(struct r128_ctx *ctx, struct r128_image *im, int line);
void r128_alloc_lines(struct r128_ctx *ctx, struct r128_image *i);
int r128_mmap_converted(struct r128_ctx *c, struct r128_image *im, int fd, char *tempnam);
u_int8_t * r128_alloc_for_conversion(struct r128_ctx *c, struct r128_image *im, char *filename, char *operation, int *fd_store, char **tempnam_store);
struct r128_image * r128_blur_image(struct r128_ctx * ctx, int n);
void r128_free_image(struct r128_ctx *c, struct r128_image *im);
char * r128_tempnam(struct r128_ctx *c);
void r128_defaults(struct r128_ctx *c);
