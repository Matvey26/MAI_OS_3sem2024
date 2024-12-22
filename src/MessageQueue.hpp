#pragma once

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <cstring>
#include <string>
#include <stdexcept>

const size_t BUFFER_SIZE = 1024;

struct RingBuffer {
    char buffer[BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t size;
};

class MessageQueue {
public:
    MessageQueue(const std::string& name) {
        shm_name = "/shm_" + name;
        mutex_name = "/mtx_" + name;
        items_name = "/items_" + name;
        spaces_name = "/spaces_" + name;

        try {
            initialize(false); // Попытка открыть существующую очередь
        } catch (const std::exception& ex) {
            initialize(true); // Если не удалось, создаём новую очередь
        }
    }

    ~MessageQueue() {
        munmap(buffer, sizeof(RingBuffer));
        close(shm_fd);
        sem_close(mutex);
        sem_close(items);
        sem_close(spaces);
    }

    bool push(const std::string& message) {
        size_t len = message.length();
        if (len == 0 || len >= BUFFER_SIZE) {
            error_message = "Invalid message size";
            return false;
        }

        for (size_t i = 0; i < len; ++i) {
            sem_wait(spaces); // Wait for free space
        }

        sem_wait(mutex); // Lock access to the buffer
        for (size_t i = 0; i < len; ++i) {
            buffer->buffer[buffer->head] = message[i];
            buffer->head = (buffer->head + 1) % BUFFER_SIZE;
            buffer->size++;
        }
        sem_post(mutex); // Unlock buffer
        for (size_t i = 0; i < len; ++i) {
            sem_post(items); // Increment item count
        }
        return true;
    }

    std::string pop() {
        sem_wait(items); // Wait for an item

        sem_wait(mutex); // Lock access to the buffer
        std::string message;
        while (buffer->size > 0 && buffer->buffer[buffer->tail] != '\0') {
            message += buffer->buffer[buffer->tail];
            buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
            buffer->size--;
        }
        sem_post(mutex); // Unlock buffer

        sem_post(spaces); // Increment free space count
        return message;
    }

    bool empty() {
        sem_wait(mutex);
        bool is_empty = (buffer->size == 0);
        sem_post(mutex);
        return is_empty;
    }

    std::string get_error_message() const {
        return error_message;
    }

private:
    int shm_fd;
    RingBuffer* buffer;
    sem_t* mutex;   // Protects shared buffer
    sem_t* items;   // Tracks available items
    sem_t* spaces;  // Tracks available space
    std::string error_message;
    std::string shm_name;
    std::string mutex_name;
    std::string items_name;
    std::string spaces_name;

    void initialize(bool create) {
        if (create) {
            shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1) {
                throw std::runtime_error("Failed to create shared memory");
            }

            if (ftruncate(shm_fd, sizeof(RingBuffer)) == -1) {
                throw std::runtime_error("Failed to set size of shared memory");
            }

            buffer = static_cast<RingBuffer*>(mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
            if (buffer == MAP_FAILED) {
                throw std::runtime_error("Failed to map shared memory");
            }

            buffer->head = 0;
            buffer->tail = 0;
            buffer->size = 0;

            sem_unlink(mutex_name.c_str());
            sem_unlink(items_name.c_str());
            sem_unlink(spaces_name.c_str());

            mutex = sem_open(mutex_name.c_str(), O_CREAT, 0666, 1);
            items = sem_open(items_name.c_str(), O_CREAT, 0666, 0);
            spaces = sem_open(spaces_name.c_str(), O_CREAT, 0666, BUFFER_SIZE);

            if (mutex == SEM_FAILED || items == SEM_FAILED || spaces == SEM_FAILED) {
                throw std::runtime_error("Failed to create semaphores");
            }
        } else {
            shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
            if (shm_fd == -1) {
                throw std::runtime_error("Failed to open shared memory");
            }

            buffer = static_cast<RingBuffer*>(mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
            if (buffer == MAP_FAILED) {
                throw std::runtime_error("Failed to map shared memory");
            }

            mutex = sem_open(mutex_name.c_str(), 0);
            items = sem_open(items_name.c_str(), 0);
            spaces = sem_open(spaces_name.c_str(), 0);

            if (mutex == SEM_FAILED || items == SEM_FAILED || spaces == SEM_FAILED) {
                throw std::runtime_error("Failed to open semaphores");
            }
        }
    }
};
