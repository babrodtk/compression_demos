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

#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

/**
  * Test data set if we don't have a file at hand
  */
extern const unsigned char test_data[];
extern const unsigned int test_data_size;

/**
  * Enum to keep track of which algorithms to perform
  */
enum Compress_t {
    LZW,
    HUFFMAN
};

std::ostream& operator<<(std::ostream& os_, const Compress_t& t_) {
    switch (t_) {
    case LZW: os_ << "LZW"; break;
    case HUFFMAN: os_ << "Huffman"; break;
    default: os_ << "UNKNOWN_COMPRESS_T"; break;
    }
    return os_;
 }

/**
  * Helper function to print out contents of vector as to screen
  */
template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& v_) {
    out << "[";

    if (v_.size() > 32) {
        for (size_t i=0; i<16; ++i) {
            out << v_[i];
        }
        out << " ... ";
        for (size_t i=v_.size()-16; i<v_.size(); ++i) {
            out << v_[i];
        }
    }
    else {
        for (size_t i=0; i<v_.size(); ++i) {
            out << v_[i];
        }
    }
    out << "]";
    return out;
}

/**
  * function which reads a whole file into memory
  */
inline std::vector<unsigned char> readFile(std::string filename_) {
    std::vector<unsigned char> output;
    std::ifstream file(filename_, std::ios::binary);
    if (file.good()) {
        file.seekg(0, std::ios::end);
        size_t bytes = file.tellg();
        file.seekg(0, std::ios::beg);
        output.resize(bytes);
        char* ptr = reinterpret_cast<char*>(&output[0]);
        size_t bytes_read = 0;
        while (bytes_read != bytes ) {
            file.read(ptr + bytes_read, bytes-bytes_read);
            size_t bytes_read_this_iter = file.gcount();
            bytes_read += bytes_read_this_iter;
            if (bytes_read_this_iter == 0) {
                break;
            }
        }
        if (bytes_read != bytes) {
            std::cerr << "Tried reading " << bytes << " bytes from file, and got " << bytes_read << std::endl;
            exit(-1);
        }
        file.close();
    }
    else {
        std::cerr << "Could not open '" << filename_ << "' as a regular file..." << std::endl;
        exit(-1);
    }
    return output;
}

/**
  * Main entry point
  */
int main(int argc, char** argv) {
    std::vector<unsigned char> input;
    std::vector<unsigned char> output;
    std::vector<Compress_t> compress_ops;
    std::string filename;

    //Get options from commandline
    std::cout << "Compression demo of LZW and Huffman" << std::endl;
    std::cout << "Usage: <program> [options] <filename>" << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << " -lzw        Enable LZW compression" << std::endl;
    std::cout << " -huffman    Enable Huffman compression" << std::endl;
    std::cout << "You may enter the same flag multiple times" << std::endl;
    std::cout << std::endl;
    for (int i=1; i<argc; ++i) {
        if (strcmp(argv[i], "-lzw") == 0) {
            compress_ops.push_back(LZW);
        }
        else if (strcmp(argv[i], "-huffman") == 0) {
            compress_ops.push_back(HUFFMAN);
        }
        else {
            filename = argv[i];
        }
    }

    if (compress_ops.size() == 0) {
        std::cerr << "Please enter at least one compression algorithm." << std::endl;
        std::cerr << "Example: <program> -lzw -huffman -lzw -huffman" << std::endl;
        exit(-1);
    }

    //Get data set
    if (filename == "") {
        std::cout << "Using test dataset" << std::endl;
        input.insert(input.begin(), test_data, test_data+test_data_size);
    }
    else {
        std::cout << "Using '" << filename << "' as input data." << std::endl;
        input = readFile(filename);
    }

    //Print out what we are about to do
    if (compress_ops.size() > 0) {
        std::cout << compress_ops[0];
        for (size_t i=1; i<compress_ops.size(); ++i) {
            std::cout << " - " << compress_ops[i];
        }
        std::cout << std::endl;
    }

    //Now perform actual compression
    std::cout << "Compressing:" << std::endl;
    std::cout << "Input: " << input.size() << " bytes" << std::endl;
    std::vector<unsigned char> data = input;
    for (size_t i=0; i<compress_ops.size(); ++i) {
        std::cout << " +" << compress_ops[i] << ":";
        switch(compress_ops[i]) {
        case LZW: output = lzw_compress(data); break;
        case HUFFMAN: output = huffman_compress(data); break;
        default: output = data; break;
        }
        std::cout << output.size() << " bytes" << std::endl;
        data = output;
    }
    std::cout << std::endl;

    //Then decompress
    std::reverse(compress_ops.begin(), compress_ops.end());
    std::cout << "Decompressing:" << std::endl;
    std::cout << "Input: " << data.size() << " bytes" << std::endl;
    for (size_t i=0; i<compress_ops.size(); ++i) {
        std::cout << " -" << compress_ops[i] << ":";
        switch(compress_ops[i]) {
        case LZW: output = lzw_decompress(data); break;
        case HUFFMAN: output = huffman_decompress(data); break;
        default: output = data; break;
        }
        std::cout << output.size() << " bytes" << std::endl;
        data = output;
    }
    std::cout << std::endl;

    //Compare original with decompressed
    if (input.size() != output.size()) {
        std::cerr << "Input:  " << input << std::endl;
        std::cerr << "Output: " << output << std::endl;
        std::cerr << "Input: " << input.size() << " bytes." << std::endl;
        std::cerr << "Output: " << output.size() << " bytes." << std::endl;
        std::cerr << "Input and output sizes do not match!" << std::endl;
        exit(-1);
    }

    for (size_t i=0; i<input.size(); ++i) {
        if (input[i] != output[i]) {
            std::cerr << "Input:  " << input << std::endl;
            std::cerr << "Output: " << output << std::endl;
            std::cerr << "At position " << i << " I got " << std::hex << output[i] << ", but expected " << std::hex << input[i] << std::endl;
            exit(-1);
        }
    }

    std::cout << "Input equal output: Success!" << std::endl;

    return 0;
}