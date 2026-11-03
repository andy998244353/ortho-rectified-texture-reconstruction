"""
UV-aware texture inpainting with connected-component grouping.

Triangles of the 3D model are grouped into connected components (UV islands)
using spatial adjacency and normal-angle criteria.  Inpainting is then
performed per connected component, followed by a final holistic pass.

Usage:
  python inpaint.py --input=<img> --model=<model> [--output=<out>]
                         [--pdt=<mm>] [--nat=<deg>]
"""

import argparse
import math

import cv2
import numpy as np
import torch
import trimesh
from PIL import Image
from torch.nn.functional import conv2d
import math

def get_gaussian_kernel(sigma, size):
    w = torch.tensor([[math.exp(-(x**2 + y**2) / (2*sigma**2))  for y in range(-(size - 1)//2, (size - 1)//2 + 1)] for x in range(-(size - 1)//2, (size - 1)//2 + 1)])
    w /= w.sum()
    return w

def get_decomposed_gaussian_kernel(sigma, size):
    w = torch.tensor([math.exp(-x**2 / (2*sigma**2) ) for x in range(-(size - 1)//2, (size - 1)//2 + 1)])
    w /= w.sum()
    return w

def conv_inpainting_method(img: np.ndarray, mask: np.ndarray) -> np.ndarray:
    height, width, channels = img.shape
    I = torch.from_numpy(img.copy().astype(np.float32)).cuda().permute(2, 0, 1).reshape(1, 3, height, width)
    sigma = 2.5
    size = 15
    w1 = get_decomposed_gaussian_kernel(sigma, size)
    w2 = w1.clone()
    w1 = w1.cuda().reshape(1, 1, size, 1).repeat(3, 1, 1, 1)
    w2 = w2.cuda().reshape(1, 1, 1, size).repeat(3, 1, 1, 1)
    
    M = torch.from_numpy(mask).cuda().bool().reshape(1, 1, height, width).repeat(1,3,1,1)
    print("conv_inpainting")
    for it in range(1000):
        C = conv2d(I, w1, padding='same', groups=3)
        C = conv2d(C, w2, padding='same', groups=3)
        I[M] = C[M]
        print(f"\r迭代次数: {it+1}", end="", flush=True)
    I = I.cpu()
    torch.cuda.empty_cache()
    I = I.reshape(3, height, width).permute(1, 2, 0).numpy()
    I = np.clip(I, 0, 255).astype(np.uint8)
    return I

def cv2_telea_inpainting_method(img: np.ndarray, mask: np.ndarray) -> np.ndarray:
    """
    OpenCV Telea inpainting callable with the signature required by
    :func:`inpaint`.

    Uses ``cv2.inpaint`` with the ``INPAINT_TELEA`` algorithm (fast marching
    method).  Unlike LaMa, this is a classical (non-learned) method with no
    model download – it is fast but produces simpler results.

    Parameters
    ----------
    img : np.ndarray
        (H, W, 3) uint8 RGB image.
    mask : np.ndarray
        (H, W) bool or uint8.  ``1`` / ``True`` → pixel needs inpainting.

    Returns
    -------
    np.ndarray
        (H, W, 3) uint8 inpainted image.
    """
    # OpenCV expects mask as uint8, 0 = keep, non-zero = inpaint.
    mask_u8 = mask.astype(np.uint8)
    # Convert RGB to BGR for OpenCV, then back after inpainting.
    img_bgr = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
    result_bgr = cv2.inpaint(img_bgr, mask_u8, inpaintRadius=9,
                             flags=cv2.INPAINT_TELEA)
    result_rgb = cv2.cvtColor(result_bgr, cv2.COLOR_BGR2RGB)
    return result_rgb

# ============================================================================
# Step 1 – Model loading
# ============================================================================


def load_model(model_path: str) -> list:
    """
    Load a 3D model and return its triangles as a list.

    Each triangle is a list of 3 vertices.
    Each vertex is a list ``[x, y, z, u, v]`` (3D position + UV).

    Supports all formats that *trimesh* can read (OBJ, PLY, GLB, GLTF, …).

    Parameters
    ----------
    model_path : str
        Path to the 3D-model file.

    Returns
    -------
    list
        ``triangles[k] = [[x0,y0,z0,u0,v0], [x1,y1,z1,u1,v1], [x2,y2,z2,u2,v2]]``
    """
    scene_or_mesh = trimesh.load(model_path, force="mesh")

    # trimesh may return a Scene (multi-object file) or a single Trimesh.
    if isinstance(scene_or_mesh, trimesh.Scene):
        geometries = list(scene_or_mesh.geometry.values())
        if not geometries:
            raise ValueError(f"No mesh geometry found in {model_path}")
        # Merge all sub-meshes into one.
        mesh = geometries[0]
        for g in geometries[1:]:
            mesh = mesh + g
    else:
        mesh = scene_or_mesh

    # Check for UV data.
    uv = mesh.visual.uv
    if uv is None:
        raise ValueError(
            f"The model {model_path} has no UV coordinates. "
            f"Please unwrap the model first."
        )

    vertices_xyz = mesh.vertices   # (V, 3)
    faces = mesh.faces             # (F, 3) indices into vertices_xyz and uv

    triangles = []
    for face in faces:
        # face = [i0, i1, i2]  – indices shared by xyz and uv.
        v0 = vertices_xyz[face[0]]
        v1 = vertices_xyz[face[1]]
        v2 = vertices_xyz[face[2]]
        uv0 = uv[face[0]]
        uv1 = uv[face[1]]
        uv2 = uv[face[2]]

        tri = [
            [float(v0[0]), float(v0[1]), float(v0[2]), float(uv0[0]), float(uv0[1])],
            [float(v1[0]), float(v1[1]), float(v1[2]), float(uv1[0]), float(uv1[1])],
            [float(v2[0]), float(v2[1]), float(v2[2]), float(uv2[0]), float(uv2[1])],
        ]
        triangles.append(tri)

    return triangles


# ============================================================================
# Step 2 – Adjacency graph
# ============================================================================


def _vertex_dist(v_a, v_b):
    """
    Euclidean distance between two vertices (x, y, z only; UV is ignored).
    v_a, v_b are each ``[x, y, z, u, v]``.
    """
    dx = v_a[0] - v_b[0]
    dy = v_a[1] - v_b[1]
    dz = v_a[2] - v_b[2]
    return math.sqrt(dx * dx + dy * dy + dz * dz)


def _triangle_normal(tri):
    """
    Return the unit normal of a triangle (length-3 numpy vector).

    tri : ``[[x0,y0,z0,...], [x1,y1,z1,...], [x2,y2,z2,...]]``
    """
    p0 = np.array([tri[0][0], tri[0][1], tri[0][2]], dtype=np.float64)
    p1 = np.array([tri[1][0], tri[1][1], tri[1][2]], dtype=np.float64)
    p2 = np.array([tri[2][0], tri[2][1], tri[2][2]], dtype=np.float64)

    # cross(edge01, edge02)
    e1 = p1 - p0
    e2 = p2 - p0
    n = np.cross(e1, e2)
    norm = np.linalg.norm(n)
    if norm < 1e-15:
        return np.array([0.0, 0.0, 1.0])  # degenerate triangle fallback
    return n / norm


def _normal_angle_degrees(n_a, n_b):
    """
    Angle between two normals, taking the smaller of angle(n_a, n_b) and
    angle(n_a, -n_b) (since normals can be flipped).
    Returns degrees in [0, 180].
    """
    # cos(theta) = dot(n_a, n_b) — clamped for numerical safety.
    dot = np.dot(n_a, n_b)
    dot = max(-1.0, min(1.0, dot))
    angle_same = math.degrees(math.acos(dot))

    # opposite direction
    dot_flip = np.dot(n_a, -n_b)
    dot_flip = max(-1.0, min(1.0, dot_flip))
    angle_flipped = math.degrees(math.acos(dot_flip))

    return min(angle_same, angle_flipped)


def _triangles_share_edge(tri_a, tri_b, pdt):
    """
    Return True if *tri_a* and *tri_b* share an edge.

    Two edges are considered the same edge when each pair of corresponding
    endpoints lies within *pdt* of one another (either match orientation).
    """
    # Enumerate the 3 edges of each triangle.
    edges_a = [(0, 1), (1, 2), (2, 0)]
    edges_b = [(0, 1), (1, 2), (2, 0)]

    for ia, ja in edges_a:
        for ib, jb in edges_b:
            # Two possible pairings:
            match_direct = (
                _vertex_dist(tri_a[ia], tri_b[ib]) < pdt
                and _vertex_dist(tri_a[ja], tri_b[jb]) < pdt
            )
            match_crossed = (
                _vertex_dist(tri_a[ia], tri_b[jb]) < pdt
                and _vertex_dist(tri_a[ja], tri_b[ib]) < pdt
            )
            if match_direct or match_crossed:
                return True
    return False


def build_graph(triangles: list, pdt: float, nat: float) -> list:
    """
    Build the triangle adjacency graph.

    Two triangles are adjacent iff:

    1. they share an edge (corresponding vertex distance < *pdt*), **and**
    2. their normals are similar (min angle < *nat* degrees, accounting for
       possible normal-flip).

    Parameters
    ----------
    triangles : list
        Output of :func:`load_model`.
    pdt : float
        Point-distance threshold (same unit as model coordinates).
    nat : float
        Normal-angle threshold in degrees.

    Returns
    -------
    list of list of int
        ``adj[k]`` is a list of triangle ids that are adjacent to triangle *k*.
    """
    n = len(triangles)

    # Pre-compute normals (lazily, but n ≤ ~100 so just compute all upfront).
    normals = [_triangle_normal(t) for t in triangles]

    adj = [[] for _ in range(n)]

    # O(n²) brute-force pair check – acceptable for tens of triangles.
    for i in range(n):
        for j in range(i + 1, n):
            # --- edge check ---------------------------------------------------
            if not _triangles_share_edge(triangles[i], triangles[j], pdt):
                continue

            # --- normal check -------------------------------------------------
            angle = _normal_angle_degrees(normals[i], normals[j])
            if angle >= nat:
                continue

            # Both conditions satisfied → adjacency.
            adj[i].append(j)
            adj[j].append(i)

    return adj


# ============================================================================
# Step 3 – Connected components
# ============================================================================


def get_connected_components(adj: list) -> list:
    """
    Compute connected components from an adjacency graph.

    Parameters
    ----------
    adj : list of list of int
        Adjacency graph as returned by :func:`build_graph`.

    Returns
    -------
    list of set of int
        Each element is a set of triangle ids belonging to one component.
    """
    n = len(adj)
    visited = [False] * n
    components = []

    for i in range(n):
        if visited[i]:
            continue
        # BFS / DFS to collect the component.
        comp = set()
        stack = [i]
        visited[i] = True
        while stack:
            v = stack.pop()
            comp.add(v)
            for nb in adj[v]:
                if not visited[nb]:
                    visited[nb] = True
                    stack.append(nb)
        components.append(comp)

    return components


# ============================================================================
# Step 4 – Rasterize connected component UV to pixel mask
# ============================================================================


def rasterize_uv(component: set, triangles: list, h: int, w: int) -> np.ndarray:
    """
    Rasterize all UV triangles of a connected component into a single boolean
    pixel mask.

    Parameters
    ----------
    component : set of int
        Triangle ids belonging to one connected component.
    triangles : list
        Output of :func:`load_model`.
    h, w : int
        Height and width of the target texture in pixels.

    Returns
    -------
    np.ndarray
        ``(h, w)`` bool mask – ``True`` where a pixel centre falls inside any
        of the component's UV triangles.
    """
    # Collect all pixel-space triangles as integer point arrays.
    polygons = []
    for tid in component:
        tri = triangles[tid]  # [[x,y,z,u,v], [x,y,z,u,v], [x,y,z,u,v]]
        # Extract UV from each vertex: (u, v) are indices 3 & 4.
        # UV origin is conventionally bottom-left; image origin is top-left.
        px = np.empty((3, 2), dtype=np.int32)
        px[:, 0] = np.round(np.array([tri[k][3] for k in range(3)]) * (w - 1)).astype(np.int32)
        px[:, 1] = np.round((1.0 - np.array([tri[k][4] for k in range(3)])) * (h - 1)).astype(np.int32)
        polygons.append(px)

    mask = np.zeros((h, w), dtype=np.uint8)
    cv2.fillPoly(mask, polygons, color=1)
    return mask.astype(bool)


# ============================================================================
# Core inpainting
# ============================================================================


def _is_black(img: np.ndarray, threshold: int = 0) -> np.ndarray:
    """Return ``(H, W)`` bool mask – ``True`` where pixel is (near) pure black."""
    return np.all(img <= threshold, axis=-1)


def inpaint(
    img: np.ndarray,
    triangles: list,
    components: list,
) -> np.ndarray:
    """
    Parameters
    ----------
    img : np.ndarray
        (H, W, 3) uint8 – the original texture.
    triangles : list
        Output of :func:`load_model`.
    components : list of set of int
        Connected components as returned by :func:`get_connected_components`.

    Returns
    -------
    np.ndarray
        (H, W, 3) uint8 inpainted texture.
    """
    h, w = img.shape[:2]
    black_mask = _is_black(img) # black = True
    result = np.zeros_like(img)
    filled = np.zeros((h, w), dtype=bool)
    total = len(components)

    for idx, comp in enumerate(components):
        comp_mask = rasterize_uv(comp, triangles, h, w)
        if not comp_mask.any():
            continue
        keep = comp_mask & ~black_mask
        if not keep.any():
            continue
        inpainted = cv2_telea_inpainting_method(img, ~keep)
        inpainted = conv_inpainting_method(inpainted, comp_mask & ~keep)
        new_region = comp_mask & ~filled
        result[new_region] = inpainted[new_region]
        filled |= comp_mask
        print(f"  component {idx + 1}/{total}  "
              f"({len(comp)} triangles)")

    # ---- final holistic pass -------------------------------------------------
    remaining_black = _is_black(result)
    if remaining_black.any():
        print("  final holistic pass …")
        result = conv_inpainting_method(result, remaining_black)

    return result


# ============================================================================
# CLI
# ============================================================================


def main():
    parser = argparse.ArgumentParser(
        description="UV-aware texture inpainting (LaMa) with connected components",
    )
    parser.add_argument(
        "--input", required=True,
        help="Path to the input texture image (PNG, JPG, …).",
    )
    parser.add_argument(
        "--model", required=True,
        help="Path to the 3D model (OBJ, PLY, GLB, …) from which UVs are extracted.",
    )
    parser.add_argument(
        "--output", default=None,
        help="Path for the output image.  Defaults to overwriting --input.",
    )
    parser.add_argument(
        "--pdt", type=float, default=0.001,
        help="Point-distance threshold for edge matching (model units).  "
             "Default: 0.001.",
    )
    parser.add_argument(
        "--nat", type=float, default=5.0,
        help="Normal-angle threshold in degrees.  "
             "Two normals are considered equal when the smaller of "
             "angle(n1,n2) and angle(n1,-n2) is below this value.  "
             "Default: 10.0.",
    )
    args = parser.parse_args()

    # ---- load image -----------------------------------------------------------
    img_pil = Image.open(args.input).convert("RGB")
    img_array = np.array(img_pil)
    print(f"Loaded image  {args.input}  "
          f"({img_array.shape[1]}×{img_array.shape[0]})")

    # ---- load model → triangles -----------------------------------------------
    triangles = load_model(args.model)
    print(f"Loaded model  {args.model}  ({len(triangles)} triangles)")

    # ---- build adjacency graph ------------------------------------------------
    adj = build_graph(triangles, args.pdt, args.nat)
    total_edges = sum(len(nb) for nb in adj) // 2
    print(f"Adjacency graph: {total_edges} edges  "
          f"(pdt={args.pdt}, nat={args.nat}°)")

    # ---- connected components -------------------------------------------------
    components = get_connected_components(adj)
    print(f"Connected components: {len(components)}")

    # ---- inpaint --------------------------------------------------------------
    print("Inpainting …")
    result_array = inpaint(img_array, triangles, components)

    # ---- save -----------------------------------------------------------------
    output_path = args.output if args.output else args.input
    Image.fromarray(result_array).save(output_path)
    print(f"Saved result  → {output_path}")


if __name__ == "__main__":
    main()
