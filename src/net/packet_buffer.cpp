#include "packet_buffer.h"

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <cstdlib>

namespace netstack 
{
    template <typename T>
    size_t AlignUp(size_t size)
    {
        size_t alignment = std::alignment_of<T>();
        return (size + alignment - 1) & ~(alignment - 1);
    }


    PacketBlock::PacketBlock(size_t size)
	: total_size_(size), next_(nullptr),
	    prev_(nullptr), curr_pos_(0), data_(nullptr)
    {
        data_ = (unsigned char*)calloc(size, sizeof(char));
    }

    PacketBlock::~PacketBlock()
    {
        if (data_)
        {
            delete data_;
            data_ = nullptr;
        }
    }


    int PacketBlock::GetData(size_t size, unsigned char* dst, int offset)
    {
        if (data_ == nullptr || dst == nullptr)
            return -1;
        if (size > (total_size_ - offset))	// 数据不够读取
            return -1;

        memcpy(dst, data_ + offset, size);

        return (offset + size);
    }

    int PacketBlock::SetData(size_t size, const unsigned char* data, int offset)
    {
        if (data_ == nullptr || data == nullptr)
            return -1;
        if ((offset != 0) && offset < curr_pos_)
            return -2;
        if (size > (total_size_ - curr_pos_))
            return -1;

        offset = offset == 0 ? curr_pos_ : offset;

        memcpy(data_ + offset, data, size);
        curr_pos_ += size;

        return curr_pos_;
    }

    void PacketBlock::SetPrevNode(PacketBlock* block)
    {
        if (block)
        {
            prev_ = block;
            block->next_ = this;
        }
    }

    void PacketBlock::CopyData(PacketBlock* src_pkt, int offset, size_t size)
    {
        unsigned char* start_ptr = src_pkt->data_ + offset;
        memcpy(data_, start_ptr, size);
    }

    PacketBlock* PacketBlock::CuttingPacket(PacketBlock* block, int offset, size_t size)
    {
        PacketBlock* after_block = AllocateBlock(size);
        after_block->CopyData(block, offset, size);

        return after_block;
    }


    PacketBlock* AllocateBlock(size_t size)
    {
        PacketBlock* block = nullptr;
        do 
        {
            // 循环等待内存分配成功.防止内存不足后续程序崩溃
            // TODO: 日志输出内存不足,分配PacketBlock
            block = new PacketBlock(size);
        } while (!block);
        return block;
    }


    








    ////////////////////////////////////// PacketBuffer
    PacketBuffer::PacketBuffer(size_t size)
        : total_size_(0),
        data_size_(0),
        curr_block_(nullptr)
    {
        CreateBlock(size);
    }

    PacketBuffer::~PacketBuffer()
    {

    }


    int PacketBuffer::AddHeader(size_t size, const unsigned char* data)
    {
        PacketBlock* block = AllocateBlock(size);
        block->SetData(size, data);
        data_size_ += size;
        total_size_ += AlignUp<size_t>(size);

        PacketBlock* head = *blocks_.begin();
        head->SetPrevNode(block);
        blocks_.push_front(block);
        block->next_ = head;
        
        return 0;
    }

    int AddTail(size_t, const unsigned char*)
    {

        return 0;
    }

    int PacketBuffer::RemoveHeader(size_t size)
    {
        if (blocks_.empty())
            return -1;
        PacketBlock* block = blocks_.front();
        
        if (block->TotalSize() > size)
        {
            // 拆分
            blocks_.pop_front();
            // 丢弃开头size字节大小的数据
            PacketBlock* after_cutting_pkt = 
                PacketBlock::CuttingPacket(block, size, block->TotalSize() - size);
            blocks_.push_front(after_cutting_pkt);
            delete block;
        }
        else if (block->TotalSize() == size)
        {
            blocks_.pop_front();
            delete block;
        }
        else if (block->TotalSize() < size)
            return -1;
        
        return 0;
    }

    int PacketBuffer::RemoveTail(size_t size)
    {
        if (blocks_.empty())
            return -1;
        PacketBlock* tail = blocks_.back();
        if (tail->TotalSize() < size)
            return -1;
        else if (tail->TotalSize() == size)
        {
            blocks_.pop_back();
            delete tail;
        }
        else if (tail->TotalSize() > size)
        {
            blocks_.pop_back();
            // 不要末尾size大小的字节数据
            PacketBlock* after_cutting_pkt = 
                PacketBlock::CuttingPacket(tail, 0, tail->TotalSize() - size);
        }



        return 0;
    }

    int PacketBuffer::Resize(size_t)
    {
        return 0;
    }

    int PacketBuffer::Append(PacketBuffer& dest)
    {
        return 0;
    }

    int PacketBuffer::Write(const unsigned char* data, size_t size)
    {
        if (data == nullptr || size == 0)
            return -1;


        size_t free_size = curr_block_ ? curr_block_->FreeSize() : 0;
        // 没有内存块可用
        if (curr_block_ == nullptr || free_size == 0)
            CreateBlock(size);

        data_size_ += size;
        free_size = curr_block_->FreeSize();
        if (free_size >= size)
        {
            curr_block_->SetData(size, data);
            return 0;
        }
        if (free_size != 0)
        {
            curr_block_->SetData(free_size, data);
            CreateBlock(size - free_size);
            curr_block_->SetData(size - free_size, data + free_size);
            return 0;
        }

        return -1;
    }

    int PacketBuffer::Read(unsigned char* dest, size_t size, PacketBlock* block)
    {
        if (dest == nullptr || size == 0 || index_ + size > total_size_)
            return -1;
        
        int start = 0;
        PacketBlock* curr_block = *blocks_.begin();
        if (index_ != 0)
        {
            int remaing_size = index_;
            while (remaing_size && curr_block != nullptr)
            {
                if (curr_block->DataSize() < remaing_size)
                {
                    remaing_size -= curr_block->DataSize();
                }
                else if (curr_block->DataSize() >= remaing_size)
                {
                    start += remaing_size;
                    remaing_size = 0;
                }

                curr_block = curr_block->next_;
            }
        }

        int curr_copy_size = curr_block->DataSize() - start;
        int index = 0;
        while (size && curr_block)
        {
            memcpy(dest + index, (char*)curr_block->GetDataPtr() + start, curr_copy_size);
            size -= curr_copy_size;
            curr_block = curr_block->next_;
        }
        
        return 0;
    }

    int PacketBuffer::Seek(int offset)
    {
        if (offset < 0)
        {
            int positive = 0 - offset;
            if (index_ < positive)
                return -1;
            index_ -= positive;
            return 0;
        }

        if (index_ + offset > total_size_)
            return -1;

        index_ += offset;
        return 0;
    }

    size_t PacketBuffer::DataSize()
    {
        return data_size_;
    }



    PacketBlock* PacketBuffer::CreateBlock(size_t size)
    {
        PacketBlock* block = nullptr;
        size_t alloc_mem_size = AlignUp<size_t>(size);

        block = AllocateBlock(alloc_mem_size);
        blocks_.push_back(block);
        block->SetPrevNode(curr_block_);
        curr_block_ = block;
        total_size_ += alloc_mem_size;

        return block;
    }
}