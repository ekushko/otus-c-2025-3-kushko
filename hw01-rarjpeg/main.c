#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

char
is_jpeg(const char* file_path) {

    const char jpeg_signature[] = { 0xFF, 0xD8 };

    FILE* fp = fopen(file_path, "rb");

    if (fp == NULL) {
        perror(file_path);
    } else {
        char signature[2];

        if (fread(&signature, sizeof(signature), 1, fp) > 0) {
            if (signature[0] == jpeg_signature[0]
                && signature[1] == jpeg_signature[1]) {
                return 1;
            }
        }
    }

    return 0;
}

void
print_zip_content(const char* file_path) {
    const char lfh_signature[] = { 0x50, 0x4B, 0x03, 0x04 };
    const int file_name_length_offset = 21;
    const int file_name_offset = 2;

    FILE* fp = fopen(file_path, "rb");

    if (fp == NULL) {
        perror(file_path);
    } else {
        int ch = 0;
        size_t lfh_signature_counter = 0;
        char is_zip_content_found = 0;

        while ((ch = getc(fp)) != EOF) {
            if (lfh_signature_counter == sizeof(lfh_signature) / sizeof(char)) {
                if (is_zip_content_found == 0) {
                    printf("File %s is JPEG and it contains ZIP: ", file_path);
                    is_zip_content_found = 1;
                }

                uint16_t file_name_length;

                if (fseek(fp, file_name_length_offset, SEEK_CUR) == 0) {
                    if (fread(&file_name_length, sizeof(file_name_length), 1, fp) > 0) {
                        char* file_name = malloc(file_name_length + 1);
                        memset(file_name + file_name_length, '\0', 1);

                        if (fseek(fp, file_name_offset, SEEK_CUR) == 0) {
                            if (fread(file_name, file_name_length, 1, fp) > 0) {
                                printf("%s ", file_name);
                            }
                        }

                        free(file_name);
                    }
                }
            }

            if (ch == lfh_signature[lfh_signature_counter]) {
                ++lfh_signature_counter;
            } else {
                lfh_signature_counter = 0;
            }
        }

        if (is_zip_content_found == 0) {
            printf("File %s is JPEG but it doesn't contain ZIP!\n", file_path);
        } else {
            printf("\n");
        }

        fclose(fp);
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("File list is empty!\n");
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        const char* file_path = argv[i];

        if (is_jpeg(file_path) > 0)
            print_zip_content(file_path);
        else
            printf("File %s isn't JPEG\n", file_path);
    }

    return 0;
}
