#ifndef MACROS_H_BUTIA83G
#define MACROS_H_BUTIA83G

#define STRFUNC ((const char*) (__PRETTY_FUNCTION__))
#define GNUC_UNUSED __attribute__((__unused__))

#define GET_PTR(type, ...) ((type*) &((type){__VA_ARGS__}))

#define INT_TO_PTR(v) ((void*) (long) (v))
#define PTR_TO_INT(v) ((int) (long) (v))

#define UINT_TO_PTR(v) ((void*) (unsigned long) (v))
#define PTR_TO_UINT(v) ((unsigned int) (unsigned long) (v))

#define TOSTR0(v) #v
#define TOSTR(v) TOSTR0(v)

#define mass_cell(m, e, i) (&((char*) (m))[(i) * (e)])

#define salloc(struct_type, n_structs) ((struct_type*) ((n_structs > 0) ? (malloc(sizeof(struct_type) * n_structs)) : (NULL)))
#define salloc0(struct_type, n_structs) ((struct_type*) ((n_structs > 0) ? (calloc(n_structs, sizeof(struct_type))) : (NULL)))

#endif /* end of include guard: MACROS_H_BUTIA83G */
