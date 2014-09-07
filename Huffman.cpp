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

#include "Huffman.h"
#include <vector>
#include <queue>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <memory>

/**
  * Class which represents a Huffman (variable length) symbol
  */
class HuffmanSymbol {
public:
    HuffmanSymbol() : m_symbol(0), m_symbol_width(0) {}

    HuffmanSymbol(uint64_t symbol_, unsigned int symbol_width_) 
        : m_symbol(symbol_), m_symbol_width(symbol_width_) {
    }
    
    HuffmanSymbol(HuffmanSymbol&& rhs) {
        m_symbol = std::move(rhs.m_symbol);
        m_symbol_width = std::move(rhs.m_symbol_width);
    }

    HuffmanSymbol rightChild() {
        HuffmanSymbol child;
        child.m_symbol = (m_symbol << 1) + 1;
        child.m_symbol_width = m_symbol_width + 1;
        return child;
    }

    HuffmanSymbol leftChild() {
        HuffmanSymbol child;
        child.m_symbol = m_symbol << 1;
        child.m_symbol_width = m_symbol_width + 1;
        return child;
    }

    uint64_t m_symbol;
    unsigned int m_symbol_width;
};

/**
  * Class which represents a node in the huffman tree
  */
class HuffmanNode {
public:
    HuffmanNode() : m_count(0), m_right(nullptr), m_left(nullptr), m_symbol() {}

    HuffmanNode(unsigned int count_, HuffmanNode* right_, HuffmanNode* left_) 
        : m_count(count_), m_right(right_), m_left(left_), m_symbol() {
    }

    HuffmanNode(HuffmanNode&& rhs) {
        m_count = std::move(rhs.m_count);
        m_right = std::move(rhs.m_right);
        m_left = std::move(rhs.m_left);
        m_symbol = std::move(rhs.m_symbol);
    }
    
    virtual inline bool operator<(const HuffmanNode& rhs) const {
        if (m_count < rhs.m_count) {
            return true;
        }
        else if ((m_count == rhs.m_count) && (this < &rhs)) {
            return true;
        }
        return false;
    }
    
    virtual inline bool operator>(const HuffmanNode& rhs) const {
        return !operator<(rhs);
    }

    unsigned int m_count;
    HuffmanNode* m_right;
    HuffmanNode* m_left;
    HuffmanSymbol m_symbol;
};

/**
  * Class used to compare pointers to Huffman nodes
  */
class HuffmanNodeComparator {
public:
	bool operator()(const HuffmanNode* lhs, const HuffmanNode* rhs) const {
        return lhs->operator>(*rhs);
	}
};

/**
  * Class used to represent a leaf node in the huffman tree
  */
class HuffmanLeafNode : public HuffmanNode {
public:
    HuffmanLeafNode(unsigned char char_) 
         : HuffmanNode(0, nullptr, nullptr), m_char(char_) {
    }

    HuffmanLeafNode(unsigned char char_, unsigned int count_) 
        : HuffmanNode(count_, nullptr, nullptr), m_char(char_) {
    }

    HuffmanLeafNode(HuffmanLeafNode&& rhs) : HuffmanNode(rhs) {
        m_char = std::move(rhs.m_char);
    }

    unsigned char m_char;
};


/**
  * Function which creates a list of occurances for each character in a vector
  */
inline std::vector<unsigned int> findCharacterFrequency(const std::vector<unsigned char>& data_) {
    std::vector<unsigned int> frequencies(256, 0);

    for (size_t i=0; i<data_.size(); ++i) {
        unsigned int character = data_[i];
        frequencies[character] += 1;
    }

    return frequencies;
}

inline void traverseTree(HuffmanNode* root) {
    HuffmanLeafNode* leaf = dynamic_cast<HuffmanLeafNode*>(root);
    
    if (root->m_right) {
        root->m_right->m_symbol = root->m_symbol.rightChild();
        traverseTree(root->m_right);
    }
    
    if (root->m_left) {
        root->m_left->m_symbol = root->m_symbol.leftChild();
        traverseTree(root->m_left);
    }
}

void writeHuffmanSymbol(std::vector<unsigned char>& output_, unsigned int& bit_index_, const HuffmanSymbol& symbol_) {
    const unsigned char bit_masks[] = {
        1,
        3,
        7,
        15,
        31,
        63,
        127,
        255,
    };

    unsigned int bits_left = symbol_.m_symbol_width;
    uint64_t bits = symbol_.m_symbol;

    while (bits_left) {
        int bits_written = 0;

        //Do we have to allocate more memory?
        if (bit_index_ == 0) {
            output_.push_back(0);
        }

        //Will we fill the current byte?
        if (bit_index_+bits_left >= 8) {
            //Read from bits
            unsigned char out = static_cast<unsigned char>(bits);

            //Bit shift to align with existing bits and or
            output_.back() ^= (out << bit_index_);

            //Move bit pointers
            bits_written = 8-bit_index_;
        }
        else {
            //Read from bits
            unsigned char out = static_cast<unsigned char>(bits);

            //Discard bits we don't own
            out &= bit_masks[bits_left];

            //Write out
            output_.back() ^= (out << bit_index_);
            
            bits_written = bits_left;
        }
        
        //Move bit pointers
        bits_left -= bits_written;
        bit_index_ = (bit_index_ + bits_written) % 8;

        //Discard bits written from bits
        bits = bits >> bits_written;
    }
}

