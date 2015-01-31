
struct r128_codetree_node
{
  int code;
  struct r128_codetree_node * zero;
  struct r128_codetree_node * one;
};


struct r128_line
{
  u_int8_t *gray_data;
  int linesize;
};

struct r128_image 
{
  u_int8_t *gray_data;
  struct r128_line *lines;
};

struct r128_ctx
{
  char *strategy;
  int flags;
#define R128_FL_READALL 1
#define R128_FL_NOCKSUM 2

  int logging_level;
  int page_scan_id;

  /* document size */
  int width, height;
  
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

  struct r128_image *im;
  struct r128_image *im_blurred;
  
  /* current scan info */
  int *line_scan_status;

};


#define R128_EC_SUCCESS 0
#define R128_EC_CHECKSUM 1 
#define R128_EC_SYNTAX 2
#define R128_EC_NOEND 3
#define R128_EC_NOCODE 4
#define R128_EC_NOLINE 5
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