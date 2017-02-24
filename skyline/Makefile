CC=dpucc

SDK_HOME=/home/vzois/git/code/sdk/
##
# Sequential radix select kernel
##
SRDX=kernels/radixselect.c
RDX_CONFIG=config/radixselect_x
SRDX_EXEC=radixselect
SSYMS=radixselect.syms

##
# Parallel radix select kernel
##
PRDX=kernels/pradixselect.c
PSYMS=pradixselect.syms
PRDX_EXEC=pradixselect

##
# Host compile name definition
##
HOST_INC = $(SDK_HOME)usr/share/upmem/include/host
HOST_CODE = main.c
HOST_EXEC = kss

##
# Compile kskyline window kernel
##
KSS_CONFIG = config/kss_x
KSS_EXEC = kss
KSS_SYMS = kss.syms
KSS = kernels/kskyline.c

##
# Merge Kernel
##
M_CONFIG=config/kskyline_merge_x
M_EXEC=kskyline_merge
M_SYMS=kskyline_merge.syms
M=kernels/kskyline_merge.c

##
#	dskyline kernel
##
DSKY=kernels/dskyline.c
DSKY_EXEC=dskyline
DSKY_SYMS=dskyline.syms
DSKY_CONFIG=dskyline_x

all: px16s kss16s merge16s hh
	
hh:
	gcc -Wall -O3 -I$(HOST_INC) $(HOST_CODE) -o $(HOST_EXEC) -ldpu -ldpusimcore -lpthread

dsky:
	$(CC) -b -C $(DSKY_CONFIG)16s.json -o $(DSKY_EXEC) -xs $(DSKY_SYMS) $(DSKY)

x16s:
	$(CC) -b -C $(RDX_CONFIG)16s.json -o $(RDX_EXEC) -xs $(SSYMS) $(SRDX)
	
px16s:
	$(CC) -b -C $(RDX_CONFIG)16s.json -o $(PRDX_EXEC) -xs $(PSYMS) $(PRDX)
	
kss16s:
	$(CC) -b -C $(KSS_CONFIG)16s.json -o $(KSS_EXEC) -xs $(KSS_SYMS) $(KSS)
	
merge16s:
	$(CC) -b -C $(M_CONFIG)16s.json -o $(M_EXEC) -xs $(M_SYMS) $(M)

single:
	$(CC) -b -C singleTasklet -o $(RDX_EXEC) -xs $(ASSEMBLY) $(RDX)
	    
clean:
	rm -rf kskyline_merge.bin
	rm -rf kskyline_merge.syms
	rm -rf kss.bin kss.syms
	rm -rf radixselect.bin radixselect.syms
	rm -rf pradixselect.bin pradixselect.syms
	rm -rf $(HOST_EXEC)
	rm -rf d_*
	rm -rf runtime_data/points.bin
	rm -rf runtime_data/sum.bin
	rm -rf candidates0.bin
	rm -rf dskyline.bin
	rm -rf dskyline.syms
	
	
	
	
	