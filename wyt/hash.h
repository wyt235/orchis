#ifndef _WYT_HASH_H
#define _WYT_HASH_H

#include <string>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <bits/hash_bytes.h>
namespace wyt{


template <typename T>
struct HashCalc{
    uint64_t operator()(const T& data, uint64_t seed){
        return static_cast<uint64_t>(std::_Hash_bytes(&data, sizeof(data), seed));
    }
};

template <>
struct HashCalc<std::string>{
    uint64_t operator()(const std::string& data, uint64_t seed){
        return static_cast<uint64_t>(
            std::_Hash_bytes(data.c_str(), data.length(), seed)
        );
    }
};


template<typename _Key, typename _Value, typename _Hash = HashCalc<_Key>>
class Hash{
private:
    /// @brief hash桶，用户存储数据需要追加在该结构末尾的位置
    struct HashBucket{
        uint64_t hash; //哈希值
        HashBucket* next; //相同位置的下一个元素，用于解决hash冲突
        _Key   key;
        _Value data;
    };

    /// @brief 获取hash值所在hash bucket索引
    inline uint64_t getBucketIndex(uint64_t hash){
        int width;
        uint64_t idx;
        width = m_logical_exponent;
        idx = (((uint64_t)(~0)) >> (64 - width) & hash);
        if (idx < m_split) {
            width += 1;
            idx = (((uint64_t)(~0)) >> (64 - width) & hash);
        }
        return idx;
    }

    static void deleteBucket(HashBucket* bucket){
        HashBucket* tmp;
        while(bucket != nullptr){
            tmp = bucket->next;
            delete bucket;
            bucket = tmp;
        }
    }

    static HashBucket* copyBucket(HashBucket* bucket){
        HashBucket head, *iterator, *prev;
        if(bucket == nullptr) return nullptr;
        prev = &head;
        iterator = new HashBucket;
        while(bucket != nullptr){
            prev = prev->next = iterator;
            iterator->data = bucket->data;
            iterator->hash = bucket->hash;
            iterator->key = bucket->key;
            bucket = bucket->next;
        }
        iterator->next = nullptr;
        return head.next;
    }
    

public:
    Hash() : Hash(8){}
    Hash(int size){
        int real_size, exponent;        
        exponent = 3; //确定最小逻辑大小为8，最小实际大小为16
        while( (1 << exponent) < size){
            exponent += 1;
        }
        real_size = 2 << exponent;
        this->m_buckets = new HashBucket* [real_size];
        bzero(this->m_buckets, sizeof(HashBucket*) * real_size);
        this->m_split = 0;
        this->m_logical_exponent = exponent;
        this->m_seed = (uint64_t)this;
    }
    ~Hash(){
        clean();
        delete[] m_buckets;
    }

    Hash(const Hash & other){
        int i, real_size;
        real_size = 2 << other.m_logical_exponent;
        this->m_logical_exponent = other.m_logical_exponent;
        this->m_seed = other.m_seed;
        this->m_split = other.m_split;
        this->m_buckets = new HashBucket*[real_size];
        for(i = 0; i < real_size; ++i){
            this->m_buckets[i] = copyBucket(other.m_buckets[i]);
        }
    }

    Hash(Hash && other){
        this->m_buckets = other.m_buckets;
        this->m_logical_exponent = other.m_logical_exponent;
        this->m_seed = other.m_seed;
        this->m_split = other.m_split;
        other.m_buckets = nullptr;
        other.m_split = 0;
        other.m_logical_exponent = 0;
        other.m_seed = 0;
    }

public:
    _Value& operator[](const _Key& key){
        uint64_t idx;
        _Hash hashCalc;
        HashBucket *bucket, *find, *prev;
        assert(this->m_buckets != nullptr);

        find = this->find(key, idx, &prev);
        if(find != nullptr) return find->data;

        if(m_split >= (uint32_t)(1 << m_logical_exponent))
            this->realloc();

        bucket = new HashBucket();
        bucket->hash = hashCalc(key, m_seed);
        bucket->key = key;

        if(prev != nullptr){
            bucket->next = prev->next;
            prev->next = bucket;
        }else{
            bucket->next = nullptr;
            m_buckets[idx] = bucket;
        }
        return bucket->data;
    }

private:

