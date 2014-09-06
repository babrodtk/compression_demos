/**
  *
  * Compression demos - shows how some classical compression techniques
  * can be implemented in C++. Copyright (C) 2014 André R. Brodtkorb
  * 
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  * 
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  ***/

#include "LZW.h"
#include "Huffman.h"

#include <iostream>
#include <iomanip>
#include <memory>

extern const unsigned char shakespeare_data[];
extern const unsigned int shakespeare_data_size;

std::ostream& operator<<(std::ostream& out, const std::vector<unsigned char>& v_) {
    out << "[";
    for (size_t i=0; i<v_.size(); ++i) {
        out << v_[i];
    }
    out << "]";
    return out;
}

int main(int argc, char** argv) {
    //Create something to compress
    std::vector<unsigned char> input(shakespeare_data, shakespeare_data+shakespeare_data_size);

    //Compress it
    std::vector<unsigned char> compressed = huffman_compress(input);

    //Decompress it
    std::vector<unsigned char> decompressed = huffman_decompress(compressed);

    //Compare original with decompressed
    if (input.size() != decompressed.size()) {
        std::cerr << "Input and output sizes do not match!" << std::endl;
        std::cerr << "Input:  " << input << std::endl;
        std::cerr << "Output: " << decompressed << std::endl;
        std::cerr << "Input and output sizes do not match!" << std::endl;
        exit(-1);
    }

    for (size_t i=0; i<input.size(); ++i) {
        if (input[i] != decompressed[i]) {
            std::cerr << "At position " << i << " I got " << std::hex << decompressed[i] << ", but expected " << std::hex << input[i] << std::endl;
            std::cerr << "Input:  " << input << std::endl;
            std::cerr << "Output: " << decompressed << std::endl;
            std::cerr << "At position " << i << " I got " << std::hex << decompressed[i] << ", but expected " << std::hex << input[i] << std::endl;
            exit(-1);
        }
    }

    std::cout << "Original size:   " << input.size() << " bytes." << std::endl;
    std::cout << "Compressed size: " << compressed.size() << " bytes (" << 100*compressed.size() / input.size() << "%)." << std::endl;
    std::cout << "Input equal output: Success!" << std::endl;

    return 0;
}