// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include <fstream>
#include "turbojpeg.h"
//#include <turbojpeg.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;

static bool point_cloud_color_to_depth(k4a_transformation_t transformation_handle,
    const k4a_image_t depth_image,
    const k4a_image_t color_image,
    std::string file_name)
{
    int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
    int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);
    k4a_image_t transformed_color_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
        depth_image_width_pixels,
        depth_image_height_pixels,
        depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
        &transformed_color_image))
    {
        printf("Failed to create transformed color image\n");
        return false;
    }

    k4a_image_t point_cloud_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
        depth_image_width_pixels,
        depth_image_height_pixels,
        depth_image_width_pixels * 3 * (int)sizeof(int16_t),
        &point_cloud_image))
    {
        printf("Failed to create point cloud image\n");
        return false;
    }

    // <> registration can be done for us, just find a way to save that image
    if (K4A_RESULT_SUCCEEDED != k4a_transformation_color_image_to_depth_camera(transformation_handle,
        depth_image,
        color_image,
        transformed_color_image))
    {
        printf("Failed to compute transformed color image\n");
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_depth_image_to_point_cloud(transformation_handle,
        depth_image,
        K4A_CALIBRATION_TYPE_DEPTH,
        point_cloud_image))
    {
        printf("Failed to compute point cloud\n");
        return false;
    }

    tranformation_helpers_write_point_cloud(point_cloud_image, transformed_color_image, file_name.c_str());

    k4a_image_release(transformed_color_image);
    k4a_image_release(point_cloud_image);

    return true;
}

