#!/bin/bash

gcc main.c -o main

# Функция, чтобы замерять время исполнения программы для разного числа потоков
function get_exec_time {
    local start_time=$(date +%s%3N)
    ./main 100 100000 $1
    local end_time=$(date +%s%3N)
    echo $((end_time-start_time)) 
}

# Замеряем время для двух потоков (для одного слишком быстро xD)
T1=$(get_exec_time 2)

for p in {3..50}; do
    Tp=$(get_exec_time $p)
    Sp=$(echo "scale=2; $T1 / $Tp" | bc)
    Xp=$(echo "scale=2; $Sp / $p" | bc)
    echo "Количество потоков: $p, Ускорение: $Sp, Эффективность: $Xp" 
done
