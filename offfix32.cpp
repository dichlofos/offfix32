/**
 * offfix32
 * Version 0.1
 * Author: Michael aka mzv @ rutracker.org [ex- torrents.ru
 * Based on prefix32 tool written by ]DichlofoS[
 * aka Mikhail Veltishchev <dichlofos-mv@yandex.ru>
 * Support and special thanks to dmvn (dmvn@mccme.ru)
 * -----------------------------------------------------
 * A program for OFFset FIXation after coping Audio-CD
 * using wrong offset correction value
 * -----------------------------------------------------
 * This program is freeware and open-source, you may
 * freely redistribute or modify it, or use code in your
 * applications.
 * -----------------------------------------------------
 **/

#if _MSC_VER >= 1400
    // this option is for Safe STDLIB fopen_s, etc functions
    #define MSC_SAFECODE 1
#else
    #undef MSC_SAFECODE
#endif

#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_BUFFER_SIZE 1024

#define MAX_OFFSET 2000

// Function prototypes
bool CheckDWORD(unsigned int* nRIFFHeader, unsigned int nOffset, unsigned int nProperValue, const char* szMessage);
bool CheckRIFFHeader(unsigned int* nRIFFHeader);
void Shutdown(const char* szMessage, FILE* fI, FILE* fO);

// Checks that DWORD value at nOffset in RIFF header is equal to nProperValue
bool CheckDWORD(unsigned int* nRIFFHeader, unsigned int nOffset, unsigned int nProperValue, const char* szMessage) {
    if (nRIFFHeader[nOffset] != nProperValue) {
        printf("%s\n", szMessage);
        return false;
    }
    return true;
}

// Checks entire RIFF header
bool CheckRIFFHeader(unsigned int* nRIFFHeader) {
    if (!CheckDWORD(nRIFFHeader, 0x00, 0x46464952, "*** non-RIFF format!")) return false;
    if (!CheckDWORD(nRIFFHeader, 0x02, 0x45564157, "*** non-WAVE format!")) return false;
    if (!CheckDWORD(nRIFFHeader, 0x03, 0x20746d66, "*** cannot find format chunk!")) return false;
    if (!CheckDWORD(nRIFFHeader, 0x04, 0x00000010, "*** invalid format chunk size!")) return false;
    if (!CheckDWORD(nRIFFHeader, 0x05, 0x00020001, "*** invalid audio format!")) return false;
    if (!CheckDWORD(nRIFFHeader, 0x09, 0x61746164, "*** cannot find data chunk!")) return false;
    return true;
}

// Prints error message (if != NULL) and closes file handles fI, fO if necessary
void Shutdown(const char* szMessage, FILE* fI, FILE* fO) {
    if (szMessage)
        printf("%s\n", szMessage);

    if (fI) {
        fclose(fI);
        printf("input file closed ok\n");
    }
    if (fO) {
        fclose(fO);
        printf("output file closed ok\n");
    }
    if (szMessage)
        exit(-1);
}

