#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

void print_usage_and_exit(
    const char* exe)
{
    printf("Usage:\n%s input_file output_file variable_name\n", exe);
    exit(-1);
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        print_usage_and_exit(argv[0]);
    }

    const char* inputFilePath = argv[1];
    const char* outputFilePath = argv[2];
    const char* variableName = argv[3];

    FILE* inputFile = fopen(inputFilePath, "rb");
    if (inputFile == NULL)
    {
        printf("Failed to open input file '%s'\n", inputFilePath);
        exit(1);
    }

    fseek(inputFile, 0, SEEK_END);
    const size_t inputSize = ftell(inputFile);
    uint8_t* inputData = malloc(inputSize);
    fseek(inputFile, 0, SEEK_SET);
    fread(inputData, 1, inputSize, inputFile);
    fclose(inputFile);

    FILE* outputFile = fopen(outputFilePath, "w");
    if (outputFile == NULL)
    {
        printf("Failed to open output file '%s'\n", outputFilePath);
        exit(1);
    }
    char header[256];
    sprintf(header, "static const unsigned char %s[] = {", variableName);
    fwrite(header, 1, strlen(header), outputFile);

    char hexMap[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
    char hex[5];
    hex[0] = '0';
    hex[1] = 'x';
    hex[2] = '0';
    hex[3] = '0';
    hex[4] = ',';
    for (size_t i = 0; i < inputSize; ++i)
    {
        hex[2] = hexMap[inputData[i] >> 4];
        hex[3] = hexMap[inputData[i] & 0xf];   
        fwrite(hex, 1, sizeof(hex), outputFile);
    }

    if (argc == 5)
    {
        hex[2] = '0';
        hex[3] = '0';
        fwrite(hex, 1, sizeof(hex), outputFile);
    }

    const char* footer = "};\n";
    fwrite(footer, 1, strlen(footer), outputFile);
    fclose(outputFile);

    return 0;
}