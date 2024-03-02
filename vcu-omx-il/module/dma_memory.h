#pragma once

#include "memory_interface.h"

struct DMAMemory final : MemoryInterface
{
    explicit DMAMemory(char const* device);
    ~DMAMemory() override;
    void move(AL_TBuffer* destination, int destination_offset, AL_TBuffer const* source, int source_offset, size_t size) override;
    void set(AL_TBuffer* destination, int destination_offset, int value, size_t size) override;

private:
    int fd;
  
    // Shiroha: for debugging only
    int is_fixed_bytes_skipped = 0;
    int print_counter = 0;
    int total_print_counter = 0;
    int moving_round_counter = 0;
    char char_arr_4_print[4];
    int current_writing_ptr = 0;
    int int_4_print;
};
