#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
    if(argc != 3)
	return 0;

    char* fn = argv[1];
    FILE* f = fopen(fn, "rb");
    printf("unsigned char %s[] = {\n", argv[2]);
    unsigned long n = 0;

    while(!feof(f)) {
        unsigned char c;
        if(fread(&c, 1, 1, f) == 0) break;
        printf("0x%.2X, ", (int)c);
        ++n;
        if(n % 10 == 0) printf("\n");
    }

    fclose(f);
    printf("};\n");

    printf("int %s_size = %ld;\n", argv[2], n);
    return 0;
}
