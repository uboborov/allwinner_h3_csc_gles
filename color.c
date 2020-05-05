#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <string.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <math.h>  

static const char* VERTEX_SHADER =    
      "attribute vec4 vPosition;    \n"  
      "attribute vec2 a_texCoord;   \n"  
      "varying vec2 tc;     \n"  
      "void main()                  \n"  
      "{                            \n"  
      "   gl_Position = vPosition;  \n"  
      "   tc = a_texCoord;  \n"  
      "}                            \n";  

static const char* FRAG_SHADER =  
    "varying lowp vec2 tc;\n"  
    "uniform sampler2D SamplerY;\n"  
    "uniform sampler2D SamplerU;\n"  
    "uniform sampler2D SamplerV;\n"  
    "void main(void)\n"  
    "{\n"  
        "mediump vec3 yuv;\n"  
        "lowp vec3 rgb;\n"  
        "yuv.x = texture2D(SamplerY, tc).r;\n"  
        "yuv.y = texture2D(SamplerU, tc).r - 0.5;\n"  
        "yuv.z = texture2D(SamplerV, tc).r - 0.5;\n"  
        "rgb = mat3( 1,   1,   1,\n"  
                    "0,       -0.39465,  2.03211,\n"  
                    "1.13983,   -0.58060,  0) * yuv;\n"  
        "gl_FragColor = vec4(rgb, 1);\n"  
    "}\n"; 

  
enum {  
    ATTRIB_VERTEX,  
    ATTRIB_TEXTURE,  
};  
  
static GLuint g_texYId;  
static GLuint g_texUId;  
static GLuint g_texVId;  
static GLuint simpleProgram;  


static GLuint g_render_buf;
static GLuint g_frame_buf;

static EGLImageKHR g_ImageKHR;
static EGLDisplay  g_display;
  
static char *              g_buffer = NULL;  
static int                 g_width = 0;  
static int                 g_height = 0;  
static char *              g_buffer_out = NULL; 

//#define USE_PBO

#ifdef USE_PBO 
# define PBO_COUNT   2
static GLuint pboIds[PBO_COUNT];
#endif  

PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC egl_image_target_renderbuffer_storage_oes = NULL;
  
void checkGlError(const char* op) {  
    GLint error;  
    for (error = glGetError(); error; error = glGetError()) {  
        printf("error::after %s() glError (0x%x)\n", op, error);  
    }  
}  
  
static GLuint bind_texture(GLuint texture, const char *buffer, GLuint w , GLuint h) {  
//  GLuint texture;  
//  glGenTextures ( 1, &texture );  
    checkGlError("glGenTextures");  
    glBindTexture ( GL_TEXTURE_2D, texture );  
    checkGlError("glBindTexture");  
    glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);  
    checkGlError("glTexImage2D");  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );  
    checkGlError("glTexParameteri");  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );  
    checkGlError("glTexParameteri");  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );  
    checkGlError("glTexParameteri");  
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );  
    checkGlError("glTexParameteri");  
    //glBindTexture(GL_TEXTURE_2D, 0);  
  
    return texture;  
}  
  
