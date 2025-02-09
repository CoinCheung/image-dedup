
#
## apt install libopencv-dev pkg-config libssl-dev
#


rm ./run_dedup

export PKG_CONFIG_PATH=/opt/opencv/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/opt/opencv/lib:$LD_LIBRARY_PATH
g++ -O2 main.cpp -std=c++14 -o run_dedup -lpthread -lcrypto $(pkg-config --libs --cflags opencv4)

echo "compile done"

export n_proc=64
root_dir=../combine/
prefix=./image_list.txt

rm ${root_dir}/run_dedup
cp ./run_dedup ${root_dir}/


# pushd ${root_dir}/
#     rm ${prefix}.filt
#     rm ${prefix}.filt.md5
#     rm ${prefix}.filt.md5.dedup
#     rm ${prefix}.filt.md5.dedup.dhash
#     rm ${prefix}.filt.md5.dedup.dhash.dedup
#
    # rm ${prefix}.filt
    # rm ${prefix}.filt.md5
    # rm ${prefix}.filt.md5.dedup
    # rm ${prefix}.filt.md5.dedup.dhash
    # rm ${prefix}.filt.md5.dedup.dhash.pair
    # rm ${prefix}.filt.md5.dedup.dhash.dedup
    # rm ${prefix}.filt.md5.dedup.dhash.dedup.dhash
# popd

prefix=./in21k.txt
pushd ${root_dir}/
    rm ${prefix}.filt
    rm ${prefix}.filt.md5
    rm ${prefix}.filt.md5.dedup
    rm ${prefix}.filt.md5.dedup.dhash
    rm ${prefix}.filt.md5.dedup.dhash.pair
    rm ${prefix}.filt.md5.dedup.dhash.dedup
    rm ${prefix}.filt.md5.dedup.dhash.dedup.dhash

    time ./run_dedup filter $n_proc ${prefix}
    time ./run_dedup gen_md5 $n_proc ${prefix}.filt
    time ./run_dedup dedup_md5 $n_proc ${prefix}.filt.md5
    time ./run_dedup gen_dhash $n_proc ${prefix}.filt.md5.dedup
    time ./run_dedup dedup_dhash $n_proc ${prefix}.filt.md5.dedup.dhash
popd

