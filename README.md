# image-dedup

This is a codebase I use for deduplication of images in datasets. It is based on dhash.  


## Install dependencies 

OpenCV is required:  
```
    $ sudo apt install libopencv-dev  pkg-config
```


## Build 

Only one source file:  
```
    $ g++ -O2 main.cpp -std=c++14 -o run_dedup -lpthread $(pkg-config --libs --cflags opencv)
```


## Usage  

### 1. deduplicate images in one dataset  
Firstly, prepare a file contains paths to images in such format:  
```
    /path/to/image1
    /path/to/image2
    /path/to/image3
    ...
```
Store this file as `annos/images.txt`.
<br /><br />
Then run the command:  
```
    ## NOTE: do not change the order of the args
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup one_dataset $n_proc annos/images.txt
```
This would generate a `annos/images.txt.dedup`, which is in same format as `annos/images.txt` but only contains unique image paths.  


### 2. merge multiple deduplicated image datasets
We can also merge multiple datasets which has already been deduplicated, like this:  
```
    ## must run dedup on each datasets
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup one_dataset $n_proc annos/images_1.txt
    $ ./run_dedup one_dataset $n_proc annos/images_2.txt
    $ ./run_dedup one_dataset $n_proc annos/images_3.txt

    $ ./run_dedup merge $n_proc annos/images_1.txt annos/images_2.txt annos/images_3.txt annos/merged.txt
```
The generated `annos/merged.txt` is the file with merged image paths.  


