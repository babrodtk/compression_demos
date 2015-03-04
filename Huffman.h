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

#pragma once

#include <vector>

std::vector<unsigned char> huffman_compress(const std::vector<unsigned char>& data_, bool compute_entropy_=false);
std::vector<unsigned char> huffman_decompress(const std::vector<unsigned char>& data_);
