#ifndef R_IMAGEJPG_H_
#define R_IMAGEJPG_H_

void RE_SaveJPG(char * filename, int quality, int image_width, int image_height, unsigned char *image_buffer, int padding);
size_t RE_SaveJPGToBuffer(unsigned char *buffer, size_t bufSize, int quality, int image_width, int image_height, unsigned char *image_buffer, int padding);

#endif
