#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>   // Для mmap() и munmap()
#include <errno.h>
#include <sys/select.h> // Для select()

#define MAX_LINES 1000
#define BUFFER_SIZE 1024
#define TIMEOUT_SECONDS 5

typedef struct {
    off_t offset;    // Позиция начала строки в файле (смещение от начала mmap-области)
    size_t length;   // Длина строки (без символа новой строки)
} line_info_t;

// Функция для вывода всего содержимого файла (используя указатель на память)
void print_entire_file_mmap(char *file_data, size_t file_size) {
    printf("\nTime's up! Printing entire file:\n");
    printf("==============================\n");
    
    // Просто печатаем весь массив байтов
    // Мы полагаемся на то, что это текстовый файл, и символов \0 внутри нет.
    // Если бы были, нужно было бы печатать блоками.
    write(STDOUT_FILENO, file_data, file_size);
    
    printf("\n==============================\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Открываем файл для чтения
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 1. Получаем размер файла с помощью fstat
    struct stat file_stats;
    if (fstat(fd, &file_stats) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }
    size_t file_size = file_stats.st_size;

    if (file_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    // 2. Отображаем файл в память (mmap)
    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Файловый дескриптор больше не нужен, так как у нас есть отображение в память
    close(fd); 

    line_info_t lines[MAX_LINES];
    int line_count = 0;
    off_t line_start = 0;

    // 3. Читаем данные ИЗ ПАМЯТИ и строим таблицу строк
    // Теперь мы просто итерируемся по массиву file_data
    for (size_t i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            if (line_count < MAX_LINES) {
                lines[line_count].offset = line_start;
                lines[line_count].length = i - line_start;
                line_count++;
            }
            line_start = i + 1;
        }
    }

    // Обрабатываем последнюю строку, если она не заканчивается \n
    if (line_start < file_size && line_count < MAX_LINES) {
        lines[line_count].offset = line_start;
        lines[line_count].length = file_size - line_start;
        line_count++;
    }

    // Выводим таблицу для отладки
    printf("\nLine table for file '%s':\n", argv[1]);
    printf("Line#\tOffset\tLength\n");
    printf("-----\t------\t------\n");
    
    int i;
    for (i = 0; i < line_count; i++) {
        printf("%d\t%ld\t%zu\n", 
               i + 1, 
               (long)lines[i].offset, 
               lines[i].length);
    }
    printf("Total lines: %d\n\n", line_count);

    // Основной цикл запросов с таймаутом
    while (1) {
        printf("Enter line number (0 to exit). You have %d seconds: ", TIMEOUT_SECONDS);
        fflush(stdout);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;
        
        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
        
        if (ready == -1) {
            perror("select");
            break;
        } else if (ready == 0) {
            // Таймаут - выводим весь файл и завершаемся
            print_entire_file_mmap(file_data, file_size);
            break;
        } else {
            // Есть данные для чтения
            int line_number;
            
            if (scanf("%d", &line_number) != 1) {
                printf("Invalid input. Please enter a number.\n");
                while (getchar() != '\n'); // Очищаем буфер ввода
                continue;
            }

            if (line_number == 0) {
                printf("Process_end\n");
                break;
            }

            if (line_number < 1 || line_number > line_count) {
                printf("Line number must be between 1 and %d\n", line_count);
                continue;
            }

            line_info_t *line = &lines[line_number - 1];
            
            char line_buffer[BUFFER_SIZE];
            size_t read_length = line->length;
            if (read_length >= sizeof(line_buffer)) {
                read_length = sizeof(line_buffer) - 1;
            }
            
            // Копируем байты из mmap-памяти в буфер
            memcpy(line_buffer, file_data + line->offset, read_length);
            line_buffer[read_length] = '\0';

            printf("Line %d: %s\n", line_number, line_buffer);
        }
    }

    // 5. Освобождаем отображенную память
    if (munmap(file_data, file_size) == -1) {
        perror("munmap");
    }

    return 0;
}
