#include "die_database.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

DIEDatabaseManager g_DIEDatabase;


#ifdef _WIN32
bool DIEDatabaseManager::DownloadFile(const char* url, const char* destPath)
{
  HINTERNET hInternet = InternetOpenA("HexViewer/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (!hInternet)
    return false;

  HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
  if (!hUrl)
  {
    InternetCloseHandle(hInternet);
    return false;
  }

  char dirPath[512];
  strCopy(dirPath, destPath);
  for (int i = (int)strLen(dirPath) - 1; i >= 0; i--)
  {
    if (dirPath[i] == '\\' || dirPath[i] == '/')
    {
      dirPath[i] = 0;
      CreateDirectoryA(dirPath, NULL);
      break;
    }
  }

  HANDLE hFile = CreateFileA(destPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return false;
  }

  char buffer[4096];
  DWORD bytesRead, bytesWritten;

  while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
  {
    WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
  }

  CloseHandle(hFile);
  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInternet);

  return true;
}
#else
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  FILE* fp = (FILE*)userp;
  return fwrite(contents, size, nmemb, fp);
}

bool DIEDatabaseManager::DownloadFile(const char* url, const char* destPath)
{
  CURL* curl = curl_easy_init();
  if (!curl)
    return false;

  char dirPath[512];
  strCopy(dirPath, destPath);
  for (int i = (int)strLen(dirPath) - 1; i >= 0; i--)
  {
    if (dirPath[i] == '/')
    {
      dirPath[i] = 0;
      mkdir(dirPath, 0755);
      break;
    }
  }

  FILE* fp = fopen(destPath, "wb");
  if (!fp)
  {
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

  CURLcode res = curl_easy_perform(curl);

  fclose(fp);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK);
}
#endif

DIEDatabaseManager::DIEDatabaseManager()
{
  binaryDb.signatures = nullptr;
  binaryDb.signatureCount = 0;
  binaryDb.signatureCapacity = 0;
  binaryDb.loaded = false;

  peDb.signatures = nullptr;
  peDb.signatureCount = 0;
  peDb.signatureCapacity = 0;
  peDb.loaded = false;

  elfDb.signatures = nullptr;
  elfDb.signatureCount = 0;
  elfDb.signatureCapacity = 0;
  elfDb.loaded = false;

  machDb.signatures = nullptr;
  machDb.signatureCount = 0;
  machDb.signatureCapacity = 0;
  machDb.loaded = false;

  dbDirectory[0] = '\0';
}

DIEDatabaseManager::~DIEDatabaseManager()
{
  if (binaryDb.signatures)
    platformFree(binaryDb.signatures);
  if (peDb.signatures)
    platformFree(peDb.signatures);
  if (elfDb.signatures)
    platformFree(elfDb.signatures);
  if (machDb.signatures)
    platformFree(machDb.signatures);
}

bool DIEDatabaseManager::Initialize(const char* dieExecutablePath)
{
  if (dieExecutablePath && dieExecutablePath[0] != '\0')
  {
    strCopy(dbDirectory, dieExecutablePath);

    int len = (int)strLen(dbDirectory);
    for (int i = len - 1; i >= 0; i--)
    {
      if (dbDirectory[i] == '\\' || dbDirectory[i] == '/')
      {
        dbDirectory[i] = '\0';
        break;
      }
    }

#ifdef _WIN32
    strCat(dbDirectory, "\\db");
#else
    strCat(dbDirectory, "/db");
#endif

    return true;
  }

#ifdef _WIN32
  wchar_t localAppData[MAX_PATH];
  DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
  if (len > 0 && len < MAX_PATH)
  {
    WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, dbDirectory, 500, nullptr, nullptr);
    int slen = (int)strLen(dbDirectory);
    strCopy(dbDirectory + slen, "\\HexViewer\\DIE_DB");
  }
  else
  {
    strCopy(dbDirectory, ".\\DIE_DB");
  }
#else
  const char* home = getenv("HOME");
  if (home)
  {
    strCopy(dbDirectory, home);
    int len = (int)strLen(dbDirectory);
    strCopy(dbDirectory + len, "/.config/HexViewer/DIE_DB");
  }
  else
  {
    strCopy(dbDirectory, "./DIE_DB");
  }
