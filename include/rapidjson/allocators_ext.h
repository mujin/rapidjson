#ifndef RAPIDJSON_ALLOCATORS_EXT_H_
#define RAPIDJSON_ALLOCATORS_EXT_H_

#include <array>
#include "rapidjson/allocators.h"

RAPIDJSON_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////

// This template class allows us to adapt any allocator that implements the RapidJSON Allocator concept
// into a CrtAllocator, so that we can use alternative allocation strategies for the internal stack.
// ActualAllocator template argument must have a constructor that accepts a pointer to a buffer and its size.
// Comes with a built-in stack-allocated buffer.
// Possible future to-do: implement enable_if enablers for users to disable the built-in buffer.
// Class and methods are all marked final. To customize the class, use the template parameters.
template <typename ActualAllocator = rapidjson::MemoryPoolAllocator<>, unsigned int BufferSize = 1024 * 16>
class StackPoolAllocator final : public rapidjson::CrtAllocator {
public:
    static const unsigned int kBuiltInBufferSize = BufferSize;
    static const bool kNeedFree = ActualAllocator::kNeedFree;

    StackPoolAllocator()
        : rapidjson::CrtAllocator()
        , _actualAllocator(_stackBuffer.data(), _stackBuffer.size()) {}

    StackPoolAllocator(const StackPoolAllocator&) = delete; // Disallow copying

    StackPoolAllocator& operator=(const StackPoolAllocator&) = delete; // Disallow copying

    virtual ~StackPoolAllocator() {}

    // Concept allocator
    inline virtual void* Malloc(size_t size) override final {
        return _actualAllocator.Malloc(size);
    }

    inline virtual void* Realloc(void* originalPtr, size_t originalSize, size_t newSize) override final {
        return _actualAllocator.Realloc(originalPtr, originalSize, newSize);
    }

    static void Free(void* ptr) {
        ActualAllocator::Free(ptr);
    }

    void Clear() {
        if (HasClearFn<ActualAllocator>::value) {
            _actualAllocator.Clear();
        }
    }

protected:
    std::array<char, BufferSize> _stackBuffer;
    ActualAllocator _actualAllocator;

    template <typename T, typename = void>
    struct HasClearFn : std::false_type {};

    template <typename T>
    struct HasClearFn<T, typename std::enable_if<std::is_member_function_pointer<decltype(&T::Clear)>::value>::type> : std::true_type {};
};

///////////////////////////////////////////////////////////////////////////////

// Looks like a MemoryPoolAllocator, but it carries with it its own heap-allocated buffer.
template <typename ActualAllocator = rapidjson::MemoryPoolAllocator<>>
class HeapPoolAllocator final : public rapidjson::CrtAllocator {
public:
    static const bool kNeedFree = ActualAllocator::kNeedFree;

    HeapPoolAllocator() : HeapPoolAllocator(1024 * 16) {}

    HeapPoolAllocator(size_t startingSize)
        : rapidjson::CrtAllocator()
        , _heapBuffer(startingSize, 0)
        , _actualAllocator(_heapBuffer.data(), startingSize) {}

    HeapPoolAllocator(const HeapPoolAllocator&) = delete; // Disallow copying

    HeapPoolAllocator& operator=(const HeapPoolAllocator&) = delete; // Disallow copying

    virtual ~HeapPoolAllocator() {}

    // Concept allocator
    inline virtual void* Malloc(size_t size) override final {
        return _actualAllocator.Malloc(size);
    }

    inline virtual void* Realloc(void* originalPtr, size_t originalSize, size_t newSize) override final {
        return _actualAllocator.Realloc(originalPtr, originalSize, newSize);
    }

    static void Free(void* ptr) {
        ActualAllocator::Free(ptr);
    }

    void Clear() {
        _actualAllocator.Clear();
    }

    ActualAllocator& GetUnderlyingAllocator() {
        return _actualAllocator;
    }

protected:
    std::vector<char> _heapBuffer;
    ActualAllocator _actualAllocator;
};

RAPIDJSON_NAMESPACE_END

#endif // RAPIDJSON_ALLOCATORS_H_