# Sparse-Voxel-Octree-Raycasting
**Sparse Voxel Octree Raycasting with Image Warping exploiting Frame-to-Frame Coherence**


The idea is to exploit temporal coherence between two consecutive rendered images to speed up ray-casting. The algorithm stores the x- y- and z-coordinate for each pixel in the scene in a coordinate-buffer and re-project it into the following screen using the differential view matrix. 

The method then gathers empty 2x2 pixel blocks on the screen and stores them into an indexbuffer for raycasting the holes. Raycasting single pixels too inefficient. Small holes remaining after the hole-filling pass are closed by a simple image filter. To improve the overall quality, the method updates the screen in tiles (8x4) by raycasting an entire tile and overwriting the cache. Doing so, the entire cache is refreshed after 32 frames. Further, a triple buffer system is used. That means two image caches which are copied to alternately and one buffer that is written to. This is done since it often happens that a pixel is overwritten in one frame, but becomes visible already in the next frame. Therefore, before the hole filling starts, the two cache buffers are projected to the main image buffer.

Youtube Vid below (Raycasting 1920x1024@30-50fps)

[![HVOX Engine](https://github.com/sp4cerat/Sparse-Voxel-Octree-Raycasting/blob/master/screenshot.png?raw=true)](http://www.youtube.com/watch?v=ij0vw8yTCsY)


**Limitations:**


The method also comes with limitations of course. So the speed up depends on the motion in the scene obviously, and the method is only suitable for primary rays and pixel properties that remain constant over multiple frames, such as static ambient lighting. Further, during fast motions, the silhouettes of geometry close to the camera tends to loose precision and geometry in the background will not move as smooth as if the scene is fully raytraced each time. There, future work might include creating suitable image filters to avoid these effects.

**Results:**


Most of the pixels can be re-used using this technology. As only a fraction of the original needs to be raycasted, the speed up is significant and up to 5x the original speed, depending on the scene. 