int main(int argc, char** argv) {
    printf("offfix32 v0.1 (c) 2008, Michael aka mzv\n");
    printf("based on prefix32 v0.1 (c) 2007, ]DichlofoS[ Systems\n");

    FILE* fI = 0;
    FILE* fO = 0;
    if (argc < 3 || argc > 4)
        Shutdown("*** invalid cmdline arguments!\nsyntax: offfix32 source.wav destination.wav [+|-][offset]", fI, fO);

    const char* sSourceFileName = argv[1];
    const char* sDestFileName = argv[2];

    int nOffsetValue = MAX_OFFSET + 1; // invalid initial value

    if (argc == 4)
#ifdef MSC_SAFECODE
        sscanf_s(argv[3], "%d", &nOffsetValue);
#else
        sscanf(argv[3], "%d", &nOffsetValue);
#endif

    else
    {
        printf("please specify offset correction value [+|-][0..%d]: ", MAX_OFFSET);
#ifdef MSC_SAFECODE
        scanf_s("%d", &nOffsetValue);
#else
        scanf("%d", &nOffsetValue);
#endif
    }
    printf("using offset correction value: %d\n", nOffsetValue);
    if (nOffsetValue < -MAX_OFFSET || nOffsetValue > MAX_OFFSET) Shutdown("*** invalid offset value!", fI, fO);
    
#ifdef MSC_SAFECODE
    fopen_s(&fI, sSourceFileName, "rb");
#else
    fI = fopen(sSourceFileName, "rb");
#endif
    if (!fI)
        Shutdown("*** cannot open input file!", fI, fO);
    printf("input file opened ok\n");

    unsigned int nRIFFHeader[11];
    fread(nRIFFHeader, 11, 4, fI);

    // check header 
    if (!CheckRIFFHeader(nRIFFHeader))
        Shutdown("*** RIFF header check failed!", fI, fO);
    printf("RIFF header checked ok\n");
    unsigned int nDataSize = nRIFFHeader[10];

#ifdef MSC_SAFECODE
    fopen_s(&fO, sDestFileName, "wb");
#else
    fO = fopen(sDestFileName, "wb");
#endif
    if (!fO)
        Shutdown("*** cannot open output file!", fI, fO);
    printf("output file opened ok\n");

    // translate pregap length into bytes
    unsigned int nOffsetLength = abs(nOffsetValue) * 4;

    unsigned char chBuffer[4 * SAMPLE_BUFFER_SIZE];

    unsigned int nSampleCount = nDataSize >> 2;
    unsigned int nTargetDataSize = nDataSize;

    if (fwrite(nRIFFHeader, 4, 11, fO) != 11)
        Shutdown("*** cannot write RIFF header to output file!", fI, fO);

    if (nOffsetValue < 0) {
        nOffsetValue = -nOffsetValue;
        for (unsigned int i = 0; i < nOffsetLength; i++)
            if (fputc(0, fO) == EOF) Shutdown("*** cannot write offset silence to output file!", fI, fO);

        printf("please wait while writing output file...\n");
        printf("processing %d samples...\n", nSampleCount);

        unsigned int i = 0;
        while (i < nSampleCount - nOffsetValue) {
            unsigned int nCurrentBlockSize = nSampleCount - i - nOffsetValue;
            if (nCurrentBlockSize > SAMPLE_BUFFER_SIZE)
                nCurrentBlockSize = SAMPLE_BUFFER_SIZE;
            
            if (fread(chBuffer, 4, nCurrentBlockSize, fI) != nCurrentBlockSize)
                Shutdown("*** cannot read from input file!", fI, fO);

            if (fwrite(chBuffer, 4, nCurrentBlockSize, fO) != nCurrentBlockSize)
                Shutdown("*** cannot write to output file!", fI, fO);

            i += nCurrentBlockSize;
        }
    }
    else if (nOffsetValue > 0) {
        fseek(fI, nOffsetLength, SEEK_CUR);

        printf("please wait while writing output file...\n");
        printf("processing %d samples...\n", nSampleCount);

        unsigned int i = 0;
        while (i < nSampleCount - nOffsetValue) {
            unsigned int nCurrentBlockSize = nSampleCount - i - nOffsetValue;
            if (nCurrentBlockSize > SAMPLE_BUFFER_SIZE)
                nCurrentBlockSize = SAMPLE_BUFFER_SIZE;

            if (fread(chBuffer, 4, nCurrentBlockSize, fI) != nCurrentBlockSize)
                Shutdown("*** cannot read from input file!", fI, fO);

            if (fwrite(chBuffer, 4, nCurrentBlockSize, fO) != nCurrentBlockSize)
                Shutdown("*** cannot write to output file!", fI, fO);

            i += nCurrentBlockSize;
        }

        for (unsigned int i = 0; i < nOffsetLength; i++)
            if (fputc(0, fO) == EOF)
                Shutdown("*** cannot write offset silence to output file!", fI, fO);

    }
    else {
        printf("nothing to do, sorry...\n");
    }

    Shutdown(0, fI, fO);
    return 0;
}
