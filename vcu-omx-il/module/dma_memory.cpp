#include "dma_memory.h"
#include "dmaproxy.h"

#include <algorithm> // move
#include <cstring> // memset
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <utility/logger.h>

extern "C"
{
#include <lib_fpga/DmaAlloc.h>
#include <lib_fpga/DmaAllocLinux.h>
}

DMAMemory::DMAMemory(char const* device)
{
    printf("[myles]%s: going to open DMA device: %s.\n", __func__, device);
    fd = ::open(device, O_RDWR);

    if(fd < 0)
    {
    LOG_ERROR(::strerror(errno) + std::string { ": '" } +std::string { device } +std::string { "'. DMA channel is not available, CPU move will be performed" });
    }
}

DMAMemory::~DMAMemory()
{
  if(fd >= 0)
    ::close(fd);
}

// The function below is for debugging only (Myles)
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
int dump_stack(void)
{
        int j, nptrs;
        void *buffer[20];
        char ** strings;

        nptrs = backtrace(buffer,20);

        printf("backtrace() returned %d addresses\n",nptrs);

        strings = backtrace_symbols(buffer, nptrs);
        if (strings == NULL) {
                printf("error backtrace_symbols\n");
                return -1;
        }
        for(j = 0; j<nptrs; j++)
                printf("[%02d] %s\n",j,strings[j]);

        free(strings);
        return 0;
}

void DMAMemory::move(AL_TBuffer* destination, int destination_offset, AL_TBuffer const* source, int source_offset, size_t size)
{
    if(fd < 0)
    {
        std::move(AL_Buffer_GetData(source) + source_offset, AL_Buffer_GetData(source) + source_offset + size, AL_Buffer_GetData(destination) + destination_offset);
        return;
    }

    auto src_allocator = source->pAllocator;
    int src_fd = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)src_allocator, source->hBufs[0]);

    auto dst_allocator = destination->pAllocator;
    int dst_fd = AL_LinuxDmaAllocator_GetFd((AL_TLinuxDmaAllocator*)dst_allocator, destination->hBufs[0]);

    dmaproxy_arg_t dmaproxy {};
    dmaproxy.size = size;
    dmaproxy.dst_offset = destination_offset;
    dmaproxy.src_offset = source_offset;
    dmaproxy.src_fd = src_fd;
    dmaproxy.dst_fd = dst_fd;

    // printf("[myles]%s: size: %d, dst: 0x%lx (phy: 0x%lx), dst_offset: 0x%lx, src: 0x%lx (phy: 0x%lx), src_offset: 0x%lx, src_fd: %d, dst_fd: %d.\n", __func__, dmaproxy.size, AL_Buffer_GetData(destination), AL_Buffer_GetPhysicalAddress(destination), dmaproxy.dst_offset, AL_Buffer_GetData(source), AL_Buffer_GetPhysicalAddress(source), dmaproxy.src_offset, dmaproxy.src_fd, dmaproxy.dst_fd);

    // if (print_counter == 0){
    //     printf("[myles]%s: dump_stack before.\n", __func__);
    //     dump_stack();
    //     printf("[myles]%s: dump_stack after.\n", __func__);
    // }

    // std::move(AL_Buffer_GetData(source) + source_offset, AL_Buffer_GetData(source) + source_offset + size, AL_Buffer_GetData(destination) + destination_offset);

    // print_counter = 0;
    // ++moving_round_counter;
    // current_writing_ptr = 0;
    // // if ((moving_round_counter > 10) && (moving_round_counter < 28)) {
    // if (moving_round_counter < 18) {
    //     for (int temp_size_c = 0; temp_size_c < size; ++temp_size_c) {
    //         char_arr_4_print[current_writing_ptr++] = *(char*)(AL_Buffer_GetData(source) + source_offset + temp_size_c);

    //         // Slice seperation (0x100000) print (3-byte)
    //         if ((current_writing_ptr == 3) && (char_arr_4_print[0] == 0x00) && (char_arr_4_print[1] == 0x00) && (char_arr_4_print[2] == 0x01)) {
    //             char_arr_4_print[3] = 0x00; // generated extra 1-byte for just printing
    //             memcpy(&int_4_print, char_arr_4_print, 4);
    //             printf("[myles]%s: %dth, %dth 3-byte(total: %dth) (src: %d, src_offset: %d, offset: %d, size: %d): 0x%x.\n", __func__, moving_round_counter, ++print_counter, ++total_print_counter, source, source_offset, temp_size_c, size, int_4_print);
    //             current_writing_ptr = 0;
    //             continue;
    //         }

    //         // Print first valid 3 bytes
    //         if ((moving_round_counter == 3) && (print_counter == 2) && (current_writing_ptr == 3)) {
    //             for (; current_writing_ptr < 4; ++current_writing_ptr)
    //                 char_arr_4_print[current_writing_ptr] = 0;
    //             memcpy(&int_4_print, char_arr_4_print, 4);
    //             printf("[myles]%s: %dth, %dth 3-byte(total: %dth) (src: %d, src_offset: %d, offset: %d, size: %d): 0x%x.\n", __func__, moving_round_counter, ++print_counter, ++total_print_counter, source, source_offset, temp_size_c, size, int_4_print);
    //             current_writing_ptr = 0;
    //             continue;
    //         }

    //         if ((current_writing_ptr == 4) && (print_counter < 1000)) {
    //             memcpy(&int_4_print, char_arr_4_print, 4);
    //             printf("[myles]%s: %dth, %dth 4-byte(total: %dth) (src: %d, src_offset: %d, offset: %d, size: %d): 0x%x.\n", __func__, moving_round_counter, ++print_counter, ++total_print_counter, source, source_offset, temp_size_c, size, int_4_print);
    //             current_writing_ptr = 0;
    //         } else if ((temp_size_c + 1 == size) && (print_counter < 1000))
    //         {
    //             int temp_current_num_of_bytes = current_writing_ptr;
    //             for (; current_writing_ptr < 4; ++current_writing_ptr)
    //                 char_arr_4_print[current_writing_ptr] = 0;
    //             memcpy(&int_4_print, char_arr_4_print, 4);
    //             printf("[myles]%s: %dth, %dth %d-byte(total: %dth) (src: %d, src_offset: %d, offset: %d, size: %d): 0x%x.\n", __func__, moving_round_counter, ++print_counter, temp_current_num_of_bytes, ++total_print_counter, source, source_offset, temp_size_c, size, int_4_print);
    //             current_writing_ptr = 0;
    //             break;
    //         } else if (print_counter >= 1000)
    //             break;
    //     }
    // }

    if(::ioctl(fd, DMAPROXY_COPY, &dmaproxy) < 0)
    {
        LOG_WARNING(::strerror(errno) + std::string { ": DMA channel is not available, CPU move will be performed" });
        std::move(AL_Buffer_GetData(source) + source_offset, AL_Buffer_GetData(source) + source_offset + size, AL_Buffer_GetData(destination) + destination_offset);
        // printf("[myles]%s: cpu_move of size: %d success.\n", __func__, size);
    } else 
    {
        // printf("[myles]%s: dma_move of size: %d success.\n", __func__, size);
    }
}

void DMAMemory::set(AL_TBuffer* destination, int destination_offset, int value, size_t size)
{
  // To be optimized later if such DMA capability exists
  std::memset(AL_Buffer_GetData(destination) + destination_offset, value, size);
}

