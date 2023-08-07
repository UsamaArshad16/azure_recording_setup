#!/bin/bash

# Run the record_azure command in the background and store the process ID
./record_azure & record_azure_pid=$!
python3 record_audio.py & record_audio_pid=$!
# Wait for 15 seconds
sleep 15

# Run the python3 rgb_uploading.py command in the background and store the process ID
python3 rgb_uploading.py & rgb_pid=$!
python3 point_cloud_uploading.py & pc_pid=$!
python3 audio_uploading.py & audio_pid=$!

# Trap Ctrl+C and forcefully terminate processes
trap 'kill -9 $record_azure_pid $record_audio_pid $rgb_pid $pc_pid $audio_pid' INT

# Wait for any background process to finish
wait
