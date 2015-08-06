# Sparse-Voxel-Octree-Raycasting
Sparse Voxel Octree Raycasting with Image Warping exploiting Frame-to-Frame Coherence

Introduction: 
-------------
Since real-time raytracing is getting faster like with the Brigade Raytracer e.g., I believe this technology can be an important contribution to this area, as it might bring raytracing one step closer to being usable for video games.

Algorithm: 
-------------
A technology I am working on since a while now is to exploit temporal coherence between two consecutive rendered images to speed up ray-casting. The idea is to store the x- y- and z-coordinate for each pixel in the scene in a coordinate-buffer and re-project it into the following screen using the differential view matrix. The resulting image will look as Fig.1.

The method then gathers empty 2x2 pixel blocks on the screen and stores them into an indexbuffer for raycasting the holes. Raycasting single pixels too inefficient. Small holes remaining after the hole-filling pass are closed by a simple image filter. To improve the overall quality, the method updates the screen in tiles (8x4) by raycasting an entire tile and overwriting the cache. Doing so, the entire cache is refreshed after 32 frames. Further, a triple buffer system is used. That means two image caches which are copied to alternately and one buffer that is written to. This is done since it often happens that a pixel is overwritten in one frame, but becomes visible already in the next frame. Therefore, before the hole filling starts, the two cache buffers are projected to the main image buffer.

Limitations: 
-------------
The method also comes with limitations of course. So the speed up depends on the motion in the scene obviously, and the method is only suitable for primary rays and pixel properties that remain constant over multiple frames, such as static ambient lighting. Further, during fast motions, the silhouettes of geometry close to the camera tends to loose precision and geometry in the background will not move as smooth as if the scene is fully raytraced each time. There, future work might include creating suitable image filters to avoid these effects.

Results: 
-------------
Most of the pixels can be re-used using this technology. As only a fraction of the original needs to be raycasted, the speed up is significant and up to 5x the original speed, depending on the scene (see Fig.2 - Fig.4). Resolution for that test was 1024x768, the GPU was an NVIDIA GeForce GTX765M.

[![HVOX Engine](http://img.youtube.com/vi/ij0vw8yTCsY/0.jpg)](http://www.youtube.com/watch?v=ij0vw8yTCsY)
