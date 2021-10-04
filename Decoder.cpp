#include "JPEG.h"
#include <iostream>
#include <fstream>

void readAPPN(std::ifstream& inFile, Header* const header) {
    std::cout << "Reading APPN marker\n";
    unsigned int length = (inFile.get() << 8) + inFile.get();

    for (unsigned int i = 0; i < length - 2; ++i)
        inFile.get();

}

void readStartOfFrame(std::ifstream& inFile, Header* const header) {
    std::cout << "Reading SOF marker\n";
    if (header->numComponents != 0) {
        std::cout << "Error - Multiple SOFs detected\n";
        header->valid = false;
        return;
    }

    unsigned int length = (inFile.get() < 8) + inFile.get();

    unsigned char precision = inFile.get();
    if (precision != 8) {
        std::cout << "Error - Invalid precision: " << (unsigned int) precision << std::endl;
        header->valid = false;
        return;
    }

    header->height = (inFile.get() < 8) + inFile.get();
    header->width = (inFile.get() < 8) + inFile.get();
    if (header->width == 0 || header->height == 0) {
        std::cout << "Error - Invalid dimensions\n";
        header->valid = false;
        return;
    }

    header->numComponents = inFile.get();
    if (header->numComponents == 4) {
        std::cout << "Error - CMYK color mode not supported\n";
        header->valid = false;
        return;
    }

    if (header->numComponents == 0) {
        std::cout << "Error - Number of color components must not be zero\n";
        header->valid = false;
        return;
    }

    for (unsigned int i = 0; i < header->numComponents; ++i) {
        unsigned char componentID = inFile.get();
        // component IDs are by the rule book, 1, 2, 3 but rarely can be seen as 0, 1 , 2
        //always force them into 1, 2, 3 for consistency

        if (componentID == 0) {
            header->zeroBased = true;
        }

        if (header->zeroBased) {
            componentID += 1;
        }

        if (componentID == 4 || componentID == 5) {
            std::cout << "Error - YIQ color mode not supported\n";
            header->valid = false;
            return;
        }
        if (componentID == 0 || componentID > 5) {
            std::cout << "Error - Invalid component ID: " << (unsigned int) componentID << std::endl;
            header->valid = false;
            return;
        }
        ColorComponent *component = &header->colorComponents[componentID - 1];
        if (component->used) {
            std::cout << "Error - Duplicate color component ID\n";
            header->valid = false;
            return;
        }
        component->used = true;
        unsigned char samplingFactor = inFile.get();
        component->horizontalSamplingFactor = samplingFactor >> 4;
        component->verticalSamplingFactor = samplingFactor & 0x0F;

        if (component->horizontalSamplingFactor != 1 || component->verticalSamplingFactor != 1) {
            std::cout << "Error - Sampling factors not yet supported\n";
            header->valid = false;
            return;
        }

        component->quantizationTableID = inFile.get();
        if (component->quantizationTableID > 3) {
            std::cout << "Error - Invalid quantization table ID in frame components\n";
            header->valid = false;
            return;
        }
    }
    if (length - 8 - (3 * header->numComponents) != 0) {
        std::cout << "Error - SOF invalid\n";
        header->valid = false;
    }
}

void readQuantizationTable (std::ifstream& inFile, Header* const header) {
    std::cout << "Reading DQT marker\n";
    int length = (inFile.get() << 8) + (inFile.get());
    length -= 2;

    while (length > 0) {
        unsigned char tableInfo = inFile.get();
        length -= 1;
        unsigned char tableID = tableInfo & 0x0F;

        if (tableID > 3) {
            std::cout << "Error - Invalid quantization table ID: " << (unsigned int) tableID << std::endl;
            header->valid = false;
            return;
        }
        header->quantizationTables[tableID].set = true;

        if (tableInfo >> 4 != 0) {
            for (unsigned int i = 0; i < 64; ++i)
                header->quantizationTables[tableID].table[zigZagMap[i]] = (inFile.get() << 8) + inFile.get();
            length -= 128;
        }
        else {
            for (unsigned int i = 0; i < 64; ++i)
                header->quantizationTables[tableID].table[zigZagMap[i]] = inFile.get();
            length -= 64;
        }
    }
    if (length != 0) {
        std::cout << "Error - DQT marker invalid\n";
        header->valid = false;
    }
}

