#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *text;
    struct Node *prev;
    struct Node *next;
} Node;

Node *insert(Node *head, const char *text) {
    Node *new = malloc(sizeof(Node));

    new->text = malloc(strlen(text) + 1);
    strcpy(new->text, text);

    new->prev = NULL;
    new->next = head;

    if (head != NULL) {
        head->prev = new;
    }

    return new;
}

 Node *find(Node *head, const char *text) {
    Node *current = head;

    while (current != NULL) {
        if (strcmp(current->text, text) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

Node *delete(Node *head, const char *text) {
    Node *to_delete = find(head, text);

    if (to_delete == NULL) {
        return head;
    }

    if (to_delete->prev != NULL) {
        to_delete->prev->next = to_delete->next;
    } else {
        head = to_delete->next;
    }

    if (to_delete->next != NULL) {
        to_delete->next->prev = to_delete->prev;
    }

    free(to_delete->text);
    free(to_delete);

    return head;
}

int main(void) {
    Node *list = NULL;

    list = insert(list, "1");
    list = insert(list, "2");
    list = insert(list, "3");

    if (find(list, "2") != NULL) {
        printf("Found 2\n");
    }

    list = delete(list, "2");

    if (find(list, "2") == NULL) {
        printf("2 deleted\n");
    }

    list = delete(list, "1");
    list = delete(list, "3");

    return 0;
}
