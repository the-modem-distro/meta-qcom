#!/bin/bash

if [ -f "/persist/openqti.log" ]; then
  # Really a shame that readarray is not supported in this bash shell
  # Doing this the manual, hard way
  # readarray -t logFiles < <(find /persist -type f -name 'openqti.log*' -o -type f -name 'openqti-*.log')
  declare -a logFiles
  for logFile in /persist/openqti.log* /persist/openqti-*.log; do
    [ ! -f "$logFile" ] && continue
    unset logNum
    if [[ "${logFile}" =~ openqti-[1-9][0-9]*.log ]]; then
      logNum=$(basename "${logFile}" | awk -F'[-.]' '{print $2}')
    elif [[ "${logFile}" =~ openqti.log.[1-9][0-9]* ]]; then
      logNum=$(basename "${logFile}" | awk -F. '{print $NF}')
    else
      # Skip unrecognized filename
      continue
    fi
    logNum=$((logNum - 1))
    logFiles[$logNum]="$logFile"
  done
  for ((i=${#logFiles[@]}; i>0; i--)); do
    [ $i -gt 20 ] && continue
    index=$((i - 1))
    fileNum=$((i +1))
    echo "Processing ${logFiles[$index]}"
    fileDir=$(dirname "${logFiles[$index]}")
    echo "moving ${logFiles[${index}]} -> ${fileDir}/openqti-${fileNum}.log"
    mv ${logFiles[${index}]} ${fileDir}/openqti-${fileNum}.log
  done
  echo "moving /persist/openqti.log -> /persist/openqti-1.log"
  mv /persist/openqti.log /persist/openqti-1.log
  touch /persist/openqti.log
  unset logFiles
fi

if [ -f "/persist/thermal.log" ]; then
  # Really a shame that readarray is not supported in this bash shell
  # Doing this the manual, hard way
  # readarray -t logFiles < <(find /persist -type f -name 'thermal.log*' -o -type f -name 'thermal-*.log')
  declare -a logFiles
  for logFile in /persist/thermal.log* /persist/thermal-*.log; do
    [ ! -f "$logFile" ] && continue
    unset logNum
    if [[ "${logFile}" =~ thermal-[1-9][0-9]*.log ]]; then
      logNum=$(basename "${logFile}" | awk -F'[-.]' '{print $2}')
    elif [[ "${logFile}" =~ thermal.log.[1-9][0-9]* ]]; then
      logNum=$(basename "${logFile}" | awk -F. '{print $NF}')
    else
      # Skip unrecognized filename
      continue
    fi
    logNum=$((logNum - 1))
    logFiles[$logNum]="$logFile"
  done
  for ((i=${#logFiles[@]}; i>0; i--)); do
    [ $i -gt 20 ] && continue
    index=$((i - 1))
    fileNum=$((i +1))
    echo "Processing ${logFiles[$index]}"
    fileDir=$(dirname "${logFiles[$index]}")
    echo "moving ${logFiles[${index}]} -> ${fileDir}/thermal-${fileNum}.log"
    mv ${logFiles[${index}]} ${fileDir}/thermal-${fileNum}.log
  done
  echo "moving /persist/thermal.log -> /persist/thermal-1.log"
  mv /persist/thermal.log /persist/thermal-1.log
  touch /persist/thermal.log
  unset logFiles
fi
