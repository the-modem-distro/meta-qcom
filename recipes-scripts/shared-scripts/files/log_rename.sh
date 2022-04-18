#/bin/bash
# Shamelessly copied from https://unix.stackexchange.com/questions/603309/how-to-rename-a-batch-of-log-files-incrementally-without-overwrite-using-bash

if [ -f "/persist/openqti.log" ]; then
    log_names=$(for logfile in $(find /persist -type f -name '*.log*'); do echo ${logfile%.[0-9]*}; done | sort -u)
    for name in $log_names; do
        echo "Processing $name"
        i=20
        until [[ "$i" -eq 0 ]]; do
            if [[ -f "$name.$i" ]]; then
                next_num=$((i+1))
                mv -v "$name.$i" "$name.$next_num"
            fi
            i=$((i-1))
        done
        if [[ -f "$name" ]]; then
            mv -v "$name" "$name.1"
        fi
        touch "$name"
    done
fi