std::vector<unsigned char> huffman_compress(const std::vector<unsigned char>& data_) {
    //First, find the actual frequency of each character in the stream
    std::vector<unsigned int> frequencies = findCharacterFrequency(data_);

    //Now loop through the map and create a priority queue containing all characters
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, HuffmanNodeComparator > queue;
    std::vector<std::shared_ptr<HuffmanLeafNode> > leaf_nodes(frequencies.size());
    unsigned char num_characters = 0;
    for (size_t i=0; i<frequencies.size(); ++i) {
        unsigned char c = i;
        if (frequencies[i] > 0) {
            std::shared_ptr<HuffmanLeafNode> leaf(new HuffmanLeafNode(c, frequencies[i]));
            leaf_nodes[i] = leaf;
            queue.push(leaf.get());
            ++num_characters;
        }
    }
    
    //Create the tree of nodes
    std::vector<std::shared_ptr<HuffmanNode> > non_leaf_nodes;
    while (queue.size() > 1) {
        HuffmanNode* right = queue.top();
        queue.pop();
        HuffmanNode* left = queue.top();
        queue.pop();

        unsigned int sum = right->m_count + left->m_count;
        std::shared_ptr<HuffmanNode> non_leaf(new HuffmanNode(sum, right, left));
        non_leaf_nodes.push_back(non_leaf);
        queue.push(non_leaf.get());
    }

    //Traverse the tree, and add the code words for each node
    traverseTree(queue.top());
    queue.pop();

    //Write the symbol table to the character buffer
    std::vector<unsigned char> output;
    output.push_back(num_characters);
    for (size_t i=0; i<leaf_nodes.size(); ++i) {
        std::shared_ptr<HuffmanLeafNode> node = leaf_nodes[i];
        if (node) {
            unsigned char character = node->m_char;
            unsigned char symbol_width = node->m_symbol.m_symbol_width;
            unsigned char* symbol = reinterpret_cast<unsigned char*>(&(node->m_symbol.m_symbol));

            //Write out symbol
            output.push_back(character);

            //Write out symbol length
            output.push_back(symbol_width);

            //Write out symbol itself 
            for (size_t j=0; j*8<symbol_width; ++j) {
                output.push_back(symbol[j]);
            }

            std::cout << character << "=" << node->m_symbol.m_symbol << "(" << node->m_symbol.m_symbol_width << ")" << std::endl;
        }
    }

    
    //Now traverse text, and replace chars with symbols and write to output
    unsigned int bit_index = 0;
    for (size_t i=0; i<data_.size(); ++i) {
        unsigned int index = data_[i];
        writeHuffmanSymbol(output, bit_index, leaf_nodes[index]->m_symbol);
    }

    return output;
}

std::vector<unsigned char> huffman_decompress(const std::vector<unsigned char>& data_) {
    size_t offset = 0;
    std::shared_ptr<HuffmanNode> root(new HuffmanNode());
    std::vector<std::shared_ptr<HuffmanNode> > non_leaf_nodes;
    non_leaf_nodes.push_back(root);

    std::cout << "\n\n";
    
    //Read the symbol table
    size_t num_characters = data_[offset++];
    std::vector<std::shared_ptr<HuffmanLeafNode> > leaf_nodes(256);
    for (size_t i=0; i<num_characters; ++i) {
        unsigned char character = data_[offset++];
        unsigned char symbol_width = data_[offset++];
            assert(symbol_width < 8*8);
        uint64_t symbol = 0;
        unsigned char* symbol_ptr = reinterpret_cast<unsigned char*>(&symbol);
        for (size_t j=0; j*8<symbol_width; ++j) {
            symbol_ptr[j] = data_[offset++];
        }
        
        //std::cout << character << "=" << symbol << std::endl;
        std::cout << character << "=" << symbol << "=>";

        //Now loop through the symbol, and create all non-leaf nodes
        HuffmanNode* node = root.get();
        for (unsigned char j=1; j<symbol_width; ++j) {
            //1 signifies right child
            if ((symbol >> (symbol_width-j)) & 1) {
                std::cout << "1";
                if (!node->m_right) {
                    std::shared_ptr<HuffmanNode> child(new HuffmanNode());
                    non_leaf_nodes.push_back(child);
                    node->m_right = child.get();
                }
                node = node->m_right;
            }
            //0 signifies left
            else {
                std::cout << "0";
                if (!node->m_left) {
                    std::shared_ptr<HuffmanNode> child(new HuffmanNode());
                    non_leaf_nodes.push_back(child);
                    node->m_left = child.get();
                }
                node = node->m_left;
            }
        }

        //Finally, add the leaf node
        if (symbol & 1) {
            std::cout << "1:";
            std::shared_ptr<HuffmanLeafNode> child(new HuffmanLeafNode(character));
            child->m_symbol.m_symbol = symbol;
            leaf_nodes.push_back(child);
            node->m_right = child.get();
        }
        else {
            std::cout << "0:";
            std::shared_ptr<HuffmanLeafNode> child(new HuffmanLeafNode(character));
            child->m_symbol.m_symbol = symbol;
            leaf_nodes.push_back(child);
            node->m_left = child.get();
        }
        std::cout << std::endl;
    }

    //Now that we have the tree, lets traverse it as we decompress our data
    std::vector<unsigned char> output;
    unsigned int bit_index = 0;
    unsigned char byte = 0;
    while (offset < data_.size()) {
        HuffmanNode* node = root.get();

        //Decode one symbol
        for (;;) {
            if (bit_index == 0) {
                byte = data_[offset++];
            }

            //Right child
            if ((byte >> (7-bit_index)) & 1) {
                node = node->m_right;
            }
            //Left child
            else {
                node = node->m_left;
            }

            bit_index = (bit_index+1) % 8;

            HuffmanLeafNode* leaf = dynamic_cast<HuffmanLeafNode*>(node);
            if (leaf) {
                std::cout << leaf->m_char;
                output.push_back(leaf->m_char);
                break;
            }
        }
    }

    return data_;
}
