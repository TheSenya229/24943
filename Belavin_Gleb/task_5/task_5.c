#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1024

typedef struct {
    off_t offset;    // Позиция начала строки в файле
    size_t length;   // Длина строки (без символа новой строки)
} line_info_t;

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

    // Основной цикл запросов
    while (1) {
        int line_number;
        printf("Enter line number (0 to exit): ");
        
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

    close(fd);
    return 0;
}
