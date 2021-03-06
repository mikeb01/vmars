
#ifndef VMARS_VMARS_ATOMIC_H
#define VMARS_VMARS_ATOMIC_H

#if defined(__x86_64__)

static inline int64_t vmars_atomic_load_seq_cst_i64(int64_t *field)
{
    int64_t i = *field;
    __asm__ volatile ("" ::: "memory");
    return i;
}

static inline void vmars_atomic_store_release_i64(int64_t *field, int64_t value)
{
    __asm__ volatile ("" ::: "memory");
    *field = value;
}

static inline void vmars_atomic_store_seq_cst_i64(int64_t *field, int64_t value)
{
    __asm__ volatile ("lock; xchgq %0, %1" : "+q" (value), "+m" (*field));
}

static inline void vmars_atomic_store_seq_cst_int(int *field, int value)
{
    __asm__ volatile ("lock; xchg %0, %1" : "+q" (value), "+m" (*field));
}

#else

#error "Unable to determine atomic operations for your platform"

#endif

#endif //VMARS_VMARS_ATOMIC_H
