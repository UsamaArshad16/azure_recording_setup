#include <k4a/k4a.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <opencv2/opencv.hpp>

#define FPS 15
#define FRAME_INTERVAL (1000000 / FPS)

double getFolderSize(const char *path) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "du -s -b %s", path);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1.0;
    }

    int64_t size;
    fscanf(fp, "%ld", &size);
    pclose(fp);

    return (double)size / (1024 * 1024 * 1024); // Convert to GB and return as a double
}

int getHighestNumberedFile(const char *folder)
{
    int highestNumber = 0;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(folder)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            int number = atoi(ent->d_name + 6); // Assuming filenames are in the format "image_<number>.jpg"
            if (number > highestNumber)
            {
                highestNumber = number;
            }
        }
        closedir(dir);
    }
    return highestNumber;
}

int main()
{
    uint32_t count = k4a_device_get_installed_count();
    if (count == 0)
    {
        printf("No k4a devices attached!\n");
        return 1;
    }

    // Open the first plugged-in Azure Kinect device
    k4a_device_t device = NULL;
    if (K4A_FAILED(k4a_device_open(K4A_DEVICE_DEFAULT, &device)))
    {
        printf("Failed to open k4a device!\n");
        return 1;
    }

    // Configure a stream of 1920x1080 BGRA32 color data at 15 frames per second
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.camera_fps = K4A_FRAMES_PER_SECOND_15;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED; // Set the depth mode for point cloud

    // Start the camera with the given configuration
    if (K4A_FAILED(k4a_device_start_cameras(device, &config)))
    {
        printf("Failed to start cameras!\n");
        k4a_device_close(device);
        return 1;
    }

    // Create a buffer to hold the image data
    size_t buffer_size = 1920 * 1080 * 4;
    uint8_t *buffer = (uint8_t *)malloc(buffer_size);

    // Create a buffer to hold the point cloud data
    size_t point_cloud_size = 1920 * 1080 * 3 * sizeof(float); // 3 channels for x, y, z for each pixel
    float *point_cloud_buffer = (float *)malloc(point_cloud_size);

    // Variables for frame rate control
    uint64_t last_frame_time = 0;
    uint64_t current_frame_time = 0;
    uint64_t frame_duration = FRAME_INTERVAL;

    // Create the parent directory "azure_recordings" in home directory
    const char *home_dir = getenv("HOME");
    char save_folder[256];
    snprintf(save_folder, sizeof(save_folder), "%s/azure_recordings", home_dir);
    if (access(save_folder, F_OK) == -1)
    {
        mkdir(save_folder, 0700);
    }

    // Create the "rgb_images" and "point_cloud" directories
    char rgb_folder[512], pc_folder[512];
    snprintf(rgb_folder, sizeof(rgb_folder), "%s/rgb_images", save_folder);
    snprintf(pc_folder, sizeof(pc_folder), "%s/point_cloud", save_folder);
    if (access(rgb_folder, F_OK) == -1)
    {
        mkdir(rgb_folder, 0700);
    }
    if (access(pc_folder, F_OK) == -1)
    {
        mkdir(pc_folder, 0700);
    }

    // Save images and point cloud for 1 minute (10 seconds * 15 fps)
    int frame_count = 0;
    // Find the highest numbered file in the "azure_recordings/rgb_images" folder
    int image_sequence = getHighestNumberedFile(rgb_folder) + 1;
    double max_size = 5; // in GBs
    printf("rgb & point cloud recording started\n");

    while (true)
    {
        double folder_size = getFolderSize(save_folder);
        if (folder_size-0.3 > max_size)
        {
            printf(" rgb images & point recording paused. Available space: %.3f GB\n", (double)(max_size - folder_size));
            usleep(2000000); // Sleep for 2 seconds before checking again
            while (true) // Wait loop
            { double folder_size = getFolderSize(save_folder);
                double available_space = max_size - folder_size;
                if (available_space > max_size-2)
                {
                    printf("Available space: %.3f GB. rgb images & point cloud resuming recording...\n", available_space);
                    break;
                }

                usleep(10*1000000); // Sleep for 20 seconds before checking again
                folder_size = getFolderSize(save_folder);
            }
        }

        // Capture a frame
        k4a_capture_t capture;
        k4a_wait_result_t result = k4a_device_get_capture(device, &capture, 0);
        if (result == K4A_WAIT_RESULT_SUCCEEDED)
        {
            // Access the color image
            k4a_image_t color_image = k4a_capture_get_color_image(capture);
            if (color_image != NULL)
            {
                // Get the image data and size
                uint8_t *image_data = k4a_image_get_buffer(color_image);
                size_t image_size = k4a_image_get_size(color_image);

                // Copy the image data to the buffer
                memcpy(buffer, image_data, image_size);

                // Get the current timestamp
                current_frame_time = k4a_image_get_device_timestamp_usec(color_image);

                // Get the current time in milliseconds from the internet
                struct timeval tv;
                gettimeofday(&tv, NULL);
                long long timestamp_millis = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;

                // Construct the timestamp string
                char timestamp[256];
                snprintf(timestamp, sizeof(timestamp), "_%lld", timestamp_millis);

                // Save the image with a timestamp
                char filename[1024];
                snprintf(filename, sizeof(filename), "%s/image_%d%s.jpg", rgb_folder, image_sequence, timestamp);

                // Use OpenCV to save the image
                cv::Mat cv_image(1080, 1920, CV_8UC4, buffer);
                cv::imwrite(filename, cv_image);

                // Release the color image
                k4a_image_release(color_image);

                // Control frame rate
                if (frame_count > 0)
                {
                    uint64_t time_elapsed = current_frame_time - last_frame_time;
                    if (time_elapsed < frame_duration)
                    {
                        usleep(frame_duration - time_elapsed);
                    }
                }

                last_frame_time = current_frame_time;

                frame_count++;
            }

            // Access the depth image
            k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
            if (depth_image != NULL)
            {
                // Get the depth image data and size
                uint16_t *depth_data = (uint16_t *)k4a_image_get_buffer(depth_image);
                int depth_width = k4a_image_get_width_pixels(depth_image);
                int depth_height = k4a_image_get_height_pixels(depth_image);

                // Generate the point cloud from the depth image
                int point_cloud_index = 0;
                for (int y = 0; y < depth_height; y++)
                {
                    for (int x = 0; x < depth_width; x++)
                    {
                        // Get the depth value at the current pixel
                        uint16_t depth_value = depth_data[y * depth_width + x];

                        // Calculate the 3D coordinates of the pixel
                        float point_x = (float)x;
                        float point_y = (float)y;
                        float point_z = (float)depth_value;

                        // Store the point in the point cloud buffer
                        point_cloud_buffer[point_cloud_index++] = point_x;
                        point_cloud_buffer[point_cloud_index++] = point_y;
                        point_cloud_buffer[point_cloud_index++] = point_z;
                    }
                }

                // Get the current time in milliseconds from the internet
                struct timeval tv;
                gettimeofday(&tv, NULL);
                long long timestamp_millis = (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;

                // Construct the timestamp string
                char timestamp[256];
                snprintf(timestamp, sizeof(timestamp), "_%lld", timestamp_millis);

                // Save the point cloud with a timestamp
                char pc_filename[1024];
                snprintf(pc_filename, sizeof(pc_filename), "%s/pointcloud_%d%s.npy", pc_folder, image_sequence, timestamp);
                FILE *pc_file = fopen(pc_filename, "wb");
                fwrite(point_cloud_buffer, sizeof(float), point_cloud_index, pc_file);
                fclose(pc_file);

                // Release the depth image
                k4a_image_release(depth_image);
            }

            // Release the capture
            k4a_capture_release(capture);

            // Increment the image sequence
            image_sequence++;
        }
    }

    // Stop the camera
    k4a_device_stop_cameras(device);

    // Close the camera
    k4a_device_close(device);

    // Free the buffers
    free(buffer);
    free(point_cloud_buffer);

    return 0;
}