static void render_frame() {  
    // yuv420p => RGBA ( ffplay -f rawvideo -pix_fmt rgba )
    static GLfloat squareVertices[] = {  
        -1.0f, -1.0f,  
        1.0f, -1.0f,  
        -1.0f,  1.0f,  
        1.0f,  1.0f,  
    };  
    
    static GLfloat coordVertices[] = {  
        0.0f, 0.0f,  
        1.0f, 0.0f,  
        0.0f,  1.0f,  
        1.0f,  1.0f,  
    };  

    glUseProgram(simpleProgram);
    checkGlError("glUseProgram");
    glClearColor(0.5f, 0.5f, 0.5f, 1);  
    checkGlError("glClearColor");    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    checkGlError("glClear");  

    GLint tex_y = glGetUniformLocation(simpleProgram, "SamplerY");  
    checkGlError("glGetUniformLocation");  
    GLint tex_u = glGetUniformLocation(simpleProgram, "SamplerU");  
    checkGlError("glGetUniformLocation");  
    GLint tex_v = glGetUniformLocation(simpleProgram, "SamplerV");  
    checkGlError("glGetUniformLocation");  
  
    glBindAttribLocation(simpleProgram, ATTRIB_VERTEX, "vPosition");  
    checkGlError("glBindAttribLocation");  
    glBindAttribLocation(simpleProgram, ATTRIB_TEXTURE, "a_texCoord");  
    checkGlError("glBindAttribLocation");  
  
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);  
    checkGlError("glVertexAttribPointer");  
    glEnableVertexAttribArray(ATTRIB_VERTEX);  
    checkGlError("glEnableVertexAttribArray");  
  
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, coordVertices);  
    checkGlError("glVertexAttribPointer");  
    glEnableVertexAttribArray(ATTRIB_TEXTURE);  
    checkGlError("glEnableVertexAttribArray");  
  
    glActiveTexture(GL_TEXTURE0);  
    checkGlError("glActiveTexture");  
    glBindTexture(GL_TEXTURE_2D, g_texYId);  
    checkGlError("glBindTexture");  
    glUniform1i(tex_y, 0);  
    checkGlError("glUniform1i");  
  
    glActiveTexture(GL_TEXTURE1);  
    checkGlError("glActiveTexture");  
    glBindTexture(GL_TEXTURE_2D, g_texUId);  
    checkGlError("glBindTexture");  
    glUniform1i(tex_u, 1);  
    checkGlError("glUniform1i");  
  
    glActiveTexture(GL_TEXTURE2);  
    checkGlError("glActiveTexture");  
    glBindTexture(GL_TEXTURE_2D, g_texVId);  
    checkGlError("glBindTexture");  
    glUniform1i(tex_v, 2);  
    checkGlError("glUniform1i");  
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  
    checkGlError("glDrawArrays");  
}  
  
static GLuint build_shader(const char* source, GLenum shaderType) {  
    GLuint shaderHandle = glCreateShader(shaderType);  
  
    if (shaderHandle) {  
        glShaderSource(shaderHandle, 1, &source, 0);  
        glCompileShader(shaderHandle);  
  
        GLint compiled = 0;  
        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compiled);  
        if (!compiled) {  
            GLint infoLen = 0;  
            glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &infoLen);  
            if (infoLen) {  
                char* buf = (char*) malloc(infoLen);  
                if (buf) {  
                    glGetShaderInfoLog(shaderHandle, infoLen, NULL, buf);  
                    printf("error::Could not compile shader %d:\n%s\n", shaderType, buf);  
                    free(buf);  
                }  
                glDeleteShader(shaderHandle);  
                shaderHandle = 0;  
            }  
        }  
  
    }  
      
    return shaderHandle;  
}  
  
static GLuint build_program(const char* vertexShaderSource, const char* fragmentShaderSource) {  
    GLuint vertexShader =   build_shader(vertexShaderSource, GL_VERTEX_SHADER);  
    GLuint fragmentShader = build_shader(fragmentShaderSource, GL_FRAGMENT_SHADER);  
    GLuint programHandle =  glCreateProgram();  
  
    if (programHandle) {  
        glAttachShader(programHandle, vertexShader);  
        checkGlError("glAttachShader");  
        glAttachShader(programHandle, fragmentShader);  
        checkGlError("glAttachShader");  
        glLinkProgram(programHandle);  
  
        GLint linkStatus = GL_FALSE;  
        glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus);  
        if (linkStatus != GL_TRUE) {  
            GLint bufLength = 0;  
            glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &bufLength);  
            if (bufLength) {  
                char* buf = (char*) malloc(bufLength);  
                if (buf) {  
                    glGetProgramInfoLog(programHandle, bufLength, NULL, buf);  
                    printf("error::Could not link program:\n%s\n", buf);  
                    free(buf);  
                }  
            }  
            glDeleteProgram(programHandle);  
            programHandle = 0;  
        }  
  
    }  
  
    return programHandle;  
}  
  

void gl_initialize(int w, int h) {  
    g_buffer = NULL;  
  
    simpleProgram = build_program(VERTEX_SHADER, FRAG_SHADER);  
    glUseProgram(simpleProgram);  

    glGenTextures(1, &g_texYId);  
    glGenTextures(1, &g_texUId);  
    glGenTextures(1, &g_texVId);  

    glGenFramebuffers(1, &g_frame_buf);
    glBindFramebuffer(GL_FRAMEBUFFER, g_frame_buf);
    checkGlError("initRendering::glBindFramebuffer");

    glGenRenderbuffers(1, &g_render_buf);
    glBindRenderbuffer(GL_RENDERBUFFER, g_render_buf);
    glRenderbufferStorage(GL_RENDERBUFFER, /*GL_DEPTH_COMPONENT16*/GL_RGBA4, w, h);
    checkGlError("initRendering::glBindRenderbuffer");
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_render_buf);


