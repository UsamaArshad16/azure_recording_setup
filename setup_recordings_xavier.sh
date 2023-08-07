#!/bin/bash

# Run the commands in parallel and store the process IDs
./record_azure & record_azure_pid=$!
python3 record_audio.py & record_audio_pid=$!
sleep 10
python3 rgb_uploading.py & rgb_pid=$!
python3 point_cloud_uploading.py & pc_pid=$!
python3 audio_uploading.py & audio_pid=$!

# Wait for Ctrl+C
trap 'kill_processes' INT

# Function to forcefully terminate the processes
kill_processes() {
    echo "Forcefully shutting down..."
    pkill -P $record_azure_pid
    pkill -P $record_audio_pid
    pkill -P $rgb_pid
    pkill -P $pc_pid
    pkill -P $audio_pid
    exit
}

# Wait for any background process to finish
wait
