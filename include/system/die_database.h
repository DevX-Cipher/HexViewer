#pragma once

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#elif __APPLE__
#include <curl/curl.h>
#elif __linux__
#include <curl/curl.h>
#endif

#include "global.h"


#define DIE_DB_BASE_URL "https://github.com/horsicq/Detect-It-Easy/raw/master/db/"
#define DIE_DB_BINARY_URL DIE_DB_BASE_URL "Binary/binary.db"
#define DIE_DB_PE_URL DIE_DB_BASE_URL "PE/pe.db"
#define DIE_DB_ELF_URL DIE_DB_BASE_URL "ELF/elf.db"
#define DIE_DB_MACH_URL DIE_DB_BASE_URL "MACH/mach.db"

struct DIESignature
{
  char name[256];
  char type[64];
  uint8_t pattern[256];
  int patternLength;
  int offset;
};

struct DIEDatabase
{
  DIESignature* signatures;
  int signatureCount;
  int signatureCapacity;
  bool loaded;
};

class DIEDatabaseManager
{
private:
  DIEDatabase binaryDb;
  DIEDatabase peDb;
  DIEDatabase elfDb;
  DIEDatabase machDb;

  char dbDirectory[512];

  bool DownloadFile(const char* url, const char* destPath);
  bool ParseDatabase(const char* dbPath, DIEDatabase& db);
  bool MatchSignature(const uint8_t* data, size_t dataSize, const DIESignature& sig);

public:
  DIEDatabaseManager();
  ~DIEDatabaseManager();

  bool Initialize(const char* dieExecutablePath = nullptr);
  bool DownloadDatabases(void (*progressCallback)(const char*, int) = nullptr);
  bool LoadDatabases();

  bool AnalyzeFile(const uint8_t* data, size_t dataSize,
  char* outType, char* outCompiler, char* outArch);

  const char* GetDatabaseDirectory() const { return dbDirectory; }
  bool AreDatabasesDownloaded() const;
};

extern DIEDatabaseManager g_DIEDatabase;

bool InitializeDIEDatabase(const char* dieExecutablePath = nullptr);
bool DownloadDIEDatabases(void (*progressCallback)(const char*, int) = nullptr);
void AnalyzeFileWithDIE(const uint8_t* data, size_t dataSize,
char* outType, char* outCompiler, char* outArch);
