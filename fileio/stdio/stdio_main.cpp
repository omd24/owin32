/* ===========================================================
   #File: stdio_main.c #
   #Date: 15 March 2021 #
   #Revision: 1.0 #
   #Creator: Omid Miresmaeili #
   #Description: Standard I/O library sample #
   #Notice: (C) Copyright 2021 by Omid. All Rights Reserved. #
   =========================================================== */

#include <stdio.h>

struct Pirate {
    char name[50];
    unsigned long bounty;
    unsigned int crew_count;
};

int main (void) {
    char str[100];
    int c = 0;
    int res = 0;
    size_t n_read = 0;
    size_t n_written = 0;
    errno_t err;
    FILE * in = nullptr;
    FILE * out = nullptr;
    FILE * stream = nullptr;
    Pirate kaizokuO = {};
    Pirate kurohige = {.name = "Edward Teach", .bounty = 950, .crew_count = 48};

#pragma region fputc, fputs

// int fputc (int c, FILE * stream);
/*
    fputc() writes the byte specified by c (cast to an unsigned char) to the stream
*/
    fopen_s(&stream, "out", "w");
    if (stream) {
        if (fputc('T', stream) == EOF)
            printf("error fputc\n");
        fclose(stream);
    }
    // int fputs (char const * str, FILE * stream);
    /*
        fputs() writes all of the null-terminated string pointed at by str to the stream
    */
    fopen_s(&stream, "out", "a");
    if (stream) {
        if (fputs("he ship is made of wood.\n", stream) == EOF)
            printf("error fputs\n");
        fclose(stream);
    }

#pragma endregion

#pragma region fgetc, fgets
// int fgetc (FILE * stream);
/*
    fgetc reads the next character from stream and returns it as an unsigned char cast to an int.
*/
    fopen_s(&stream, "out", "r");
    if (stream) {
        c = fgetc(stream);
        if (c == EOF)
            printf("error fgetc\n");
        else
            printf("c = %c\n", (char)c);
        fclose(stream);
    }
// char * fgets (char * str, int size, FILE * stream);
/*
    fgets reads up to (size - 1) bytes from stream and stores the results in str.
*/
    fopen_s(&stream, "out", "r");
    if (stream) {
        if (!fgets(str, 100, stream))
            printf("error fgets\n");
        else
            printf("str = %s\n", str);
        fclose(stream);
    }
#pragma endregion

#pragma region fwrite
// size_t fwrite (void * buf, size_t size, size_t nr, FILE * stream);
/*
    fwrite() writes to stream up to nr elements, each size bytes in length,
    from the data pointed at by buf.

    Returns: The number of elements (not the number of bytes!) successfully written.
    A return value less than nr denotes error.
*/
    err = fopen_s(&out, "data", "w");
    if (err != 0) {
        perror("fopen");
        res = 1;
    }

    if (out) {
        n_written = fwrite(&kurohige, sizeof(Pirate), 1, out);
        if (0 == n_written) {
            perror ("fwrite");
            res = 1;
        }
        if (fclose(out)) {
            perror("fclose");
            res = 1;
        }
    }
#pragma endregion

#pragma region fread
// size_t fread (void * buf, size_t size, size_t nr, FILE *stream);
/*
    fread() reads up to nr elements of data, each of size bytes, from stream
    into the buffer pointed at by buf.
    The file pointer is advanced by the number of bytes read.

    Returns: The number of elements read (not the number of bytes read!).
    A return value less than nr denotes error.
*/
    err = fopen_s(&in, "data", "r");
    if (err != 0) {
        perror("fopen");
        res = 1;
    }
    if (in) {
        n_read = fread(&kaizokuO, sizeof(Pirate), 1, in);
        if (0 == n_read) {
            perror ("fread");
            res = 1;
        }
        if (fclose(in)) {
            perror("fclose");
            res = 1;
        }
    }
    printf(
        "Name = \"%s\",   Bounty = $%lu,   Crew size = %u members\n",
        kaizokuO.name, kaizokuO.bounty, kaizokuO.crew_count
    );

#pragma endregion

    return res;
}
