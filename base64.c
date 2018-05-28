#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define DEBUG 1

uint32_t fsize(const char* filename)
{
    // Taken from https://stackoverflow.com/a/8384
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return -1;
}

uint32_t calc_required_buffer_size(uint32_t file_len)
{
    uint32_t add = 0;
    if (file_len % 3 == 0)
        add = 1;
    return (file_len + add) / 3 * 4;
}

char* base64_encode_block(char* block, uint32_t len)
{
    char* enc      = calloc(3, 1);
    char padding   = '=';
    char table[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
        'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
        'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', '+', '/'
    };

    uint32_t index;

    // Split the three bytes into 4 six-bit chunks
    // Take the first six bits of the first byte
    index  = (block[0] & 0xFC) >> 2;
    enc[0] = table[index];

    // Take the last two bits of the first byte
    // And the first four bits of the second byte
    index  = (block[0] & 0x03) << 4;
    index ^= (block[1] & 0xF0) >> 4;
    enc[1] = table[index];

    // Take the last four bits from the second byte
    // And the first two bits from the third byte
    index  = (block[1] & 0x0F) << 2;
    index ^= (block[2] & 0xC0) >> 6;
    enc[2] = table[index];

    // Take the last six bits off the third byte
    index  = (block[2] & 0x3F);
    enc[3] = table[index];

    // Add padding if neccessary
    if (len == 1)
        enc[2] = padding;
    if (len <= 2)
        enc[3] = padding;

    return enc;
}

char* base64_encode_file(const char* filename)
{
    FILE* file = fopen(filename, "r");
    uint32_t out_index = 0;
    char* output = malloc(
            calc_required_buffer_size(fsize(filename)));

    size_t len;
    do {
        char block[3] = {0};
        len = fread(block, 1, sizeof(block), file);
        if (!len) break;

        char* enc_block = base64_encode_block(block, len);
        if (DEBUG)
            printf("[%d] %02x %02x %02x => %02x %02x %02x %02x\n",
                    out_index, block[0] &0xFF, block[1] &0xFF, block[2] &0xFF,
                    enc_block[0] &0xFF, enc_block[1] &0xFF, enc_block[2] &0xFF, enc_block[3] &0xFF);

        memcpy(&output[out_index], enc_block, 4);
        out_index += 4;
    } while (len == 3);
    output[out_index] = '\0';
    return output;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("[*] No file!\n");
        exit(1);
    }
    char* output = base64_encode_file(argv[1]);
    printf("%s\n", output);

    free(output);
    return 0;
}

