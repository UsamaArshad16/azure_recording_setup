import os
import requests

def upload_files_in_folder(url, folder_path):
    for root, _, files in os.walk(folder_path):
        for file in files:
            file_path = os.path.join(root, file)
            upload_file(url, file_path)
            delete_file(file_path)

def upload_file(url, file_path):
    try:
        with open(file_path, 'rb') as file:
            files = {'file': file}
            response = requests.post(url, files=files)

            if response.status_code == 200:
                print(f"File '{file_path}' uploaded successfully.")
            else:
                print(f"Failed to upload file '{file_path}'. Status code: {response.status_code}")
    except Exception as e:
        print(f"An error occurred while uploading '{file_path}': {e}")

def delete_file(file_path):
    try:
        os.remove(file_path)
        print(f"File '{file_path}' deleted successfully.")
    except Exception as e:
        print(f"An error occurred while deleting '{file_path}': {e}")

if __name__ == "__main__":
    url = "https://rehabrobottesting.azurewebsites.net/Storage/Mobile/UploadFiles"
    main_folder = os.path.expanduser("~/azure_recordings")
    audio_folder = os.path.join(main_folder, "audio")

    if os.path.exists(audio_folder):
        try:
            while True:
                upload_files_in_folder(url, audio_folder)
        except KeyboardInterrupt:
            print("\nLoop terminated by user.")
    else:
        print("audio_folder folder not found.")