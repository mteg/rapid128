
struct r128_codetree_node
{
  int code;
  struct r128_codetree_node * zero;
  struct r128_codetree_node * one;
};

struct r128_queue_entry
{
  /* (inclusive) */
  int line_from;
  int line_to;
};

struct r128_image 
{
  u_int8_t *gray_data;

  u_int8_t **line;
  int *linesize;
};

struct r128_ctx
{
  int flags;
#define R128_FL_READALL 1
#define R128_FL_NOCKSUM 2

  /* document size */
  int width, height;
  
  /* min/max code unit width to assume */
  double min_uwidth, max_uwidth, min_doc_cuw, max_doc_cuw;
  
  /* how far to go with resolution tests */
  int x_depth1, x_depth2;
  
  /* how far to go with line distance in BFS scan */
  int y_depth1, y_depth2;

  struct r128_image *im;
  struct r128_image *im_blurred;
  
  /* current scan info */
  u_int8_t *line_done;
  
  
  /* line queue */
  struct r128_queue_entry *bfs_queue;
  int q_head, q_len;
  
  struct r128_queue_entry *
  
};


#define R128_EC_SUCCESS 0
#define R128_EC_CHECKSUM 1 
#define R128_EC_SYNTAX 2
#define R128_EC_NOEND 3
#define R128_EC_NOCODE 4

#define R128_ISDONE(ctx, rc) ((rc) == R128_EC_SUCCESS && !((ctx)->flags & R128_FL_READALL))
     248
       
000  000 w   0 
001  100 w 0.5
010  010 w 0.25
011  110 w 0.75
100  001 w 0.125
101  101 w 0.625
110  011 w 0.375
111  111 w 0.875
