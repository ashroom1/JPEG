//
// Created by Ashwin Murali on 3/29/21.
//

#include <vector>

#ifndef JPEGINCPLUSPLUS_JPEG_H

// Start of Frame markers, non-differential, Huffman coding
const unsigned char SOF0 = 0xC0; // Baseline DCT
const unsigned char SOF1 = 0xC1; // Extended sequential DCT
const unsigned char SOF2 = 0xC2; // Progressive DCT
const unsigned char SOF3 = 0xC3; // Lossless (sequential)

// Start of Frame markers, differential, Huffman coding
const unsigned char SOF5 = 0xC5; // Differential sequential DCT
const unsigned char SOF6 = 0xC6; // Differential progressive DCT
const unsigned char SOF7 = 0xC7; // Differential lossless (sequential)

// Start of Frame markers, non-differential, arithmetic coding
const unsigned char SOF9 = 0xC9; // Extended sequential DCT
const unsigned char SOF10 = 0xCA; // Progressive DCT
const unsigned char SOF21 = 0xCB; // Lossless (sequential)

// Start of Frame markers, differential, arithmetic coding
const unsigned char SOF13 = 0xC0; // Differential sequential DCT
const unsigned char SOF14 = 0xC1; // Differential progressive DCT
const unsigned char SOF15 = 0xC2; // Differential lossless (sequential)

//APPn markers
const unsigned char APP0 = 0xE0;
const unsigned char APP1 = 0xE1;
const unsigned char APP2 = 0xE2;
const unsigned char APP3 = 0xE3;
const unsigned char APP4 = 0xE4;
const unsigned char APP5 = 0xE5;
const unsigned char APP6 = 0xE6;
const unsigned char APP7 = 0xE7;
const unsigned char APP8 = 0xE8;
const unsigned char APP9 = 0xE9;
const unsigned char APP10 = 0xEA;
const unsigned char APP11 = 0xEB;
const unsigned char APP12 = 0xEC;
const unsigned char APP13 = 0xED;
const unsigned char APP14 = 0xEE;
const unsigned char APP15 = 0xEF;

// Restart interval Markers
const unsigned char RST0 = 0xD0;
const unsigned char RST1 = 0xD1;
const unsigned char RST2 = 0xD2;
const unsigned char RST3 = 0xD3;
const unsigned char RST4 = 0xD4;
const unsigned char RST5 = 0xD5;
const unsigned char RST6 = 0xD6;
const unsigned char RST7 = 0xD7;

// Other Markers
const unsigned char SOI = 0xD8; // Start of Image
const unsigned char EOI = 0xD9; // End of Image
const unsigned char DNL = 0xDC; // Define Number of Lines
const unsigned char DQT = 0xDB; // Define Quantization Table(s)
const unsigned char DRI = 0xDD; // Define Restart Interval
const unsigned char JPG = 0xC8; // JPEG Extensions
const unsigned char DAC = 0xCC; // Define Arithmetic Conditioning(s)
const unsigned char DHT = 0xC4; // Define Huffman Table(s)
const unsigned char SOS = 0xDA; // Start of Scan
const unsigned char COM = 0xFE; // Comment
const unsigned char DHP = 0xDE; // Define hierarchical Progression
const unsigned char EXP = 0xDF; // Expand Reference Component(s)

// Misc Markers

const unsigned char JPG0 = 0xF0;
const unsigned char JPG1 = 0xF1;
const unsigned char JPG2 = 0xF2;
const unsigned char JPG3 = 0xF3;
const unsigned char JPG4 = 0xF4;
const unsigned char JPG5 = 0xF5;
const unsigned char JPG6 = 0xF6;
const unsigned char JPG7 = 0xF7;
const unsigned char JPG8 = 0xF8;
const unsigned char JPG9 = 0xF9;
const unsigned char JPG10 = 0xFA;
const unsigned char JPG11 = 0xFB;
const unsigned char JPG12 = 0xFC;
const unsigned char JPG13 = 0xFD;
const unsigned char TEM = 0x01;


struct HuffmanTable {

    unsigned char offsets[17] = {0};
    unsigned char symbols[162] = {0};
    bool set = false;

};

struct MCU {
    union {
        int y[64] = {0};
        int r[64];
    };

    union {
        int cb[64] = {0};
        int g[64];
    };
    union {
        int cr[64] = {0};
        int b[64];
    };
};

struct ColorComponent {

    unsigned char horizontalSamplingFactor = 1;
    unsigned char verticalSamplingFactor = 1;
    unsigned char quantizationTableID = 0;
    unsigned char huffmanDCTableID = 0;
    unsigned char huffmanACTableID = 0;
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

    unsigned char startofSelection = 0;
    unsigned endOfSelection = 63;
    unsigned char successiveApproximationHigh = 0;
    unsigned char successiveApproximationLow = 0;

    unsigned int restartInternal = 0;

    ColorComponent colorComponents[3];

    std::vector<unsigned char> huffmanData;

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
} ;


#define JPEGINCPLUSPLUS_JPEG_H

#endif //JPEGINCPLUSPLUS_JPEG_H