#endif

  return true;
}

bool DIEDatabaseManager::AreDatabasesDownloaded() const
{
  char binaryPath[512];
  char pePath[512];

  strCopy(binaryPath, dbDirectory);
  strCat(binaryPath, "/binary.db");

  strCopy(pePath, dbDirectory);
  strCat(pePath, "/pe.db");

#ifdef _WIN32
  WIN32_FIND_DATAA  findData;
  HANDLE hfind = FindFirstFileA(binaryPath, &findData);
  if (hfind == INVALID_HANDLE_VALUE)
    return false;
  FindClose(hfind);

  hfind = FindFirstFileA(pePath, &findData);
  if (hfind == INVALID_HANDLE_VALUE)
    return false;
  FindClose(hfind);
#else
  FILE* fp = fopen(binaryPath, "rb");
  if (!fp)
    return false;
  fclose(fp);

  fp = fopen(pePath, "rb");
  if (!fp)
    return false;
  fclose(fp);
#endif

  return true;
}

bool DIEDatabaseManager::DownloadDatabases(void (*progressCallback)(const char*, int))
{
  char destPath[512];

  if (progressCallback)
    progressCallback("Downloading binary.db...", 25);

  strCopy(destPath, dbDirectory);
#ifdef _WIN32
  strCat(destPath, "\\binary.db");
#else
  strCat(destPath, "/binary.db");
#endif

  if (!DownloadFile(DIE_DB_BINARY_URL, destPath))
    return false;

  if (progressCallback)
    progressCallback("Downloading pe.db...", 50);

  strCopy(destPath, dbDirectory);
#ifdef _WIN32
  strCat(destPath, "\\pe.db");
#else
  strCat(destPath, "/pe.db");
#endif

  if (!DownloadFile(DIE_DB_PE_URL, destPath))
    return false;

  if (progressCallback)
    progressCallback("Downloading elf.db...", 75);

  strCopy(destPath, dbDirectory);
#ifdef _WIN32
  strCat(destPath, "\\elf.db");
#else
  strCat(destPath, "/elf.db");
#endif

  if (!DownloadFile(DIE_DB_ELF_URL, destPath))
    return false;

  if (progressCallback)
    progressCallback("Downloading mach.db...", 90);

  strCopy(destPath, dbDirectory);
#ifdef _WIN32
  strCat(destPath, "\\mach.db");
#else
  strCat(destPath, "/mach.db");
#endif

  if (!DownloadFile(DIE_DB_MACH_URL, destPath))
    return false;

  if (progressCallback)
    progressCallback("Download complete!", 100);

  return true;
}

bool DIEDatabaseManager::ParseDatabase(const char* dbPath, DIEDatabase& db)
{
#ifdef _WIN32
  HANDLE hFile = CreateFileA(
    dbPath,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );

  if (hFile == INVALID_HANDLE_VALUE)
    return false;

  DWORD fileSize = GetFileSize(hFile, NULL);
  if (fileSize == INVALID_FILE_SIZE)
  {
    CloseHandle(hFile);
    return false;
  }

  char* fileData = (char*)platformAlloc(fileSize + 1);
  if (!fileData)
  {
    CloseHandle(hFile);
    return false;
  }

  DWORD bytesRead = 0;
  if (!ReadFile(hFile, fileData, fileSize, &bytesRead, NULL) || bytesRead != fileSize)
  {
    platformFree(fileData);
    CloseHandle(hFile);
    return false;
  }

  fileData[fileSize] = '\0';
  CloseHandle(hFile);

  db.signatureCapacity = 100;
  db.signatures = (DIESignature*)platformAlloc(sizeof(DIESignature) * db.signatureCapacity);
  db.signatureCount = 0;

  db.loaded = true;

  platformFree(fileData);
  return true;

#else
  int fd = open(dbPath, O_RDONLY);
  if (fd < 0)
    return false;

  off_t fileSize = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  char* fileData = (char*)platformAlloc(fileSize + 1);
  if (!fileData)
  {
    close(fd);
    return false;
  }

  read(fd, fileData, fileSize);
  fileData[fileSize] = '\0';
  close(fd);

  db.signatureCapacity = 100;
  db.signatures = (DIESignature*)platformAlloc(sizeof(DIESignature) * db.signatureCapacity);
  db.signatureCount = 0;
  db.loaded = true;

  platformFree(fileData);
  return true;
#endif
}

