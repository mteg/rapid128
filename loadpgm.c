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
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <wait.h>
#include "rapid128.h"

unsigned char * skip_to_newline_a(unsigned char *ptr, int *len, char *copy_to, int copy_len)
{
  while((*len) > 0)
  {
    if(!isspace(*ptr)) break;
    ptr++; (*len)--;
  }

  while((*len) > 0)
  {
    if(isspace(*ptr)) break;
    if(copy_len > 0)
    {
      *(copy_to++) = *ptr;
      copy_len--;
    }
    ptr++; (*len)--;
  }
  
  if(copy_len) *copy_to = 0;
  
  
  return ptr;
}

#define skip_to_newline(ptr, len) skip_to_newline_a(ptr, len, NULL, 0)

unsigned char * skip_comments(unsigned char *ptr, int *len)
{
  if((*ptr) == '#')
  {
    while((*len) > 0)
    {
      ptr++;
      (*len)-- ;        
      if(ptr[-1] == '\n') break;
    }
  }

  while((*len) > 0)
  {
    if(!isspace(*ptr)) break;
    ptr++; (*len)--;
  }
  
  return ptr;
}


int r128_parse_pgm(struct r128_ctx *c, struct r128_image *im, char *filename)
{
#define PGM_MAX_LINE 256
#define PGM_MIN_HEADER 8
/*
P1 (3 bytes)
1 1 (4 bytes)
. (1 byte)
*/

  int type = 0, maxval = 255;
  char line[PGM_MAX_LINE];
  unsigned char *b = (unsigned char*) im->file;
  int len = im->file_size, rc = R128_EC_NOIMAGE, explen, as;
  
  if(!filename) filename = im->filename;

  if(len < PGM_MIN_HEADER)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: file too short.\n", filename);
  
  if(b[0] != 'P' && !isspace(b[2]))
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: file too short.\n", filename);
  
  type = b[1] - '0';
  if(type < 4 || type > 6)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: image type P%c is not supported.\n", filename, b[1]);
  
  b = skip_to_newline(b, &len);
  b = skip_comments(b, &len);
  b = skip_to_newline_a(b, &len, line, PGM_MAX_LINE - 1);
  if(!len) 
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header.\n", filename);
  
  line[PGM_MAX_LINE - 1] = 0;

  if(sscanf(line, "%d", &im->width) != 1)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header (unreadable width: %s).\n", filename, line);

  b = skip_comments(b, &len);
  b = skip_to_newline_a(b, &len, line, PGM_MAX_LINE - 1);

  if(sscanf(line, "%d", &im->height) != 1)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header (unreadable height: %s).\n", filename, line);
  
  if(im->width < 1 || im->height < 1 || im->width > 65535 || im->height > 65536)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: unsupported image size %d x %d.\n", filename, im->width, im->height);
  

  if(type > 4)
  {

    b = skip_comments(b, &len);
    b = skip_to_newline_a(b, &len, line, PGM_MAX_LINE - 1);
    

    if(!len) 
      return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid header.\n", filename);
      
    if(sscanf(line, "%d", &maxval) != 1)
      return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: invalid range.\n", filename);
    
    if(maxval < 1 || maxval > 255)
      return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: unsupported pixel value range %d.\n", filename, maxval);
  }
  
  explen = im->width * im->height;
  if(type == 4) explen = explen / 8;
  if(type == 6) explen *= 3;
  
  if(len < explen)
    return r128_log_return(c, R128_ERROR, rc, "Cannot parse PGM file %s: file is truncated (remaining %d, we need %d).\n", filename, len, explen);
    
  if(type == 4 || type == 6 || maxval != 255) /* Needs processing */
  {
    int pix, pixcnt = im->width * im->height;
    u_int8_t *data = NULL;
    u_int8_t *line = NULL;
    int fd, x = 0, cr = 0;
    char *tempnam;
    
    if(!(data = line = r128_alloc_for_conversion(c, im, im->filename, "conversion", &fd, &tempnam)))
      return R128_EC_NOIMAGE;

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
      if(pix >= pixcnt) break;
      switch(type)
      {
        case 4:
          if((pix & 7) == 0)
            cr = *(b++);
          line[x++] = (cr & 0x80) ? 0 : 255;
          cr <<= 1;
          break;
        case 5: line[x++] = (*(b++)) * 255 / maxval; break;
        case 6: line[x++] = (b[0] + b[1] + b[2]) / 3; b+= 3; break;
      }
    }
    r128_free_image(c, im);
    
    if(c->flags & R128_FL_MMAPALL)
      return r128_mmap_converted(c, im, fd, tempnam);
    else
    {
      im->file = (char*) data;
      im->gray_data = data;
    }
  }
  else
    im->gray_data = b;

  im->lines = (struct r128_line*) r128_malloc(c, as = sizeof(struct r128_line) * (im->height + im->width));
  memset(im->lines, 0xff, as);
  
  return r128_log_return(c, R128_DEBUG1, R128_EC_NOIMAGE - 1, "Successfuly parsed PGM file %s (%d x %d, P5, 255)\n", filename, im->width, im->height);
}

