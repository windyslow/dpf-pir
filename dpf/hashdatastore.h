#pragma once
#include <vector>
#include <cstdint>
#include <x86intrin.h>
#include "alignment_allocator.h"
#include <string>
#include <cassert>

class hashdatastore
{
public:
    typedef __m256i hash_type;

    hashdatastore() = default;

    void reserve(size_t n) { data_.reserve(n); }
    void push_back(std::string keyword_str, std::string data_str)
    {
        hash_type data = string2m256i(data_str);
        data_.push_back(data);
        size_t keyword = string2uint64(keyword_str);
        keyword_.push_back(keyword);
    }

    void init(int i)
    {
        data_s.resize(i);
    }

    void push_back(std::string keyword_str, std::vector<std::string> data_str_s, int k)
    {
        hash_type data;
        for(int j=0; j<k;j ++){
            data = string2m256i(data_str_s[j]);
            data_s[j].push_back(data);
        }
        size_t keyword = string2uint64(keyword_str);
        keyword_.push_back(keyword);
    }
    void push_back(const hash_type &data) { data_.push_back(data); }
    void push_back(hash_type &&data) { data_.push_back(data); }

    void dummy(size_t n) { data_.resize(n, _mm256_set_epi64x(1, 2, 3, 4)); }

    size_t size() const { return data_.size(); }

    hash_type answer_pir1(const std::vector<uint8_t> &indexing) const;
    hash_type answer_pir2(const std::vector<uint8_t> &indexing) const;
    hash_type answer_pir2_s(const std::vector<uint8_t> &indexing, int k) const;
    hash_type answer_pir3(const std::vector<uint8_t> &indexing) const;
    hash_type answer_pir4(const std::vector<uint8_t> &indexing) const;
    hash_type answer_pir5(const std::vector<uint8_t> &indexing) const;
    hash_type answer_pir_idea_speed_comparison(const std::vector<uint8_t> &indexing) const;

private:
    hash_type string2m256i(std::string data_str)
    {
        assert(data_str.size() <= 32);
        size_t fin[4] = {};
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 8 && (i * 8 + j) < data_str.size(); j++)
            {
                fin[i] = fin[i] + static_cast<uint8_t>(data_str[i * 8 + j]) * (1ULL << 8 * j);
            }
        }
        return _mm256_set_epi64x(fin[0], fin[1], fin[2], fin[3]);
    }
    std::string m256i2string(hashdatastore::hash_type value)
    {
        std::string result;
        size_t re0 = _mm256_extract_epi64(value, 3);
        size_t re1 = _mm256_extract_epi64(value, 2);
        size_t re2 = _mm256_extract_epi64(value, 1);
        size_t re3 = _mm256_extract_epi64(value, 0);
        for (int j = 0; j < 8; j++)
        {
            unsigned char byte = (re0 >> (j * 8)) & 0xFF;
            result += static_cast<char>(byte);
        }
        for (int j = 0; j < 8; j++)
        {
            unsigned char byte = (re1 >> (j * 8)) & 0xFF;
            result += static_cast<char>(byte);
        }
        for (int j = 0; j < 8; j++)
        {
            unsigned char byte = (re2 >> (j * 8)) & 0xFF;
            result += static_cast<char>(byte);
        }
        for (int j = 0; j < 8; j++)
        {
            unsigned char byte = (re3 >> (j * 8)) & 0xFF;
            result += static_cast<char>(byte);
        }
        return result;
    }
    size_t string2uint64(std::string keyword_str)
    {
        size_t result = 0;
        for (int i = 0; i < 8 && i < keyword_str.size(); i++)
        {
            result |= static_cast<size_t>(keyword_str[i]) << (8 * i);
        }
        return result;
    }

public:
    std::vector<size_t> keyword_;
    std::vector<std::vector<hash_type, AlignmentAllocator<hash_type, sizeof(hash_type)>>> data_s;
private:
    std::vector<hash_type, AlignmentAllocator<hash_type, sizeof(hash_type)>> data_;

};
