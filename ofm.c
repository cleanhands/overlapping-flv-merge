#include <stdio.h>
#include <stdlib.h>
#define TAG_SIZE 16024000

unsigned int readBodyLength(unsigned char* buf)
{
  return (buf[1] << 16) | (buf[2] << 8) | buf[3]; 
}

unsigned int readTime(unsigned char* buf)
{
  return (buf[7] << 24) | (buf[4] << 16) | (buf[5] << 8) | buf[6];
}

void writeTime(unsigned char* buf, int val)
{
  buf[4] = val >> 16;
  buf[5] = val >> 8;
  buf[6] = val;
  buf[7] = val >> 24;
  return;
}

int checkForTagMatch(unsigned char* buf1, unsigned char* buf2)
{
  if (buf1[0] != buf2[0])
    return 0;
  if (readBodyLength(buf1) != readBodyLength(buf2))
    return 0;
  unsigned int i;
  for (i = 11; i < readBodyLength(buf1) + 11; i++)
    if (buf1[i] != buf2[i])
      return 0;
  return 1;
}

int main(int argc, char** argv)
{
  FILE *fd;
  unsigned char header[13];
  int i, ii, headerWritten, searchForMatch;
  unsigned int timestampOffset = 0, length;
  unsigned char *ptr1, *ptr2, *ptr3, *ptr4, *tmpptr;
  ptr1 = (unsigned char*) malloc(TAG_SIZE);
  ptr2 = (unsigned char*) malloc(TAG_SIZE);
  ptr3 = (unsigned char*) malloc(TAG_SIZE);
  ptr4 = (unsigned char*) malloc(TAG_SIZE);

  headerWritten = 0;

  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s file1 file2 [fileN]...\n", argv[0]);
    return 0;
  }

  for (i = 1; i < argc; i++)
  {
    fd = fopen(argv[i], "rb");

    if (!fd)
    {
      fprintf(stderr, "Failed to open file: %s\n", argv[i]);
      return 0;
    }

    if (fread(header, 1, 13, fd) != 13 || header[0] != 'F' || header[1] != 'L' || header[2] != 'V' || header[3] != 0x01)
    {
      fprintf(stderr, "%s is not a valid FLV\n", argv[i]);
      return 0;
    }

    if (!headerWritten)
    {
      fwrite(header, 1, 13, stdout);
      headerWritten = 1;
    }

    ii = 1;
    while (readTag(ptr1, fd))
    {
      if (searchForMatch) 
      {
        if (checkForTagMatch(ptr1, ptr4))
        {
          timestampOffset = readTime(ptr4) - readTime(ptr1);
          searchForMatch = 0;
        }
      }
      else
      {
        if (ii == 4)
        {
          length = readBodyLength(ptr4) + 15;
          fwrite(ptr4, 1, length, stdout);
        }
        if (timestampOffset)
          writeTime(ptr1, readTime(ptr1) + timestampOffset);
        tmpptr = ptr4;
        ptr4 = ptr3;
        ptr3 = ptr2;
        ptr2 = ptr1;
        ptr1 = tmpptr;
        if (ii < 4)
          ii++;
      }
    }
    if (searchForMatch)
    {
      fprintf(stderr, "no match found in file: %s\n", argv[i]);
    }
    searchForMatch = 1;

  }

}

int readTag(unsigned char* tag, FILE* fd)
{
  unsigned int bodyLength;
  if (fread(tag, 1, 11, fd) != 11)
  {
    fprintf(stderr, "\r\nReached end of file in tag header.\n");
    return 0;
  }
  bodyLength = readBodyLength(tag);
  if (bodyLength + 15 > TAG_SIZE)
  {
    fprintf(stderr, "Maximum tag size exceeded. Size: %d Max:%d\n", bodyLength + 15, TAG_SIZE);
    return 0;
  }
  if (fread(tag + 11, 1, bodyLength + 4, fd) != bodyLength + 4)
  {
    fprintf(stderr, "\r\nReached end of file in tag body.\n");
    return 0;
  }
  return 1;
}


