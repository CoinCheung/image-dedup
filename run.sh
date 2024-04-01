
#
## apt install libopencv-dev pkg-config libssl-dev
#


rm ./run_dedup

export PKG_CONFIG_PATH=/opt/opencv/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/opt/opencv/lib:$LD_LIBRARY_PATH
g++ -O2 main.cpp -std=c++14 -o run_dedup -lpthread -lcrypto $(pkg-config --libs --cflags opencv4)

echo "compile done"


export n_proc=128
root_dir=../../../../datasets/imagenet
prefix=./train.txt
rm ${root_dir}/run_dedup

cp ./run_dedup ${root_dir}/
pushd ${root_dir}/
    rm ${prefix}.md5
    rm ${prefix}.md5.dedup
    rm ${prefix}.md5.dedup.dhash
    rm ${prefix}.md5.dedup.dhash.dedup

    time ./run_dedup gen_md5 $n_proc ${prefix}
    time ./run_dedup dedup_md5 $n_proc ${prefix}.md5
    time ./run_dedup gen_dhash $n_proc ${prefix}.md5.dedup
    time ./run_dedup dedup_dhash $n_proc ${prefix}.md5.dedup.dhash
popd
