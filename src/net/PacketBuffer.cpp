#include "PacketBuffer.h"
#include "net_err.h"
#include "util.h"
#include <cstring>

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
                {
                    block->node_->next_ = first_block->node_;
                    first_block->node_->prev_ = block->node_;
                }
                first_block = block;
            }
            else 
            {
                if (first_block == nullptr)
                    first_block = block;
                block->size_ = curr_size;
                block->data_ = block->payload_;
                if (prev_block)
                {
                    prev_block->node_->next_ = block->node_;
                    block->node_->prev_ = prev_block->node_;
                }
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
        if (curr)
            delete curr;
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
        pos_ = 0;
        curr_block_ = list_.front();
        total_size_ = size;
        block_offset_ = curr_block_ ? curr_block_->data_ : nullptr;
    }

    PacketBuffer::~PacketBuffer()
    {
        for (auto beg = list_.begin(); beg != list_.end(); ++beg)
            (*beg)->Deallocate();
        list_.clear();
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
        if (dest == nullptr || src == nullptr)
            return NET_ERR_NULL;

        dest->list_.insert(dest->list_.end(), src->list_.begin(), src->list_.end());

        return NET_ERR_OK;
    }

    
    /**
     * @brief 将包的最开始size个字节配置成连续空间
     *      常用于对包头进行解析时,或者有其他选项字节时
     * 
     * @param size 
     * @return NetErr_t 
     */
    NetErr_t PacketBuffer::SetStartContinuous(int size)
    {
        if (size > total_size_)
        {
            // TODO: 日志输出,必须有足够的长度
            return NET_ERR_SIZE;
        }
        if (size > kPacketBlockSize)    // 超过1个POOL的大小则返回错误
        {
            // TODO: 大小太大,日志输出
            return NET_ERR_SIZE;
        }

        PacketBlock* first_block = list_.front();
        if (size <= first_block->size_)   // 已经处于连续空间不用处理
            return NET_ERR_OK;
        
        // 将第一个block的数据往前挪,以在尾部腾出size空间
        uint8_t* dest = first_block->payload_;
        for (int i = 0; i < first_block->size_; i++)
            *dest++ = first_block->data_[i];
        first_block->data_ = first_block->payload_;

        // 再依次将后续的空间搬到buf中,直到buf中的大小达到size
        int free_size = size - first_block->size_;
        PacketBlock* curr_block = GetPacketBlockNextNode(first_block);
        while (free_size && curr_block)
        {
            // 计算本次移动的数据量
            int curr_size = (curr_block->size_ > free_size) ? free_size : curr_block->size_;

            memcpy(dest, curr_block->data_, curr_size);
            dest += curr_size;
            curr_block->data_ += curr_size;
            curr_block->size_ += curr_size;
            first_block->size_ += curr_size;
            free_size -= curr_size;

            // 复制完后,curr_block可能没有数据,则释放掉,从后面一个包继续复制
            if (curr_block->size_ == 0)
            {
                PacketBlock* next_block = GetPacketBlockNextNode(curr_block);
                next_block->node_->prev_ = next_block->node_->next_;
                next_block->Deallocate();

                curr_block = next_block;
            }
        }
        return NET_ERR_OK;
    }


    NetErr_t PacketBuffer::Write(uint8_t* data, int size)
    {
        if (!data || size <= 0)
            return NET_ERR_PARAM;
        
        // 计算实际应该写入的数据量
        int free_size = TotalBlockFree();
        if (free_size < size)
        {
            // TODO: 日志输出: 剩余空间不满足
            return NET_ERR_SIZE;
        }

        while (size > 0)
        {
            int block_size = CurrBlockFree();
            int curr_copy = size > block_size ? block_size : size;
            memcpy(block_offset_, data, curr_copy);

            // 移动指针
            MoveForward(curr_copy);

            data += curr_copy;
            size -= curr_copy;
        }
        return NET_ERR_OK;
    }


    NetErr_t PacketBuffer::Read(uint8_t* dest, int size)
    {
        if (!dest || size <= 0)
            return NET_ERR_PARAM;

        int free_size = TotalBlockFree();
        if (free_size < size)
        {
            // TODO: 日志输出,剩余空间不足
            return NET_ERR_SIZE;
        }

        while (size > 0)
        {
            int block_size = CurrBlockFree();
            int curr_copy = size > block_size ? block_size : size;
            memcpy(dest, block_offset_, curr_copy);

            // 超出当前buf,移至下一个buf
            MoveForward(curr_copy);
            
            dest += curr_copy;
            size -= curr_copy;
        }

        return NET_ERR_OK;
    }

    NetErr_t PacketBuffer::Seek(int offset)
    {
        if (pos_ == offset)
            return NET_ERR_OK;

        if ((offset < 0) || (offset >= total_size_))
            return NET_ERR_SIZE;

        int move_bytes;
        if (offset < pos_)
        {
            // 从后往前移,定位到buf的最开头在开始移动
            curr_block_ = list_.front();
            block_offset_ = curr_block_->data_;
            pos_ = 0;

            move_bytes = offset;
        }
        else 
            // 往前移动,计算移动的字节量
            move_bytes = offset - pos_;
            
        // 不断移动位置,在移动过程中,主要调整total_offset,buf_offset和curr
        // offset的值可能大于余下的空间,此时只移动部分,但仍然正确
        while (move_bytes)
        {
            int free_size = CurrBlockFree();
            int curr_move = move_bytes > free_size ? free_size : move_bytes;

            // 往前移动,可能会超出当前的buf
            MoveForward(curr_move);
            move_bytes -= curr_move;
        }
        return NET_ERR_OK;
    }

    NetErr_t PacketBuffer::Copy(PacketBuffer* dest, PacketBuffer* src, int size)
    {
        if (dest->TotalBlockFree() < size || src->TotalBlockFree() < size)
            return NET_ERR_SIZE;

        while (size)
        {
            int dest_free = dest->CurrBlockFree();
            int src_free = src->CurrBlockFree();
            int copy_size = dest_free > src_free ? src_free : dest_free;
            copy_size = copy_size > size ? size : copy_size;

            memcpy(dest->block_offset_, src->block_offset_, copy_size);

            dest->MoveForward(copy_size);
            src->MoveForward(copy_size);
            size -= copy_size;
        }
        return NET_ERR_OK;
    }

    NetErr_t PacketBuffer::Fill(PacketBuffer* buf, uint8_t val, int size)
    {
        if (size <= 0)
            return NET_ERR_PARAM;

        int free_size = TotalBlockFree();
        if (free_size < size)
        {
            // TODO: 剩余空间不足,日志输出
            return NET_ERR_SIZE;
        }

        while (size > 0)
        {
            int block_size = CurrBlockFree();

            int curr_fill = size > block_size ? block_size : size;
            memset(block_offset_, val, curr_fill);

            MoveForward(curr_fill);
            size -= curr_fill;
        }
        return NET_ERR_OK;
    }   



    void PacketBuffer::MoveForward(int size)
    {
        pos_ += size;
        block_offset_ += size;

        // 可能超出了当前块,所以要移动到下一个块
        if (block_offset_ >= curr_block_->data_ + curr_block_->size_)
        {
            curr_block_ = GetPacketBlockNextNode(curr_block_);
            if (curr_block_)
                block_offset_ = curr_block_->data_;
            else
                block_offset_ = nullptr;
        }
    }


    int PacketBuffer::TotalBlockFree()
    {
        return total_size_ - pos_;
    }

    int PacketBuffer::CurrBlockFree()
    {
        if (curr_block_ == nullptr)
            return 0;

        return (int)(curr_block_->data_ + curr_block_->size_ - block_offset_);
    }


    uint16_t PacketBuffer::CheckSum(int size, uint32_t prev_sum, int complement)
    {
        int free_size = TotalBlockFree();
        if (free_size < size)
        {
            // TODO: 日志输出
            return 0;
        }

        // 不断16位累加数据区
        uint32_t sum = prev_sum;
        uint32_t offset = 0;
        while (size > 0)
        {
            int block_size = CurrBlockFree();
            int curr_size = (block_size > size ? size : block_size);
            sum = util::Checksum16(offset, block_offset_, curr_size, sum, 0);
            MoveForward(curr_size);
            size -= curr_size;
            offset += curr_size;
        }
        return complement ? (uint16_t)~sum : (uint16_t)sum;
    }
}