void readRestartInterval(std::ifstream& inFile, Header* const header) {
    std::cout << "Reading DRI marker\n";
    unsigned int length = (inFile.get() << 8) + inFile.get();

    header->restartInternal = (inFile.get() << 8) + inFile.get();
    if (length - 4 != 0) {
        std::cout << "Error - DRI invalid\n";
        header->valid = false;
    }
}

void readHuffmanTable(std::ifstream& inFile, Header* const header) {
    std::cout << "Reading DHT marker\n";
    int length = (inFile.get() << 8) + inFile.get();
    length -= 2;

    while (length > 0) {
        unsigned int tableInfo = inFile.get();
        unsigned int tableID = tableInfo & 0x0F;
        bool ACTable = tableInfo >> 4;

        if (tableID > 3) {
            std::cout << "Error - Invalid Huffman table ID: " << (unsigned int) tableID << '\n';
            header->valid = false;
            return;
        }

        HuffmanTable* hTable;
        if (ACTable) {
            hTable = &header->huffmanACTables[tableID];
        }
        else {
            hTable = &header->huffmanDCTables[tableID];
        }
        hTable->set = true;

        hTable->offsets[0] = 0;
        unsigned int allSymbols = 0;
        for (unsigned int i = 1; i <= 16; ++i) {
            allSymbols += inFile.get();
            hTable->offsets[i] = allSymbols;
        }

        if (allSymbols > 162) {
            std::cout << "Error - Too many symbols in Huffman table\n";
            header->valid = false;
            return;
        }

        for (unsigned int i = 0; i < allSymbols; ++i) {
            hTable->symbols[i] = inFile.get();
        }

        length -= 17 + allSymbols;
    }

    if (length != 0) {
        std::cout << "Error - DHT invalid\n";
        header->valid = false;
    }
}

void readStartOfScan(std::ifstream& inFile, Header* const header) {

    std::cout << "Reading SOS Marker\n";
    if (header->numComponents == 0) {
        std::cout << "Error - SOS detected before SOF\n";
        header->valid = false;
        return;
    }

    unsigned int length = (inFile.get() << 8) + inFile.get();

    for (unsigned int i = 0; i < header->numComponents; ++i) {
        header->colorComponents[i].used = false;
    }

    unsigned char numComponents = inFile.get();
    for (unsigned int i = 0; i < numComponents; ++i) {
        unsigned char componentID = inFile.get();
        // component IDs are usually 1, 2, 3 but can rarely be 0, 1, 2
        if (header->zeroBased) {
            componentID += 1;
        }
        if (componentID > header->numComponents) {
            std::cout << "Error - Invalid color component ID: " << (unsigned int) componentID << '\n';
            header->valid = false;
            return;
        }
        ColorComponent *component = &header->colorComponents[componentID - 1];
        if (component->used) {
            std::cout << "Error - Duplicate color component ID: " << (unsigned int) componentID << '\n';
            header->valid = false;
            return;
        }

        component->used = true;

        unsigned char huffmanTableIDs = inFile.get();
        component->huffmanDCTableID = huffmanTableIDs >> 4;
        component->huffmanACTableID = huffmanTableIDs & 0x0F;

        if (component->huffmanDCTableID > 3) {
            std::cout << "Error - Invalid Huffman DC table ID: " << (unsigned int) component->huffmanDCTableID << '\n';
            header->valid = false;
            return;
        }
        if (component->huffmanACTableID > 3) {
            std::cout << "Error - Invalid Huffman AC table ID: " << (unsigned int) component->huffmanACTableID << '\n';
            header->valid = false;
            return;
        }
    }

    header->startofSelection = inFile.get();
    header->endOfSelection = inFile.get();
    unsigned char successiveApproximation = inFile.get();
    header->successiveApproximationHigh = successiveApproximation > 4;
    header->successiveApproximationLow = successiveApproximation & 0x0F;

    // Baseline JPEGs don't use spectral selection or successive approximation
    if (header->startofSelection != 0 || header->endOfSelection != 63) {
        std::cout << "Error - Invalid spectral selection | May not be baseline JPEG\n";
        header->valid = false;
        return;
    }

    if (header->successiveApproximationHigh != 0 || header->successiveApproximationLow != 0) {
        std::cout << "Error - Invalid successive approximation | May not be baseline JPEG\n";
        header->valid = false;
        return;
    }

    if (length - 6 - (2 * numComponents) != 0) {
        std::cout << "Error - SOS invalid\n";
        header->valid = false;
    }

}

