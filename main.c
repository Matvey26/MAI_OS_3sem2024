#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

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

int get_element_from_array(int* arr, size_t i) {
    // return arr[i];
    return rand() % 201 - 100;
}

void* sum_array(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int local_sum = 0;

    for (size_t i = 0; i < data->size; ++i) {
        local_sum += get_element_from_array(data->array, i);
    }

    pthread_mutex_lock(data->mutex);
    *(data->result) += local_sum;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <number of arrays> <length of arrays> <number of threads>\n", argv[0]);
        return 1;
    }

    size_t threads_numbers = atoi(argv[3]);

    size_t k = atoll(argv[1]);
    size_t m = atoll(argv[2]);

    int result = 0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    size_t num_threads = (k < threads_numbers) ? k : threads_numbers;
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    ThreadData* th_data = (ThreadData*)malloc(sizeof(ThreadData) * num_threads);

    size_t cur_array = 0;
    while (cur_array < k) {
        size_t i = 0;
        for (; i < num_threads && cur_array < k; ++i, ++cur_array) {
            th_data[i].array = NULL;
            th_data[i].size = m;
            th_data[i].result = &result;
            th_data[i].mutex = &mutex;
            if (pthread_create(&threads[i], NULL, sum_array, &th_data[i]) != 0) {
                perror("Error creating thread");
                return 1;
            }
        }

        // Ожидание завершения всех созданных потоков
        for (size_t j = 0; j < i; ++j) {
            pthread_join(threads[j], NULL);
        }
    }

    printf("%d", result);

    pthread_mutex_destroy(&mutex);
    free(th_data);
    free(threads);

    return 0;
}
