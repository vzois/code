Short Description

To simulate the skyline implementation, we need to first generate the data, then compile 
the kernel and then execute the simulation using the kernel and data binaries.

The dataset are generated using a python script and C++ data generator. The data generator
resides in workload/randdataset-1.1.0/. Before trying to generate any data, we need to compile
the generator. We do so by going into workload/randdataset-1.1.0/ folder and typing the following
two commands

./configure --prefix=<full path to workload folder>
make install

Check the workload/randdataset-1.1.0/src/ folder for randdataset binary to verify it has been generated

The required dataset can be generated in two different ways. Just the data using the skydata.py script
or preprocessed data that are ready to be pumped into the simulator to execute the skyline algorithm.

If you want to generate the data, go into the skydata.py file and uncomment the following lines

#N = int(sys.argv[1])
#D = int(sys.argv[2])
#distr = sys.argv[3]
#genData(N,D,distr)

Once you do so, you can execute python skydata.py <N> <D> <distr> to generate the specific dataset
with size N, dimensions D, and distribution <distr>
E.g. python skydata.py 8192 8 i generates 8192 points with 8 dimension each following the independent
distribution. distr can take values c=correlated,i=independent,a=anticorrelated.

If you want to generate the data for testing with the simulator make sure that the previous lines are
commented out and use the dskyline.py script as such:
python dskyline.py.

This script generates the dataset with characteristics defined in the common/config.h file. The following
variables are used to manipulate the way data are generated and how they are preprocessed before being
written to a binary file that is going to be loaded in the simulator

#define DISTR "i" //Distribution choice// Possible values: c,i,a
#define DATA_N 4096//Number of total points
#define DPUS 1//Active DPU number
#define N (DATA_N / DPUS)//Size of data per DPU// Generally leave DPUS = 1 and play around with DATA_N to increase/decrease number of points
#define D 4//Dimensions of each point
#define PSIZE 256 //Partition size use to preprocess data into partitions//Generally leave it fixed for now
#define PSIZE_BYTES ((PSIZE*D) << 2)//Internal use for the dskyline_v2.h kernel
#define PSIZE_SHF 8//Internal use for the dskyline_v2.h kernel// Change accordingly to PSIZE//

Once you execute python dskyline.py the required data are generated and the dskyline algorithm is executed.
For debugging there are several information that are printed out.
1) The first two partitions and the skyline point discovered in them as well as the flags indicating which
points are alive in hex.
2) SFS is also executed as comparison, to verify the results are correct
3) Information about the stopping point and partition
4) Total number of full DTs and reduced number of full DTs resulting from using cheap bit vector testing

After this execution, we create a binary file called dsky.bin in runtime_data/ folder, storing the points
the levels, and bit vectors for each partition in order. This binary file will be used as input to the simulator.

Once dsky.bin has been generated, go into the skyline folder and type the following

make dsky

This will generate the .syms and .bin for the dskyline.c and dskyline_v2.h kernel.
Start the simulator and execute

file dskyline.bin dskyline.syms
load mram bin runtime_data/dsky.bin
boot
print mram 0x370F000 32

The last command prints the flags for the points in the first partition. You can
compare the values printed by dskyline.py with these values to check the results.
You can also move to the next partition 0x370F020 32 to check the values there too.





 