static bool point_cloud_depth_to_color(k4a_transformation_t transformation_handle,
    const k4a_image_t depth_image,
    const k4a_image_t color_image,
    std::string file_name)
{
    // transform color image into depth camera geometry
    int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
    int color_image_height_pixels = k4a_image_get_height_pixels(color_image);
    // <> this image is what we are after, transformed depth image
    k4a_image_t transformed_depth_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
        color_image_width_pixels,
        color_image_height_pixels,
        color_image_width_pixels * (int)sizeof(uint16_t),
        &transformed_depth_image))
    {
        printf("Failed to create transformed depth image\n");
        return false;
    }

    k4a_image_t point_cloud_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
        color_image_width_pixels,
        color_image_height_pixels,
        color_image_width_pixels * 3 * (int)sizeof(int16_t),
        &point_cloud_image))
    {
        printf("Failed to create point cloud image\n");
        return false;
    }

    if (K4A_RESULT_SUCCEEDED !=
        k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
    {
        printf("Failed to compute transformed depth image\n");
        return false;
    }
    // <> At this point the depth image has the same dimensions as the color one!! Now save it!
    /*cv::Mat frame;
    uint8_t *buffer = k4a_image_get_buffer(transformed_depth_image);
    uint16_t *depth_buffer = reinterpret_cast<uint16_t *>(buffer);
    // remove opencv
    // just print the contents verify the way they are stored, color_image_width_pixels * color_image_height_pixels * channels * sizeof(uint16_t)
    // just save somehow else, you dont need opencv
    int channels = 1;
    cv::Mat mat(color_image_height_pixels, color_image_width_pixels, CV_MAKETYPE(DataType<uint16_t>::type, channels));
    memcpy(mat.data, depth_buffer, color_image_width_pixels * color_image_height_pixels * channels * sizeof(uint16_t));
    mat.copyTo(frame);
    //<>OLD: create_mat_from_buffer<uint16_t>(depth_buffer, color_image_width_pixels,
    //color_image_height_pixels).copyTo(frame);
    if (frame.empty())
    {
        printf("transformed_depth_image is empty\n");
    }
    imwrite("transformed_depth_image.png", frame);*/
    // <> ceci

    // BAD WAY OF SAVING FRAMES
    /*
    uint8_t* buffer = k4a_image_get_buffer(transformed_depth_image);
    uint16_t* depth_buffer = reinterpret_cast<uint16_t*>(buffer);
    std::ofstream fout("img.txt");
    fout << color_image_height_pixels << " " << color_image_width_pixels << " ";
    for (int i = 0; i < color_image_width_pixels * color_image_height_pixels; i++)
        fout << depth_buffer[i] << " ";
    // save the color and the depth frame the same way to verify saving is done correctly
    std::ofstream fout2("depthTmp.txt");
    int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
    int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);
    buffer = k4a_image_get_buffer(depth_image);
    depth_buffer = reinterpret_cast<uint16_t*>(buffer);
    fout2 << depth_image_height_pixels << " " << depth_image_width_pixels << " ";
    for (int i = 0; i < depth_image_height_pixels * depth_image_width_pixels; i++)
        fout2 << depth_buffer[i] << " ";
    std::ofstream fout3("colorTmp.txt");
    uint8_t* color_buffer = (uint8_t*)(void*)k4a_image_get_buffer(color_image);
    fout3 << color_image_height_pixels << " " << color_image_width_pixels << " ";
    for (int i = 0; i < color_image_width_pixels * color_image_height_pixels * 3; i++)
        fout3 << color_buffer[i] << " ";
    */
    // GOOD WAY OF SAVING FRAMES WITH OPENCV
    Mat frame;
    uint8_t* buffer = k4a_image_get_buffer(transformed_depth_image);
    uint16_t* depth_buffer = reinterpret_cast<uint16_t*>(buffer);
    // remove opencv
    // just print the contents verify the way they are stored, color_image_width_pixels * color_image_height_pixels * channels * sizeof(uint16_t)
    // just save somehow else, you dont need opencv
    int channels = 1;
    cv::Mat mat(color_image_height_pixels, color_image_width_pixels, CV_MAKETYPE(DataType<uint16_t>::type, channels));
    //CV_16UC1
    memcpy(mat.data, depth_buffer, color_image_width_pixels * color_image_height_pixels * channels * sizeof(uint16_t));
    mat.copyTo(frame);
    //<>OLD: create_mat_from_buffer<uint16_t>(depth_buffer, color_image_width_pixels,
    //color_image_height_pixels).copyTo(frame);
    if (frame.empty())
    {
        printf("transformed_depth_image is empty\n");
    }
    imwrite("transformed_depth_image_QUACK.png", frame);

    // second attempt
    cv::Mat depth_frame_new = cv::Mat(color_image_height_pixels, color_image_width_pixels, CV_16UC1, depth_buffer, cv::Mat::AUTO_STEP);
    imwrite("transformed_depth_QUACK_2.png", depth_frame_new);

    // also save the color frame
    uint8_t* image_data = (uint8_t*)(void*)k4a_image_get_buffer(color_image);
    cv::Mat color_frame = cv::Mat(k4a_image_get_height_pixels(color_image), k4a_image_get_width_pixels(color_image), CV_8UC4, image_data, cv::Mat::AUTO_STEP);

    // how about you print the values of the frame so that we see them, 8 bit means up to 255, not sure how it converts to 16 bit
    imwrite("col_image_QUACK.png", color_frame);// you wrote the same frame dude.....

    exit(1);

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_depth_image_to_point_cloud(transformation_handle,
        transformed_depth_image,
        K4A_CALIBRATION_TYPE_COLOR,
        point_cloud_image))
    {
        printf("Failed to compute point cloud\n");
        return false;
    }

    tranformation_helpers_write_point_cloud(point_cloud_image, color_image, file_name.c_str());

    k4a_image_release(transformed_depth_image);
    k4a_image_release(point_cloud_image);

    return true;
}

