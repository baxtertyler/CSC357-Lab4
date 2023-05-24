#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#pragma pack(push, 1)

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned char BYTE;

/* struct declarations */
typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
}fileHeader;

typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
}infoHeader;

int main(int argc, char *argv[]) {

    int row_size;
    float brightness;
    int parallel;
    int i, j;
    clock_t start, end;
    int pid;

    FILE *in; 
    fileHeader in_fh; 
    infoHeader in_ih; 
    BYTE* in_data;
    BYTE* pad;

    FILE *out;
    BYTE* out_data;

    in = fopen(argv[1], "rb");
    brightness = atof(argv[2]);
    parallel = atoi(argv[3]);
    out = fopen(argv[4], "wb");

/*
    in = fopen("flowers.bmp", "rb");
    brightness = 0.2;
    parallel = 1;
    out = fopen("outfile_lab4REG.bmp", "w");
*/

    fread(&in_fh.bfType, sizeof(WORD), 1, in);
    fread(&in_fh.bfSize, sizeof(DWORD), 1, in);
    fread(&in_fh.bfReserved1, sizeof(WORD), 1, in);
    fread(&in_fh.bfReserved2, sizeof(WORD), 1, in);
    fread(&in_fh.bfOffBits, sizeof(DWORD), 1, in);
    fread(&in_ih.biSize, sizeof(DWORD), 1, in);
    fread(&in_ih.biWidth, sizeof(LONG), 1, in);
    fread(&in_ih.biHeight, sizeof(LONG), 1, in);
    fread(&in_ih.biPlanes, sizeof(WORD), 1, in);
    fread(&in_ih.biBitCount, sizeof(WORD), 1, in);
    fread(&in_ih.biCompression, sizeof(DWORD), 1, in);
    fread(&in_ih.biSizeImage, sizeof(DWORD), 1, in);
    fread(&in_ih.biXPelsPerMeter, sizeof(LONG), 1, in);
    fread(&in_ih.biYPelsPerMeter, sizeof(LONG), 1, in);
    fread(&in_ih.biClrUsed, sizeof(DWORD), 1, in);
    fread(&in_ih.biClrImportant, sizeof(DWORD), 1, in);

    fwrite(&in_fh.bfType, sizeof(WORD), 1, out);
    fwrite(&in_fh.bfSize, sizeof(DWORD), 1, out);
    fwrite(&in_fh.bfReserved1, sizeof(WORD), 1, out);
    fwrite(&in_fh.bfReserved2, sizeof(WORD), 1, out);
    fwrite(&in_fh.bfOffBits, sizeof(DWORD), 1, out);
    fwrite(&in_ih.biSize, sizeof(DWORD), 1, out);
    fwrite(&in_ih.biWidth, sizeof(LONG), 1, out);
    fwrite(&in_ih.biHeight, sizeof(LONG), 1, out);
    fwrite(&in_ih.biPlanes, sizeof(WORD), 1, out);
    fwrite(&in_ih.biBitCount, sizeof(WORD), 1, out);
    fwrite(&in_ih.biCompression, sizeof(DWORD), 1, out);
    fwrite(&in_ih.biSizeImage, sizeof(DWORD), 1, out);
    fwrite(&in_ih.biXPelsPerMeter, sizeof(LONG), 1, out);
    fwrite(&in_ih.biYPelsPerMeter, sizeof(LONG), 1, out);
    fwrite(&in_ih.biClrUsed, sizeof(DWORD), 1, out);
    fwrite(&in_ih.biClrImportant, sizeof(DWORD), 1, out);

    row_size = ((in_ih.biWidth * in_ih.biBitCount + 31) / 32) * 4;

    in_data = mmap(NULL, row_size * in_ih.biHeight * 3 * sizeof(BYTE), 
        PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    if (in_data == (void*)-1) {
        printf("MMAP FAILED for INFILE DATA");
        exit(1);
    }
    out_data = mmap(NULL, row_size * in_ih.biHeight * 3 * sizeof(BYTE), 
        PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    if (out_data == (void*)-1) {
        printf("MMAP FAILED for OUTFILE DATA");
        exit(1);
    }
    pad = mmap(NULL, in_ih.biHeight * 4 * sizeof(BYTE) + 1, 
        PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    if (pad == (void*)-1) {
        printf("MMAP FAILED for IN/OUTFILE PADDING DATA");
        exit(1);
    }

    in_data = (unsigned char*)malloc(row_size * in_ih.biWidth * 3 * sizeof(BYTE));
    pad = (unsigned char*)malloc(4 * in_ih.biHeight + 1);

    for (i = 0; i < in_ih.biHeight * 3; i++) {
        for (j = 0; j < row_size; j++) {
            if (j < (in_ih.biWidth*3)) {
                fread(in_data + i * row_size + j, sizeof(BYTE), 1, in);
            } else {
                fread(&pad, sizeof(BYTE), 1, in);
            }
        }
    }

    if (parallel == 1) {
        start = clock();
        for (i = 0; i < (in_ih.biWidth * in_ih.biHeight * 3 * 3)+1; i++) {
            int temp = in_data[i] + (255 * brightness);
            if (temp > 255) {
                temp = 255;
            }
            out_data[i] = temp;
        }
        end = clock();
    }
    else {
        start = clock();
        pid = fork();
        if (pid == 0) {
            for (i = 0; i < ((in_ih.biWidth * in_ih.biHeight *3 * 3)+1)/2; i++) {
                int temp = in_data[i] + (255 * brightness);
                if (temp > 255) {
                    temp = 255;
                }
                out_data[i] = temp;
            }
        }
        else {
            for (i = ((in_ih.biWidth * in_ih.biHeight * 3 * 3)+1)/2; i < (in_ih.biWidth * in_ih.biHeight * 3 * 3)+1; i++) {
                int temp = in_data[i] + (255 * brightness);
                if (temp > 255) {
                    temp = 255;
                }
                out_data[i] = temp;
            }
            wait(0);
        }
        end = clock();
    }

    for (i = 0; i < in_ih.biHeight * 3; i++) {
        for (j = 0; j < row_size; j++) {
            if (j < (in_ih.biWidth*3)) {
                fwrite(out_data + i * row_size + j, sizeof(BYTE), 1, out);
            } else {
                fwrite(&pad, sizeof(BYTE), 1, out);
            }
        }
    }
    if ((end - start) > 0) {
        printf("time taken: %f\n", (double)(end - start));
    }

    fclose(in);
    fclose(out);
    munmap(in_data, row_size * in_ih.biHeight * 3 * sizeof(BYTE));
    munmap(out_data, row_size * in_ih.biHeight * 3 * sizeof(BYTE));
    munmap(pad, in_ih.biHeight * 4 * sizeof(BYTE) + 1);
    return 0;   
}
