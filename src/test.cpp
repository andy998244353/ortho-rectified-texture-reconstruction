#include <cstring>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <shader.h>
#include <model.h>
#include "ColmapViews.h"
#include "buffer.h"
#include <cxxopts.hpp>
using namespace std;

void init_app(void)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24); 
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);


	GLFWwindow* window = glfwCreateWindow(114, 514, "viewer", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(-1);
	}
	stbi_set_flip_vertically_on_load(true);
}

void config_app(void)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glDisable(GL_BLEND);
}

string get_name_with_out_postfix(string name)
{
    return name.substr(0, name.find_last_of('.'));
}

void path_filter(string &path)
{
    replace(path.begin(), path.end(), '\\', '/');
}

int main(int argc, char ** argv)
{
	string model_path;
	string camera_path;
	string resource_path;
	string output_path;

	cxxopts::Options options("test", "");
	options.add_options()
		("m,model", "Proxy model path", cxxopts::value<std::string>())
		("resource", "", cxxopts::value<string>()->default_value("../shaders"))
        ("camera", "", cxxopts::value<string>())
		("output", "", cxxopts::value<string>())
		("h,help", "Print usage");
	auto result = options.parse(argc, argv);

	if (result.count("help"))
    {
    	std::cout << options.help() << std::endl;
    	exit(0);
    }

	model_path = result["model"].as<string>();
	camera_path = result["camera"].as<string>();
	resource_path = result["resource"].as<string>();
	output_path = result["output"].as<string>();
	path_filter(model_path);
	path_filter(camera_path);
	path_filter(resource_path);
	path_filter(output_path);

	init_app();
	config_app();

	Model model(model_path);
	Shader shader((resource_path + "/shaders/render.vs").c_str(), (resource_path + "/shaders/render.fs").c_str());
	auto cameras = loadColmapViews(camera_path);
	for(int i = 0; i < cameras.size(); i++){
		auto view = cameras[i].viewMat;
		auto proj = cameras[i].projMat;
		FrameBufferObject fbo(cameras[i].width, cameras[i].height, GL_RGB);
		fbo.use();
		glClearColor(1,1,1,1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.use();
		shader.setMat4("view", view);
		shader.setMat4("proj", proj);
		model.Draw(shader);
		CPUImageBuffer buf(cameras[i].width, cameras[i].height);
		fbo.read1(buf.data);
		buf.flip();
		buf.write_to_png(output_path + "/" + get_name_with_out_postfix(cameras[i].imageName) + ".png");
	}

	glfwTerminate();
	return 0;
}