// Timestamp in milliseconds. Defaults to 1 sec as the first couple frames don't contain color
static int playback(char* input_path, int timestamp = 1000, std::string output_filename = "output.ply")
{
    /* Predef of params */
    float r11, r12, r13, r21, r22, r23, r31, r32, r33, t1, t2, t3;
    float cx, cy, fx, fy, k1, k2, k3, k4, k5, k6, codx, cody, p1, p2;
    const k4a_calibration_intrinsic_parameters_t* params;

    int returnCode = 1;
    k4a_playback_t playback = NULL;
    k4a_calibration_t calibration; // <>
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;

    k4a_result_t result;
    k4a_stream_result_t stream_result;

    // Open recording
    result = k4a_playback_open(input_path, &playback);
    if (result != K4A_RESULT_SUCCEEDED || playback == NULL)
    {
        printf("Failed to open recording %s\n", input_path);
        goto Exit;
    }

    result = k4a_playback_seek_timestamp(playback, timestamp * 1000, K4A_PLAYBACK_SEEK_BEGIN);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to seek timestamp %d\n", timestamp);
        goto Exit;
    }
    printf("Seeking to timestamp: %d/%d (ms)\n",
        timestamp,
        (int)(k4a_playback_get_recording_length_usec(playback) / 1000));

    stream_result = k4a_playback_get_next_capture(playback, &capture);
    if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
    {
        printf("Failed to fetch frame\n");
        goto Exit;
    }
    // <> get calibration from playback
    if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(playback, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }
    // From calibration you can print the intristic information of both cameras
    printf("DEPTH CAMERA INTRISTICS\n");
    const k4a_calibration_camera_t depth_camera = calibration.depth_camera_calibration;
    params = &depth_camera.intrinsics.parameters;
    cx = params->param.cx;
    cy = params->param.cy;
    fx = params->param.fx;
    fy = params->param.fy;
    k1 = params->param.k1;
    k2 = params->param.k2;
    k3 = params->param.k3;
    k4 = params->param.k4;
    k5 = params->param.k5;
    k6 = params->param.k6;
    codx = params->param.codx; // center of distortion is set to 0 for Brown Conrady model
    cody = params->param.cody;
    p1 = params->param.p1;
    p2 = params->param.p2;
    printf("cx: %f\n", cx);
    printf("cy: %f\n", cy);
    printf("fx: %f\n", fx);
    printf("fy: %f\n", fy);
    printf("k1: %f\n", k1);
    printf("k2: %f\n", k2);
    printf("k3: %f\n", k3);
    printf("k4: %f\n", k4);
    printf("k5: %f\n", k5);
    printf("k6: %f\n", k6);
    printf("codx: %f\n", codx);
    printf("cody: %f\n", cody);
    printf("p1: %f\n", p1);
    printf("p2: %f\n", p2);
    printf("----------------------------\n");
    printf("COLOR CAMERA INTRISTICS\n");
    const k4a_calibration_camera_t color_camera = calibration.color_camera_calibration;
    params = &color_camera.intrinsics.parameters;
    cx = params->param.cx;
    cy = params->param.cy;
    fx = params->param.fx;
    fy = params->param.fy;
    k1 = params->param.k1;
    k2 = params->param.k2;
    k3 = params->param.k3;
    k4 = params->param.k4;
    k5 = params->param.k5;
    k6 = params->param.k6;
    codx = params->param.codx; // center of distortion is set to 0 for Brown Conrady model
    cody = params->param.cody;
    p1 = params->param.p1;
    p2 = params->param.p2;
    printf("cx: %f\n", cx);
    printf("cy: %f\n", cy);
    printf("fx: %f\n", fx);
    printf("fy: %f\n", fy);
    printf("k1: %f\n", k1);
    printf("k2: %f\n", k2);
    printf("k3: %f\n", k3);
    printf("k4: %f\n", k4);
    printf("k5: %f\n", k5);
    printf("k6: %f\n", k6);
    printf("codx: %f\n", codx);
    printf("cody: %f\n", cody);
    printf("p1: %f\n", p1);
    printf("p2: %f\n", p2);
    printf("----------------------------\n");
    printf("COLOR CAMERA EXTRINSICS\n");
    r11 = color_camera.extrinsics.rotation[0];
    r12 = color_camera.extrinsics.rotation[1];
    r13 = color_camera.extrinsics.rotation[2];
    r21 = color_camera.extrinsics.rotation[3];
    r22 = color_camera.extrinsics.rotation[4];
    r23 = color_camera.extrinsics.rotation[5];
    r31 = color_camera.extrinsics.rotation[6];
    r32 = color_camera.extrinsics.rotation[7];
    r33 = color_camera.extrinsics.rotation[8];
    t1 = color_camera.extrinsics.translation[0];
    t2 = color_camera.extrinsics.translation[1];
    t3 = color_camera.extrinsics.translation[2];
    printf("r11: %f\n", r11);
    printf("r12: %f\n", r12);
    printf("r13: %f\n", r13);
    printf("r21: %f\n", r21);
    printf("r22: %f\n", r22);
    printf("r23: %f\n", r23);
    printf("r31: %f\n", r31);
    printf("r32: %f\n", r32);
    printf("r33: %f\n", r33);
    printf("t1: %f\n", t1);
    printf("t2: %f\n", t2);
    printf("t3: %f\n", t3);
    printf("----------------------------\n");

    // <> created transformation
    transformation = k4a_transformation_create(&calibration);

    // Fetch depth and color frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto Exit;
    }

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto Exit;
    }

    ///////////////////////////////
    // Convert color frame from mjpeg to bgra
    k4a_image_format_t format;
    format = k4a_image_get_format(color_image);
    if (format != K4A_IMAGE_FORMAT_COLOR_MJPG)
    {
        printf("Color format not supported. Please use MJPEG\n");
        goto Exit;
    }

    int color_width, color_height;
    color_width = k4a_image_get_width_pixels(color_image);
    color_height = k4a_image_get_height_pixels(color_image);

    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
        color_width,
        color_height,
        color_width * 4 * (int)sizeof(uint8_t),
        &uncompressed_color_image))
    {
        printf("Failed to create image buffer\n");
        goto Exit;
    }

    // <>MAYBE HERE WE CAN REGISTER THE DEPTH AND RGB FRAMES
    tjhandle tjHandle;
    tjHandle = tjInitDecompress();
    if (tjDecompress2(tjHandle,
        k4a_image_get_buffer(color_image),
        static_cast<unsigned long>(k4a_image_get_size(color_image)),
        k4a_image_get_buffer(uncompressed_color_image),
        color_width,
        0, // pitch
        color_height,
        TJPF_BGRA,
        TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
    {
        printf("Failed to decompress color frame\n");
        if (tjDestroy(tjHandle))
        {
            printf("Failed to destroy turboJPEG handle\n");
        }
        goto Exit;
    }
    if (tjDestroy(tjHandle))
    {
        printf("Failed to destroy turboJPEG handle\n");
    }
    ///////////////////////////////

    // Compute color point cloud by warping depth image into color camera geometry
    if (point_cloud_depth_to_color(transformation, depth_image, uncompressed_color_image, output_filename) == false)
    {
        printf("Failed to transform depth to color\n");
        goto Exit;
    }

    returnCode = 0;

Exit:
    if (playback != NULL)
    {
        k4a_playback_close(playback);
    }
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    if (uncompressed_color_image != NULL)
    {
        k4a_image_release(uncompressed_color_image);
    }
    if (capture != NULL)
    {
        k4a_capture_release(capture);
    }
    if (transformation != NULL)
    {
        k4a_transformation_destroy(transformation);
    }
    return returnCode;
}

/* DONT READ, NOT IMPORTANT FOR IMPLEMENTATION */
static void print_usage()
{
    printf("To convert a 5fps video out.mkv to a dataset of synced and registered frames\n");
    printf("Usage: vid2dataset frames [directory_including_video]\n");
    printf("To convert a 5fps video out.mkv to a point cloud given camera trajectory information\n");
    printf("Usage: vid2dataset pointcloud [camera_trajectory_file] [directory_including_video]\n");
    printf("Note: out.mkv videos need to have depth and rgb channels. The frames and point clouds are saved in the same directory as out.mkv\n");
}

int main(int argc, char** argv)
{
    int returnCode = 0;

    if (argc < 2)
    {
        print_usage();
    }
    else
    {
        std::string mode = std::string(argv[1]);
        if (mode == "frames")
        {
            printf("FRAME MODE\n");
            printf("UNIMPLEMENTED\n");
        }
        else if (mode == "pointcloud")
        {
            printf("POINTCLOUD MODE\n");
            if (argc == 3)
            {
                returnCode = playback(argv[2]);
            }
            else
            {
                print_usage();
            }
        }
        else
        {
            print_usage();
        }
    }
    return returnCode;
}
