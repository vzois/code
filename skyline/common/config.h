#ifndef CONFIG_H
#define CONFIG_H

#define MIN(x,y) (x > y ? y : x)
#define MAX(x,y) (x > y ? x : y)

//Dataset Configuration
#define DISTR "i"
#define DATA_N 8192
#define DPUS 1
#define N (DATA_N / DPUS)
#define D 4
#define PSIZE 256 //Partition size
#define PSIZE_BYTES ((PSIZE*D) << 2)
#define PSIZE_SHF 8
#define L 2 // Partition level
#define P (DATA_N / PSIZE) // Partition number
#define K 128
#define MERGE_CHUNK 512
#define BVECTOR_N (N >> 5) // N/32 Bit vectors

////////////////////////////////////////////////
//radixselect Runtime Configuration
#define BUFFER 1024
#define BINS 17
#define TASKLETS 16
#define TASKLETS_SHF 4
#define ACC_BUCKET 0

//Radix Select Addresses
#define DATA_ADDR 0x1000
#define BUCKET_ADDR (DATA_ADDR + (N << 2))

//Radix Select Type
#define CSELECT 0x1000
#define CWINDOW 0x1001

////////////////////////////////////////////
//kskyline Runtime Configuration
////////////////////////////////////////////
#define WRAM_BUFFER 1024 //Part of the buffer for each tasklet to store the window// 16 tasklets = 16*256 bytes // Window cannot be larger that 4096 bytes
#define WRAM_BUFFER_SHF 10 //log256 = 8
#define WINDOW_SIZE (K*D)
#define WINDOW_SIZE_BYTES (WINDOW_SIZE << 2) //Total window size//Assume that points fit in WRAM
#define TASKLET_LOAD (WINDOW_SIZE_BYTES >> WRAM_BUFFER_SHF) //Which tasklets participate in the window load//Determines also size of shared wram window

#define VALUES_PER_BUFFER (WRAM_BUFFER >> 2)//Values that can fit into buffer // 256 bytes -> 64 32 bit values
#define POINTS_PER_BUFFER (VALUES_PER_BUFFER / D)//How many points fit into a single buffer load // Less points as we increase point dimensions//

//kskyline Addresses
#define POINTS_ADDR 0x1000
#define WINDOW_ADDR (POINTS_ADDR + ((N * D)<<2))
#define RANK_ADDR (WINDOW_ADDR + ((K * D)<<2))
#define MASK_ADDR 0x3000000


////////////////////////////////////////////
//dskyline Runtime Configuration
////////////////////////////////////////////
#define DSKY_POINTS_ADDR 0x3000// Starting offset of point partitions
#define DSKY_RANK_ADDR (DSKY_POINTS_ADDR + ((N * D)<<2))//Precomputed rank for each point within each partition
#define DSKY_BVECS_ADDR (DSKY_RANK_ADDR +  (N << 2))
#define DSKY_FLAGS_ADDR 0x370F000//Flags to determine which points are alive or not

#define DSKY_REMOTE_PSTOP_ADDR 0x373F000//global stop point
#define DSKY_REMOTE_FLAGS_ADDR (DSKY_REMOTE_PSTOP_ADDR + 4)//flags for current partition window
#define DSKY_REMOTE_BVECS_ADDR (DSKY_REMOTE_FLAGS_ADDR + ((PSIZE/32) << 2))//bit vectors of remote partition
#define DSKY_REMOTE_POINTS_ADDR (DSKY_REMOTE_BVECS_ADDR + (PSIZE << 2))//Remote partition during communication stage

#define POINTS_PER_T (PSIZE / TASKLETS)
#define POINTS_PER_T_VALUES ( POINTS_PER_T * D )
#define POINTS_PER_T_BYTES ( POINTS_PER_T_VALUES << 2)
#define POINTS_PER_T_MSK ( (0x1 << POINTS_PER_T) - 1 )
#define PSIZE_POINTS_VALUES (PSIZE * D)



////////////////////////////////////////////
//merging Runtime Configuration
////////////////////////////////////////////

//General Runtime Addresses
#define VAR_0_ADDR 0x0
#define VAR_1_ADDR 0x4
#define VAR_2_ADDR 0x8
#define VAR_3_ADDR 0xC
#define VAR_4_ADDR 0x10
#define VAR_5_ADDR 0x14
#define VAR_6_ADDR 0x18
#define VAR_7_ADDR 0x1C

#endif
