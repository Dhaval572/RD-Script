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

    static const size_t BLOCKS_PER_CHUNK = 16;
    static const size_t ALIGNMENT = alignof(std::max_align_t);
    
    t_Chunk* m_Chunks;
    t_Block* m_FreeBlocks;
    size_t m_BlockSize;

    void AddChunk()
    {
        size_t chunk_size = sizeof(t_Chunk) + (m_BlockSize * BLOCKS_PER_CHUNK) - 1;
        t_Chunk* chunk = static_cast<t_Chunk*>(std::malloc(chunk_size));
        if (!chunk)
        {
            // If malloc fails, we cannot add a chunk.
            // Return to caller who should handle the memory exhaustion.
            return;
        }
        
        chunk->next = m_Chunks;
        m_Chunks = chunk;
        
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
        while (available >= m_BlockSize)
        {
            t_Block* block = reinterpret_cast<t_Block*>(data);
            block->next = m_FreeBlocks;
            m_FreeBlocks = block;
            data += m_BlockSize;
            available -= m_BlockSize;
        }
    }

public:
    t_MemoryPool(size_t block_size_) : m_Chunks(nullptr), m_FreeBlocks(nullptr)
    {
        // Ensure block size is at least as large as a pointer and properly aligned
        m_BlockSize = (block_size_ > sizeof(t_Block)) ? 
                      block_size_ : sizeof(t_Block);

        m_BlockSize = (m_BlockSize + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }
    
    ~t_MemoryPool()
    {
        t_Chunk* chunk = m_Chunks;
        while (chunk)
        {
            t_Chunk* next = chunk->next;
            std::free(chunk);
            chunk = next;
        }
    }
    
    void* Allocate()
    {
        if (!m_FreeBlocks)
        {
            AddChunk();
            if (!m_FreeBlocks)
            {
                return nullptr;
            }
        }
        
        t_Block* block = m_FreeBlocks;
        m_FreeBlocks = block->next;
        return block;
    }
    
    void Deallocate(void* ptr)
    {
        if (ptr)
        {
            t_Block* block = static_cast<t_Block*>(ptr);
            block->next = m_FreeBlocks;
            m_FreeBlocks = block;
        }
    }
    
    // Reset the entire pool (invalidates all pointers)
    void Reset()
    {
        m_FreeBlocks = nullptr;
        t_Chunk* chunk = m_Chunks;
        while (chunk)
        {
            char* data = chunk->data;
            size_t chunk_size = sizeof(t_Chunk) + (m_BlockSize * BLOCKS_PER_CHUNK) - 1;
            size_t available = chunk_size - offsetof(t_Chunk, data);
            
            // Align the data pointer
            uintptr_t align_offset = 
            (
                ALIGNMENT - (reinterpret_cast<uintptr_t>(data) % ALIGNMENT)
            ) % ALIGNMENT;
            data += align_offset;
            available -= align_offset;
            
            // Create free blocks
            while (available >= m_BlockSize)
            {
                t_Block* block = reinterpret_cast<t_Block*>(data);
                block->next = m_FreeBlocks;
                m_FreeBlocks = block;
                data += m_BlockSize;
                available -= m_BlockSize;
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
    t_MemoryPool* m_Pool;

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
        m_Pool(pool_) {}
    
    template<typename U>
    t_PoolAllocator(const t_PoolAllocator<U>& other) noexcept : 
        m_Pool(other.m_Pool) {}

    pointer allocate(size_type n)
    {
        if (n == 1)
        {
            return static_cast<pointer>(m_Pool->Allocate());
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
            m_Pool->Deallocate(ptr);
        }
        else
        {
            std::free(ptr);
        }
    }

    template<typename U>
    bool operator==(const t_PoolAllocator<U>& other) const noexcept
    {
        return m_Pool == other.m_Pool;
    }

    template<typename U>
    bool operator!=(const t_PoolAllocator<U>& other) const noexcept
    {
        return !(*this == other);
    }
};