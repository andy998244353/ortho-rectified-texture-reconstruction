#include <iostream>
#include <string>
#include "CLIargs.h"
#include <cxxopts.hpp>
using namespace std;

void path_filter(string &path)
{
    replace(path.begin(), path.end(), '\\', '/');
}

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
){
	cxxopts::Options options("OrthoTexture", "A 3D viewer with keyboard control.");
	options.add_options()
		("m,model", "Proxy model path", cxxopts::value<std::string>())
		("g,geometry-reference", "", cxxopts::value<std::string>())
		("target-size", "", cxxopts::value<vector<int>>()->default_value("3000,3000"))
		("proxy-tolerance", "", cxxopts::value<float>())
		("depth-tolerance", "", cxxopts::value<float>()->default_value("0.01"))
        ("point-radius", "", cxxopts::value<float>())
		("resource", "", cxxopts::value<string>()->default_value("../shaders"))
        ("image", "", cxxopts::value<string>())
        ("mask", "", cxxopts::value<string>())
        ("camera", "", cxxopts::value<string>())
        ("depth", "", cxxopts::value<string>())
        ("angle-threshold", "", cxxopts::value<float>())
        ("output", "", cxxopts::value<string>()->default_value("."))
		("h,help", "Print usage");
	auto result = options.parse(argc, argv);

	if (result.count("help"))
    {
    	std::cout << options.help() << std::endl;
    	exit(0);
    }

	proxy_model_path = result["model"].as<string>();
    path_filter(proxy_model_path);
	geo_ref_path = result["geometry-reference"].as<string>();
    path_filter(geo_ref_path);
	proxy_tolerance = result["proxy-tolerance"].as<float>();
	depth_tolerance = result["depth-tolerance"].as<float>();
    angle_threshold = result["angle-threshold"].as<float>();
    point_radius = result["point-radius"].as<float>();
    target_width = result["target-size"].as<vector<int>>()[0];
    target_height = result["target-size"].as<vector<int>>()[1];
    resource_path = result["resource"].as<string>();
    path_filter(resource_path);
    camera_path = result["camera"].as<string>();
    path_filter(camera_path);
    img_path = result["image"].as<string>();
    path_filter(img_path);
    depth_path = result["depth"].as<string>();
    path_filter(depth_path);
    mask_path = result["mask"].as<string>();
    path_filter(mask_path);
    output_path = result["output"].as<string>();
}
