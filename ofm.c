#include <stdio.h>
#include <stdlib.h>
#define TAG_SIZE 40000
#define TAG_TRIM 3

struct flvtag {
  unsigned int length;
  size_t size;
  unsigned char *data;
};

unsigned int readTime(unsigned char* buf)
{
  return (buf[7] << 24) | (buf[4] << 16) | (buf[5] << 8) | buf[6];
}

void writeTime(unsigned char* buf, unsigned int val)
{
  buf[4] = val >> 16;
  buf[5] = val >> 8;
  buf[6] = val;
  buf[7] = val >> 24;
  return;
}

int checkForTagMatch(struct flvtag* tag1, struct flvtag* tag2)
{
  if (tag1->data[0] != tag2->data[0])
    return 0;
  if (tag1->length != tag2->length)
    return 0;
  unsigned int i;
  for (i = 11; i < tag1->length + 11; i++)
    if (tag1->data[i] != tag2->data[i])
      return 0;
  return 1;
}

int main(int argc, char** argv)
{
  FILE *fd;
  unsigned char header[13];
  int i, ii, iii, headerWritten, searchForMatch;
  unsigned int timestampOffset = 0, length;
  struct flvtag *tag[TAG_TRIM + 2];

  /* allocate memory for tag pointers minus one */
  for (i = 0; i < TAG_TRIM + 1; i++)
  {
    tag[i] = malloc(sizeof(*tag[i]));
    if (tag[i] == NULL) 
    {
      fprintf(stderr, "out of memory\n");
      return 1;
    }
    tag[i]->data = malloc(TAG_SIZE);
    tag[i]->size = TAG_SIZE;
    if (tag[i]->data == NULL) 
    {
      fprintf(stderr, "out of memory\n");
      return 1;
    }
  }

  headerWritten = 0;

  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s file1 file2 [fileN]...\n", argv[0]);
    return 1;
  }

  for (i = 1; i < argc; i++)
  {
    fd = fopen(argv[i], "rb");

    if (!fd)
    {
      fprintf(stderr, "Failed to open file: %s\n", argv[i]);
      return 1;
    }

    if (fread(header, 1, 13, fd) != 13 || header[0] != 'F' || header[1] != 'L' || header[2] != 'V' || header[3] != 0x01)
    {
      fprintf(stderr, "%s is not a valid FLV\n", argv[i]);
      return 1;
    }

    if (!headerWritten)
    {
      fwrite(header, 1, 13, stdout);
      headerWritten = 1;
    }

    ii = 0;
    while (readTag(tag[0], fd))
    {
      if (searchForMatch) 
      {
        if (checkForTagMatch(tag[0], tag[TAG_TRIM]))
        {
          timestampOffset = readTime(tag[TAG_TRIM]->data) - readTime(tag[0]->data);
          searchForMatch = 0;
        }
      }
      else
      {
        if (ii == TAG_TRIM)
        {
          length = tag[TAG_TRIM]->length + 15;
          fwrite(tag[TAG_TRIM]->data, 1, length, stdout);
        }
        if (timestampOffset)
          writeTime(tag[0]->data, readTime(tag[0]->data) + timestampOffset);
        /* shift buffered tags */
        for (iii = TAG_TRIM + 1; iii > 0; iii--)
          tag[iii] = tag[iii - 1];
        /* recycle written tag */
        tag[0] = tag[TAG_TRIM + 1];
        if (ii < TAG_TRIM)
          ii++;
      }
    }
    if (searchForMatch)
    {
      fprintf(stderr, "no match found in file: %s\n", argv[i]);
    }
    else
    {
      length = tag[TAG_TRIM]->length + 15;
      fwrite(tag[TAG_TRIM]->data, 1, length, stdout);
    }
    searchForMatch = 1;

  }

  return 0;
}

int readTag(struct flvtag* tag, FILE* fd)
{
  if (fread(tag->data, 1, 11, fd) != 11)
  {
    fprintf(stderr, "\r\nReached end of file in tag header.\n");
    return 0;
  }
  tag->length = (tag->data[1] << 16) | (tag->data[2] << 8) | tag->data[3];
  if (tag->length + 15 > tag->size)
  {
    tag->data = realloc(tag->data, tag->length + 15);
    tag->size = tag->length + 15;
    if (tag->data == NULL)
    {
      fprintf(stderr, "Error reallocating memory.\n");
      exit (1);
    }
  }
  if (fread(tag->data + 11, 1, tag->length + 4, fd) != tag->length + 4)
  {
    fprintf(stderr, "\r\nReached end of file in tag body.\n");
    return 0;
  }
  return 1;
}