bool DIEDatabaseManager::LoadDatabases()
{
  char dbPath[512];

  strCopy(dbPath, dbDirectory);
#ifdef _WIN32
  strCat(dbPath, "\\binary.db");
#else
  strCat(dbPath, "/binary.db");
#endif
  ParseDatabase(dbPath, binaryDb);

  strCopy(dbPath, dbDirectory);
#ifdef _WIN32
  strCat(dbPath, "\\pe.db");
#else
  strCat(dbPath, "/pe.db");
#endif
  ParseDatabase(dbPath, peDb);

  strCopy(dbPath, dbDirectory);
#ifdef _WIN32
  strCat(dbPath, "\\elf.db");
#else
  strCat(dbPath, "/elf.db");
#endif
  ParseDatabase(dbPath, elfDb);

  strCopy(dbPath, dbDirectory);
#ifdef _WIN32
  strCat(dbPath, "\\mach.db");
#else
  strCat(dbPath, "/mach.db");
#endif
  ParseDatabase(dbPath, machDb);

  return true;
}

bool DIEDatabaseManager::MatchSignature(const uint8_t* data, size_t dataSize, const DIESignature& sig)
{
  if (sig.offset + sig.patternLength > (int)dataSize)
    return false;

  for (int i = 0; i < sig.patternLength; i++)
  {
    if (data[sig.offset + i] != sig.pattern[i])
      return false;
  }

  return true;
}

bool DIEDatabaseManager::AnalyzeFile(const uint8_t* data, size_t dataSize,
  char* outType, char* outCompiler, char* outArch)
{
  if (!data || dataSize < 4)
    return false;

  if (data[0] == 0x4D && data[1] == 0x5A)
  {
    strCopy(outType, "PE Executable");

    if (dataSize > 0x3C + 4)
    {
      uint32_t peOffset = *(uint32_t*)(data + 0x3C);
      if (peOffset < dataSize - 4 &&
        data[peOffset] == 'P' && data[peOffset + 1] == 'E')
      {
        strCopy(outCompiler, "Microsoft Visual C++");
        strCopy(outArch, "x86");
        return true;
      }
    }
  }
  else if (data[0] == 0x7F && data[1] == 'E' && data[2] == 'L' && data[3] == 'F')
  {
    strCopy(outType, "ELF Executable");
    strCopy(outCompiler, "GCC");

    if (dataSize > 4)
    {
      if (data[4] == 1)
        strCopy(outArch, "x86");
      else if (data[4] == 2)
        strCopy(outArch, "x86-64");
      else
        strCopy(outArch, "Unknown");
    }
    return true;
  }
  else if (data[0] == 0xCF && data[1] == 0xFA && data[2] == 0xED && data[3] == 0xFE)
  {
    strCopy(outType, "Mach-O Executable");
    strCopy(outCompiler, "Clang/LLVM");
    strCopy(outArch, "x86-64");
    return true;
  }
  else if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
  {
    strCopy(outType, "PNG Image");
    strCopy(outCompiler, "N/A");
    strCopy(outArch, "N/A");
    return true;
  }
  else if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
  {
    strCopy(outType, "JPEG Image");
    strCopy(outCompiler, "N/A");
    strCopy(outArch, "N/A");
    return true;
  }

  strCopy(outType, "Unknown");
  strCopy(outCompiler, "Unknown");
  strCopy(outArch, "Unknown");

  return false;
}

bool InitializeDIEDatabase(const char* dieExecutablePath)
{
  return g_DIEDatabase.Initialize(dieExecutablePath);
}

bool DownloadDIEDatabases(void (*progressCallback)(const char*, int))
{
  return g_DIEDatabase.DownloadDatabases(progressCallback);
}

void AnalyzeFileWithDIE(const uint8_t* data, size_t dataSize,
  char* outType, char* outCompiler, char* outArch)
{
  g_DIEDatabase.AnalyzeFile(data, dataSize, outType, outCompiler, outArch);
}