int r128_load_pgm(struct r128_ctx *c, struct r128_image *im, char *filename)
{
  int rc;
  
  r128_log(c, R128_DEBUG1, "Reading file %s.\n", filename);
  if(!strcmp(filename, "-"))
    im->fd = 0;
  else
    if((im->fd = open(filename, O_RDONLY)) == -1)
      return r128_log_return(c, R128_ERROR, R128_EC_NOFILE, "Cannot read file %s: open(): %s\n", filename, strerror(errno));
    
  if(c->flags & R128_FL_RAMALL)
  {
    int alloc_len = R128_READ_STEP;
    im->file = (char*) r128_malloc(c, alloc_len);
    
    while((rc = read(im->fd, im->file + im->file_size, alloc_len - im->file_size)) > 0)
    {
      im->file_size += rc;
      if(im->file_size == alloc_len)
      {
        alloc_len += R128_READ_STEP;
        im->file = (char*) r128_realloc(c, im->file, alloc_len);
      }
    }
    
    if(rc == -1)
    {
      close(im->fd); im->fd = -1;
      r128_free(im->file); im->file = NULL;
      r128_log(c, R128_ERROR, "Cannot load file %s: read(): %s\n", filename, strerror(errno));
      return R128_EC_NOFILE;
    }
    r128_log(c, R128_DEBUG1, "Successfully loaded file %s into RAM.\n", filename);
  }
  else
  {
    struct stat s;
    if(fstat(im->fd, &s) == -1)
    {
      close(im->fd); im->fd = -1;
      r128_log(c, R128_ERROR, "Cannot mmap file %s: stat(): %s\n", filename, strerror(errno));
      return R128_EC_NOFILE;
    }
    
    if((im->file = mmap(NULL, im->file_size = s.st_size, PROT_READ, MAP_SHARED, im->fd, 0)) == MAP_FAILED)
    {
      im->file = NULL;
      close(im->fd); im->fd = -1;
      r128_log(c, R128_ERROR, "Cannot mmap file %s: mmap(): %s\n", filename, strerror(errno));
      return R128_EC_NOFILE;
    }
    im->mmaped = 1;
    r128_log(c, R128_DEBUG1, "Successfully mmap()ed file %s.\n", filename); 
  }
  
  if((rc = r128_parse_pgm(c, im, filename)) >= R128_EC_NOIMAGE)
    r128_free_image(c, im);

  return rc;
}

