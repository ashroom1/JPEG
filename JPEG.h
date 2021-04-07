//
// Created by Ashwin Murali on 3/29/21.
//

#ifndef JPEGINCPLUSPLUS_JPEG_H

//APPn markers
unsigned char APP0 = 0xE0;
unsigned char APP1 = 0xE1;
unsigned char APP2 = 0xE2;
unsigned char APP3 = 0xE3;
unsigned char APP4 = 0xE4;
unsigned char APP5 = 0xE5;
unsigned char APP6 = 0xE6;
unsigned char APP7 = 0xE7;
unsigned char APP8 = 0xE8;
unsigned char APP9 = 0xE9;
unsigned char APP10 = 0xEA;
unsigned char APP11 = 0xEB;
unsigned char APP12 = 0xEC;
unsigned char APP13 = 0xED;
unsigned char APP14 = 0xEE;
unsigned char APP15 = 0xEF;

const unsigned char SOI = 0xD8;
const unsigned char DQT = 0xDB;
const unsigned char DRI = 0xDD;
const unsigned char SOF0 = 0xC0;
const unsigned char DHT = 0xC4;


struct HuffmanTable {

    unsigned char offsets[17] = {0};
    unsigned char symbols[162] = {0};
    bool set = false;

};


struct ColorComponent {

    unsigned char horizontalSamplingFactor = 1;
    unsigned char verticalSamplingFactor = 1;
    unsigned char quantizationTableID = 0;
    bool used = false;

};

struct QuantizationTable {

    unsigned int table[64] = {0};
    bool set = false;

};

struct Header {

    QuantizationTable quantizationTables[4];
    HuffmanTable huffmanDCTables[4];
    HuffmanTable huffmanACTables[4];

    unsigned char frameType = 0;
    unsigned int height = 0;
    unsigned int width = 0;
    unsigned char numComponents = 0;
    bool zeroBased = false;

    unsigned int restartInternal = 0;

    ColorComponent colorComponents[3];

    bool valid = true;

};

const unsigned int zigZagMap[] = {
        0, 1, 8, 16, 9, 2, 3, 10,
        17, 24, 32, 25, 18, 11, 4, 5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13, 6, 7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
};



#define JPEGINCPLUSPLUS_JPEG_H

#endif //JPEGINCPLUSPLUS_JPEG_H