void readComment(std::ifstream& inFile, Header* const header) {

    std::cout << "Reading COM Marker\n";
    unsigned int length = (inFile.get() << 8) + inFile.get();

    for (unsigned int i = 0; i < length - 2; ++i) {
        inFile.get();
    }

}

Header* readJPG(const std::string& filename) {
    std::ifstream inFile = std::ifstream(filename, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) {
        std::cout << "Error - Error opening file " << filename << std::endl;
        return nullptr;
    }

    Header *header = new (std::nothrow) Header;

    if (header == nullptr) {
        inFile.close();
        std::cout << "Error - Memory error\n";
        return nullptr;
    }

    unsigned int last = inFile.get();
    unsigned int current = inFile.get();

    if (last != 0xFF || current != SOI) {
        header->valid = false;
        inFile.close();
        return header;
    }

    last = inFile.get();
    current = inFile.get();

    while (header->valid) {
        if (!inFile) {
            std::cout << "File ended prematurely\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        if (last != 0xFF) {
            std::cout << "Error - Expected a marker\n";
            inFile.close();
            return header;
        }

        if (current == SOS) {
            readStartOfScan(inFile, header);
            break;
        }

        else if (current == DHT) {
            readHuffmanTable(inFile, header);
        }

        else if (current == DRI) {
            readRestartInterval(inFile, header);
        }

        else if (current == SOF0) {
            header->frameType = SOF0;
            readStartOfFrame(inFile, header);
        }


        else if (current == DQT) {
            readQuantizationTable(inFile, header);
        }

        else if (current >= APP0 && current <= APP15) {
            readAPPN(inFile, header);
        }

        else if (current == COM || (current >= JPG0 && current <= JPG13) || current == DNL || current == DHP || current == EXP) {
            readComment(inFile, header);
        }

        else if (current == TEM) {

        }

        else if (current == 0xFF) {     // any number of 0xFF in a row are allowed and should be skipped
            current = inFile.get();
            continue;
        }

        else if (current == SOI) {
            std::cout << "Error - Embedded JPGs not supported\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        else if (current == EOI) {
            std::cout << "Error - EOI detected before SOS\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        else if (current == DAC) {
            std::cout << "Error - Arithmetic coding mode not supported\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        else if (current >= SOF0 && current <= SOF15) {
            std::cout << "Error - SOF marker not supported: 0x\n" << std::hex << (unsigned int) current << std::dec << '\n';
            header->valid = false;
            inFile.close();
            return header;
        }

        else if (current >= RST0 && current <= RST7) {
            std::cout << "Error - RSTN detected before SOS\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        else {
            std::cout << "Error - Unknown marker: 0x" << std::hex << (unsigned int) current << std::dec << '\n';
            header->valid = false;
            inFile.close();
            return header;
        }

        last = inFile.get();
        current = inFile.get();
    }

    // after SOS

    if (header->valid) {

        current = inFile.get();

        //Read compressed image data

        while (true) {
            if (!inFile) {
                std::cout << "Error - File ended premature\n";
                header->valid = false;
                inFile.close();
                return header;
            }

            last = current;
            current = inFile.get();

            // if marker is found
            if (last == 0xFF) {
                // end of image

                if (current == EOI) {
                    break;
                }

                //0xFF00 means put a literal 0xFF in image data and ignore 0x00
                else if (current == 0x00) {
                    header->huffmanData.push_back(last);
                    // overwrite 0x00 with next byte
                    current = inFile.get();
                }
                // restart marker
                else if (current >= RST0 && current <= RST7) {
                    // overwrite marker with next byte
                    current = inFile.get();
                }
                // ignore multiple 0xFF's in a row
                else if (current == 0xFF) {
                    continue;
                }

                else {
                    std::cout << "Error - Invalid marker during compressed data scan: 0x" << std::hex << (unsigned int) current << std::dec << '\n';
                    header->valid = false;
                    inFile.close();
                    return header;
                }
            }

            else {
                header->huffmanData.push_back(last);
            }
        }
    }

    // validate header info

    if (header->numComponents != 1 && header->numComponents != 3) {
        std::cout << "Error - " << (unsigned int) header->numComponents << " color components given (1 or 3 required)\n";
        header->valid = false;
        inFile.close();
        return header;
    }

    for (unsigned int i = 0; i < header->numComponents; ++i) {
        if (!header->quantizationTables[header->colorComponents[i].quantizationTableID].set) {
            std::cout << "Error - Color component using uninitialized quantization table\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        if (!header->huffmanDCTables[header->colorComponents[i].huffmanDCTableID].set) {
            std::cout << "Error - Color component using uninitialized Huffman DC table\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        if (!header->huffmanACTables[header->colorComponents[i].huffmanACTableID].set) {
            std::cout << "Error - Color component using uninitialized Huffman AC table\n";
            header->valid = false;
            inFile.close();
            return header;
        }
    }

    inFile.close();
    return header;
}

