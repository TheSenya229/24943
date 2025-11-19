#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/select.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1024
#define TIMEOUT_SECONDS 5

typedef struct {
    off_t offset;    // Позиция начала строки в файле
    size_t length;   // Длина строки (без символа новой строки)
} line_info_t;

// Функция для вывода всего содержимого файла
void print_entire_file(int fd) {
    printf("\nTimeout! Printing entire file:\n");
    printf("==============================\n");
    
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return;
    }
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    
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

    line_info_t lines[MAX_LINES];
    int line_count = 0;
    off_t current_offset = 0;
    off_t line_start = 0;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Читаем файл и строим таблицу строк
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        int i;
        for (i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                // Найден конец строки
                if (line_count < MAX_LINES) {
                    lines[line_count].offset = line_start;
                    lines[line_count].length = current_offset + i - line_start;
                    line_count++;
                }
                line_start = current_offset + i + 1;
            }
        }
        current_offset += bytes_read;
    }

    // Обрабатываем последнюю строку, если она не заканчивается \n
    if (line_start < current_offset && line_count < MAX_LINES) {
        lines[line_count].offset = line_start;
        lines[line_count].length = current_offset - line_start;
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
        fflush(stdout); // Важно: сбрасываем буфер вывода
        
        // Настраиваем select для таймаута
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
            printf("\nTime's up! No input received within %d seconds.\n", TIMEOUT_SECONDS);
            print_entire_file(fd);
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
            
            if (lseek(fd, line->offset, SEEK_SET) == (off_t)-1) {
                perror("lseek");
                continue;
            }

            char line_buffer[BUFFER_SIZE];
            size_t read_length = line->length;
            if (read_length >= sizeof(line_buffer)) {
                read_length = sizeof(line_buffer) - 1;
            }

            ssize_t actual_read = read(fd, line_buffer, read_length);
            if (actual_read == -1) {
                perror("read");
                continue;
            }

            line_buffer[actual_read] = '\0';
            printf("Line %d: %s\n", line_number, line_buffer);
        }
    }

    close(fd);
    return 0;
}