#!/bin/bash

# Run the commands in parallel and store the process IDs
sudo ./record_azure & record_azure_pid=$!
sudo python3 record_audio.py & record_audio_pid=$!
sleep 10
sudo python3 rgb_uploading.py & rgb_pid=$!
sudo python3 point_cloud_uploading.py & pc_pid=$!
sudo python3 audio_uploading.py & audio_pid=$!

# Wait for Ctrl+C
trap 'kill_processes' INT

# Function to forcefully terminate the processes
kill_processes() {
    echo "Forcefully shutting down..."
    sudo pkill -P $record_azure_pid
    sudo pkill -P $record_audio_pid
    sudo pkill -P $rgb_pid
    sudo pkill -P $pc_pid
    sudo pkill -P $audio_pid
    exit
}

# Wait for any background process to finish
wait
