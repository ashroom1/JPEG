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

void readHuffmanTable(std::ifstream& inFile, const Header* const header) {

    std::cout << "Reading DHT marker\n";
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

        if (current == DHT) {
            readHuffmanTable(inFile, header);
        }

        else if (current == DRI) {
            readRestartInterval(inFile, header);
        }

        else if (current == SOF0) {
            header->frameType = SOF0;
            readStartOfFrame(inFile, header);
            break;
        }


        else if (current == DQT) {
            readQuantizationTable(inFile, header);
        }

        else if (current >= APP0 && current <= APP15) {
            readAPPN(inFile, header);
        }

        last = inFile.get();
        current = inFile.get();

    }
    return header;
}

void printHeader(const Header* const header) {
    if (header == nullptr)
        return;
    std::cout << "DQT====================\n";
    for (uint i = 0; i < 4; ++i) {
        if (header->quantizationTables[i].set) {
            std::cout << "Table ID: " << i << "\n";
            std::cout << "Table Data:";
            for (uint j = 0; j < 64; ++j) {
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

    std::cout << "Restart Interval: " << header->restartInternal << '\n';
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

        delete header;
    }
    return 0;
}
