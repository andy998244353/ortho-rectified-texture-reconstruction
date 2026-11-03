#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <stb_image.h>
#include <shader.h>
#include <model.h>
#include "ColmapViews.h"
#include "buffer.h"
#include "CLIargs.h"
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

void render_tolerance_map(Model &proxy_model, FrameBufferObject &target, Shader &shader, float proxy_tolerance, float depth_tolerance);
void render_position_map(Model &proxy_model, Model &geo_ref, FrameBufferObject &target, Shader &shader, float proxy_tolerance, float point_radius, unsigned int texture_tolerance);
void path_filter(string &path);
void render_texture(Model &proxy_model, FrameBufferObject &target, FrameBufferObject &final_output, Shader &shader_texture, Shader &shader_normalization, unsigned int texture_position, string camera_path, string img_path, string mask_path, string depth_path, string resource_path, float depth_tolerance, float angle_threshold);

int main(int argc, char ** argv)
{
    string proxy_model_path;
    string geo_ref_path;
    float proxy_tolerance;
    float depth_tolerance;
    float point_radius;
    int target_width;
    int target_height;
    string resource_path;
    string camera_path;
    string img_path;
    string depth_path;
    string mask_path;
    float angle_threshold;
    string output_path;

    cli_args(argc, argv, proxy_model_path, geo_ref_path, proxy_tolerance, depth_tolerance, point_radius, target_width, target_height, resource_path, camera_path, img_path, depth_path, mask_path, angle_threshold, output_path);

	init_app();
	config_app();

    Model proxy_model(proxy_model_path), geo_ref(geo_ref_path);
    FrameBufferObject tolerance_map(target_width, target_height, GL_R32F);
    Shader tolerance_shader((resource_path + "/shaders/tolerance.vs").c_str(), (resource_path + "/shaders/tolerance.fs").c_str());
    render_tolerance_map(proxy_model, tolerance_map, tolerance_shader, proxy_tolerance, depth_tolerance);

    FrameBufferObject position_map(target_width, target_height, GL_RGB, GL_R32F);
    Shader position_map_shader((resource_path + "/shaders/position_map.vs").c_str(), (resource_path + "/shaders/position_map.fs").c_str());
    render_position_map(proxy_model, geo_ref, position_map, position_map_shader, proxy_tolerance, point_radius, tolerance_map.texture1);
    CPUImageBuffer position_map_visual(target_width, target_height);
    position_map.read1(position_map_visual.data);
    position_map_visual.flip();
    position_map_visual.write_to_png(output_path + "/pos.png");

    FrameBufferObject texture_target(target_width, target_height, GL_RGB32F, GL_R32F);
    FrameBufferObject output_fbo(target_width, target_height, GL_RGB);
    Shader texture_shader((resource_path + "/shaders/texture.vs").c_str(), (resource_path + "/shaders/texture.fs").c_str());
    Shader normalization_shader((resource_path + "/shaders/blend.vs").c_str(), (resource_path + "/shaders/blend.fs").c_str());
    render_texture(proxy_model, texture_target, output_fbo, texture_shader, normalization_shader, position_map.texture2, camera_path, img_path, mask_path, depth_path, resource_path, depth_tolerance, angle_threshold);
    CPUImageBuffer output_buf(target_width, target_height);
    output_fbo.read1(output_buf.data);
    output_buf.flip();
    output_buf.write_to_png(output_path + "/texture.png");

	glfwTerminate();
	return 0;
}

void render_tolerance_map(Model &proxy_model, FrameBufferObject &target, Shader &shader, float proxy_tolerance, float depth_tolerance)
{
    target.use();
    GLfloat clearColor0[4] = {proxy_tolerance, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor0);
    glClear(GL_DEPTH_BUFFER_BIT);

    shader.use();
	for(int i = 0; i < proxy_model.meshes[0].indices.size(); i += 3){
		auto p1 = proxy_model.meshes[0].vertices[proxy_model.meshes[0].indices[i + 0]];
		auto p2 = proxy_model.meshes[0].vertices[proxy_model.meshes[0].indices[i + 1]];
		auto p3 = proxy_model.meshes[0].vertices[proxy_model.meshes[0].indices[i + 2]];
		auto n = glm::normalize(p1.Normal);
		auto uv1 = p1.TexCoords*2.0f - 1.0f;
		auto uv2 = p2.TexCoords*2.0f - 1.0f;
		auto uv3 = p3.TexCoords*2.0f - 1.0f;
		shader.setVec3("p1", p1.Position);
		shader.setVec3("p2", p2.Position);
		shader.setVec3("p3", p3.Position);
		shader.setVec2("uv1", uv1);
		shader.setVec2("uv2", uv2);
		shader.setVec2("uv3", uv3);
		shader.setVec3("n", n);
		shader.setFloat("proxy_tolerance", proxy_tolerance);
        shader.setFloat("depth_tolerance", depth_tolerance);
        proxy_model.Draw(shader);
    }
    config_app();
}

