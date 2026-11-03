#ifndef COLMAP_VIEWS_H
#define COLMAP_VIEWS_H
#include <glm/glm.hpp>
#include <string>
#include <vector>

class PosedView{
public:
	std::string imageName;
	glm::mat4 viewMat;
	glm::mat4 projMat;
	int width;
	int height;
	PosedView(std::string path, float qw, float qx, float qy, float qz, float tx, float ty, float tz, int width, int height, float fx, float fy, float cx, float cy);
};

std::vector<PosedView> loadColmapViews(std::string path);

#endif
