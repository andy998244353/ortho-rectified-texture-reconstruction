#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <string>

void cli_args(int argc, char **argv, 
    std::string &proxy_model_path,
    std::string &geo_ref_path,
    float &proxy_tolerance,
    float &depth_tolerance,
    float &point_radius,
    int &target_width,
    int &target_height,
    std::string &resource_path,
    std::string &camera_path,
    std::string &img_path,
    std::string &depth_path,
    std::string &mask_path,
    float &angle_threshold,
    std::string &output_path
);


#endif