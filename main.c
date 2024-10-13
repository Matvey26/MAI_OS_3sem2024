#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

int** read_data(const char* path, size_t* k, size_t* m) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fscanf(file, "%lu %lu", k, m);
    int** arrays = (int**)malloc(sizeof(int*) * (*k));
    if (arrays == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }

    for (size_t i = 0; i < *k; ++i) {
        arrays[i] = (int*)malloc(sizeof(int) * (*m));
        if (arrays[i] == NULL) {
            perror("Memory allocation error");
            fclose(file);
            return NULL;
        }

        for (size_t j = 0; j < *m; ++j) {
            fscanf(file, "%d", &arrays[i][j]);
        }
    }

    fclose(file);
    return arrays;
}

typedef struct {
    int* array;
    size_t size;
    int* result;
    pthread_mutex_t* mutex;
} ThreadData;

void* sum_array(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int local_sum = 0;

    for (size_t i = 0; i < data->size; ++i) {
        local_sum += data->array[i];
    }

    pthread_mutex_lock(data->mutex);
    *(data->result) += local_sum;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <test name> <number of threads>\n", argv[0]);
        return 1;
    }

    char input_filename[256], output_filename[256];
    snprintf(input_filename, sizeof(input_filename), "tests/%s_inp.txt", argv[1]);
    snprintf(output_filename, sizeof(output_filename), "tests/%s_out.txt", argv[1]);

    size_t threads_numbers = atoi(argv[2]);

    size_t k, m;
    int** data = read_data(input_filename, &k, &m);
    if (data == NULL) return 1;

    int result = 0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    size_t num_threads = (k < threads_numbers) ? k : threads_numbers;
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);`
    ThreadData* th_data = (ThreadData*)malloc(sizeof(ThreadData) * num_threads);

    size_t cur_array = 0;
    while (cur_array < k) {
        for (size_t i = 0; i < num_threads && cur_array < k; ++i, ++cur_array) {
            th_data[i].array = data[cur_array];
            th_data[i].size = m;
            th_data[i].result = &result;
            th_data[i].mutex = &mutex;
            if (pthread_create(&threads[i], NULL, sum_array, &th_data[i]) != 0) {
                perror("Error creating thread");
                return 1;
            }
        }

        for (size_t i = 0; i < num_threads; ++i) {
            pthread_join(threads[i], NULL);
        }
    }

    FILE* output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return 1;
    }
    fprintf(output_file, "%d", result);
    fclose(output_file);

    pthread_mutex_destroy(&mutex);
    for (size_t i = 0; i < k; ++i) {
        free(data[i]);
    }
    free(data);
    free(th_data);
    free(threads);

    return 0;
}
