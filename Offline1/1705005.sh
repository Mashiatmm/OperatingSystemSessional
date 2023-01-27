#!/bin/bash
if [ $# == 2 ]; then
	directory=$1
	input_file=$2
elif [ $# == 1 ]; then
	input_file=$1
	directory=.
else
	echo "Input working directory and file name";
	exit 1;
fi
# check if input_file exists
if [ ! -f $input_file ]; then
	echo "File not found";
	exit 1;
fi
# find files with extensions
# store find command in a variable
var=""
n=0	
while IFS=$' \t\r\n' read -r type
do 
	# Add Or operation
	n=$((n+1))
	if [ $n -gt 1 ]; then
		var="${var} -o";
	fi
	var="${var} -iname *.${type}"	
done < "$input_file"

# make output directory
mkdir output_dir -p
# total number of files
all="$(find "$directory" -type f | wc -l)"

# execute find command
find $directory -type f -not \($var \) | while read val; do
	filename=$(basename -- "$val")
	#extension="${filename##*.}"
	extension=$([[ "$filename" = *.* ]] && echo ".${filename##*.}" || echo '')
	# stripping dot at the end
	extension=${extension/.}
	# fix sub_directory
	if [ -z "${extension}" ]; then
		# if file doesn't have an extension
		folder="others"
	else
		#if file has an extension
		folder=${extension}
	fi
	subdir="output_dir/${folder}"
	# check if sub_dir exists, otherwise create it
	if [ ! -d $subdir ]; then
		# create subdirectory
		mkdir $subdir
		# create text file
		touch ${subdir}/desc_${folder}.txt
	fi
	
	# copy file to desired folder
	if [ ! -e "$subdir"/"$filename" ]; then
		cp "$val" $subdir
		echo "$val" >> ${subdir}/desc_${folder}.txt
	fi
done

# total number of files copied
length=0
#length="$(find output_dir -type f -not \($var \)| wc -l)"

# create csv file
touch output.csv
echo "file_type,no_of_files" > output.csv

# get all folder list
list=($(find output_dir -mindepth 1 -type d))
for val in ${list[@]}; do
	# get folder name
	folder_name=$(basename "$val")
	# get count of files inside the folder
	count="$(find "${val}/" -type f | wc -l)"
	echo $folder_name,$((count-1)) >> output.csv
	length=$((length+count-1))
done

# ignored files numbers
echo "ignored",$((all-length)) >> output.csv
