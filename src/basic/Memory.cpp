namespace amu {

namespace memory {

FORCE_INLINE void* 
allocate(upt size) {
    void* out = malloc(size);
    zero(out, size);
    return out;
}

FORCE_INLINE void* 
reallocate(void* ptr, upt size) {
    return realloc(ptr, size);
}

template<typename T> FORCE_INLINE T*
reallocate(T* ptr, upt size) {
    return (T*)realloc(ptr, size);
}

FORCE_INLINE void 
free(void* ptr) {
    ::free(ptr);
}

FORCE_INLINE void
copy(void* destination, void* source, upt bytes) {
    memcpy(destination, source, bytes);
}

template<typename T> FORCE_INLINE T*
copy(T* source, upt bytes) {
    T* out = (T*)memory::allocate(bytes);
    memory::copy((void*)out, (void*)source, bytes);
    return out;
}

FORCE_INLINE void
move(void* destination, void* source, upt bytes) {
    memmove(destination, source, bytes);
}   

FORCE_INLINE void
zero(void* ptr, upt bytes) {
    memset(ptr, 0, bytes);
}

} // namespace memory
} // namespace amu