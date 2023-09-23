
#
## apt install libopencv-dev pkg-config
#

rm ./run_dedup

g++ -O2 main.cpp -std=c++14 -o run_dedup -lpthread $(pkg-config --libs --cflags opencv)

export n_proc=64

time ./run_dedup one_dataset $n_proc ./annos/tmp_1.txt
time ./run_dedup one_dataset $n_proc ./annos/tmp_2.txt
time ./run_dedup one_dataset $n_proc ./annos/tmp_3.txt
time ./run_dedup one_dataset $n_proc ./annos/tmp_4.txt

time ./run_dedup merge $n_proc ./annos/tmp_1.txt ./annos/tmp_2.txt ./annos/tmp_3.txt ./annos/tmp_4.txt ./annos/merge.txt
