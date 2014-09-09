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

#include <utility>
#include <map>
#include <cstdint>
#include <cassert>
#include <algorithm>

namespace { //Avoid contaminating global namespace

/**
  * An LZW code is 12 bits in our code
  */
typedef uint16_t lzw_code;

inline bool operator<(const std::vector<unsigned char>& lhs, const std::vector<unsigned char>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

/**
  * An LZW dictionary in which "strings" are the keys, and 
  * lzw_codes are the values.
  */
class LZWCompressingDictionary {
public:
    LZWCompressingDictionary() : m_next_code(0) {
        init();
    }

    inline void init() {
        //Initialize dictionary with first 256 values
        for (unsigned int i=0; i<256; ++i) {
            std::vector<unsigned char> key(1, i);
            addStringToDict(key);
        }
    }

    /**
      * Adds w_ to the dictionary using the next unused symbol
      */
    inline void addStringToDict(const std::vector<unsigned char>& w_) {
        //Reset dictionary if over-reaching 12 bits
        if (m_next_code == 4096) {
            m_dict.clear();
            m_next_code = 0;
            init();
        }
        lzw_code value = m_next_code;
        std::pair<std::vector<unsigned char>, lzw_code> entry(w_, value);
        m_dict.insert(entry);
        m_next_code += 1;
    }

    inline bool hasString(const std::vector<unsigned char>& s) {
        return (m_dict.count(s) == 1);
    }

    inline lzw_code getCode(const std::vector<unsigned char>& s) {
        assert(hasString(s));
        return m_dict[s];
    }

private:
    lzw_code m_next_code;
    std::map<std::vector<unsigned char>, lzw_code> m_dict;
};


/**
  * An LZW dictionary in which lzw_codes are the keys, and 
  * strings are the values.
  */
class LZWDecompressingDictionary {
public:
    LZWDecompressingDictionary() : m_next_code(0) {
        init();
    }

    inline void init() {
        //Initialize dictionary with first 256 values
        for (unsigned int i=0; i<256; ++i) {
            std::vector<unsigned char> key(1, i);
            addStringToDict(key);
        }
    }

    inline void reinit() {
        m_dict.clear();
        m_next_code = 0;
        init();
    }
    
    /**
      * Adds w_ to the dictionary using the next unused symbol
      */
    inline void addStringToDict(const std::vector<unsigned char>& w_) {
        //Reset dictionary if over-reaching 12 bits
        if (m_next_code == 4096) {
            reinit();
        }
        lzw_code value = m_next_code;
        std::pair<lzw_code, std::vector<unsigned char>> entry(value, w_);
        m_dict.insert(entry);
        m_next_code += 1;
    }

    inline bool hasCode(const lzw_code& c) {
        //Bugfix? Nut 100% sure about this, but the idea
        if (m_next_code == 4096 && c == 256) {
            reinit();
        }
        return (m_dict.count(c) == 1);
    }

    inline const std::vector<unsigned char>& getString(const lzw_code& c) {
        assert(hasCode(c));
        return m_dict[c];
    }

private:
    lzw_code m_next_code;
    std::map<lzw_code, std::vector<unsigned char>> m_dict;
};

/**
  * An output class which concatenates lzw_codes into the
  * same vector of chars
  */
class LZWOutput {
public:
    LZWOutput() : m_num_elements_written(0) {}
    
    /**
      * Adds the code to the output character buffer
      */
    inline void appendCode(lzw_code c) {
        if (m_num_elements_written % 2 == 0) {
            unsigned char low = c & 0x00FF; //First 8 bits
            unsigned char high = (c >> 8) & 0x000F; //Last 4 bits
            m_data.emplace_back(low);
            m_data.emplace_back(high);
        }
        else {
            unsigned char low = (c & 0x000F) << 4; //First 4 bits
            unsigned char high = (c >> 4) & 0x00FF; //Last 8 bits
            m_data.back() ^= low; //"Merge" with existing nibble
            m_data.emplace_back(high);
        }
        m_num_elements_written += 1;
    }

    inline std::vector<unsigned char> getData() {
        return m_data;
    }

private:
    size_t m_num_elements_written;
    std::vector<unsigned char> m_data;
};

/**
  * LZW input stream which reads lzw_codes from a vector of
  * chars
  */
class LZWInput {
public:
    LZWInput(const std::vector<unsigned char>& data_) : m_num_elements_read(0), m_data(data_), m_even(true) {}

    /**
      * Reads the next code from the character buffer
      */
    inline lzw_code readCode() {
        if (m_even) {
            lzw_code low = m_data[m_num_elements_read++];
            lzw_code high = m_data[m_num_elements_read];

            high = (high & 0x000F) << 8;

            m_even = false;

            return low ^ high;
        }
        else {
            lzw_code low = m_data[m_num_elements_read++];
            lzw_code high = m_data[m_num_elements_read++];

            low = (low >> 4) & 0x000F;
            high = (high & 0x00FF) << 4;

            m_even = true;

            return low ^ high;
        }
    }

    bool hasMoreData() {
        if (m_even) {
            return m_num_elements_read < m_data.size();
        }
        else {
            return m_num_elements_read < m_data.size()-1;
        }
    }

private:
    size_t m_num_elements_read;
    const std::vector<unsigned char>& m_data;
    bool m_even;
};

} // Namespace

/**
  * Function which compresses a character stream using LZW.
  */
std::vector<unsigned char> lzw_compress(const std::vector<unsigned char>& input_) {
    LZWCompressingDictionary dict;
    LZWOutput output;

    std::vector<unsigned char> w;
    for (size_t i=0; i<input_.size(); ++i) {
        //Read character from stream
        std::vector<unsigned char> k(1, input_[i]);
        std::vector<unsigned char> wk = w;
        wk.insert(wk.end(), k.begin(), k.end());

        //If wk is in the dictionary, continue reading
        if (dict.hasString(wk)) {
            w = wk;
            continue;
        }
        //Else, output code, and add wk to dictionary
        else {
            output.appendCode(dict.getCode(w));
            dict.addStringToDict(wk);
            w = k;
        }
    }
    output.appendCode(dict.getCode(w));

    return output.getData();
}

/**
  * Function which decompresses a character stream using LZW.
  */
std::vector<unsigned char> lzw_decompress(const std::vector<unsigned char>& input_) {
    LZWDecompressingDictionary dict;
    LZWInput input(input_);
    std::vector<unsigned char> output;

    lzw_code code = input.readCode();
    std::vector<unsigned char> w = dict.getString(code);
    output.insert(output.end(), w.begin(), w.end());

    while(input.hasMoreData()) {
        lzw_code next_code = input.readCode();

        //If next_code is in the dictionary, write out
        if (dict.hasCode(next_code)) {
            std::vector<unsigned char> k = dict.getString(next_code);
            output.insert(output.end(), k.begin(), k.end());
            std::vector<unsigned char> wk = w;
            wk.emplace_back(k[0]);
            dict.addStringToDict(wk);
            w = k;
        }
        //Otherwise, deduce what the next_code is
        else {
            std::vector<unsigned char> k = dict.getString(code);
            unsigned char k_start = k[0];
            k.emplace_back(k_start);
            output.insert(output.end(), k.begin(), k.end());
            std::vector<unsigned char> wk = w;
            wk.emplace_back(k[0]);
            dict.addStringToDict(wk);
            w = k;
        }
        code = next_code;
    }

    return output;
}
