
// gcc -o drmgl Linux_DRM_OpenGLES.c `pkg-config --cflags --libs libdrm` -lgbm -lEGL -lGLESv2

/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 * Copyright (c) 2017 Miouyouyou <Myy> <myy@miouyouyou.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "color.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


static struct {
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    GLuint program;
    GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
    GLuint vbo;
    GLuint positionsoffset, colorsoffset, normalsoffset;
} gl;

static struct {
    struct gbm_device *dev;
    struct gbm_surface *surface;
} gbm;

static struct {
    int fd;
    drmModeModeInfo *mode;
    uint32_t crtc_id;
    uint32_t connector_id;
} drm;

/*
 * That is supposed to work with virtual KMS drivere (kernel option DRM_VKMS=yes)
 * so we don't need full initialization of DRM and GBM
 */
static int init_drm(void) {
    drm.fd = open("/dev/dri/card0", O_RDWR);
    
    if (drm.fd < 0) {
        printf("could not open drm device\n");
        return -1;
    }
    return 0;
}

static int init_gbm(void) {
    gbm.dev = gbm_create_device(drm.fd);
    return 0;
}

static int init_gl(int w, int h) {
    EGLint major, minor, n;

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

     EGLint pbuffer_attribs[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_NONE,
    };

    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
    get_platform_display = (void *) eglGetProcAddress("eglGetPlatformDisplayEXT");
    assert(get_platform_display != NULL);

#ifdef EGL_KHR_platform_gbm
    gl.display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);
#endif
    if (gl.display == EGL_NO_DISPLAY) {
        gl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

    if (gl.display == EGL_NO_DISPLAY) {
    	printf("Platform has no display\n");
    	return -1;
    }

    if (!eglInitialize(gl.display, &major, &minor)) {
        printf("failed to initialize display\n");
        return -1;
    }

    printf("Using display %p with EGL version %d.%d\n", gl.display, major, minor);

    printf("EGL Version \"%s\"\n", eglQueryString(gl.display, EGL_VERSION));
    printf("EGL Vendor \"%s\"\n", eglQueryString(gl.display, EGL_VENDOR));
    printf("EGL Extensions \"%s\"\n", eglQueryString(gl.display, EGL_EXTENSIONS));

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("failed to bind api EGL_OPENGL_ES_API\n");
        return -1;
    }

    if (!eglChooseConfig(gl.display, config_attribs, &gl.config, 1, &n) || n != 1) {
        printf("failed to choose config: %d\n", n);
        return -1;
    }

    gl.context = eglCreateContext(gl.display, gl.config, EGL_NO_CONTEXT, context_attribs);
    if (gl.context == NULL) {
        printf("failed to create context\n");
        return -1;
    }

    gl.surface = eglCreatePbufferSurface(gl.display, gl.config, pbuffer_attribs);
	if (gl.surface == EGL_NO_SURFACE) {
        printf("Error! Failed to eglCreatePbufferSurface\n");
        return -1;
	}   

    /* connect the context to the surface */
    eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);

    return 0;
}


#define N_FRAMES 100

int main(int argc, char *argv[]) {
    int frame = 0;
	uint32_t i = 0;
    int fw, fh;
    struct timeval start, end;
    float sum = 0, fps;
    int ret, opt;
    char input_file[150] = "frame1_1280x720.yuv";

    fw = 1280; //1280
    fh = 720; //720

    while ((opt = getopt(argc, argv, "v:i:o:w:h:f:")) != -1) {
        switch (opt) {
            case 'i':
                strcpy(input_file, optarg);
                break;
            case 'w':
                fw = atoi(optarg);
                break;   
            case 'h':
                fh = atoi(optarg);
                break;    
            default:
                printf("Usage: %s -i input file -w width -h height\n", argv[0]);
                exit(0);
                break;    
        }
    }


    ret = init_drm();
    if (ret) {
        printf("failed to initialize DRM\n");
        return ret;
    }
    printf("DRM OK\n");

    ret = init_gbm();
    if (ret) {
        printf("failed to initialize GBM\n");
        return ret;
    }

    printf("GDM OK\n");

    ret = init_gl(fw, fh);
    if (ret) {
        printf("failed to initialize EGL\n");
        return ret;
    }
    printf("GL OK\n");

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("failed to bind api EGL_OPENGL_ES_API\n");
        return -1;
    }

    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    int sz = conv_init(fw, fh, gl.display);
    conv_get_frame(input_file, sz);

    printf("Do the job\n");
    gettimeofday(&start, NULL);

    while (i++ < N_FRAMES) {
        /* do the job */
    	conv_render_frame(fw, fh);
        eglSwapBuffers(gl.display, gl.surface);
        
        if (++frame % 60 == 0) {
          gettimeofday(&end, NULL);
          float t = end.tv_sec + end.tv_usec * 1e-6 - (start.tv_sec + start.tv_usec * 1e-6);
          fps = 60 / t;
          sum += fps;
          start = end;
        }

    }
    fps = sum / (N_FRAMES / 60);
    printf("DONE! FPS: %f\n", fps);

    return 0;
}
