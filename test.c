#if 0
#!/bin/bash
gcc -o test *.c -Wall -Wfatal-errors -O3
if [ "$?" = "0" ]; then
	time ./test
fi
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ref.h"

#define err() do {printf("%i Error!\n", __LINE__); exit(1);} while (0);


typedef struct B {
	ref ref;
} B;

typedef struct A {
	ref ref;
	B *b;
} A;

A *A_Init(void) {
	gcInit(A, a, .ref.members_length = 1);
	return a;
}

void test_return_void(void) {
	A *a = A_Init();
	gcStart(gcRef(a));
	gcEnd();
	return;
}

A *test_return_pointer(void) {
	A *a = A_Init();
	gcStart(gcRef(a));
	gcReturn(a);
}

void test_init_1(void)
{
	gcInit(B, b);
	assert(b->ref.keep == 0);
	gcSet(b, NULL);
}

void test_init_2(void)
{
	A *a = A_Init();
	gcInit(B, b);
	gcSet(a->b, b);
	if (b->ref.keep != 1) err();
	// printf("gcMember %llu\n", (unsigned long long)gcMember(a, 0));
	if ((B*)gcMember(a, 0) != a->b) err();
	gcSet(a, NULL);
	if (b->ref.keep != 0) err();
	gcSet(b, NULL);
}

void test_stack_1(void)
{
	// Assign pointer allocated to stack.
	// Checking that the garbage collector knows how to distinguish
	// between memory allocated on the heap and memory on the stack.
	A *a = A_Init();
	gcStart(gcRef(a));
	A *a2 = &(A){};
	gcSet(a2, a);
	if (a2->ref.keep != 1) err();
	gcEnd();
	if (a2->ref.keep != 0) err();
	gcSet(a2, NULL);
}

void test_gcStart_1(void)
{
	A *a = A_Init();
	gcStart(gcRef(a));
	gcEnd();
	if (a != NULL) err();
}

void test_gcCopy_1(void)
{
	A *a = A_Init();
	A *a2 = A_Init();
	gcStart(gcRef(a), gcRef(a2));
	gcCopy(a2, a);
	gcEnd();
	if (a != NULL) err();
	if (a2 != NULL) err();
}

void test_gcCopy_2(void)
{
	A a = {.ref.members_length = 1};
	B b = {};
	a.b = &b;
	A *c = &(A){};
	gcCopy(c, &a);
	if (b.ref.keep != 0) err();
}

void test_gcCopy_3(void)
{
	A a = {.ref.members_length = 1};
	gcInit(B, b);
	gcStart(gcRef(b));
	a.b = b;
	A *c = &(A){};
	gcCopy(c, &a);
	if (b->ref.keep != 1) err();
	gcSet(c, NULL);
	if (b->ref.keep != 0) err();
	gcEnd();
}

void test_gcSet_1(void)
{
	A *a = A_Init();
	A *a2 = A_Init();
	gcInit(B, b);
	gcStart(gcRef(a), gcRef(a2), gcRef(b));
	gcSet(a->b, b);
	gcCopy(a2, a);
	if (a2->b != b) err();
	if (b->ref.keep != 2) err();
	gcEnd();
}

void test_gcSet_2(void)
{
	B a = {};
	gcInit(B, b);
	gcSet(b, &a);
	gcSet(b, NULL);
}

void test_gcSet_3(void)
{
	B a = {};
	gcInit(B, b);
	gcStart(gcRef(b));
	gcSet(b, &a);
	gcEnd();
}

void test_return_pointer_1(void)
{
	A *a = test_return_pointer();
	gcSet(a, NULL);
}

void test_return_pointer_2(void)
{
	A *a = test_return_pointer();
	gcSet(a, NULL);
	a = test_return_pointer();
	gcSet(a, NULL);
}

void test_gcIgnore_1(void)
{
	gcIgnore(test_return_pointer());
}

typedef struct
{
	ref ref;
	int n;
	int items[];
} int_array;

void test_gcInitFlexible_1(void)
{
	gcInitFlexible(int_array, a, char*, 4, .n = 4);
	
	int i;
	for (i = 0; i < a->n; i++) {
		a->items[i] = i;
	}
	
	gcSet(a, NULL);
}

typedef struct
{
	ref ref;
	int n;
	char *items[];
} string_array;

void string_array_Delete(void *ptr)
{
	string_array *arr = ptr;
	int i;
	for (i = 0; i < arr->n; i++) {
		free(arr->items[i]);
	}
}

void test_gcInitFlexible_2(void)
{
	gcInitFlexible(string_array, a, char*, 4, .n = 4,
				   .ref.destructor = string_array_Delete);
	
	int i;
	for (i = 0; i < a->n; i++) {
		a->items[i] = malloc(sizeof(char)*10);
	}
	
	gcSet(a, NULL);
}

typedef struct {
	ref ref;
	int n;
	int_array *items[];
} int_matrix;

void int_matrix_Delete(void *ptr)
{
	int_matrix *m = ptr;
	int i;
	for (i = 0; i < m->n; i++) {
		gcSet(m->items[i], NULL);
	}
}

void test_gcInitFlexible_3(void)
{
	gcInitFlexible(int_matrix, a, int_array*, 4, .n = 4,
				   .ref.destructor = int_matrix_Delete);
	memset(a->items, 0, sizeof(int_array*)*a->n);
	gcSet(a, NULL);
}

void test(void) {
	test_init_1();
	test_init_2();
	test_stack_1();
	test_gcStart_1();
	
	test_gcSet_1();
	test_gcSet_2();
	test_gcSet_3();
	
	test_gcCopy_1();
	test_gcCopy_2();
	test_gcCopy_3();
	
	test_return_void();
	test_return_pointer_1();
	test_return_pointer_2();
	
	test_gcIgnore_1();
	
	test_gcInitFlexible_1();
	test_gcInitFlexible_2();
	test_gcInitFlexible_3();
}

int main(void) {
	int i;
	int end = 1 << 20;
	for (i = 0; i < end; i++) {
		test();
	}
	printf("%i Unit tests succeeded!\n", end);

	return 0;
}
