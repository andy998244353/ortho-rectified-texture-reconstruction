#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ColmapViews.h"
using namespace std;

struct PinholeCamera
{
	int id;
	int width;
	int height;
	float fx;
	float fy;
	float cx;
	float cy;
};

struct Image{
	int id;
	float qw;
	float qx;
	float qy;
	float qz;
	float tx;
	float ty;
	float tz;
	int cam_id;
	string name;
};

vector<PinholeCamera> readCameras(string filename)
{
    std::vector<PinholeCamera> cameras;
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return cameras;
    }

    std::string line;
    while (std::getline(fin, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        int id, width, height;
        std::string model;
        double fx, fy, cx, cy;

        if (ss >> id >> model >> width >> height >> fx >> fy >> cx >> cy)
        {
            if (model == "PINHOLE")
            {
                PinholeCamera cam;
                cam.id = id;
                cam.width = width;
                cam.height = height;
                cam.fx = static_cast<float>(fx);
                cam.fy = static_cast<float>(fy);
                cam.cx = static_cast<float>(cx);
                cam.cy = static_cast<float>(cy);
                cameras.push_back(cam);
            }
        }
    }

    return cameras;
}



PosedView::PosedView(string path, float qw, float qx, float qy, float qz, float tx, float ty, float tz, int width, int height, float fx, float fy, float cx, float cy){
    this->width = width;
    this->height = height;
	viewMat = glm::rotate(glm::mat4(1), glm::radians((float)180.0), glm::vec3(1, 0, 0)) * glm::translate(glm::mat4(1), glm::vec3(tx, ty, tz)) * glm::mat4_cast(glm::quat(qw, qx, qy, qz));

	float near = 0.01;
	float far = 1000;
	float left   = -cx * near / fx;
	float right  =  (width - cx) * near / fx;
	float bottom = -cy * near / fy;
	float top    =  (height - cy) * near / fy;

	glm::mat4 P(0.0f);

	P[0][0] = 2.0f * near / (right - left);
	P[1][1] = 2.0f * near / (top - bottom);
	P[2][0] = (right + left) / (right - left);
	P[2][1] = (top + bottom) / (top - bottom);
	P[2][2] = -(far + near) / (far - near);
	P[2][3] = -1.0f;
	P[3][2] = -2.0f * far * near / (far - near);
	projMat = P;

	imageName = path;
}

vector<Image> readImages(string filename) {
    std::vector<Image> images;
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return images;
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') 
            continue;

        std::stringstream ss(line);
        int id, cam_id;
        double qw, qx, qy, qz, tx, ty, tz;
        std::string name;

        if (ss >> id >> qw >> qx >> qy >> qz >> tx >> ty >> tz >> cam_id >> name) {
            Image img;
            img.id = id;
            img.qw = static_cast<float>(qw);
            img.qx = static_cast<float>(qx);
            img.qy = static_cast<float>(qy);
            img.qz = static_cast<float>(qz);
            img.tx = static_cast<float>(tx);
            img.ty = static_cast<float>(ty);
            img.tz = static_cast<float>(tz);
            img.cam_id = cam_id;
            img.name = name;

            images.push_back(img);
        }
        std::getline(fin, line);
    }

    return images;
}

vector<PosedView> loadColmapViews(string path)
{
	vector<PosedView> res;
	vector<PinholeCamera> cameras = readCameras(path + "/" + "cameras.txt");
	vector<Image> images = readImages(path + "/" + "images.txt");
	for(auto img: images){
		auto cam = cameras[img.cam_id - 1];
		res.push_back(
			PosedView(
				img.name,
				img.qw,
				img.qx,
				img.qy,
				img.qz,
				img.tx,
				img.ty,
				img.tz,
				cam.width,
				cam.height,
				cam.fx,
				cam.fy,
				cam.cx,
				cam.cy
			)
		);
	}
	return res;
}


