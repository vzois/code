
for i in {1..1}
do

make clean 
python py_algo/dskyline.py
make dsky

echo "Running Simulation!!!" 
dpushell -s sim_scripts/dsky_test.sh > sim.out
echo "Results stored in sim.out!!!"

python acc_cycles.py

done

rm -rf d_*