#if 0
    PFNEGLCREATEIMAGEKHRPROC egl_create_image_khr = NULL;
    EGLint eglImgAttrs[] = { 
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
        EGL_NONE, EGL_NONE 
    };
    egl_create_image_khr = (void *) eglGetProcAddress("eglCreateImageKHR");
    if (egl_create_image_khr != NULL) {
        g_ImageKHR = egl_create_image_khr(g_display, EGL_NO_CONTEXT, EGL_GL_RENDERBUFFER_KHR, (EGLClientBuffer)g_render_buf, eglImgAttrs);
        checkGlError("egl_create_image_khr");
        if (g_ImageKHR == NULL) {
            printf("Failed to create ImageKHR\n");
            exit(0);
        }
    }

    egl_image_target_renderbuffer_storage_oes = (void *) eglGetProcAddress("EGLImageTargetRenderbufferStorageOES");
    if (egl_create_image_khr == NULL) {
        printf("Failed to get proc addr of %s\n", "EGLImageTargetRenderbufferStorageOES");
        exit(0);
    }
#endif

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    printf("GL: init done\n");
}  
  
void gl_uninitialize() {  
    g_width = 0;  
    g_height = 0;  
  
    if (g_buffer) {  
        free(g_buffer);  
        g_buffer = NULL;  
    }  
}  

/*
    ffplay -f rawvideo -pix_fmt rgba -video_size 1280x720 00000049.raw
 */
void save_to_file(unsigned char *data, int w, int h, int num) {
    char filename[32]; static int n = 0; 
    sprintf(filename, "%08d.raw", n++);
    if (!(n%50)) { 
        printf("Save size %d, w %d, h %d, num %d\n", w*h*num, w, h, num);
        FILE *video_dst_file = fopen(filename, "wb"); 
        fwrite(data, 1, w * h * num, video_dst_file);
        fclose(video_dst_file); 
    }
          
}


void gl_render_frame(int w, int h) {  
    const char *buffer = g_buffer;  
    int width = g_width;  
    int height = g_height;  
    int x = (w - width)/2;
    int y = (h - height)/2;

    if (0 == g_width || 0 == g_height)  
        return;  

    glViewport(0, 0, width, height);  
    checkGlError("glViewport");
    bind_texture(g_texYId, buffer, width, height);  
    bind_texture(g_texUId, buffer + width * height, width/2, height/2);  
    bind_texture(g_texVId, buffer + width * height * 5 / 4, width/2, height/2);  

    glBindFramebuffer(GL_FRAMEBUFFER, g_frame_buf);
    checkGlError("glBindFramebuffer");
    glBindRenderbuffer(GL_RENDERBUFFER, g_render_buf);
    checkGlError("glBindRenderbuffer");

    render_frame();  
    glFinish(); 

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, g_buffer_out);
    checkGlError("glReadPixels");

    //EGLImageTargetRenderbufferStorageOES(enum target, );

    save_to_file(g_buffer_out, width, height, 4);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    
}  

int conv_init(int w, int h, void *display) {
    int frame_size = w * h * 3 / 2;
    g_display = (EGLDisplay) display;

    gl_initialize(w, h);

    g_width = w;  
    g_height = h;  
    g_buffer = (char *)malloc(frame_size); 
    if (! g_buffer) {
        printf("Failed to init GL\n");
        return 0;
    }

    g_buffer_out = (char *)malloc(w*h*4); 

    if (! g_buffer_out) {
        printf("Failed to init GL\n");
        return 0;
    }
    
    return frame_size;
}

int conv_get_frame(char *fname, int size) {
    FILE *fp;  
    int rd;
    if (!g_buffer) return -1;
    
    if((fp = fopen(fname,"rb")) == NULL) {  
        printf("cant open the file");  
        return -1;  
    }  

    memset(g_buffer, 0 ,size);  
    rd = fread(g_buffer, 1, size, fp);  
    printf("read data size: %d\n", rd);  
    fclose(fp);  

    return 0;
}

int conv_render_frame(int x, int y) {
    gl_render_frame(x, y);
    return 0;
}
