/**
 * @file b64.c
 * @brief Base64 encoding and decoding
 * https://datatracker.ietf.org/doc/html/rfc4648
*/
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define ENOERR 0

/**
 * Base64 encode an input buffer to an output string buffer
 *  null-terminates the output string
 * @see https://datatracker.ietf.org/doc/html/rfc4648#section-4
 *
 * @param input
 * @param in_length
 * @param output
 * @param out_length
 * @param required : optional output parameter to get the required output buffer size
 * @return
 *     ENOERR on success
 *     ENOSPACE if the output buffer is too small
*/
static int b64_encode(const uint8_t* input, size_t in_length,
               char* output, size_t out_length,
               size_t *required)
{
    size_t output_size = (4 * ((in_length + 2) / 3)) + 1; // Calculate the size for the encoded output plus null char
    size_t i, j;
    const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int rc = ENOERR;

    if (out_length < output_size) {
        rc = ENOMEM;
        goto bail;
    }

    for (i = 0, j = 0; i < in_length; i += 3, j += 4) {
        uint8_t a = input[i];
        uint8_t b = (i + 1 < in_length) ? input[i + 1] : 0;
        uint8_t c = (i + 2 < in_length) ? input[i + 2] : 0;

        output[j] = base64_table[a >> 2];
        output[j + 1] = base64_table[((a & 0x03) << 4) | (b >> 4)];
        output[j + 2] = (i + 1 < in_length) ? base64_table[((b & 0x0F) << 2) | (c >> 6)] : '=';
        output[j + 3] = (i + 2 < in_length) ? base64_table[c & 0x3F] : '=';
    }

    output[output_size-1] = '\0'; // Null-terminate the output string
    rc = ENOERR;

 bail:
    if(NULL != required) {
        *required = output_size;
    }
    return rc;
}

static inline uint8_t b64_decode_bin(char c)
{
    // NOTE: This may be a bit inefficent, compared to a 256byte map.
    return (c >= 'A' && c <= 'Z') ?  (uint8_t)(c - 'A')
           : (c >= 'a' && c <= 'z') ? (uint8_t)(c - 'a' + 26)
           : (c >= '0' && c <= '9') ? (uint8_t)(c - '0' + 52)
           : (c == '+') ? (uint8_t)62
           : (c == '/') ? (uint8_t)63
           : (uint8_t)0;
}

/**
 * Base64 decode an input string buffer to an output buffer
 * @see https://datatracker.ietf.org/doc/html/rfc4648#section-4
 *
 * @param input : null-terminated input string
 * @param input_length : length of input string
 * @param output : output buffer
 * @param output_length : length of output buffer
 * @param required : optional output parameter to get the required output buffer size
 * @return
 *    ENOERR on success
 *    ENOMEM if the output buffer is too small
*/
static int b64_decode(const char* input, size_t input_length,
               uint8_t* output, size_t output_length,
               size_t* required)
{
    size_t i, j = 0;
    uint8_t a, b, c, d;
    int rc = ENOERR;
    size_t output_size = strlen(input) * 3 / 4; // Calculate the maximum size for the output buffer

    if (output_length < output_size) {
        rc = ENOMEM;
        goto bail;
    }

    for (i = 0; i < input_length; i += 4) {
        a = b64_decode_bin(input[i]);
        b = b64_decode_bin(input[i + 1]);
        c = b64_decode_bin(input[i + 2]);
        d = b64_decode_bin(input[i + 3]);

        output[j++] = (uint8_t)(a << 2) | (b >> 4);
        if (input[i + 2] != '=' && j < output_length)
            output[j++] = (uint8_t)(b << 4) | (c >> 2);
        if (input[i + 3] != '=' && j < output_length)
            output[j++] = (uint8_t)(c << 6) | d;
    }

    output_size = j;

 bail:
    if (NULL != required) {
        *required = output_size;
    }

    return rc;
}

#define BUILD_TEST
#ifdef BUILD_TEST
#include <stdio.h>

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

