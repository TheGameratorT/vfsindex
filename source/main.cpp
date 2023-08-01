#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <vector>
#include <cstring>
#include <stdint.h>

#include "args.h"

/*
Structure of an "~INDEX" file:

struct FileEntry {
    u64 fileSize;
    u64 filenameOffset;
};

struct INDEX {
    u64 fileCount;
    FileEntry files[fileCount];
    char filenames[];
};

open("~INDEX").read() = INDEX
*/

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

class DestructorGuard {
public:
    inline DestructorGuard(std::function<void()> destroyer) : m_destroyer(destroyer) {}

    inline ~DestructorGuard() {
		m_destroyer();
    }

private:
	std::function<void()> m_destroyer;
};

namespace DataUtil {
	template<typename T>
	T read(void* ptr, int addr) {
		T val;
		std::memcpy(&val, static_cast<char*>(ptr) + addr, sizeof(T));
		return val;
	}

	template<typename T>
	void write(void* ptr, int addr, T val) {
		std::memcpy(static_cast<char*>(ptr) + addr, &val, sizeof(T));
	}
}

static std::string toLower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
	return str;
}

static bool readBool() {
	bool fail;
	std::string svalue;
	bool value;

	do {
		fail = false;
		std::getline(std::cin, svalue);
		svalue = toLower(svalue);
		if (svalue == "y") {
			value = true;
		} else if (svalue == "n") {
			value = false;
		} else {
			std::cout << "Invalid option.\n";
			std::cout << "Try again: " << std::flush;
			fail = true;
		}
	} while (fail);

	return value;
}

static bool saveOutputFile(const std::filesystem::path& outputPath, const void* data, std::size_t dataSize) {
	std::ofstream outputFile = std::ofstream(outputPath, std::ios::binary);
	if (!outputFile.is_open()) {
		std::cerr << "Failed to open output file: " << outputPath.string() << std::endl;
		return false;
	}

	outputFile.write(reinterpret_cast<const char*>(data), dataSize);
	outputFile.close();
	return true;
}

static int printData(gengetopt_args_info& args_info) {
	std::filesystem::path inputFilePath = args_info.input_arg;

	std::ifstream inputFile(inputFilePath, std::ios::binary);
	if (!inputFile.is_open()) {
		std::cerr << "Failed to open input file: " << inputFilePath.string() << std::endl;
		return 1;
	}

	std::size_t inputFileSize = std::filesystem::file_size(inputFilePath);

	char* inputData = new char[inputFileSize];
	DestructorGuard inputData_guard([&](){ delete[] inputData; });

	inputFile.read(inputData, inputFileSize);

	inputFile.close();

	u64 fileCount = DataUtil::read<u64>(inputData, 0);

	if (fileCount == 0) {
		std::cout << "The index file is empty." << std::endl;
		return 0;
	}

	std::cout << "#:{filesize,filename}" << std::endl;

	const char* stroff = &inputData[8 + (fileCount * 16)];

	for (int i = 0; i < fileCount; i++) {
		int offset = 8 + (i * 16);
		std::cout << i << ':' << '{'
			<< DataUtil::read<u64>(inputData, offset) << ','
			<< '"' << (stroff + DataUtil::read<u64>(inputData, offset + 8)) << "\"}"
			<< std::endl;
	}

	return 0;
}

static int buildIndex(gengetopt_args_info& args_info) {
	std::filesystem::path outputPath = args_info.output_given ? args_info.output_arg : "~INDEX";

	if (!args_info.overwrite_given && std::filesystem::exists(outputPath)) {
		std::cout << "Output file already exists, overwrite? (Y/n): " << std::flush;
		if (!readBool()) {
			std::cout << "Index file creation cancelled by user." << std::endl;
			return 0;
		}
	}

	std::vector<char> header;
	std::vector<char> filenames;

	header.resize(8);

	u64 fileCount = 0;

	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(args_info.input_arg)) {
		if (dirEntry.is_directory()) {
			continue;
		}

		std::string filename = std::filesystem::path(dirEntry).lexically_relative(args_info.input_arg).string();
		std::replace(filename.begin(), filename.end(), '\\', '/');
		
		int filenameOff = filenames.size();
		filenames.resize(filenameOff + filename.size() + 2);
		filenames[filenameOff] = '/';
		std::memcpy(&filenames[filenameOff+1], filename.c_str(), filename.size()+1);

		u64 filesize = std::filesystem::file_size(dirEntry);
		
		int headerOff = header.size();
		header.resize(headerOff + 16);
		DataUtil::write<u64>(&header[headerOff], 0, filesize);
		DataUtil::write<u64>(&header[headerOff], 8, filenameOff);

		fileCount++;
	}

	DataUtil::write<u64>(&header[0], 0, fileCount);

	std::vector<char> data;
	data.resize(header.size() + filenames.size());
	std::memcpy(&data[0], &header[0], header.size());
	if (filenames.size() > 0) {
		std::memcpy(&data[header.size()], &filenames[0], filenames.size());
	}

	if (!saveOutputFile(outputPath, &data[0], data.size())) {
		return 1;
	}

	std::cout << "Index file created successfully with " << fileCount << " file entr" << (fileCount == 1 ? "y" : "ies") << "." << std::endl;
	return 0;
}

int main(int argc, char* argv[]) {
	gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) {
		return 1;
	}
	DestructorGuard args_info_guard([&](){ cmdline_parser_free(&args_info); });

	// if it is a directory, we want to create an index file

	if (std::filesystem::is_directory(args_info.input_arg)) {
		return buildIndex(args_info);
	}

	// otherwise it's a file, so let's just try to dump the index file

	if (args_info.output_given) {
		std::cout << "Output argument ignored, this command requires no output." << std::endl;
	}

	return printData(args_info);
}
