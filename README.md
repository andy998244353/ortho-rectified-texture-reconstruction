# Ortho-rectified texture reconstruction for 3D planar proxy models

This is the official implementation of the paper "Ortho-rectified Texture Reconstruction for 3D Planar Proxy Models". This project performs texture reconstruction for a given planar proxy model (simplified model) using multi-view RGB images, depth images, and a dense point cloud. After texture reconstruction, the missing regions are filled using an inpainting step.

```
TODO: bibtex to be provided upon acceptance
```

## Dependencies

Our texture reconstruction pipeline was built on **Windows 11** with **CMake 4.1.0** and **MinGW 14.2.0**. It should also compile successfully on other operating systems and with other compilers, although we have not tested them.

If you want to run the inpainting module, you also need to install a Python environment:

- Python==3.10
- numpy==1.26.4
- opencv-python==4.11.0.86
- torch==2.5.1
- trimesh==4.10.0
- Pillow==9.5.0

Please follow the instructions on the official PyTorch website (https://pytorch.org/get-started/locally/) to install the CUDA-enabled version of torch.

## Compilation

1. `git clone https://github.com/andy998244353/ortho-rectified-texture-reconstruction.git`
2. `cd ortho-rectified-texture-reconstruction/build`
4. `cmake -G "MinGW Makefiles" ..`
5. `cmake .` (You can also use the `-j <num-threads>` option to enable parallel compilation.)

## Run on our datasets

Data are provided on the Release page.

Make sure to place the compiled executable file (you can also download the precompiled executable file from the Release page) in the `build` directory, and put the dataset in the `datasets` directory. The project directory structure should look like this:

```
ortho-rectified-texture-reconstruction
	build
		texture.exe
		...
	datasets
		<dataset-name>
			proxy.obj
			results
			run.bat
			...
		...
	...
```

Run `run.bat` in the dataset folder to start the program. If you do not need to run the inpainting module, please comment out the corresponding command in `run.bat`. The reconstructed texture will be saved in the `results` directory under the dataset folder. You can use tools such as Blender to visualize the generated textures applied to `proxy.obj`.

## Run on your own datasets

To prepare your dataset, the following items must be provided:

- Planar proxy model file

  We recommend using the OBJ file format. The model should contain UV coordinates and face normals.

- Color image set

  All images should be placed flatly in the same folder. We recommend using JPG or PNG format.

- Depth image set

  Each image in the color image set must correspond to a depth image. The correspondence is determined by appending the `.geometric.bin` suffix to the filename of each color image. For example, the depth map corresponding to `aaa.jpg` is `aaa.jpg.geometric.bin`. These depth images should also be placed flatly in a single folder. The COLMAP depth map format are used. You can export these depth maps from COLMAP. Please refer to <https://colmap.github.io/format.html#depth-and-normal-maps> for details about the depth map format.

- Point cloud file

  We recommend using the PLY file format. The point cloud can be exported from the dense reconstruction module of COLMAP.

- Camera parameters

  The folder containing the camera poses should include two files: `images.txt` and `cameras.txt`, which provide the camera poses and intrinsic parameters for each image. These files can be exported from the sparse reconstruction module of COLMAP. For details about the file format, please refer to <https://colmap.github.io/format.html#cameras-txt>. Note that this project only supports the `PINHOLE` camera model. You can use the undistortion module in COLMAP to convert other camera models. See <https://colmap.github.io/cli.html#commands> for more details.

Note that the camera parameters, depth images, proxy model, and point cloud must all be aligned within the same coordinate system.

To run this project on your own dataset, you need to execute `texture.exe` from the CLI and provide the corresponding command-line arguments. The list of arguments and their descriptions are as follows:

- `-m`

  Path to the planar proxy model file.

- `--image`

  Path to the directory containing the color image set.

- `--depth`

  Path to the directory containing the depth image set.

- `-g`

  Path to the point cloud file.

- `--camera`

  Path to the directory containing the camera parameters.

- `--resource`

  Path to the `resource` directory under the project root directory.

- `--output`

  Path to the output directory.

- `--target-size`

  Size of the output texture. For example, `"5000,5000"` indicates an output texture resolution of 5000px $\times$ 5000px.

- `--proxy-tolerance`

  Parameter $\epsilon_p$ in the paper.

- `--point-radius`

  Parameter $r_d$ in the paper.

- `--depth-tolerance`

  Parameter $\epsilon_d$ in the paper.

- `--angle-threshold`

  Parameter $\epsilon_a$ in the paper.

For examples of how to use the command-line arguments, please refer to the `run.bat` file provided in our datasets.

To perform inpainting, run:

`python "./scripts/inpaint.py" --input=<texture image path> --model=<proxy model path> --output=<output path+file name>`

## Acknowledgements

Our project uses the following third-party projects:

- <https://github.com/assimp/assimp>
- <https://github.com/jarro2783/cxxopts>
- <https://glad.dav1d.de/>
- <https://github.com/glfw/glfw>
- <https://github.com/g-truc/glm>
- <https://github.com/JoeyDeVries/LearnOpenGL>
- <https://github.com/nothings/stb>