void render_position_map(Model &proxy_model, Model &geo_ref, FrameBufferObject &target, Shader &shader, float proxy_tolerance, float point_radius, unsigned int texture_tolerance)
{
	target.use();

    GLfloat clearColor0[4] = {0.663f, 0.820f, 0.557f, 1.0f};
    GLfloat clearColor1[4] = {HUGE_VALF, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor0);
    glClearBufferfv(GL_COLOR, 1, clearColor1);
    glClear(GL_DEPTH_BUFFER_BIT);

	shader.use();
	for(int i = 0; i < proxy_model.meshes[0].indices.size(); i += 3){
		auto p1 = proxy_model.meshes[0].vertices[proxy_model.meshes[0].indices[i + 0]];
		auto p2 = proxy_model.meshes[0].vertices[proxy_model.meshes[0].indices[i + 1]];
		auto p3 = proxy_model.meshes[0].vertices[proxy_model.meshes[0].indices[i + 2]];
		auto n = glm::normalize(p1.Normal);
		auto uv1 = p1.TexCoords*2.0f - 1.0f;
		auto uv2 = p2.TexCoords*2.0f - 1.0f;
		auto uv3 = p3.TexCoords*2.0f - 1.0f;
		shader.setVec3("p1", p1.Position);
		shader.setVec3("p2", p2.Position);
		shader.setVec3("p3", p3.Position);
		shader.setVec2("uv1", uv1);
		shader.setVec2("uv2", uv2);
		shader.setVec2("uv3", uv3);
		shader.setVec3("n", n);
		shader.setFloat("proxy_tolerance", proxy_tolerance);
        shader.setFloat("point_radius", point_radius);
        shader.setInt("width", target.width);
        shader.setInt("height", target.height);
        
        // 由 point_radius 计算 gl_pointsize
        glm::mat2 m1(uv2 - uv1, uv3 - uv1);
        glm::mat2x3 m2(p2.Position - p1.Position, p3.Position - p1.Position);
        glm::mat2x3 f = m2 * glm::inverse(m1);
        glm::vec3 f10 = f * glm::vec2(1,0);
        glm::vec3 f01 = f * glm::vec2(0,1);
        float cross = glm::length(glm::cross(f10, f01));
        float l10 = glm::length(f10);
        float l01 = glm::length(f01);
        float s = max(
            2*point_radius*l10 / cross,
            2*point_radius*l01 / cross
        ) * max(target.width, target.height);
        shader.setFloat("pointsize", s + 2);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_tolerance);
        shader.setInt("tolerance_map", 0);
        geo_ref.Draw(shader);
	}
    config_app();
}

string get_name_with_out_postfix(string name)
{
    return name.substr(0, name.find_last_of('.'));
}

Mesh getFullQuad(void)
{
	Vertex v1, v2, v3, v4;
	v1.Position = glm::vec3(-1, -1, 0);
	v1.TexCoords = glm::vec2(0, 0);
	v2.Position = glm::vec3(-1, +1, 0);
	v2.TexCoords = glm::vec2(0, 1);
	v3.Position = glm::vec3(+1, -1, 0);
	v3.TexCoords = glm::vec2(1, 0);
	v4.Position = glm::vec3(+1, +1, 0);
	v4.TexCoords = glm::vec2(1, 1);
	vector<Vertex> v = {v1, v2, v3, v4};
	vector<unsigned int> indices = {0, 1, 3, 0, 2, 3};
	return Mesh(v, indices, {});
}

void render_texture(Model &proxy_model, FrameBufferObject &target, FrameBufferObject &final_output, Shader &shader_texture, Shader &shader_normalization, unsigned int texture_position, string camera_path, string img_path, string mask_path, string depth_path, string resource_path, float depth_tolerance, float angle_threshold)
{
    glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_DEPTH_TEST);
    target.use();
    GLfloat clearColor0[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat clearColor1[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor0);
    glClearBufferfv(GL_COLOR, 1, clearColor1);

    auto texture_white_mask = TextureFromFile((resource_path + "/images/white_mask.png").c_str());
    auto cameras = loadColmapViews(camera_path);

    for(int i = 0; i < cameras.size(); i++){
        cout << "processing image " << cameras[i].imageName << " ... ";
        int width = cameras[i].width, height = cameras[i].height;
        auto texture_rgb = TextureFromFile((img_path + "/" + cameras[i].imageName).c_str());
        auto texture_depth = TextureFromFile((depth_path + "/" + cameras[i].imageName + ".geometric.bin").c_str());
        auto texture_mask = texture_white_mask;
        if(mask_path != "" && filesystem::exists(mask_path + "/" + get_name_with_out_postfix(cameras[i].imageName) + ".png"))
            texture_mask = TextureFromFile((mask_path + "/" + get_name_with_out_postfix(cameras[i].imageName) + ".png").c_str());
        auto view_mat = cameras[i].viewMat;
        auto proj_mat = cameras[i].projMat;
        shader_texture.use();
        glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_rgb);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture_depth);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture_position);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, texture_mask);
        shader_texture.setInt("image", 0);
        shader_texture.setInt("depth", 1);
        shader_texture.setInt("position", 2);
        shader_texture.setInt("mask", 3);
        shader_texture.setFloat("eps", depth_tolerance);
        shader_texture.setFloat("angle_threshold", angle_threshold);
        shader_texture.setMat4("view", view_mat);
        shader_texture.setMat4("proj", proj_mat);
        proxy_model.Draw(shader_texture);
        glDeleteTextures(1, &texture_rgb);
        glDeleteTextures(1, &texture_depth);
        if(texture_mask != texture_white_mask)
            glDeleteTextures(1, &texture_mask);
        cout << "done!" << endl;
    }
    
    glDisable(GL_BLEND);
    final_output.use();
    Mesh q = getFullQuad();
    shader_normalization.use();
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, target.texture1);
    glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, target.texture2);
    shader_normalization.setInt("color", 0);
    shader_normalization.setInt("weight", 1);
    q.Draw(shader_normalization);

    config_app();
}

