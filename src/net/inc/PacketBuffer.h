#pragma once

#include "net_err.h"
#include <list>
#include <cstdint>


namespace net 
{
    const int kPacketBlockSize = 1024;
    struct Node 
    {
        Node* next_ = nullptr;
        Node* prev_ = nullptr;
    };

    struct PacketBlock
    {
        static PacketBlock* Allocate(int size, bool head_insertion = true);
        void Deallocate();
        void RemoveFirstNode();
    public:
        int size_           = 0;
        Node* node_         = nullptr;
        uint8_t* data_      = nullptr;    
        uint8_t payload_[kPacketBlockSize]; // 内部存储空间
    };

    class PacketBuffer
    {
    public:
        PacketBuffer(int size);
        ~PacketBuffer();
    public:
        NetErr_t AddHeader(int size, bool continuous = true);
        NetErr_t RemoveHeader(int size);
        NetErr_t Resize(int size);
        NetErr_t Append(PacketBuffer* dest, PacketBuffer* src);
        NetErr_t SetStartContinuous(int size);
        NetErr_t Write(uint8_t* data, int size);
        NetErr_t Read(uint8_t* dest, int size);
        NetErr_t Seek(int offset);
        NetErr_t Copy(PacketBuffer* dest, PacketBuffer* src, int size);
        NetErr_t Fill(PacketBuffer* buf, uint8_t val, int size);
    private:
        void MoveForward(int size);
        void IncRef();
        int TotalBlockFree();
        int CurrBlockFree();
        uint16_t CheckSum(int size, uint32_t prev_sum, int complement);
    private:
        std::list<PacketBlock*> list_;
        Node* next_pkt_                 = nullptr;    
        int total_size_                 = 0;
        int ref_                        = 0;        // 引用计数
        int pos_                        = 0;        // 当前位置总的偏移量
        PacketBlock* curr_block_        = nullptr;  // 当前指向的buf
        uint8_t* block_offset_          = nullptr;  // 在当前buf中的偏移量
    };
}