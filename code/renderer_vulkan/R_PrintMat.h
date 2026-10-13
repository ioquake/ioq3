/*
 * ==================================================================================
 *
 *       Filename:  R_PrintMat.h
 *
 *    Description:  print matrix values for debug
 *         Author:  Sui Jingfeng (), 18949883232@163.com
 * ==================================================================================
 */

#ifndef R_PRINT_MAT_H_
#define R_PRINT_MAT_H_

// data print helper
void printMat1x3f(const char* name, const float src[3]);
void printMat1x4f(const char* name, const float src[4]);
void printMat3x3f(const char* name, const float src[3][3]);
void printMat4x4f(const char* name, const float src[16]);


#endif
