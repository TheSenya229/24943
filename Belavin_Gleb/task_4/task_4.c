#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 4096

struct TextNode {
    char *content;
    struct TextNode *next;
};

void release_text_list(struct TextNode *head) {
    while (head != NULL) {
        struct TextNode *temp = head;
        head = head->next;
        free(temp->content);
        free(temp);
    }
}

void print_text_list(const struct TextNode *head) {
    while (head != NULL) {
        puts(head->content);
        head = head->next;
    }
}

int main(void) {
    struct TextNode *list_head = NULL;
    struct TextNode **insert_point = &list_head;  // Указатель на "next" предыдущего узла

    char input_buffer[INPUT_BUFFER_SIZE];

    printf("Enter lines..):\n");

    while (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
        if (input_buffer[0] == '.') {
            break;
        }

        size_t length = strlen(input_buffer);
        if (length > 0 && input_buffer[length - 1] == '\n') {
            input_buffer[length - 1] = '\0';
            length--;
        }

        char *stored_text = malloc(length + 1);
        if (!stored_text) {
            fprintf(stderr, "failed to allocate memory\n");
            release_text_list(list_head);
            return EXIT_FAILURE;
        }
        strcpy(stored_text, input_buffer);

        // Создаём новый узел
        struct TextNode *new_node = malloc(sizeof(struct TextNode));
        if (!new_node) {
            fprintf(stderr, "failed to allocate memory for node\n");
            free(stored_text);
            release_text_list(list_head);
            return EXIT_FAILURE;
        }
        new_node->content = stored_text;
        new_node->next = NULL;

        *insert_point = new_node;
        insert_point = &new_node->next;
    }

    printf("\n--- Saved lines ---\n");
    print_text_list(list_head);
    release_text_list(list_head);
    return 0;
}
/* Этот код представляет собой простую консольную программу на языке C, 
которая читает строки текста, вводимые пользователем, сохраняет их в динамически 
выделенной памяти в виде связного списка, а затем выводит все сохраненные строки обратно на экран.
 Ввод завершается, когда пользователь вводит строку, начинающуюся с точки (.). */