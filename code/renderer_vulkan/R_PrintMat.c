#include <stdio.h>
#include <stdlib.h>
#include "R_PrintMat.h"
#include "ref_import.h"


/*
 * ===============================================================================
 *       Filename:  R_PrintMat.h
 *
 *    Description:  print matrix values or buffer in easier way for debug
 *         Author:  SuiJingfeng, 18949883232@163.com
 * ===============================================================================
 */

void printMat1x3f(const char* name, const float src[3])
{
    ri.Printf(PRINT_ALL, "\n float %s[3] = {%f, %f, %f};\n", 
            name, src[0], src[1], src[2]);
}

void printMat1x4f(const char* name, const float src[4])
{
    ri.Printf(PRINT_ALL, "\n float %s[4] = {%f, %f, %f, %f};\n",
            name, src[0], src[1], src[2], src[3]);
}

void printMat3x3f(const char* name, const float src[3][3])
{
    ri.Printf(PRINT_ALL, 
        "\n float %s[3][3] = {\n%f, %f, %f, \n%f, %f, %f, \n%f, %f, %f };\n", name, 
        src[0][0], src[0][1], src[0][2],
        src[1][0], src[1][1], src[1][2],
        src[2][0], src[2][1], src[2][2]);
}

void printMat4x4f(const char* name, const float src[16])
{
    ri.Printf(PRINT_ALL, "\n float %s[16] = {\n %f, %f, %f, %f,\n %f, %f, %f, %f, \n %f, %f, %f, %f, \n %f, %f, %f, %f};\n", name, 
        src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7], 
        src[8], src[9], src[10], src[11], src[12], src[13], src[14], src[15] );
}
