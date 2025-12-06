#include "file_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* ReadFileToString(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open file %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1); // File contetn + null terminator
    if (buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed for file %s\n", filename);
        fclose(fp);
        return NULL;
    }

    size_t readBytes = fread(buffer, 1, length, fp);
    if (readBytes != length)
    {
        fprintf(stderr, "Failed to read full file %s\n", filename);
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[length] = '\0';

    fclose(fp);
    return buffer;
}