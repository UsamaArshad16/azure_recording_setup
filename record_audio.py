import os
import pyaudio
import wave
from datetime import datetime, timedelta
import time

RESPEAKER_RATE = 16000
RESPEAKER_CHANNELS = 6  # Change based on firmwares, 1_channel_firmware.bin as 1 or 6_channels_firmware.bin as 6
RESPEAKER_WIDTH = 2
# Run getDeviceInfo.py to get the index
RESPEAKER_INDEX = 11  # Refer to the input device id
CHUNK = 8192  # Increase the CHUNK size further
RECORD_SECONDS = 10  # Reduce the recording duration to 10 seconds
safe_memory = 0.025  # to make it stop at desired size limit
MAX_FOLDER_SIZE_GB = 5 - safe_memory

p = pyaudio.PyAudio()

stream = p.open(
    rate=RESPEAKER_RATE,
    format=p.get_format_from_width(RESPEAKER_WIDTH),
    channels=RESPEAKER_CHANNELS,
    input=True,
    input_device_index=RESPEAKER_INDEX,
)

print("* recording")

# Create the main folder if it doesn't exist
home_folder = os.path.expanduser("~")
save_folder = os.path.join(home_folder, "azure_recordings")
if not os.path.exists(save_folder):
    os.makedirs(save_folder)

# Create the subfolder for audio if it doesn't exist
audio_folder = os.path.join(save_folder, "audio")
if not os.path.exists(audio_folder):
    os.makedirs(audio_folder)

# Recording duration for each subfolder
recording_duration = timedelta(seconds=RECORD_SECONDS)
current_start_time = datetime.now()

frames = []

def get_folder_size(folder_path):
    total_size = 0
    for dirpath, _, filenames in os.walk(folder_path):
        for filename in filenames:
            file_path = os.path.join(dirpath, filename)
            try:
                total_size += os.path.getsize(file_path)
            except OSError as e:
                print(f"Error getting size of {file_path}: {e}")
    return total_size


current_recording_count = len(os.listdir(audio_folder))
try:
    while True:
        try:
            # Get audio data
            data = stream.read(CHUNK)
            frames.append(data)
        except OSError as e:
            if e.errno == -9981:  # Input overflow error
                print("Input overflowed. Continuing...")
            elif e.errno == -9988:  # Stream closed error
                print("Stream closed. Reopening...")
                stream = p.open(
                    rate=RESPEAKER_RATE,
                    format=p.get_format_from_width(RESPEAKER_WIDTH),
                    channels=RESPEAKER_CHANNELS,
                    input=True,
                    input_device_index=RESPEAKER_INDEX,
                )
            else:
                raise e

        # Check if recording duration has exceeded 20 seconds
        elapsed_time = datetime.now() - current_start_time
        if elapsed_time >= recording_duration:
            current_start_time = datetime.now()

            # Check if the folder size exceeds the maximum allowed size (in bytes)
            folder_size_bytes = get_folder_size(save_folder)
            folder_size_gb = folder_size_bytes / (1024 ** 3)  # Convert bytes to GB

            print(f"azure_recordings folder size: {folder_size_gb:.3f} GB")

            max_folder_size_bytes = MAX_FOLDER_SIZE_GB * (1024 ** 3)  # Convert GB to bytes
            if folder_size_bytes >= max_folder_size_bytes:
                print("Folder size exceeds limit. Audio Recording terminated.")
                while True:
                    folder_size_bytes = get_folder_size(save_folder)
                    folder_size_gb = folder_size_bytes / (1024 ** 3)  # Convert bytes to GB
                    if folder_size_gb < (MAX_FOLDER_SIZE_GB - 2):  # Wait until folder size is less than 18 GB
                        break
                    else:
                        print("Waiting for space to become available...")
                        time.sleep(8)  # Wait for 8 seconds before checking again
                print("Audio Recording Resuming")

            # Save the audio recording with the epoch timestamp and count
            current_recording_count = current_recording_count + 1
            timestamp = int((datetime.now() - datetime(1970, 1, 1)).total_seconds() * 1000)
            audio_file_path = os.path.join(audio_folder, f"audio_{current_recording_count}_{timestamp}.wav")
            wf = wave.open(audio_file_path, 'wb')
            wf.setnchannels(RESPEAKER_CHANNELS)
            wf.setsampwidth(p.get_sample_size(p.get_format_from_width(RESPEAKER_WIDTH)))
            wf.setframerate(RESPEAKER_RATE)
            wf.writeframes(b''.join(frames))
            wf.close()

            frames = []  # Reset frames for the next recording

except KeyboardInterrupt:
    pass

print("* done audio recording")

stream.stop_stream()
stream.close()
p.terminate()
