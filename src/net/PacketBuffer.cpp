#include "PacketBuffer.h"
#include "net_err.h"
namespace net 
{
    inline PacketBlock* GetPacketBlockLastNode(PacketBlock* block)
    {
        Node* tail = block->node_;
        while (tail)
            tail = tail->next_;
        return reinterpret_cast<PacketBlock*>(tail);
    }

    inline PacketBlock* GetPacketBlockFirstNode(PacketBlock* block)
    {
        return reinterpret_cast<PacketBlock*>(block->node_);
    }

    inline PacketBlock* GetPacketBlockNextNode(PacketBlock* block)
    {
        Node* next = reinterpret_cast<Node*>(block)->next_;
        return reinterpret_cast<PacketBlock*>(next);
    }

///////////////////////////// PacketBlock
    PacketBlock* PacketBlock::Allocate(int size, bool head_insertion)
    {
        if (size <= 0)
            return nullptr;

        PacketBlock* first_block = nullptr, *prev_block = nullptr;

        while (size)
        {
            PacketBlock* block = new PacketBlock;
            if (block == nullptr)
            {
                // TODO: 错误日志,内存分配不足
                block->Deallocate();
                return nullptr;
            }

            // 获取当前block能存储的空间大小
            // kPacketBlockSize: 当前block能存储的字节大小
            int curr_size = size > kPacketBlockSize ? kPacketBlockSize : size;
            if (head_insertion) // 头插
            {
                block->size_ = curr_size;
                block->data_ = block->payload_ + kPacketBlockSize - curr_size;
                if (first_block)
                    block->node_->next_ = first_block->node_;
                first_block = block;
            }
            else 
            {
                if (first_block == nullptr)
                    first_block = block;
                block->size_ = curr_size;
                block->data_ = block->payload_;
                if (prev_block)
                    prev_block->node_->prev_ = block->node_;
            }
            size -= curr_size;
            prev_block = block;
        }
        return first_block;
    }

    void PacketBlock::Deallocate()
    {
        Node* curr = node_;
        while (curr)
        {
            Node* next = curr->next_;
            delete curr;
            curr = next;
        }
        node_ = nullptr;
        data_ = nullptr;
        size_ = 0;
    }
    

    void PacketBlock::RemoveFirstNode()
    {
        Node* head = node_;
        node_ = head->next_;
        if (node_)
            node_->prev_ = nullptr;
        if (head)
            delete head;
    }

 ///////////////////////////// PacketBuffer
    PacketBuffer::PacketBuffer(int size)
    {
        PacketBlock* ret = nullptr;
        if (size)
        {
            ret = PacketBlock::Allocate(size);
            if (ret == nullptr)
                return;
            list_.push_back(ret);
        }
        ref_ = 1;
        curr_block_ = list_.back();
        total_size_ = size;
        block_offset_ = curr_block_ ? curr_block_->data_ : nullptr;
    }

    PacketBuffer::~PacketBuffer()
    {
        if (--ref_ == 0)
        {
            for (auto beg = list_.begin(); beg != list_.end(); ++beg)
                (*beg)->Deallocate();
            list_.clear();
        }
    }


    NetErr_t PacketBuffer::AddHeader(int size, bool continuous)
    {
        PacketBlock* block = list_.front();

        // 剩余空闲空间大小
        int free_size = (int)(block->data_ - block->payload_);
        if (size <= free_size)  // 空间足够
        {
            block->size_ += size;
            block->data_ -= size;
            total_size_ += size;
            return NET_ERR_OK;
        }

        // 没有足够空间,需要额外分配块添加到头部,第一个块即使有空间也用不了
        if (continuous) // 连续分配
        {
            if (size > kPacketBlockSize)    // 如果要求连续分配,且大小超过一个块则无法分配
            {
                // TODO: 日志输出: 无法连续分配
                return NET_ERR_NO_MEM;
            }
        }
        else // 不连续分配
        {
            block->data_ = block->payload_;
            block->size_ += free_size;
            total_size_ += free_size;
            size -= free_size;

            // 再分配其他未用的空间
            block = PacketBlock::Allocate(size);
            if (!block)
            {
                // TODO: 日志输出: 没内存
                return NET_ERR_NO_MEM;
            }
        }

        list_.push_front(block);
        return NET_ERR_OK;
    }



    NetErr_t PacketBuffer::RemoveHeader(int size)
    {
        PacketBlock* block = list_.front();
        while (size)
        {
            PacketBlock* next_block = (PacketBlock*)block->node_->next_;
            
            if (size < block->size_)    // 当前包足够减去头
            {
                block->data_ += size;
                block->size_ -= size;
                total_size_ -= size;
                break;
            }

            // 当前包不够减去头
            int curr_size = block->size_;
            list_.pop_front();
            block->Deallocate();

            size -= curr_size;
            total_size_ -= curr_size;
            block = next_block;
        }

        return NET_ERR_OK;
    }


    NetErr_t PacketBuffer::Resize(int size)
    {
        if (size == total_size_)
            return NET_ERR_OK;

        if (total_size_ == 0)
        {
            PacketBlock* block = PacketBlock::Allocate(size);
            if (!block)
            {
                // TODO: 日志输出,内存不足
                return NET_ERR_NO_MEM;
            }
            list_.push_back(block);
        }
        else if (size > total_size_)
        {
            // 扩展尾部, 不考虑最后一个块头部有空间的情况
            PacketBlock* tail_block = list_.back();

            int inc_size = size - total_size_;
            int free_size = kPacketBlockSize - (int)(tail_block->data_ - tail_block->payload_)
                - tail_block->size_;
            if (free_size >= inc_size)
            {
                tail_block->size_ += inc_size;
                total_size_ += inc_size;
            }
            else 
            {
                PacketBlock* new_block = PacketBlock::Allocate(inc_size - free_size, false);
                if (new_block == nullptr)
                {
                    // TODO: 日志输出: 内存不足
                    return NET_ERR_NO_MEM;
                }

                tail_block->size_ += free_size;
                total_size_ += free_size;
                list_.push_back(new_block);
            }
        }
        else 
        {
            // TODO:

            total_size_ = size;
        }
        return NET_ERR_OK;
    }


    NetErr_t PacketBuffer::Append(PacketBuffer* dest, PacketBuffer* src)
    {
        PacketBlock* first;
        return NET_ERR_OK;
    }
}