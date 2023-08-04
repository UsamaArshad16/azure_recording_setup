# azure_recording_setup
### record_azure.cpp

This file records the rgb and point cloud. The folder maximum size is mentioned in line 124 (currently 1 GB). After reaching the limit the thread will be terminated. 

It creates a folder “azure_recordings” in home and then creates “rgb_images” and “point_cloud” folders inside it.  

### find_respeaker_id.py: 

This code tells what is the idex_id of the respeaker_microphone_array. That will be mentioned in “record_audio.py” file 

### record_audio.py: 

It records the chunks of audio, and the length can be set. It saves the recoding in “azure_recordings/audio” folder and it also follows the folder size limit check. 

### rgb_uploading.py: 

This file uploads the rgb images on the cloud and deletes the file that is successfully uploaded. 

### point_cloud_uploading.py: 

This file uploads the point cloud images on the cloud and deletes the file that is successfully uploaded. 

### audio_uploading.py: 

This file uploads the audio files on the cloud and deletes the file that is successfully uploaded. 

 