    /**
     * @brief 拓展容量
     * @return false -> 拓展失败
     */
    void realloc(){
        int size;
        HashBucket **buckets;
        size = 2 << m_logical_exponent; //实际大小
        
        buckets = new HashBucket* [size << 1];
        memset(buckets, 0, sizeof(HashBucket*) * (size << 1));
        memcpy(buckets, m_buckets, sizeof(HashBucket*) * size);
        delete[] m_buckets;
        m_buckets = buckets;
        m_logical_exponent += 1;
        m_split = 0;
    }

    void split(){
        /**
        * 该算法结束后，table的分裂指针向后移动一位.
        * 若分裂指针需要后移，则当前分裂指针指向的链表，都位于分裂后的区域
        * 但是，链表内元素的位置之前是通过对逻辑空间大小取余数得到，并不符合算法要求
        * 因此该算法的本质是将table的split指针指向的链表重新分配到正确的位置上。
        */
        uint64_t idx;
        int old_idx, new_idx, size, real_exponent;
        HashBucket *itor, *next;
        HashBucket old_sentry, new_sentry;

        old_sentry.next = new_sentry.next = NULL;

        old_idx = m_split; //旧的索引指向
        size = 1 << m_logical_exponent; //逻辑大小
        new_idx = old_idx + size;
        real_exponent = m_logical_exponent + 1; //真实内存空间的指数
        next = m_buckets[m_split];

        while ((itor = next)) {
            next = itor->next;
            idx = (((uint64_t)(~0)) >> (64 - real_exponent) & itor->hash);
            if (idx == old_idx) {
                itor->next = old_sentry.next;
                old_sentry.next = itor;
            }
            else {
                itor->next = new_sentry.next;
                new_sentry.next = itor;
            }
        }
        m_buckets[old_idx] = old_sentry.next;
        m_buckets[new_idx] = new_sentry.next;
        m_split += 1;
    }

    HashBucket* find(const _Key& key, uint64_t& idx, HashBucket** prev){
        _Hash hashCalc;
        HashBucket *ret;
        *prev = nullptr;
        idx = getBucketIndex(hashCalc(key, m_seed));
        ret = m_buckets[getBucketIndex(hashCalc(key, m_seed))];
        while(ret != nullptr){
            if(ret->key == key) break;
            *prev = ret;
            ret = ret->next;
        };
        return ret;
    }

public:
    void clean(){
        int i, size;
        if(m_buckets == nullptr) return;
        for(i = 0, size = 2 << m_logical_exponent; i < size; ++i){
            deleteBucket(m_buckets[i]);
        }
        this->m_split = 0;
    }

public:
    bool insert(const _Key & key, const _Value& value){
        uint64_t idx;
        _Hash hashCalc;
        HashBucket *bucket, *find, *prev;
        assert(this->m_buckets != nullptr);

        find = this->find(key, idx, &prev);
        if(find != nullptr) return false;

        if(m_split >= (uint32_t)(1 << m_logical_exponent))
            this->realloc();

        bucket = new HashBucket();
        bucket->hash = hashCalc(key, m_seed);
        bucket->key = key;
        bucket->data = value;

        if(prev != nullptr){
            bucket->next = prev->next;
            prev->next = bucket;
        }else{
            bucket->next = nullptr;
            m_buckets[idx] = bucket;
        }
        return true;
    }

    void remove(const _Key & key){
        uint64_t idx;
        HashBucket *find, *prev;
        assert(this->m_buckets != nullptr);

        if((find = this->find(key, idx, &prev)) == nullptr) return;
        if(prev){
            prev->next = find->next;
        }else{
            assert(m_buckets[idx] == find);
            m_buckets[idx] = find->next;
        }
        delete find;
    }

    bool modify(const _Key & key, const _Value& value){
        uint64_t idx;
        HashBucket *find, *prev;
        assert(this->m_buckets != nullptr);

        if((find = this->find(key, idx, &prev)) == nullptr){
            return this->insert(key, value);
        }else{
            find->data = value;
        }
        return true;
    }

    bool search(const _Key & key, _Value& value){
        uint64_t idx;
        HashBucket *find, *prev;
        assert(this->m_buckets != nullptr);

        if((find = this->find(key, idx, &prev)) == nullptr)
            return false;
        value = find->data;
        return true;
    }

private:
    uint32_t        m_logical_exponent;//用于逻辑空间大小的计算，计算规则为 1 << m_logical_exponent
    uint32_t        m_split;         //分裂指针
    uint64_t        m_seed;        //计算hash值的随机种子
    HashBucket**    m_buckets;     //初始化一定要置空
};

}



#endif