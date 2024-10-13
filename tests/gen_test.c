#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int generate_input_file(const char *test_name, int num_lines, int line_length) {
    char input_filename[256];
    snprintf(input_filename, sizeof(input_filename), "tests/%s_inp.txt", test_name);

    FILE *input_file = fopen(input_filename, "w");
    if (input_file == NULL) {
        perror("Failed to create input file");
        return 1;
    }

    // Записываем количество строк и длину строк в первую строку
    fprintf(input_file, "%d %d\n", num_lines, line_length);

    // Генерируем и записываем строки с числами
    for (int i = 0; i < num_lines; ++i) {
        for (int j = 0; j < line_length; ++j) {
            fprintf(input_file, "%d ", rand() % 201 - 100); // Генерируем случайные числа от -100 до 100
        }
        fprintf(input_file, "\n");
    }

    fclose(input_file);

    return 0;
}

int generate_output_file(const char *test_name, int num_lines, int line_length) {
    char input_filename[256];
    snprintf(input_filename, sizeof(input_filename), "tests/%s_inp.txt", test_name);

    FILE *input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
    }

    // Пропускаем первую строку
    int dummy;
    fscanf(input_file, "%d %d", &dummy, &dummy);

    int sum = 0;
    for (int i = 0; i < num_lines; ++i) {
        for (int j = 0; j < line_length; ++j) {
            int num;
            fscanf(input_file, "%d", &num);
            sum += num;
        }
    }

    fclose(input_file);

    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "tests/%s_ans.txt", test_name);

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        perror("Failed to create output file");
        return 1;
    }

    // Записываем сумму всех чисел в файл
    fprintf(output_file, "%d\n", sum);

    fclose(output_file);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        perror("Usage: <test_name> <num_lines> <line_length>\n");
        return 1;
    }

    int num_lines = atoi(argv[2]);
    int line_length = atoi(argv[3]);

    if (
        generate_input_file(argv[1], num_lines, line_length) 
        ||
        generate_output_file(argv[1], num_lines, line_length)
    ) {
        return 1;
    }

    return 0;
}
