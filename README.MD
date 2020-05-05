# allwinner_h3_csc_gles
This application is a simple program to demonstrate color space conversion using GPU Mali-400 on Allwinner H3. The application was successfully tested on Orange Pi One board with HDMI disconnected.
  - Takes YUV420P file as input
  - Produces RGBA file(s) as output
 
## Requirements
* libdrm-dev
* libgbm-dev
* [Mali-400 kernel driver] - and [userspace blobs] for Allwinner
* Virtual KMS (DRM_VKMS=yes) for headless boards

## Build and install
- Copy OpenGLES r6p2 wayland blobs to /usr/lib/mali (do not forget cp with option -d while copying)
- Add /usr/lib/mali to ldconfig path
- Build linux kernel (4.20.17) (with VKMS support for headless boards)
- Build and install Mali driver (r6p2)
- Build the application on the target board
- Run as ./app -i yuv420p.file -w 640 -h 480

   [Mali-400 kernel driver]: https://github.com/mripard/sunxi-mali
   [userspace blobs]: https://github.com/bootlin/mali-blobs
   
