#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

int** read_data(const char* path, size_t* k, size_t* m) {
    FILE* file = fopen(path, "r");

    // Считываем количество массивов и количество чисел в массиве
    fscanf(file, "%lu %lu", k, m);

    int** arrays = (int**)malloc(sizeof(int*) * (*k));
    for (size_t i = 0; i < *k; ++i) {
        int buf;
        arrays[i] = (int*)malloc(sizeof(int) * (*m));
        for (size_t j = 0; j < *m; ++j) {
            fscanf(file, "%d", &buf);
            arrays[i][j] = buf;
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

    // Суммируем массив
    for (size_t i = 0; i < data->size; ++i) {
        local_sum += data->array[i];
    }

    pthread_mutex_lock(data->mutex);
    *(data->result) += local_sum;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}


int main(int argc, char* argv[]) {
    // Из аргументов получаем количество потоков
    if (argc < 2) {
        perror("Usage: [number of threads]\n");
        return 1;
    }
    size_t threads_numbers;
    sscanf(argv[1], "%lu", &threads_numbers);

    // Читаем массивы из файла
    size_t k, m;
    int** data = read_data("in.txt", &k, &m);

    // Выводим данные
    for (size_t i = 0; i < k; ++i) {
        for (size_t j = 0; j < m; ++j) {
            printf("%d ", data[i][j]);
        }
        printf("\n");
    }

    // Результат сложения всех массивов
    int result = 0;

    // Создаём мьютекс
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    size_t rest_arrays = k;
    size_t cur_array = 0;
    while (rest_arrays > 0) {
        // Создаём и запускаем потоки
        ThreadData* th_data = (ThreadData*)malloc(sizeof(ThreadData) * threads_numbers);
        pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t));
        for (size_t i = 0; i < threads_numbers; ++i) {
            th_data[i].array = (cur_array < k ? data[cur_array] : NULL);
            th_data[i].size = (cur_array < k ? m : 0);
            th_data[i].result = &result;
            th_data[i].mutex = &mutex;
            pthread_create(&threads[i], NULL, sum_array, (void*)(&th_data[i]));
            if (rest_arrays > 0) {
                --rest_arrays;
                ++cur_array;
            }
        }

        // Ждём завершения потоков
        for (size_t i = 0; i < threads_numbers; ++i) {
            pthread_join(threads[i], NULL);
        }
    }

    // Выводим результат
    printf("Sum of all arrays: %d\n", result);

    return 0;
}