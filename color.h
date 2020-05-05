#ifndef COLOR_H
#define COLOR_H
    
int conv_init(int w, int h, void *display);
int conv_get_frame(char *fname, int size);
int conv_render_frame(int x, int y);

#endif
