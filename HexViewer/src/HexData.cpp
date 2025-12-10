#include "HexData.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <iostream>
#endif

HexData::HexData() : currentBytesPerLine(16), modified(false), capstoneInitialized(false),
currentArch(CS_ARCH_X86), currentMode(CS_MODE_64) {
  initializeCapstone();
}

HexData::~HexData() {
  cleanupCapstone();
}

void HexData::debugLog(const char* message) {
#ifdef _WIN32
  int wideSize = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
  if (wideSize > 0) {
    std::wstring wideMsg(wideSize - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMsg[0], wideSize);
    OutputDebugStringW(wideMsg.c_str());
  }
#else
  std::cerr << message;
#endif
}

bool HexData::initializeCapstone() {
  if (capstoneInitialized) {
    cleanupCapstone();
  }

  cs_err err = cs_open(currentArch, currentMode, &csHandle);
  if (err != CS_ERR_OK) {
    debugLog("ERROR: Failed to initialize Capstone\n");
    capstoneInitialized = false;
    return false;
  }

  cs_option(csHandle, CS_OPT_DETAIL, CS_OPT_ON);
  cs_option(csHandle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);

  capstoneInitialized = true;
  debugLog("Capstone initialized successfully\n");
  return true;
}

void HexData::cleanupCapstone() {
  if (capstoneInitialized) {
    cs_close(&csHandle);
    capstoneInitialized = false;
  }
}

void HexData::setArchitecture(cs_arch arch, cs_mode mode) {
  currentArch = arch;
  currentMode = mode;
  initializeCapstone();

  if (!fileData.empty()) {
    regenerateHexLines(currentBytesPerLine);
  }
}

bool HexData::loadFile(const char* filepath) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    debugLog("ERROR: Failed to open file\n");
    hexLines.clear();
    hexLines.push_back("Error: Failed to open file");
    return false;
  }

  debugLog("File opened successfully\n");
  std::streamsize fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  char debugMsg[100];
  snprintf(debugMsg, sizeof(debugMsg), "File size: %lld bytes\n", (long long)fileSize);
  debugLog(debugMsg);

  fileData.clear();
  fileData.resize(fileSize);

  if (!file.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
    debugLog("ERROR: Failed to read file\n");
    hexLines.clear();
    hexLines.push_back("Error: Failed to read file");
    file.close();
    return false;
  }

  debugLog("File read successfully\n");
  file.close();

  convertDataToHex(16);
  modified = false;

  snprintf(debugMsg, sizeof(debugMsg), "Generated %zu hex lines\n", hexLines.size());
  debugLog(debugMsg);

  return true;
}

bool HexData::saveFile(const char* filepath) {
  std::ofstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    debugLog("ERROR: Failed to open file for writing\n");
    return false;
  }

  file.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
  if (!file.good()) {
    debugLog("ERROR: Failed to write file\n");
    file.close();
    return false;
  }

  file.close();
  modified = false;
  debugLog("File saved successfully\n");
  return true;
}

bool HexData::editByte(size_t offset, uint8_t newValue) {
  if (offset >= fileData.size()) {
    return false;
  }

  fileData[offset] = newValue;
  modified = true;

  regenerateHexLines(currentBytesPerLine);
  return true;
}

uint8_t HexData::getByte(size_t offset) const {
  if (offset >= fileData.size()) {
    return 0;
  }
  return fileData[offset];
}

void HexData::clear() {
  fileData.clear();
  hexLines.clear();
  disassemblyLines.clear();
  headerLine.clear();
  modified = false;
}

void HexData::regenerateHexLines(int bytesPerLine) {
  if (!fileData.empty()) {
    convertDataToHex(bytesPerLine);
  }
}

void HexData::generateHeader(int bytesPerLine) {
  char buffer[16];
  headerLine.clear();

  headerLine += "Offset    ";

  for (int i = 0; i < bytesPerLine; ++i) {
    snprintf(buffer, sizeof(buffer), "%02d ", i);
    headerLine += buffer;
  }

  headerLine += " ";

  headerLine += "Decoded text";
}

std::string HexData::disassembleInstruction(size_t offset, int& instructionLength) {
  if (!capstoneInitialized || offset >= fileData.size()) {
    instructionLength = 1;
    return "";
  }

  cs_insn* insn = nullptr;
  size_t remainingBytes = fileData.size() - offset;
  size_t bytesToDisasm = (remainingBytes < 15) ? remainingBytes : 15;

  size_t count = cs_disasm(csHandle,
    &fileData[offset],
    bytesToDisasm,
    offset,
    1,
    &insn);

  if (count > 0) {
    instructionLength = insn[0].size;

    std::string mnemonic = insn[0].mnemonic;
    std::string op_str = insn[0].op_str;
    std::string full = mnemonic + (op_str[0] ? " " + op_str : "");

    cs_free(insn, count);
    return full;
  }

  instructionLength = 1;
  return "";
}

void HexData::generateDisassembly(int bytesPerLine) {
  disassemblyLines.clear();

  if (fileData.empty()) {
    return;
  }

  for (size_t i = 0; i < fileData.size(); i += bytesPerLine) {
    std::string disasmLine;
    size_t lineOffset = i;

    while (lineOffset < i + bytesPerLine && lineOffset < fileData.size()) {
      int instrLen = 0;
      std::string instr = disassembleInstruction(lineOffset, instrLen);

      if (!instr.empty()) {
        if (!disasmLine.empty()) {
          disasmLine += "; ";
        }
        disasmLine += instr;
      }

      lineOffset += instrLen;
      break;
    }

    disassemblyLines.push_back(disasmLine);
  }
}

void HexData::convertDataToHex(int bytesPerLine) {
  hexLines.clear();

  if (fileData.empty()) {
    hexLines.push_back("No data to display");
    headerLine.clear();
    return;
  }

  bytesPerLine = std::max(8, std::min(48, bytesPerLine));
  currentBytesPerLine = bytesPerLine;

  char debugMsg[100];
  snprintf(debugMsg, sizeof(debugMsg), "Converting to hex with %d bytes per line\n", bytesPerLine);
  debugLog(debugMsg);

  generateHeader(bytesPerLine);
  generateDisassembly(bytesPerLine);

  char buffer[512];

  for (size_t i = 0; i < fileData.size(); i += bytesPerLine) {
    std::string line;

    snprintf(buffer, sizeof(buffer), "%08X  ", (unsigned int)i);
    line += buffer;

    for (int j = 0; j < bytesPerLine; ++j) {
      if (i + j < fileData.size()) {
        snprintf(buffer, sizeof(buffer), "%02X ", fileData[i + j]);
        line += buffer;
      }
      else {
        line += "   ";
      }
    }

    line += " ";

    for (int j = 0; j < bytesPerLine && i + j < fileData.size(); ++j) {
      uint8_t b = fileData[i + j];
      if (b >= 32 && b <= 126)
        line += (char)b;
      else
        line += '.';
    }

    line += "      ";

    size_t lineIndex = i / bytesPerLine;
    if (lineIndex < disassemblyLines.size()) {
      line += disassemblyLines[lineIndex];
    }

    hexLines.push_back(line);
  }
}
