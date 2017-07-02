#!/usr/bin/env bash

GSCLZ=cmake-build-debug/gsclz
POKECRYSTAL=/Users/scnorton/pokecrystal

files=$(find $POKECRYSTAL -iname "*.lz" -exec echo {} \; | rev | cut -d'.' -f2- | rev)
i=0

#echo Running gfx.py
## Run gfx.py on all files
#for fl in $files
#do
#    echo $fl
#    python $POKECRYSTAL/gfx.py unlz ${fl}.lz
#done

echo Running GSCLZ
# Run GSCLZ on all files.
for fl in $files
do
	echo $fl > ./test1_${i}.log && $GSCLZ unlz ${fl}.lz ./test_${i}.bin >> ./test1_${i}.log || {
	    echo UNLZ crashed on $fl
	    exit 1
	}
	echo $fl > ./test2_${i}.log && $GSCLZ lz ./test_${i}.bin ./test_${i}.bin.lz >> ./test2_${i}.log || {
	    echo LZ crashed on $fl
	    exit 1
	}
	echo $fl > ./test3_${i}.log && $GSCLZ unlz ./test_${i}.bin.lz ./test2_${i}.bin >> ./test3_${i}.log || {
	    echo UNLZ crashed on $fl second pass
	    exit 1
	}
	((i++))
done
echo GSCLZ returned with no errors!

# Is the decompressor correct?
i=0
for fl in $files
do
	cmp $fl ./test_${i}.bin || {
		sleep 5
		hexdump -C $fl > ./left.txt
		hexdump -C ./test_${i}.bin > ./right.txt
		diff ./left.txt ./right.txt | less
		exit 1
	}
	((i++))
done
echo The decompressor is correct!

# Is the compressor correct?
i=0
for fl in $files
do
	cmp $fl ./test2_${i}.bin || {
		sleep 5
		hexdump -C $fl > ./left.txt
		hexdump -C ./test2_${i}.bin > ./right.txt
		diff ./left.txt ./right.txt | less
		exit 1
	}
	((i++))
done
echo The compressor is correct!

# Is the compressor matching?
i=0
for fl in $files
do
	cmp ${fl}.lz ./test_${i}.bin.lz || {
		sleep 5
		diff ./test1_${i}.log ./test2_${i}.log | less
		exit 1
	}
	((i++))
done
echo The compressor is matching! All tests passed!
