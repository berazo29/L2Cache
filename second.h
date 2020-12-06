//
// Created by Bryan Erazo on 12/2/20.
//

#ifndef L1CACHE_SECOND_H
#define L1CACHE_SECOND_H

// DATA STRUCTURE
struct Node {
    unsigned long address;
    struct Node *next;
};

struct Cache {
    size_t len;
    size_t number_nodes_in_linked_list;
    size_t max_nodes_allow;
    struct Node *linked_list;
};

// Data-Structure Nodes Functions
void insertNodeInTheBeginning(struct Node** head, unsigned long new_data);
void deleteLinkedList(struct Node** head);


// Functions
long getCacheSize(char *arg);
long getBlockSize(char *arg);
int getCachePolicy(char *arg);
unsigned int getAssociativity(char *arg);

size_t calculateNumberCacheAddresses(size_t cache_size, size_t cache_block );
struct Cache *
createCache(struct Cache *cache, size_t cache_size, size_t block_size, unsigned int assocAction, size_t assoc);

// Utility functions
bool IsPowerOfTwo(unsigned long x);
bool isEven(long int n);
unsigned int checkAssociativityInput(char *arg);
long getNumberFromAssoc(char *arg);

// Data structure functions
void insertNodeInTheBeginning(struct Node** head, unsigned long new_data){

    // Allocate new node and insert O(1)
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->address = new_data;
    new_node->next = (*head);
    (*head) = new_node;

}
struct Node* removeLastNode(struct Node* head)
{
    if (head == NULL)
        return NULL;

    if (head->next == NULL) {
        free(head);
        return NULL;
    }

    // Find the second last node
    struct Node* ptr = head;
    while (ptr->next->next != NULL){
        ptr = ptr->next;
    }
    free(ptr->next);
    ptr->next = NULL;

    return head;
}
void deleteLinkedList(struct Node** head){
    if ( head == NULL){
        return;
    }
    struct Node *ptr = (*head);
    struct Node *tmp;
    while ( ptr != NULL){
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }
    *head = NULL;
}
void printList(struct Node *head){

    if (head == NULL){
        printf("(null)\n");
        return;
    }
    struct Node *ptr = head;

    while (ptr != NULL){
        printf("addr:%lu  ", (long )ptr->address);
        ptr = ptr->next;
    }
    printf("(NULL)\n");

}
void removeNodeAtAddress(struct Node **head, __int64_t key){
    // Store head node
    struct Node* temp = *head, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && temp->address == key){
        *head = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }

    // Search the key and delete
    while (temp != NULL && temp->address != key){
        prev = temp;
        temp = temp->next;
    }

    // Key was not in the linked_list
    if (temp == NULL) return;
    prev->next = temp->next;
    free(temp);
}

void printCache(struct Cache *cache, int dev){
    if (dev == 1){
        struct Cache *ptr = cache;
        printf("\n\n***CACHE***\n");
        if (ptr == NULL){
            printf("cache is empty\n");
            printf("**CACHE-END**\n\n");
            return;
        }
        size_t len = cache->len;
        for (size_t i = 0; i < len; ++i) {
            printf("Cache[%zu]--> ",i);
            printList(ptr[i].linked_list);
            printf("number_nodes_in_linked_list:%zu\n",ptr[i].number_nodes_in_linked_list);
            printf("max_nodes_allow:%zu\n\n",ptr[i].max_nodes_allow);
        }
        printf("\n***CACHE-END***\n\n");
    } else{
        struct Cache *ptr = cache;
        printf("\n\n***CACHE***\n");
        if (ptr == NULL){
            printf("cache is empty\n");
            printf("**CACHE-END**\n\n");
            return;
        }
        size_t len = cache->len;
        printf("max_nodes_allow:%zu\n\n",ptr[0].max_nodes_allow);
        for (size_t i = 0; i < len; ++i) {
            printf("Cache[%zu]--> ",i);
            printList(ptr[i].linked_list);
        }
        printf("\n***CACHE-END***\n\n");
    }
}

#endif //L1CACHE_SECOND_H
