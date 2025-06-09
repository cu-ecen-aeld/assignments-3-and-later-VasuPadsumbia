writefile=$1
writestr=$2

if [ -z "$writefile" ]; then
    echo "No file specified to write to"
    exit 1
elif [ -z "$writestr" ]; then
    echo "No string specified to write"
    exit 1
else
    echo "Writing to file $writefile"
    dir=$(dirname "$writefile")
    if [ ! -d "$dir" ]; then
        echo "Directory $dir does not exist. Creating it."
        mkdir -p "$dir"
        if [ $? -ne 0 ]; then
            echo "Failed to create directory $dir"
            exit 1
        fi
    fi
    # clear the file if it exists
    > "$writefile"
    if [ $? -ne 0 ]; then
        echo "Failed to clear file $writefile"
        exit 1
    fi
    echo "$writestr" > "$writefile"
    if [ $? -eq 0 ]; then
        echo "Successfully wrote to $writefile"
    else
        echo "Failed to write to $writefile"
        exit 1
    fi
fi
