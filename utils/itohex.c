#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <infile> [<outfile>]\n", argv[0]);
        return 1;
    }

    char* infile = argv[1];
    FILE *inptr;
    if ((inptr = fopen(infile, "r")) == NULL) {
        printf("Could not open %s. Aborting.\n", infile);
    }

    FILE *outptr = NULL;
    if (argc == 3) {
        char* outfile = argv[2];
        if ((outptr = fopen(outfile, "w")) == NULL) {
            printf("Could not open %s. Aborting.\n", outfile);
        }
    }

    printf("Begin processing...\n");
    int i;
    while (fscanf(inptr, "%d", &i) == 1) {
       if (outptr != NULL) {
           fprintf(outptr, "%02x", i);
       }
       printf("%02x", i);
    }
    printf("\n");
    
    printf("Processing complete.\n");
}
