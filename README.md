# image-dedup

This is a codebase I use for deduplication of images in datasets. It is based on dhash.  


## Install dependencies 

OpenCV is required:  
```
    $ sudo apt install libopencv-dev pkg-config libssl-dev
```


## Build 

Only one source file:  
```
    $ g++ -O2 main.cpp -std=c++14 -o run_dedup -lpthread -lcrypto $(pkg-config --libs --cflags opencv)
```


## Usage  

### filter by quality

Here we can filter out some images that is considered to be not so qualified. The rules are:<br>
* postfix is jpg/jpeg, but file neither starts with `0xff0xd8`, nor ends with `0xff0xd9`.
* postfix is png, but file neither starts with `89 50 4e 47 0d 0a 1a 0a`, nor ends with `49 45 4e 44 ae 42 60 82`.
* file size is less than 50k.
* shorter side of the image is less than 64.
* longer side of the image is greater than 2048.
* image channel number is not 3.
* ratio of longer side and shorter side is greater than 4.
<br>
Though these rules does not always point to low-quality images, I just find it is helpful for me picking out images from huge amount of images.<br>

If one needs to carry out the filtering as above mentioned, one should prepare a file `annos/images.txt` that contains paths to images in such format:
```
    /path/to/image1
    /path/to/image2
    /path/to/image3
    ...
```
and then run the command:
```
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup filter $n_proc annos/images.txt
```
This would generate a `annos/images.txt.filt` with same format as the `annos/images.txt`.


### dedup by md5
#### Step 1. generate all md5
Firstly, prepare a file contains paths to images in such format:  
```
    /path/to/image1
    /path/to/image2
    /path/to/image3
    ...
```
Suppose we store this file as `annos/images.txt`.
<br /><br />
Then we run the command:  
```
    ## NOTE: do not change the order of the args
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup gen_md5 $n_proc annos/images.txt
```
This would generate a `annos/images.txt.md5`, which is in the format of: 
```
    /path/to/image1,md5value1
    /path/to/image2,md5value2
    /path/to/image3,md5value3
    ...
```

#### Step 2. deduplicate via md5
This step is mean for filtering out identical files(with every bits identical). Just run command:
```
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup dedup_md5 annos/images.txt.md5
```
This would generate a `annos/images.txt.md5.dedup`, and each line is a pure path to a file like this:
```
    /path/to/image1
    /path/to/image2
    /path/to/image3
    ...
```


### dedup by dhash
#### Step 1. generate dhash codes of images in one dataset  
We can use an anno file with its format is like this:
```
    /path/to/image1
    /path/to/image2
    /path/to/image3
    ...
```
Here we assume that this file is saved at `annos/images.txt`:
<br /><br />

We run this command to generate dhash codes of the images specified in above anno file:
```
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup gen_dhash $n_proc annos/images.txt
```
Then we will see a `annos/images.txt.dhash` generated, and its format is like this:
```
    /path/to/image1,hash1
    /path/to/image2,hash2
    /path/to/image3,hash3
    ...
```


#### Step 2. deduplicate via dhash  
This step filter out sample pairs that the bit difference of their hash codes are within a margin.<br>
After above step, we can run this command:  
```
    ## NOTE: do not change the order of the args
    $ export n_proc=64 # how many cpu cores to use 
    $ ./run_dedup dedup_dhash $n_proc annos/images.txt.dhash
```
This would generate a `annos/images.txt.dhash.dedup`, which is in same format as `annos/images.txt`: 
```
    /path/to/image1
    /path/to/image2
    /path/to/image3
    ...
```
Here I use dhash as long as 2048 bits, rather than the 128 bits in some blog. I believe that longer dhash code can same more details of images, which is helpful.


#### merge multiple deduplicated image datasets
We can also merge multiple datasets which has already been deduplicated, like this:  
```
    ## must run dedup on each datasets
    $ export n_proc=64 # how many cpu cores to use 

    $ ./run_dedup merge $n_proc annos/images_1.txt.dhash annos/images_2.txt.dhash annos/images_3.txt.dhash annos/merged.txt
```
The generated `annos/merged.txt` is the file with merged image paths.  


