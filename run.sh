#!/bin/bash

gcc main.c -o main
./main $1 $2 $3 &
PID=$!

# Проверяем количество потоков
while kill -0 $PID 2> /dev/null; do
    # Считываем количество потоков
    THREADS=$(cat /proc/$PID/status | grep Threads | awk '{print $2}')
    
    # Выводим информацию, если количество потоков больше 1
    if [ $THREADS -gt 1 ]; then
        echo "Current number of threads: $THREADS"
    fi

    # Ждём
    sleep 1
done
