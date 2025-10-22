#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE 256

typedef struct Node_s {
    char *string;
    struct Node_s *next;
} Node;

Node *head = NULL;
Node *tail = NULL;

void init() {
    head = NULL;
    tail = NULL;
}

void push(char *string) {
    size_t len = strlen(string) + 1;
    char *copy = malloc(len);
    if (!copy) {
        perror("malloc");
        exit(1);
    }
    strcpy(copy, string);

    Node *newNode = malloc(sizeof(Node));
    if (!newNode) {
        perror("malloc");
        free(copy);
        exit(1);
    }
    newNode->string = copy;
    newNode->next = NULL;

    if (!head) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
}

void printList() {
    Node *cur = head;
    while (cur) {
        // Выводим строку как есть (включая escape-последовательности)
        printf("%s\n", cur->string);
        cur = cur->next;
    }
}

void freeList() {
    Node *cur = head;
    while (cur) {
        Node *next = cur->next;
        free(cur->string);
        free(cur);
        cur = next;
    }
    head = tail = NULL;
}

// Управление raw-режимом
void setRawMode(int enable) {
    static struct termios orig_termios;
    static int is_saved = 0;

    if (enable) {
        if (!is_saved) {
            if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;
            is_saved = 1;
        }
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    } else {
        if (is_saved) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            is_saved = 0;
        }
    }
}

// Безопасная печать строки (для отладки: показывает непечатаемые символы)
void debugPrint(const char *s) {
    while (*s) {
        if (*s == '\x1b') {
            printf("\\x1b");
        } else if (*s == '\n') {
            printf("\\n");
        } else if (*s == '\r') {
            printf("\\r");
        } else if (*s == '\t') {
            printf("\\t");
        } else if (*s >= 32 && *s <= 126) {
            putchar(*s);
        } else {
            printf("\\x%02x", (unsigned char)*s);
        }
        s++;
    }
    putchar('\n');
}

int main() {
    char inputBuf[BUFFER_SIZE];
    init();

    printf("Enter strings.