int main()
{
    struct {
        const uint8_t* data;
        const uint8_t* result;
        int expected_length;
    } t1[] = { // Test cases for decoding (base64 to binary)
        { (const uint8_t*)"",     (const uint8_t*)"", 0 },
        { (const uint8_t*)"AA==", (const uint8_t*)"\x00", 1 },
        { (const uint8_t*)"SA==", (const uint8_t*)"H", 1 },
        { (const uint8_t*)"AP8=", (const uint8_t*)"\x00\xFF", 2 },
        { (const uint8_t*)"SGVsbG8sIHdvcmxkIQ==", (const uint8_t*)"Hello, world!", 13 },
        { (const uint8_t*)"AP+AAQI=", (const uint8_t*)"\x00\xFF\200\x01\x02", 5 },
        { (const uint8_t*)"SGVsbG8=", (const uint8_t*)"Hello", 5 },
        { (uint8_t*)"TmV2ZXIgdGhhdCB3b3JrIQ==", (uint8_t*)"Never that work!", 16 }, // "Never that work!"
        { (const uint8_t*)"YmFzZTY0IGVuY29kaW5n", (const uint8_t*)"base64 encoding", 15 },
        { (const uint8_t*)"cXV4IHN0cmluZw==", (const uint8_t*)"qux string", 10 },
        { (const uint8_t*)"MTIzNDU2Nzg5MA==", (const uint8_t*)"1234567890", 10 },
    }, t2[] = {        // Test cases for encoding (binary to base64)
        {(const uint8_t*)"", (const uint8_t*)"", 1},
        {(const uint8_t*)"f", (const uint8_t*)"Zg==", 5},
        {(const uint8_t*)"fo", (const uint8_t*)"Zm8=", 5},
        {(const uint8_t*)"foo", (const uint8_t*)"Zm9v", 5},
        {(const uint8_t*)"foob", (const uint8_t*)"Zm9vYg==", 9},
        {(const uint8_t*)"fooba", (const uint8_t*)"Zm9vYmE=", 9},
        {(const uint8_t*)"foobar", (const uint8_t*)"Zm9vYmFy", 9},
        { (const uint8_t*)"Hello", (const uint8_t*)"SGVsbG8=", 9 },
        { (const uint8_t*)"base64 encoding", (const uint8_t*)"YmFzZTY0IGVuY29kaW5n", 21 },
        { (const uint8_t*)"qux string", (const uint8_t*)"cXV4IHN0cmluZw==", 17 },
        { (const uint8_t*)"1234567890", (const uint8_t*)"MTIzNDU2Nzg5MA==", 17 },
    };

    printf("Decode Test\n");
    for (int i = 0; i < NELEMS(t1); i++) {
        size_t req;
        uint8_t decoded_output[256]; // Adjust size as needed

        b64_decode((const char*)t1[i].data, strlen((const char*)t1[i].data),
                   decoded_output, sizeof(decoded_output), &req);

        printf("[%2d] %s\n", i, (req == t1[i].expected_length
               && 0 == memcmp(decoded_output, t1[i].result, t1[i].expected_length)) ? "PASS" : "FAIL");
    }

    printf("Encode Test\n");
    for (int i = 0; i < NELEMS(t2); i++) {
        size_t req;
        char encoded_output[256]; // Adjust size as needed

        b64_encode(t2[i].data, strlen((const char*)t2[i].data),
                   encoded_output, sizeof(encoded_output), &req);

        printf("[%2d] %s\n", i, (req == t2[i].expected_length
               && 0 == memcmp(encoded_output, t2[i].result, t2[i].expected_length)) ? "PASS" : "FAIL");
    }

#if 0
    for (int i = 0; i < sizeof(t1) / sizeof(t1[0]); i++) {
        uint8_t decoded_output[256]; // Adjust size as needed
        char encoded_output[256];   // Adjust size as needed
        size_t required;

        // Test decoding (base64 to binary)
        int decoding_result = b64_decode((const char*)t1[i].data, strlen((const char*)t1[i].data), decoded_output, sizeof(decoded_output), &required);

        if (decoding_result != 0) {
            printf("Test %d (Decoding) failed: Error code %d.\n", i + 1, decoding_result);
            continue;
        }

        if (required != t1[i].expected_length) {
            printf("Test %d (Decoding) failed: Decoded length mismatch. Expected %d, got %zu.\n", i + 1, t1[i].expected_length, required);
            continue;
        }

        if (memcmp(decoded_output, t1[i].result, t1[i].expected_length) != 0) {
            printf("Test %d (Decoding) failed: Decoded data mismatch.\n", i + 1);
            continue;
        }

        // Test encoding (binary to base64)
        int encoding_result = b64_encode(t1[i].result, t1[i].expected_length, encoded_output, sizeof(encoded_output), &required);

        if (encoding_result != 0) {
            printf("Test %d (Encoding) failed: Error code %d.\n", i + 1, encoding_result);
            continue;
        }

        if (required != strlen(encoded_output)) {
            printf("Test %d (Encoding) failed: Encoded length mismatch. Expected %zu, got %zu.\n", i + 1, strlen(encoded_output), required);
            continue;
        }

        if (strcmp(encoded_output, (const char*)t1[i].data) != 0) {
            printf("Test %d (Encoding) failed: Encoded data mismatch.\n", i + 1);
            continue;
        }

        printf("Test %d PASSED.\n", i + 1);
    }
#endif

    return 0;
}
#endif