int r128_load_file(struct r128_ctx *c, struct r128_image *im)
{
  unsigned int t_start;
  int childpid, maxfd;
  int sp_stdout[2];
  int sp_stderr[2];
  char *data;
  int alloc_size = R128_READ_STEP;
  static char stderr_buf[1024];
  char *tempnam = NULL;
  int fd = -1;
  
  if(!c->loader)
    return r128_load_pgm(c, im, im->filename);

  /* assume it'll be okay! */
  data = (char*) r128_malloc(c, alloc_size);
  if(c->flags & R128_FL_MMAPALL)
  {
    tempnam = r128_tempnam(c);

    if((fd = open(tempnam, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
    {
      free(tempnam);
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot load file %s: Cannot open temporary file %s to store loader result, open(): %s.\n", im->filename, tempnam, strerror(errno));
    }
  }
  
  if(socketpair(AF_LOCAL, SOCK_STREAM, 0, sp_stdout) == -1)
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot start loader for file %s: socketpair(): %s\n", im->filename, strerror(errno));

  if(socketpair(AF_LOCAL, SOCK_STREAM, 0, sp_stderr) == -1)
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot start loader for file %s: socketpair(): %s\n", im->filename, strerror(errno));
  
  t_start = r128_time(c);
  switch((childpid = fork()))
  {
    case -1:
      return r128_log_return(c, R128_ERROR, R128_EC_NOLOADER, "Cannot start loader for file %s: fork(): %s\n", im->filename, strerror(errno));

    case 0:
    {
      char *args[4] = {"/bin/sh", "-c", NULL, NULL};
      char *cmdstr, *argptr;
      int i, l, pos;
      
      close(sp_stdout[0]);
      close(sp_stderr[0]);
#warning OVERKILL - for mmap()ed loader results simply redirect to file
      dup2(sp_stdout[1], 1);
      dup2(sp_stderr[1], 2);
      
      cmdstr = r128_malloc(c, (pos = strlen(c->loader)) + (l = strlen(im->filename)) * 5 + 16);
      args[2] = cmdstr;

      memcpy(cmdstr, c->loader, pos); cmdstr[pos] = 0;
      if((argptr = strstr(cmdstr, "$$")))
        cmdstr = argptr;
      else
        cmdstr += pos;
      
      pos = sprintf(cmdstr, " '");
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
      if((argptr = strstr(c->loader, "$$")))
      {
        int xl;
        memcpy(cmdstr + pos, argptr + 2, xl = strlen(argptr + 2));
        pos += xl;
      }
      
      cmdstr[pos] = 0;
      
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

    FD_SET(sp_stderr[0], &fds);
    FD_SET(sp_stdout[0], &fds);

    if(select(maxfd + 1, &fds, NULL, NULL, &tv) < 0)
    {
      if(errno != EINTR)
      {
        r128_log(c, R128_ERROR, "Cannot proceed with loading file %s from loader: select(): %s\n", im->filename, strerror(errno));
        break;
      }
    }
    
    if(FD_ISSET(sp_stdout[0], &fds))
    {
      int l = read(sp_stdout[0], data + im->file_size, alloc_size - im->file_size);
      if(l < 0)
      {
        r128_log(c, R128_ERROR, "Cannot proceed with loading file %s from loader: read(stdout): %s\n", im->filename, strerror(errno));
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
              r128_log(c, R128_ERROR, "Cannot proceed with loading file %s from loader: cannot write to temporary file %s, write(): %s: %s\n", im->filename, tempnam, strerror(errno));
              break;
            }
            im->file_size = 0;
          }
          else
          {
            alloc_size += R128_READ_STEP;
            data = (char*) r128_realloc(c, data, alloc_size);
          }          
        }
      }
      else
        break; /* EOF! Great! */
    }
    
    if(FD_ISSET(sp_stderr[0], &fds))
    {
      int l = read(sp_stderr[0], stderr_buf, 1023);
      if(l < 0)
      {
        r128_log(c, R128_ERROR, "Cannot proceed with loading file %s from loader: read(stderr): %s\n", im->filename, strerror(errno));
        break;
      }
      else if(l > 0)
      {
        stderr_buf[l] = 0;
        r128_log(c, R128_NOTICE, "Loader for file %s says: %s\n",  im->filename, stderr_buf);
      }
      else
        break; /* EOF! */
    }
    
    if(c->loader_limit != 0.0)
      if((r128_time(c) - t_start) > c->loader_limit)
      {
        r128_log(c, R128_ERROR, "Terminating loader for file %s: time limit exceeded.\n", im->filename);
        break;
      }
  }

  close(sp_stderr[0]);
  close(sp_stdout[0]);

  /* Get rid of the loader process */
  kill(childpid, SIGKILL);
  while(waitpid(childpid, NULL, WNOHANG) <= 0)
    usleep(100000);

  if(c->flags & R128_FL_MMAPALL)
  {
    int rc;
    
    write(fd, data, im->file_size);
    im->file_size = 0;    
    free(data);
    close(fd);
    
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