void printHeader(const Header* const header) {
    if (header == nullptr)
        return;
    std::cout << "DQT====================\n";
    for (unsigned int i = 0; i < 4; ++i) {
        if (header->quantizationTables[i].set) {
            std::cout << "Table ID: " << i << "\n";
            std::cout << "Table Data:";
            for (unsigned int j = 0; j < 64; ++j) {
                if (!(j % 8))
                    std::cout << '\n';
                std::cout << header->quantizationTables[i].table[j] << ' ';
            }
            std::cout << '\n';
        }
    }

    std::cout << "SOF================\n";
    std::cout << "Frame Type: 0x" << std::hex << (unsigned int) header->frameType << std::dec << '\n';
    std::cout << "Height: " << header->height << '\n';
    std::cout << "Width: " << header->width << '\n';
    std::cout << "Color Components:\n";
    for (unsigned int i = 0; i < header->numComponents; i++) {
        std::cout << "Component ID: " << (i + 1) << '\n';
        std::cout << "Horizontal Sampling Factor: " << (unsigned int) header->colorComponents[i].horizontalSamplingFactor << '\n';
        std::cout << "Vertical Sampling Factor: " << (unsigned int) header->colorComponents[i].verticalSamplingFactor << '\n';
        std::cout << "Quantization Table ID: " << (unsigned int) header->colorComponents[i].quantizationTableID << '\n';
    }

    std::cout << "DHT================\n";
    std::cout << "DC Tables\n";
    for (unsigned int i = 0; i < 4; ++i) {
        if (header->huffmanDCTables[i].set) {
            std::cout << "Table ID: " << i << std::endl;
            std::cout << "Symbols:\n";
            for (unsigned int j = 0; j < 16; ++j) {
                std::cout << (j + 1) << ": ";
                for (unsigned int k = header->huffmanDCTables[i].offsets[j]; k < header->huffmanDCTables[i].offsets[j + 1]; ++k) {
                    std::cout << std::hex << (unsigned int) header->huffmanDCTables[i].symbols[k] << ' ';
                }
                std::cout << '\n';
            }
        }
    }

    std::cout << "AC Tables\n";
    for (unsigned int i = 0; i < 4; ++i) {
        if (header->huffmanACTables[i].set) {
            std::cout << "Table ID: " << i << std::endl;
            std::cout << "Symbols:\n";
            for (unsigned int j = 0; j < 16; ++j) {
                std::cout << (j + 1) << ": ";
                for (unsigned int k = header->huffmanACTables[i].offsets[j]; k < header->huffmanACTables[i].offsets[j + 1]; ++k) {
                    std::cout << std::hex << (unsigned int) header->huffmanACTables[i].symbols[k] << ' ';
                }
                std::cout << '\n';
            }
        }
    }

    std::cout << "SOS==============\n";
    std::cout << "Start of Selection: " << (unsigned int) header->startofSelection << '\n';
    std::cout << "End of Selection: " << (unsigned int) header->endOfSelection << '\n';
    std::cout << "Successive Approximation High: " << (unsigned int) header->successiveApproximationHigh << '\n';
    std::cout << "Successive Approximation Low: " << (unsigned int) header->successiveApproximationLow << '\n';
    std::cout << "Color Components:\n";
    for (unsigned int i = 0; i < header->numComponents; ++i) {
        std::cout << "Component ID: " << (i + 1) << '\n';
        std::cout << "Huffman DC Table ID: " << (unsigned int) header->colorComponents[i].huffmanDCTableID << '\n';
        std::cout << "Huffman AC Table ID: " << (unsigned int) header->colorComponents[i].huffmanACTableID << '\n';
    }
    std::cout << "Length of Huffman Data: " << header->restartInternal << '\n';
    std::cout << "DRI===================\n";
    std::cout << "Restart Interval: " << header->restartInternal << '\n';
}

