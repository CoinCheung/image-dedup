
#
## apt install libopencv-dev pkg-config libssl-dev
#


rm ./run_dedup

SRC="main.cpp image_deduper.cpp topology.cpp hash_func.cpp samples.cpp image_filter.cpp utils.cpp"

export PKG_CONFIG_PATH=/opt/opencv/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/opt/opencv/lib:$LD_LIBRARY_PATH
g++ -O2 $SRC -std=c++20 -o run_dedup -lpthread -lcrypto $(pkg-config --libs --cflags opencv4)


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


prefix=./tmp/in1k.txt
pushd ${root_dir}/

    # rm ${prefix}.filt
    # time ./run_dedup filter $n_proc ${prefix}

    # rm ${prefix}.filt.md5
    # time ./run_dedup gen_md5 $n_proc ${prefix}.filt

    # rm ${prefix}.filt.md5.dedup
    # time ./run_dedup dedup_md5 $n_proc ${prefix}.filt.md5

    # rm ${prefix}.filt.md5.dedup.phash
    # time ./run_dedup gen_phash $n_proc ${prefix}.filt.md5.dedup

    # rm ${prefix}.filt.md5.dedup.phash.pair
    # rm ${prefix}.filt.md5.dedup.phash.dedup
    # rm ${prefix}.filt.md5.dedup.phash.dedup.phash
    # time ./run_dedup dedup_phash $n_proc ${prefix}.filt.md5.dedup.phash

    # rm ${prefix}.filt.md5.dedup.dhash
    # time ./run_dedup gen_dhash $n_proc ${prefix}.filt.md5.dedup

    # rm ${prefix}.filt.md5.dedup.dhash.pair
    # rm ${prefix}.filt.md5.dedup.dhash.dedup
    # rm ${prefix}.filt.md5.dedup.dhash.dedup.dhash
    # time ./run_dedup dedup_dhash $n_proc ${prefix}.filt.md5.dedup.dhash


    # time ./run_dedup merge_dhash $n_proc ./tmp/merge/new/in1k.txt.filt.md5.dedup.dhash ./tmp/merge/new/in21k_part00.dhash ./tmp/merge/new/in21k_part01.dhash ./tmp/merge/new/res.dhash

    time ./run_dedup pipeline $n_proc ./tmp/pipeline/in1k.txt 

popd

