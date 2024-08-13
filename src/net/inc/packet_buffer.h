#pragma once

#include "net_err.h"
#include "noncopyable.h"

#include <list>
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <cstdio>

namespace netstack 
{
    class PacketBlock
    {
        friend PacketBlock* AllocateBlock(size_t size);
    public:
        ~PacketBlock();

        int GetData(size_t size, unsigned char* dst, int offset = 0);
        int SetData(size_t size, const unsigned char* data, int offset = 0);
    public:
        size_t DataSize() const 
        {
            return curr_pos_;
        }

        size_t TotalSize() const
        {
            return total_size_;
        }

        size_t FreeSize() const
        {
            return total_size_ - curr_pos_;
        }

        void SetPrevNode(PacketBlock* block);
        void Print()
        {
            for (int i = 0; i < total_size_; i++)
                printf("%c", data_[i]);
            printf("\n");
        }
    private:
        PacketBlock(size_t size);
    public:
        PacketBlock* next_;
        PacketBlock* prev_;
    private:
        unsigned char* data_;		// 存储数据
        size_t total_size_;	// 空间总大小
        size_t curr_pos_;	// 当前数据的末尾位置
    };

    PacketBlock* AllocateBlock(size_t size);







    class PacketBuffer : NonCopyable
    {
    public:
        PacketBuffer();
        ~PacketBuffer();
    public:
        int AddHeader(size_t, const unsigned char*);
        int RemoveHeader();
        int Resize(size_t);
        int Append(PacketBuffer& dest);
        int Write(const unsigned char* data, size_t size);
        int Read(unsigned char* dest, size_t size, PacketBlock* block = nullptr);
        int Seek(int offset);
        size_t DataSize();
        size_t TotalSize() const
        { return total_size_; }
    private:
        void CreateBlock(size_t size);
    private:
        std::list<PacketBlock*> blocks_;
        PacketBlock* curr_block_;
        size_t total_size_;		// 数据包总大小
        size_t data_size_;		// 数据大小
    };

}