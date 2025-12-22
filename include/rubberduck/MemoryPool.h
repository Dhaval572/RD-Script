#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <new>

// Simple memory pool allocator for optimizing AST node allocations
class t_MemoryPool
{
private:
    struct t_Block
    {
        t_Block* next;
    };

    struct t_Chunk
    {
        t_Chunk* next;
        char data[1];
    };

    static const size_t BLOCK_SIZE = 1024 * 256; // 64KB chunks
    static const size_t ALIGNMENT = alignof(std::max_align_t);
    
    t_Chunk* chunks;
    t_Block* free_blocks;
    size_t block_size;

    void AddChunk()
    {
        size_t chunk_size = sizeof(t_Chunk) + (block_size * 16) - 1;
        t_Chunk* chunk = static_cast<t_Chunk*>(std::malloc(chunk_size));
        if (!chunk)
        {
            throw std::bad_alloc();
        }
        
        chunk->next = chunks;
        chunks = chunk;
        
        char* data = chunk->data;
        size_t available = chunk_size - offsetof(t_Chunk, data);
        
        // Align the data pointer
        uintptr_t align_offset = 
        (
            ALIGNMENT - (reinterpret_cast<uintptr_t>(data) % ALIGNMENT)
        ) % ALIGNMENT;
        data += align_offset;
        available -= align_offset;
        
        // Create free blocks
        while (available >= block_size)
        {
            t_Block* block = reinterpret_cast<t_Block*>(data);
            block->next = free_blocks;
            free_blocks = block;
            data += block_size;
            available -= block_size;
        }
    }

public:
    t_MemoryPool(size_t block_size_) : chunks(nullptr), free_blocks(nullptr)
    {
        // Ensure block size is at least as large as a pointer and properly aligned
        block_size = (block_size_ > sizeof(t_Block)) ? 
                      block_size_ : sizeof(t_Block);

        block_size = (block_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }
    
    ~t_MemoryPool()
    {
        t_Chunk* chunk = chunks;
        while (chunk)
        {
            t_Chunk* next = chunk->next;
            std::free(chunk);
            chunk = next;
        }
    }
    
    void* Allocate()
    {
        if (!free_blocks)
        {
            AddChunk();
        }
        
        t_Block* block = free_blocks;
        free_blocks = block->next;
        return block;
    }
    
    void Deallocate(void* ptr)
    {
        if (ptr)
        {
            t_Block* block = static_cast<t_Block*>(ptr);
            block->next = free_blocks;
            free_blocks = block;
        }
    }
    
    // Reset the entire pool (invalidates all pointers)
    void Reset()
    {
        free_blocks = nullptr;
        t_Chunk* chunk = chunks;
        while (chunk)
        {
            char* data = chunk->data;
            size_t chunk_size = BLOCK_SIZE;
            size_t available = chunk_size - offsetof(t_Chunk, data);
            
            // Align the data pointer
            uintptr_t align_offset = 
            (
                ALIGNMENT - (reinterpret_cast<uintptr_t>(data) % ALIGNMENT)
            ) % ALIGNMENT;
            data += align_offset;
            available -= align_offset;
            
            // Create free blocks
            while (available >= block_size)
            {
                t_Block* block = reinterpret_cast<t_Block*>(data);
                block->next = free_blocks;
                free_blocks = block;
                data += block_size;
                available -= block_size;
            }
            
            chunk = chunk->next;
        }
    }
};

// Allocator that uses a memory pool
template<typename T>
class t_PoolAllocator
{
private:
    t_MemoryPool* pool;

public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    template<typename U>
    struct rebind
    {
        typedef t_PoolAllocator<U> other;
    };

    t_PoolAllocator(t_MemoryPool* pool_) noexcept : 
        pool(pool_) {}
    
    template<typename U>
    t_PoolAllocator(const t_PoolAllocator<U>& other) noexcept : 
        pool(other.pool) {}

    pointer allocate(size_type n)
    {
        if (n == 1)
        {
            return static_cast<pointer>(pool->Allocate());
        }
        else
        {
            return static_cast<pointer>(std::malloc(n * sizeof(T)));
        }
    }

    void deallocate(pointer ptr, size_type n)
    {
        if (n == 1)
        {
            pool->Deallocate(ptr);
        }
        else
        {
            std::free(ptr);
        }
    }

    template<typename U, typename... Args>
    void construct(U* ptr, Args&&... args)
    {
        new(ptr) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* ptr)
    {
        ptr->~U();
    }

    template<typename U>
    bool operator==(const t_PoolAllocator<U>& other) const noexcept
    {
        return pool == other.pool;
    }

    template<typename U>
    bool operator!=(const t_PoolAllocator<U>& other) const noexcept
    {
        return !(*this == other);
    }
};