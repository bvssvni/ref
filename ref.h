/*
Ref - Garbage collection in C
BSD license.
by Sven Nilsen, 2012
http://www.cutoutpro.com

Version: 0.002
Angular degrees version notation
http://isprogrammingeasy.blogspot.no/2012/08/angular-degrees-versioning-notation.html
 
0.003	Fixed memory alginment issues for struct members.
0.002	Added gcInitFlexible for flexible structures.
0.001	Added support for destructor.
 
*/
/*
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#ifndef REF_GARBAGE_COLLECTION_GUARD
#define REF_GARBAGE_COLLECTION_GUARD

/* 
Garbage collection using reference counting.
See README for more information.
*/
typedef struct ref ref;
struct ref {
	unsigned char is_allocated;
	void (*destructor)(void *ptr);
	int keep, members_length;
};

// The only non-macro, because it is recursive.
void gcFreeRef(ref *a);
// Calculate the member reference alignment in struct.
/*
#define gcRefAlignment() (sizeof(ref)>sizeof(ref*) ? \
sizeof(ref)+sizeof(ref*)-sizeof(ref)%sizeof(ref*) : \
sizeof(ref*))
 */
#define gcRefAlignment() sizeof(ref)
// Get garbage collected pointer within struct.
// A bit complicated because it calculates memory alignment.
#define gcMember(a, ind) ((ref**)((unsigned char*)a+gcRefAlignment()))[ind]
//#define gcMember(a, ind) ((ref**)((ref*)a+1)+ind)
// Just for removing comma at end of variadic arguments.
#define gcVaArgs(...) __VA_ARGS__
#define gcInit(type, name, ...) \
type *name = malloc(sizeof(type)); \
*name = (type){gcVaArgs(.ref.is_allocated = 1, __VA_ARGS__)};
// Inits flexible array, but does not clear array to 0.
#define gcInitFlexible(type, name, arrtype, n, ...) \
type *name = malloc(sizeof(type)+sizeof(arrtype)*n); \
*name = (type){gcVaArgs(.ref.is_allocated = 1, __VA_ARGS__)};
#define gcIgnore(a) do {\
ref *macro_val = (ref*)a; \
if (macro_val != NULL) {gcFreeRef(macro_val); \
} } while (0);
#define gcEnd() do { \
int macro_size = sizeof(refs)/sizeof(ref*); \
int macro_i; ref *macro_item; \
for (macro_i = 0; macro_i < macro_size; macro_i++) { \
	macro_item = *refs[macro_i]; \
	if (macro_item == NULL) continue; \
	if (--macro_item->keep < 0) { \
		gcFreeRef(macro_item); *refs[macro_i] = NULL; \
	} } } while (0);
#define gcReturn(a) \
if ((a) != NULL && (a)->ref.is_allocated) (a)->ref.keep++; \
gcEnd(); return a;
#define gcSet(a, b) do { \
	__typeof__(b) macro_b = (b); \
	if ((a) == macro_b) break; \
	if ((a) != NULL) gcFreeRef((ref*)a); \
	(a) = macro_b; \
	if ((a) != NULL && (a)->ref.is_allocated) (a)->ref.keep++; \
} while (0);
// Before copying, release members without releasing itself.
#define gcCopy(a, ...) do { \
	ref macro_ref = a->ref; \
	if (macro_ref.keep == 0 && macro_ref.destructor != NULL) \
		macro_ref.destructor(a); \
    int macro_i; ref *macro_member; \
	for (macro_i = 0; macro_i < macro_ref.members_length; macro_i++) { \
		macro_member = gcMember(a, macro_i); \
		if (macro_member != NULL && macro_member->is_allocated && \
			--macro_member->keep < 0) { \
			gcFreeRef(macro_member); \
		} } \
	*a = *(__VA_ARGS__); \
	macro_ref.members_length = a->ref.members_length; a->ref = macro_ref; \
	for (macro_i = 0; macro_i < macro_ref.members_length; macro_i++) { \
		macro_member = gcMember(a, macro_i); \
		if (macro_member != NULL && macro_member->is_allocated) \
			macro_member->keep++; \
	} } while (0);
#define gcRef(a) (ref**)&a
#define gcStart(...) ref **refs[] = {__VA_ARGS__}

#endif
