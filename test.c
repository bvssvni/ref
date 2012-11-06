#include <stdio.h>
#include <stdlib.h>

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

void test(void) {
	{
		
	}
	{
		gcInit(B, b);
		if (b->ref.keep != 0) err();
		gcSet(b, NULL);
	}
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
	{
		A *a = A_Init();
		gcStart(gcRef(a));
		gcEnd();
		if (a != NULL) err();
	}
	{
		A *a = A_Init();
		A *a2 = A_Init();
		gcStart(gcRef(a), gcRef(a2));
		gcCopy(a2, a);
		gcEnd();
		if (a != NULL) err();
		if (a2 != NULL) err();
	}
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
	{
		test_return_void();
	}
	{
		A *a = test_return_pointer();
		gcSet(a, NULL);
	}
	{
		gcIgnore(test_return_pointer());
	}
	{
		A *a = test_return_pointer();
		gcSet(a, NULL);
		a = test_return_pointer();
		gcSet(a, NULL);
	}
	{
		B a = {};
		gcInit(B, b);
		gcSet(b, &a);
		gcSet(b, NULL);
	}
	{
		B a = {};
		gcInit(B, b);
		gcStart(gcRef(b));
		gcSet(b, &a);
		gcEnd();
	}
	{
		A a = {.ref.members_length = 1};
		B b = {};
		a.b = &b;
		A *c = &(A){};
		gcCopy(c, &a);
		if (b.ref.keep != 0) err();
	}
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
}

int main(void) {
	int i;
	int end = 1 << 22;
	for (i = 0; i < end; i++) {
		test();
	}
	printf("%i Unit tests succeeded!\n", end);

	return 0;
}
