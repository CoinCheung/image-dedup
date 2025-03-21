

#include "image_deduper.h"


using namespace std;



/* 
 *     === 
 *     === compile command:  
 *     === 
 *
 *         $ apt install libopencv-dev libssl-dev
 *         $ g++ -O2 main.cpp -std=c++14 -o main -lpthread -lcrypto $(pkg-config --libs --cflags opencv4)
 *
 *     === 
 *     === void dedup_one_dataset(string inpth);
 *     === 
 *
 *         Steps of this function:
 *             1. read image paths from inpth, and compute dhash codes of each one, also write results to disk
 *             2. count bits difference of dhash code between each pair of images, and obtain the pairs with difference less than 10, these pairs are considered as duplicated pairs. Write pairs of image to disk (only image index in the input file). 
 *             3. remove images according to the pairs, those with most connections with others are removed first
 *             4. write the deduplicated image paths back to disk
 *
 *         Take `inpth="images.txt"` as example:
 *             input file `images.txt` format: 
 *                 /path/to/image1
 *                 /path/to/image2
 *                 /path/to/image3
 *                 ...
 *    
 *             generated `images.txt.dhash` format: 
 *                 /path/to/image1,9065d85ab39396e400ffff484833ff04
 *                 /path/to/image2,9065d85ab39396e400ffff484833ff04
 *                 /path/to/image3,9065d85ab39396e400ffff484833ff04
 *                 ...
 *    
 *             generated `images.txt.pair` format: 
 *                 i=72737, j=62320, diff=0
 *                 i=15063, j=9259, diff=1
 *                 i=75546, j=8643, diff=4
 *                 ...
 *    
 *             generated `images.txt.dedup` format: 
 *                 /path/to/image1
 *                 /path/to/image2
 *                 /path/to/image3
 *                 ...
 *
 */

/*         
 *     === 
 *     === void merge_datasets(vector<string> inpths, string savepth);
 *     === 
 *
 *         This takes in multi duplicated files, deduplicates and merges the contents together, then saves into the savepth
 *         This requires deduplication has already been carried out within each input file. It is better that each file is deduplicated first with the function of `dedup_one_dataset` separately, then use this function to merge. 
 *
 *         input file format:
 *             /path/to/image1
 *             /path/to/image2
 *             /path/to/image3
 *             ...
 *
 *         output file format: 
 *             /path/to/image1
 *             /path/to/image2
 *             /path/to/image3
 *             ...
 *
 */

int main(int argc, char* argv[]) {

    ImageDeduper img_deduper;

    img_deduper.parse_args(argc, argv);
    img_deduper.run_cmd();

    // TODO: 
    // 1. add whash
    // 2. add pdq-hash
    // 3. refine compare speed, add function of nlogN, by sort or tree method.

    return 0;
}

