
#include "ref.h"

// Frees the memory of a structure if no more owners, but also the pointers.
// This has to be a function because it calls itself when having pointers in the struct.
void gcFreeRef(ref *a)
{
	int release = (a) != NULL && (a)->is_allocated && --(a)->keep < 0;
	if (!release) return;

	// In case it points to itself.
	a->is_allocated = 0;

	int macro_i;
	ref *macro_ref;
	for (macro_i = 0; macro_i < (a)->members_length; macro_i++) {
		macro_ref = gcMember(a, macro_i);
		if (macro_ref == NULL) continue;
		gcFreeRef(macro_ref);
	}

	free(a);
}