MCU* blackBox(const Header* const header) {

}

// helper function to write a 4-byte integer in little-endian
void putInt(std::ofstream& outFile, const unsigned int v) {
    outFile.put((v >> 0) & 0xFF);
    outFile.put((v >> 8) & 0xFF);
    outFile.put((v >> 16) & 0xFF);
    outFile.put((v >> 24) & 0xFF);
}

// helper function to write a 2-byte short integer in little-endian
void putShort(std::ofstream& outFile, const unsigned int v) {
    outFile.put((v >> 0) & 0xFF);
    outFile.put((v >> 8) & 0xFF);
}

void writeBMP(const Header* const header, const MCU* const mcus, const std::string& filename) {
    // open file
    std::ofstream outFile = std::ofstream(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "Error - Error opening output file\n";
        return;
    }

    const unsigned int mcuHeight = (header->height + 7) / 8;
    const unsigned int mcuWidth = (header->width + 7) / 8;
    const unsigned int paddingSize = header->width % 4;
    const unsigned int size = 14 + 12 + header->height * header->width * 3 + paddingSize * header->height;

    outFile.put('B');
    outFile.put('M');
    putInt(outFile, size);
    putInt(outFile, 0);
    putInt(outFile, 0x1A);
    putInt(outFile, 12);
    putShort(outFile, header->width);
    putShort(outFile, header->height);
    putShort(outFile, 1);
    putShort(outFile, 24);

    for (unsigned int y = header->height - 1; y < header->height; --y) {
        const uint mcuRow = y / 8;
        const uint pixelRow = y % 8;
        for (unsigned int x = 0; x < header->width; ++x) {
            const uint mcuColumn = x / 8;
            const uint pixelColumn = x % 8;
            const uint mcuIndex = mcuRow * mcuWidth + mcuColumn;
            const uint pixelIndex = pixelRow * 8 + pixelColumn;
            outFile.put(mcus[mcuIndex].b[pixelIndex]);
            outFile.put(mcus[mcuIndex].g[pixelIndex]);
            outFile.put(mcus[mcuIndex].r[pixelIndex]);
        }
        for (unsigned int i = 0; i < paddingSize; ++i) {
            outFile.put(0);
        }
    }

    outFile.close();
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "Invalid arguments\n";
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        const std::string filename(argv[1]);
        Header *header = readJPG(filename);

        if (header == nullptr)
            continue;

        if (!header->valid) {
            std::cout << "Error - Invalid JPEG\n";
            delete header;
            continue;
        }

        printHeader(header);

        // TODO:  decode Huffman data
        MCU* mcus = blackBox(header);
        if (mcus == nullptr) {
            delete header;
            continue;
        }

        // write BMP file
        const std::size_t pos = filename.find_last_of('.');
        const std::string outFilename = (pos == std::string::npos) ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");
        writeBMP(header, mcus, outFilename);

        delete[] mcus;
        delete header;
    }
    return 0;
}
