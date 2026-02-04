#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

struct TranslationEntry {
  char key[128];
  char value[256];
  TranslationEntry* next;
  
  TranslationEntry() : next(nullptr) {
    key[0] = '\0';
    value[0] = '\0';
  }
};

struct LanguageMap {
  char language[32];
  TranslationEntry* entries;
  LanguageMap* next;
  
  LanguageMap() : entries(nullptr), next(nullptr) {
    language[0] = '\0';
  }
  
  ~LanguageMap() {
    TranslationEntry* entry = entries;
    while (entry) {
      TranslationEntry* temp = entry;
      entry = entry->next;
#ifdef _WIN32
      HeapFree(GetProcessHeap(), 0, temp);
#else
      free(temp);
#endif
    }
  }
  
  static void SafestrCopy(char* dest, const char* src, size_t destSize) {
    size_t i = 0;
    while (src[i] && i < destSize - 1) {
      dest[i] = src[i];
      i++;
    }
    dest[i] = '\0';
  }
  
  static bool StrEqual(const char* s1, const char* s2) {
    while (*s1 && *s2) {
      if (*s1 != *s2) return false;
      s1++;
      s2++;
    }
    return *s1 == *s2;
  }
  
  void AddEntry(const char* k, const char* v) {
#ifdef _WIN32
    TranslationEntry* newEntry = (TranslationEntry*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TranslationEntry));
#else
    TranslationEntry* newEntry = (TranslationEntry*)malloc(sizeof(TranslationEntry));
#endif
    newEntry->next = nullptr;
    newEntry->key[0] = '\0';
    newEntry->value[0] = '\0';
    
    SafestrCopy(newEntry->key, k, sizeof(newEntry->key));
    SafestrCopy(newEntry->value, v, sizeof(newEntry->value));
    newEntry->next = entries;
    entries = newEntry;
  }
  
  const char* Get(const char* k) const {
    TranslationEntry* entry = entries;
    while (entry) {
      if (StrEqual(entry->key, k)) {
        return entry->value;
      }
      entry = entry->next;
    }
    return nullptr;
  }
};

class Translations {
private:
  static char* GetDefaultLanguagePtr() {
    static char defaultLanguage[32] = {'E','n','g','l','i','s','h','\0'};
    return defaultLanguage;
  }
  
  static LanguageMap** GetLanguagesPtr() {
    static LanguageMap* languages = nullptr;
    return &languages;
  }
  
  static LanguageMap* findLanguage(const char* lang) {
    LanguageMap* map = *GetLanguagesPtr();
    while (map) {
      if (LanguageMap::StrEqual(map->language, lang)) {
        return map;
      }
      map = map->next;
    }
    return nullptr;
  }
  
  static LanguageMap* GetOrCreateLanguage(const char* lang) {
    LanguageMap* existing = findLanguage(lang);
    if (existing) return existing;
    
#ifdef _WIN32
    LanguageMap* newLang = (LanguageMap*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LanguageMap));
#else
    LanguageMap* newLang = (LanguageMap*)malloc(sizeof(LanguageMap));
#endif
    newLang->entries = nullptr;
    newLang->next = nullptr;
    newLang->language[0] = '\0';
    
    LanguageMap::SafestrCopy(newLang->language, lang, sizeof(newLang->language));
    newLang->next = *GetLanguagesPtr();
    *GetLanguagesPtr() = newLang;
    return newLang;
  }
  
  static void AddTranslation(const char* lang, const char* key, const char* value) {
    LanguageMap* langMap = GetOrCreateLanguage(lang);
    langMap->AddEntry(key, value);
  }

public:
  static void Initialize() {
    LanguageMap** languagesPtr = GetLanguagesPtr();
    while (*languagesPtr) {
      LanguageMap* temp = *languagesPtr;
      *languagesPtr = (*languagesPtr)->next;
      
      TranslationEntry* entry = temp->entries;
      while (entry) {
        TranslationEntry* tempEntry = entry;
        entry = entry->next;
#ifdef _WIN32
        HeapFree(GetProcessHeap(), 0, tempEntry);
#else
        free(tempEntry);
#endif
      }
#ifdef _WIN32
      HeapFree(GetProcessHeap(), 0, temp);
#else
      free(temp);
#endif
    }

    AddTranslation("English", "Options", "Options");
    AddTranslation("English", "Dark Mode", "Dark Mode");
    AddTranslation("English", "Auto-reload modified file", "Auto-reload modified file");
    AddTranslation("English", "Add to context menu (right-click files)", "Add to context menu (right-click files)");
    AddTranslation("English", "Language:", "Language:");
    AddTranslation("English", "Default bytes per line:", "Default bytes per line:");
    AddTranslation("English", "8 bytes", "8 bytes");
    AddTranslation("English", "16 bytes", "16 bytes");
    AddTranslation("English", "OK", "OK");
    AddTranslation("English", "Cancel", "Cancel");
    AddTranslation("English", "File", "File");
    AddTranslation("English", "Open", "Open");
    AddTranslation("English", "Save", "Save");
    AddTranslation("English", "Exit", "Exit");
    AddTranslation("English", "View", "View");
    AddTranslation("English", "Disassembly", "Disassembly");
    AddTranslation("English", "Search", "Search");
    AddTranslation("English", "find and Replace...", "find and Replace...");
    AddTranslation("English", "Go To...", "Go To...");
    AddTranslation("English", "Tools", "Tools");
    AddTranslation("English", "Options...", "Options...");
    AddTranslation("English", "Help", "Help");
    AddTranslation("English", "About HexViewer", "About HexViewer");
    AddTranslation("English", "find:", "find:");
    AddTranslation("English", "Replace:", "Replace:");
    AddTranslation("English", "Replace", "Replace");
    AddTranslation("English", "Go", "Go");
    AddTranslation("English", "Offset:", "Offset:");
    AddTranslation("English", "Update Available!", "Update Available!");
    AddTranslation("English", "You're up to date!", "You're up to date!");
    AddTranslation("English", "Current:", "Current:");
    AddTranslation("English", "Latest:", "Latest:");
    AddTranslation("English", "What's New:", "What's New:");
    AddTranslation("English", "You have the latest version installed.", "You have the latest version installed.");
    AddTranslation("English", "Check back later for updates.", "Check back later for updates.");
    AddTranslation("English", "Update Now", "Update Now");
    AddTranslation("English", "Skip", "Skip");
    AddTranslation("English", "Close", "Close");
    AddTranslation("English", "Version", "Version");
    AddTranslation("English", "A modern cross-platform hex editor", "A modern cross-platform hex editor");
    AddTranslation("English", "Features:", "Features:");
    AddTranslation("English", "Cross-platform support", "Cross-platform support");
    AddTranslation("English", "Real-time hex editing", "Real-time hex editing");
    AddTranslation("English", "Dark mode support", "Dark mode support");
    AddTranslation("English", "Check for Updates", "Check for Updates");

    AddTranslation("Spanish", "Options", "Opciones");
    AddTranslation("Spanish", "Dark Mode", "Modo Oscuro");
    AddTranslation("Spanish", "Auto-reload modified file", "Recargar archivo modificado autom�ticamente");
    AddTranslation("Spanish", "Add to context menu (right-click files)", "Agregar al men� contextual (clic derecho en archivos)");
    AddTranslation("Spanish", "Language:", "Idioma:");
    AddTranslation("Spanish", "Default bytes per line:", "Bytes predeterminados por l�nea:");
    AddTranslation("Spanish", "8 bytes", "8 bytes");
    AddTranslation("Spanish", "16 bytes", "16 bytes");
    AddTranslation("Spanish", "OK", "Aceptar");
    AddTranslation("Spanish", "Cancel", "Cancelar");
    AddTranslation("Spanish", "File", "Archivo");
    AddTranslation("Spanish", "Open", "Abrir");
    AddTranslation("Spanish", "Save", "Guardar");
    AddTranslation("Spanish", "Exit", "Salir");
    AddTranslation("Spanish", "View", "Ver");
    AddTranslation("Spanish", "Disassembly", "Desensamblado");
    AddTranslation("Spanish", "Search", "Buscar");
    AddTranslation("Spanish", "find and Replace...", "Buscar y Reemplazar...");
    AddTranslation("Spanish", "Go To...", "Ir a...");
    AddTranslation("Spanish", "Tools", "Herramientas");
    AddTranslation("Spanish", "Options...", "Opciones...");
    AddTranslation("Spanish", "Help", "Ayuda");
    AddTranslation("Spanish", "About HexViewer", "Acerca de HexViewer");
    AddTranslation("Spanish", "find:", "Buscar:");
    AddTranslation("Spanish", "Replace:", "Reemplazar:");
    AddTranslation("Spanish", "Replace", "Reemplazar");
    AddTranslation("Spanish", "Go", "Ir");
    AddTranslation("Spanish", "Offset:", "Desplazamiento:");
    AddTranslation("Spanish", "Update Available!", "�Actualizaci�n Disponible!");
    AddTranslation("Spanish", "You're up to date!", "�Est�s actualizado!");
    AddTranslation("Spanish", "Current:", "Actual:");
    AddTranslation("Spanish", "Latest:", "�ltima:");
    AddTranslation("Spanish", "What's New:", "Novedades:");
    AddTranslation("Spanish", "You have the latest version installed.", "Tienes la �ltima versi�n instalada.");
    AddTranslation("Spanish", "Check back later for updates.", "Vuelve m�s tarde para buscar actualizaciones.");
    AddTranslation("Spanish", "Update Now", "Actualizar Ahora");
    AddTranslation("Spanish", "Skip", "Omitir");
    AddTranslation("Spanish", "Close", "Cerrar");
    AddTranslation("Spanish", "Version", "Versi�n");
    AddTranslation("Spanish", "A modern cross-platform hex editor", "Un editor hexadecimal multiplataforma moderno");
    AddTranslation("Spanish", "Features:", "Caracter�sticas:");
    AddTranslation("Spanish", "Cross-platform support", "Soporte multiplataforma");
    AddTranslation("Spanish", "Real-time hex editing", "Edici�n hexadecimal en tiempo real");
    AddTranslation("Spanish", "Dark mode support", "Soporte de modo oscuro");
    AddTranslation("Spanish", "Check for Updates", "Buscar Actualizaciones");

    AddTranslation("French", "Options", "Options");
    AddTranslation("French", "Dark Mode", "Mode Sombre");
    AddTranslation("French", "Auto-reload modified file", "Recharger automatiquement le fichier modifi�");
    AddTranslation("French", "Add to context menu (right-click files)", "Ajouter au menu contextuel (clic droit sur les fichiers)");
    AddTranslation("French", "Language:", "Langue:");
    AddTranslation("French", "Default bytes per line:", "Octets par ligne par d�faut:");
    AddTranslation("French", "8 bytes", "8 octets");
    AddTranslation("French", "16 bytes", "16 octets");
    AddTranslation("French", "OK", "OK");
    AddTranslation("French", "Cancel", "Annuler");
    AddTranslation("French", "File", "Fichier");
    AddTranslation("French", "Open", "Ouvrir");
    AddTranslation("French", "Save", "Enregistrer");
    AddTranslation("French", "Exit", "Quitter");
    AddTranslation("French", "View", "Affichage");
    AddTranslation("French", "Disassembly", "D�sassemblage");
    AddTranslation("French", "Search", "Recherche");
    AddTranslation("French", "find and Replace...", "Rechercher et Remplacer...");
    AddTranslation("French", "Go To...", "Aller �...");
    AddTranslation("French", "Tools", "Outils");
    AddTranslation("French", "Options...", "Options...");
    AddTranslation("French", "Help", "Aide");
    AddTranslation("French", "About HexViewer", "� propos de HexViewer");
    AddTranslation("French", "find:", "Rechercher:");
    AddTranslation("French", "Replace:", "Remplacer:");
    AddTranslation("French", "Replace", "Remplacer");
    AddTranslation("French", "Go", "Aller");
    AddTranslation("French", "Offset:", "D�calage:");
    AddTranslation("French", "Update Available!", "Mise � jour disponible!");
    AddTranslation("French", "You're up to date!", "Vous �tes � jour!");
    AddTranslation("French", "Current:", "Actuelle:");
    AddTranslation("French", "Latest:", "Derniere:");
    AddTranslation("French", "What's New:", "Nouveaut�s:");
    AddTranslation("French", "You have the latest version installed.", "Vous avez la derni�re version install�e.");
    AddTranslation("French", "Check back later for updates.", "Revenez plus tard pour les mises � jour.");
    AddTranslation("French", "Update Now", "Mettre � jour maintenant");
    AddTranslation("French", "Skip", "Ignorer");
    AddTranslation("French", "Close", "Fermer");
    AddTranslation("French", "Version", "Version");
    AddTranslation("French", "A modern cross-platform hex editor", "Un �diteur hexad�cimal multiplateforme moderne");
    AddTranslation("French", "Features:", "Fonctionnalit�s:");
    AddTranslation("French", "Cross-platform support", "Support multiplateforme");
    AddTranslation("French", "Real-time hex editing", "Adition hexadicimale en temps rael");
    AddTranslation("French", "Dark mode support", "Support du mode sombre");
    AddTranslation("French", "Check for Updates", "Verifier les mises � jour");

    AddTranslation("German", "Options", "Optionen");
    AddTranslation("German", "Dark Mode", "Dunkler Modus");
    AddTranslation("German", "Auto-reload modified file", "Ge�nderte Datei automatisch neu laden");
    AddTranslation("German", "Add to context menu (right-click files)", "Zum Kontextmen� hinzuf�gen (Rechtsklick auf Dateien)");
    AddTranslation("German", "Language:", "Sprache:");
    AddTranslation("German", "Default bytes per line:", "Standard-Bytes pro Zeile:");
    AddTranslation("German", "8 bytes", "8 Bytes");
    AddTranslation("German", "16 bytes", "16 Bytes");
    AddTranslation("German", "OK", "OK");
    AddTranslation("German", "Cancel", "Abbrechen");
    AddTranslation("German", "File", "Datei");
    AddTranslation("German", "Open", "Öffnen");
    AddTranslation("German", "Save", "Speichern");
    AddTranslation("German", "Exit", "Beenden");
    AddTranslation("German", "View", "Ansicht");
    AddTranslation("German", "Disassembly", "Disassemblierung");
    AddTranslation("German", "Search", "Suchen");
    AddTranslation("German", "find and Replace...", "Suchen und Ersetzen...");
    AddTranslation("German", "Go To...", "Gehe zu...");
    AddTranslation("German", "Tools", "Werkzeuge");
    AddTranslation("German", "Options...", "Optionen...");
    AddTranslation("German", "Help", "Hilfe");
    AddTranslation("German", "About HexViewer", "Über HexViewer");
    AddTranslation("German", "find:", "Suchen:");
    AddTranslation("German", "Replace:", "Ersetzen:");
    AddTranslation("German", "Replace", "Ersetzen");
    AddTranslation("German", "Go", "Gehe");
    AddTranslation("German", "Offset:", "Offset:");
    AddTranslation("German", "Update Available!", "Update verfügbar!");
    AddTranslation("German", "You're up to date!", "Sie sind auf dem neuesten Stand!");
    AddTranslation("German", "Current:", "Aktuell:");
    AddTranslation("German", "Latest:", "Neueste:");
    AddTranslation("German", "What's New:", "Was ist neu:");
    AddTranslation("German", "You have the latest version installed.", "Sie haben die neueste Version installiert.");
    AddTranslation("German", "Check back later for updates.", "Schauen Sie später nach Updates.");
    AddTranslation("German", "Update Now", "Jetzt aktualisieren");
    AddTranslation("German", "Skip", "Überspringen");
    AddTranslation("German", "Close", "Schließen");
    AddTranslation("German", "Version", "Version");
    AddTranslation("German", "A modern cross-platform hex editor", "Ein moderner plattform�bergreifender Hex-Editor");
    AddTranslation("German", "Features:", "Funktionen:");
    AddTranslation("German", "Cross-platform support", "Plattformübergreifende Unterstützung");
    AddTranslation("German", "Real-time hex editing", "Echtzeit-Hex-Bearbeitung");
    AddTranslation("German", "Dark mode support", "Dunkler Modus Unterstützung");
    AddTranslation("German", "Check for Updates", "Nach Updates suchen");

    AddTranslation("Japanese", "Options", "?????");
    AddTranslation("Japanese", "Dark Mode", "??????");
    AddTranslation("Japanese", "Auto-reload modified file", "?????????????????");
    AddTranslation("Japanese", "Add to context menu (right-click files)", "?????????????(??????????)");
    AddTranslation("Japanese", "Language:", "??:");
    AddTranslation("Japanese", "Default bytes per line:", "1??????????????:");
    AddTranslation("Japanese", "8 bytes", "8???");
    AddTranslation("Japanese", "16 bytes", "16???");
    AddTranslation("Japanese", "OK", "OK");
    AddTranslation("Japanese", "Cancel", "?????");
    AddTranslation("Japanese", "File", "????");
    AddTranslation("Japanese", "Open", "??");
    AddTranslation("Japanese", "Save", "??");
    AddTranslation("Japanese", "Exit", "??");
    AddTranslation("Japanese", "View", "??");
    AddTranslation("Japanese", "Disassembly", "??????");
    AddTranslation("Japanese", "Search", "??");
    AddTranslation("Japanese", "find and Replace...", "?????...");
    AddTranslation("Japanese", "Go To...", "??...");
    AddTranslation("Japanese", "Tools", "???");
    AddTranslation("Japanese", "Options...", "?????...");
    AddTranslation("Japanese", "Help", "???");
    AddTranslation("Japanese", "About HexViewer", "HexViewer????");
    AddTranslation("Japanese", "find:", "??:");
    AddTranslation("Japanese", "Replace:", "??:");
    AddTranslation("Japanese", "Replace", "??");
    AddTranslation("Japanese", "Go", "??");
    AddTranslation("Japanese", "Offset:", "?????:");
    AddTranslation("Japanese", "Update Available!", "??????????!");
    AddTranslation("Japanese", "You're up to date!", "????!");
    AddTranslation("Japanese", "Current:", "??:");
    AddTranslation("Japanese", "Latest:", "??:");
    AddTranslation("Japanese", "What's New:", "???:");
    AddTranslation("Japanese", "You have the latest version installed.", "?????????????????????");
    AddTranslation("Japanese", "Check back later for updates.", "??????????????????");
    AddTranslation("Japanese", "Update Now", "?????");
    AddTranslation("Japanese", "Skip", "????");
    AddTranslation("Japanese", "Close", "???");
    AddTranslation("Japanese", "Version", "?????");
    AddTranslation("Japanese", "A modern cross-platform hex editor", "????????????????16?????");
    AddTranslation("Japanese", "Features:", "??:");
    AddTranslation("Japanese", "Cross-platform support", "?????????????");
    AddTranslation("Japanese", "Real-time hex editing", "??????16???");
    AddTranslation("Japanese", "Dark mode support", "????????");
    AddTranslation("Japanese", "Check for Updates", "?????????");

    AddTranslation("Russian", "Options", "??");
    AddTranslation("Russian", "Dark Mode", "????");
    AddTranslation("Russian", "Auto-reload modified file", "???????????");
    AddTranslation("Russian", "Add to context menu (right-click files)", "????????(??????)");
    AddTranslation("Russian", "Language:", "??:");
    AddTranslation("Russian", "Default bytes per line:", "???????:");
    AddTranslation("Russian", "8 bytes", "8??");
    AddTranslation("Russian", "16 bytes", "16??");
    AddTranslation("Russian", "OK", "??");
    AddTranslation("Russian", "Cancel", "??");
    AddTranslation("Russian", "File", "??");
    AddTranslation("Russian", "Open", "??");
    AddTranslation("Russian", "Save", "??");
    AddTranslation("Russian", "Exit", "??");
    AddTranslation("Russian", "View", "??");
    AddTranslation("Russian", "Disassembly", "???");
    AddTranslation("Russian", "Search", "??");
    AddTranslation("Russian", "find and Replace...", "?????...");
    AddTranslation("Russian", "Go To...", "??...");
    AddTranslation("Russian", "Tools", "??");
    AddTranslation("Russian", "Options...", "??...");
    AddTranslation("Russian", "Help", "??");
    AddTranslation("Russian", "About HexViewer", "??HexViewer");
    AddTranslation("Russian", "find:", "??:");
    AddTranslation("Russian", "Replace:", "??:");
    AddTranslation("Russian", "Replace", "??");
    AddTranslation("Russian", "Go", "??");
    AddTranslation("Russian", "Offset:", "???:");
    AddTranslation("Russian", "Update Available!", "?????!");
    AddTranslation("Russian", "You're up to date!", "???????!");
    AddTranslation("Russian", "Current:", "??:");
    AddTranslation("Russian", "Latest:", "??:");
    AddTranslation("Russian", "What's New:", "???:");
    AddTranslation("Russian", "You have the latest version installed.", "?????????");
    AddTranslation("Russian", "Check back later for updates.", "????????");
    AddTranslation("Russian", "Update Now", "????");
    AddTranslation("Russian", "Skip", "??");
    AddTranslation("Russian", "Close", "??");
    AddTranslation("Russian", "Version", "??");
    AddTranslation("Russian", "A modern cross-platform hex editor", "????????????");
    AddTranslation("Russian", "Features:", "??:");
    AddTranslation("Russian", "Cross-platform support", "?????");
    AddTranslation("Russian", "Real-time hex editing", "????????");
    AddTranslation("Russian", "Dark mode support", "??????");
    AddTranslation("Russian", "Check for Updates", "????");

    AddTranslation("Chinese", "Options", "??");
    AddTranslation("Chinese", "Dark Mode", "????");
    AddTranslation("Chinese", "Auto-reload modified file", "???????????");
    AddTranslation("Chinese", "Add to context menu (right-click files)", "????????(??????)");
    AddTranslation("Chinese", "Language:", "??:");
    AddTranslation("Chinese", "Default bytes per line:", "???????:");
    AddTranslation("Chinese", "8 bytes", "8??");
    AddTranslation("Chinese", "16 bytes", "16??");
    AddTranslation("Chinese", "OK", "??");
    AddTranslation("Chinese", "Cancel", "??");
    AddTranslation("Chinese", "File", "??");
    AddTranslation("Chinese", "Open", "??");
    AddTranslation("Chinese", "Save", "??");
    AddTranslation("Chinese", "Exit", "??");
    AddTranslation("Chinese", "View", "??");
    AddTranslation("Chinese", "Disassembly", "???");
    AddTranslation("Chinese", "Search", "??");
    AddTranslation("Chinese", "find and Replace...", "?????...");
    AddTranslation("Chinese", "Go To...", "??...");
    AddTranslation("Chinese", "Tools", "??");
    AddTranslation("Chinese", "Options...", "??...");
    AddTranslation("Chinese", "Help", "??");
    AddTranslation("Chinese", "About HexViewer", "??HexViewer");
    AddTranslation("Chinese", "find:", "??:");
    AddTranslation("Chinese", "Replace:", "??:");
    AddTranslation("Chinese", "Replace", "??");
    AddTranslation("Chinese", "Go", "??");
    AddTranslation("Chinese", "Offset:", "???:");
    AddTranslation("Chinese", "Update Available!", "?????!");
    AddTranslation("Chinese", "You're up to date!", "???????!");
    AddTranslation("Chinese", "Current:", "??:");
    AddTranslation("Chinese", "Latest:", "??:");
    AddTranslation("Chinese", "What's New:", "???:");
    AddTranslation("Chinese", "You have the latest version installed.", "?????????");
    AddTranslation("Chinese", "Check back later for updates.", "????????");
    AddTranslation("Chinese", "Update Now", "????");
    AddTranslation("Chinese", "Skip", "??");
    AddTranslation("Chinese", "Close", "??");
    AddTranslation("Chinese", "Version", "??");
    AddTranslation("Chinese", "A modern cross-platform hex editor", "????????????");
    AddTranslation("Chinese", "Features:", "??:");
    AddTranslation("Chinese", "Cross-platform support", "?????");
    AddTranslation("Chinese", "Real-time hex editing", "????????");
    AddTranslation("Chinese", "Dark mode support", "??????");
    AddTranslation("Chinese", "Check for Updates", "????");

    AddTranslation("Italian", "Options", "Opzioni");
    AddTranslation("Italian", "Dark Mode", "Modalità Scura");
    AddTranslation("Italian", "Auto-reload modified file", "Ricarica automaticamente il file modificato");
    AddTranslation("Italian", "Add to context menu (right-click files)", "Aggiungi al menu contestuale (clic destro sui file)");
    AddTranslation("Italian", "Language:", "Lingua:");
    AddTranslation("Italian", "Default bytes per line:", "Byte predefiniti per riga:");
    AddTranslation("Italian", "8 bytes", "8 byte");
    AddTranslation("Italian", "16 bytes", "16 byte");
    AddTranslation("Italian", "OK", "OK");
    AddTranslation("Italian", "Cancel", "Annulla");
    AddTranslation("Italian", "File", "File");
    AddTranslation("Italian", "Open", "Apri");
    AddTranslation("Italian", "Save", "Salva");
    AddTranslation("Italian", "Exit", "Esci");
    AddTranslation("Italian", "View", "Visualizza");
    AddTranslation("Italian", "Disassembly", "Disassemblaggio");
    AddTranslation("Italian", "Search", "Cerca");
    AddTranslation("Italian", "find and Replace...", "Trova e Sostituisci...");
    AddTranslation("Italian", "Go To...", "Vai a...");
    AddTranslation("Italian", "Tools", "Strumenti");
    AddTranslation("Italian", "Options...", "Opzioni...");
    AddTranslation("Italian", "Help", "Aiuto");
    AddTranslation("Italian", "About HexViewer", "Informazioni su HexViewer");
    AddTranslation("Italian", "find:", "Trova:");
    AddTranslation("Italian", "Replace:", "Sostituisci:");
    AddTranslation("Italian", "Replace", "Sostituisci");
    AddTranslation("Italian", "Go", "Vai");
    AddTranslation("Italian", "Offset:", "Offset:");
    AddTranslation("Italian", "Update Available!", "Aggiornamento Disponibile!");
    AddTranslation("Italian", "You're up to date!", "Sei aggiornato!");
    AddTranslation("Italian", "Current:", "Corrente:");
    AddTranslation("Italian", "Latest:", "Ultima:");
    AddTranslation("Italian", "What's New:", "Novità:");
    AddTranslation("Italian", "You have the latest version installed.", "Hai installato l'ultima versione.");
    AddTranslation("Italian", "Check back later for updates.", "Controlla più tardi per gli aggiornamenti.");
    AddTranslation("Italian", "Update Now", "Aggiorna Ora");
    AddTranslation("Italian", "Skip", "Salta");
    AddTranslation("Italian", "Close", "Chiudi");
    AddTranslation("Italian", "Version", "Versione");
    AddTranslation("Italian", "A modern cross-platform hex editor", "Un editor esadecimale multipiattaforma moderno");
    AddTranslation("Italian", "Features:", "Caratteristiche:");
    AddTranslation("Italian", "Cross-platform support", "Supporto multipiattaforma");
    AddTranslation("Italian", "Real-time hex editing", "Modifica esadecimale in tempo reale");
    AddTranslation("Italian", "Dark mode support", "Supporto modalità scura");
    AddTranslation("Italian", "Check for Updates", "Controlla Aggiornamenti");

    AddTranslation("Portuguese", "Options", "Opções");
    AddTranslation("Portuguese", "Dark Mode", "Modo Escuro");
    AddTranslation("Portuguese", "Auto-reload modified file", "Recarregar arquivo modificado automaticamente");
    AddTranslation("Portuguese", "Add to context menu (right-click files)", "Adicionar ao menu de contexto (clique direito nos arquivos)");
    AddTranslation("Portuguese", "Language:", "Idioma:");
    AddTranslation("Portuguese", "Default bytes per line:", "Bytes padrão por linha:");
    AddTranslation("Portuguese", "8 bytes", "8 bytes");
    AddTranslation("Portuguese", "16 bytes", "16 bytes");
    AddTranslation("Portuguese", "OK", "OK");
    AddTranslation("Portuguese", "Cancel", "Cancelar");
    AddTranslation("Portuguese", "File", "Arquivo");
    AddTranslation("Portuguese", "Open", "Abrir");
    AddTranslation("Portuguese", "Save", "Salvar");
    AddTranslation("Portuguese", "Exit", "Sair");
    AddTranslation("Portuguese", "View", "Visualizar");
    AddTranslation("Portuguese", "Disassembly", "Desmontagem");
    AddTranslation("Portuguese", "Search", "Pesquisar");
    AddTranslation("Portuguese", "find and Replace...", "Localizar e Substituir...");
    AddTranslation("Portuguese", "Go To...", "Ir Para...");
    AddTranslation("Portuguese", "Tools", "Ferramentas");
    AddTranslation("Portuguese", "Options...", "Opções...");
    AddTranslation("Portuguese", "Help", "Ajuda");
    AddTranslation("Portuguese", "About HexViewer", "Sobre o HexViewer");
    AddTranslation("Portuguese", "find:", "Localizar:");
    AddTranslation("Portuguese", "Replace:", "Substituir:");
    AddTranslation("Portuguese", "Replace", "Substituir");
    AddTranslation("Portuguese", "Go", "Ir");
    AddTranslation("Portuguese", "Offset:", "Deslocamento:");
    AddTranslation("Portuguese", "Update Available!", "Atualização Disponível!");
    AddTranslation("Portuguese", "You're up to date!", "Você está atualizado!");
    AddTranslation("Portuguese", "Current:", "Atual:");
    AddTranslation("Portuguese", "Latest:", "Mais Recente:");
    AddTranslation("Portuguese", "What's New:", "Novidades:");
    AddTranslation("Portuguese", "You have the latest version installed.", "Você tem a versão mais recente instalada.");
    AddTranslation("Portuguese", "Check back later for updates.", "Volte mais tarde para verificar atualizações.");
    AddTranslation("Portuguese", "Update Now", "Atualizar Agora");
    AddTranslation("Portuguese", "Skip", "Pular");
    AddTranslation("Portuguese", "Close", "Fechar");
    AddTranslation("Portuguese", "Version", "Versão");
    AddTranslation("Portuguese", "A modern cross-platform hex editor", "Um editor hexadecimal multiplataforma moderno");
    AddTranslation("Portuguese", "Features:", "Recursos:");
    AddTranslation("Portuguese", "Cross-platform support", "Suporte multiplataforma");
    AddTranslation("Portuguese", "Real-time hex editing", "Edição hexadecimal em tempo real");
    AddTranslation("Portuguese", "Dark mode support", "Suporte ao modo escuro");
    AddTranslation("Portuguese", "Check for Updates", "Verificar Atualizações");

    AddTranslation("Korean", "Options", "옵션");
    AddTranslation("Korean", "Dark Mode", "다크 모드");
    AddTranslation("Korean", "Auto-reload modified file", "수정된 파일 자동 새로고침");
    AddTranslation("Korean", "Add to context menu (right-click files)", "컨텍스트 메뉴에 추가 (파일 마우스 오른쪽 클릭)");
    AddTranslation("Korean", "Language:", "언어:");
    AddTranslation("Korean", "Default bytes per line:", "줄당 기본 바이트:");
    AddTranslation("Korean", "8 bytes", "8바이트");
    AddTranslation("Korean", "16 bytes", "16바이트");
    AddTranslation("Korean", "OK", "확인");
    AddTranslation("Korean", "Cancel", "취소");
    AddTranslation("Korean", "File", "파일");
    AddTranslation("Korean", "Open", "열기");
    AddTranslation("Korean", "Save", "저장");
    AddTranslation("Korean", "Exit", "종료");
    AddTranslation("Korean", "View", "보기");
    AddTranslation("Korean", "Disassembly", "디스어셈블리");
    AddTranslation("Korean", "Search", "검색");
    AddTranslation("Korean", "find and Replace...", "찾기 및 바꾸기...");
    AddTranslation("Korean", "Go To...", "이동...");
    AddTranslation("Korean", "Tools", "도구");
    AddTranslation("Korean", "Options...", "옵션...");
    AddTranslation("Korean", "Help", "도움말");
    AddTranslation("Korean", "About HexViewer", "HexViewer 정보");
    AddTranslation("Korean", "find:", "찾기:");
    AddTranslation("Korean", "Replace:", "바꾸기:");
    AddTranslation("Korean", "Replace", "바꾸기");
    AddTranslation("Korean", "Go", "이동");
    AddTranslation("Korean", "Offset:", "오프셋:");
    AddTranslation("Korean", "Update Available!", "업데이트 사용 가능!");
    AddTranslation("Korean", "You're up to date!", "최신 버전입니다!");
    AddTranslation("Korean", "Current:", "현재:");
    AddTranslation("Korean", "Latest:", "최신:");
    AddTranslation("Korean", "What's New:", "새로운 기능:");
    AddTranslation("Korean", "You have the latest version installed.", "최신 버전이 설치되어 있습니다.");
    AddTranslation("Korean", "Check back later for updates.", "나중에 업데이트를 확인하세요.");
    AddTranslation("Korean", "Update Now", "지금 업데이트");
    AddTranslation("Korean", "Skip", "건너뛰기");
    AddTranslation("Korean", "Close", "닫기");
    AddTranslation("Korean", "Version", "버전");
    AddTranslation("Korean", "A modern cross-platform hex editor", "현대적인 크로스 플랫폼 16진수 편집기");
    AddTranslation("Korean", "Features:", "기능:");
    AddTranslation("Korean", "Cross-platform support", "크로스 플랫폼 지원");
    AddTranslation("Korean", "Real-time hex editing", "실시간 16진수 편집");
    AddTranslation("Korean", "Dark mode support", "다크 모드 지원");
    AddTranslation("Korean", "Check for Updates", "업데이트 확인");

    AddTranslation("Dutch", "Options", "Opties");
    AddTranslation("Dutch", "Dark Mode", "Donkere Modus");
    AddTranslation("Dutch", "Auto-reload modified file", "Gewijzigd bestand automatisch opnieuw laden");
    AddTranslation("Dutch", "Add to context menu (right-click files)", "Toevoegen aan contextmenu (rechtermuisklik op bestanden)");
    AddTranslation("Dutch", "Language:", "Taal:");
    AddTranslation("Dutch", "Default bytes per line:", "Standaard bytes per regel:");
    AddTranslation("Dutch", "8 bytes", "8 bytes");
    AddTranslation("Dutch", "16 bytes", "16 bytes");
    AddTranslation("Dutch", "OK", "OK");
    AddTranslation("Dutch", "Cancel", "Annuleren");
    AddTranslation("Dutch", "File", "Bestand");
    AddTranslation("Dutch", "Open", "Openen");
    AddTranslation("Dutch", "Save", "Opslaan");
    AddTranslation("Dutch", "Exit", "Afsluiten");
    AddTranslation("Dutch", "View", "Weergave");
    AddTranslation("Dutch", "Disassembly", "Disassemblage");
    AddTranslation("Dutch", "Search", "Zoeken");
    AddTranslation("Dutch", "find and Replace...", "Zoeken en Vervangen...");
    AddTranslation("Dutch", "Go To...", "Ga naar...");
    AddTranslation("Dutch", "Tools", "Hulpmiddelen");
    AddTranslation("Dutch", "Options...", "Opties...");
    AddTranslation("Dutch", "Help", "Help");
    AddTranslation("Dutch", "About HexViewer", "Over HexViewer");
    AddTranslation("Dutch", "find:", "Zoeken:");
    AddTranslation("Dutch", "Replace:", "Vervangen:");
    AddTranslation("Dutch", "Replace", "Vervangen");
    AddTranslation("Dutch", "Go", "Ga");
    AddTranslation("Dutch", "Offset:", "Offset:");
    AddTranslation("Dutch", "Update Available!", "Update Beschikbaar!");
    AddTranslation("Dutch", "You're up to date!", "U bent up-to-date!");
    AddTranslation("Dutch", "Current:", "Huidige:");
    AddTranslation("Dutch", "Latest:", "Nieuwste:");
    AddTranslation("Dutch", "What's New:", "Wat is er nieuw:");
    AddTranslation("Dutch", "You have the latest version installed.", "U heeft de nieuwste versie geïnstalleerd.");
    AddTranslation("Dutch", "Check back later for updates.", "Kom later terug voor updates.");
    AddTranslation("Dutch", "Update Now", "Nu Bijwerken");
    AddTranslation("Dutch", "Skip", "Overslaan");
    AddTranslation("Dutch", "Close", "Sluiten");
    AddTranslation("Dutch", "Version", "Versie");
    AddTranslation("Dutch", "A modern cross-platform hex editor", "Een moderne platformonafhankelijke hex-editor");
    AddTranslation("Dutch", "Features:", "Functies:");
    AddTranslation("Dutch", "Cross-platform support", "Platformonafhankelijke ondersteuning");
    AddTranslation("Dutch", "Real-time hex editing", "Real-time hex bewerking");
    AddTranslation("Dutch", "Dark mode support", "Donkere modus ondersteuning");
    AddTranslation("Dutch", "Check for Updates", "Controleren op Updates");

    AddTranslation("Polish", "Options", "Opcje");
    AddTranslation("Polish", "Dark Mode", "Tryb Ciemny");
    AddTranslation("Polish", "Auto-reload modified file", "Automatycznie przeładuj zmodyfikowany plik");
    AddTranslation("Polish", "Add to context menu (right-click files)", "Dodaj do menu kontekstowego (kliknij prawym przyciskiem myszy pliki)");
    AddTranslation("Polish", "Language:", "Język:");
    AddTranslation("Polish", "Default bytes per line:", "Domyślne bajty na wiersz:");
    AddTranslation("Polish", "8 bytes", "8 bajtów");
    AddTranslation("Polish", "16 bytes", "16 bajtów");
    AddTranslation("Polish", "OK", "OK");
    AddTranslation("Polish", "Cancel", "Anuluj");
    AddTranslation("Polish", "File", "Plik");
    AddTranslation("Polish", "Open", "Otwórz");
    AddTranslation("Polish", "Save", "Zapisz");
    AddTranslation("Polish", "Exit", "Wyjście");
    AddTranslation("Polish", "View", "Widok");
    AddTranslation("Polish", "Disassembly", "Dezasemblacja");
    AddTranslation("Polish", "Search", "Szukaj");
    AddTranslation("Polish", "find and Replace...", "Znajdź i Zamień...");
    AddTranslation("Polish", "Go To...", "Przejdź do...");
    AddTranslation("Polish", "Tools", "Narzędzia");
    AddTranslation("Polish", "Options...", "Opcje...");
    AddTranslation("Polish", "Help", "Pomoc");
    AddTranslation("Polish", "About HexViewer", "O programie HexViewer");
    AddTranslation("Polish", "find:", "Znajdź:");
    AddTranslation("Polish", "Replace:", "Zamień:");
    AddTranslation("Polish", "Replace", "Zamień");
    AddTranslation("Polish", "Go", "Przejdź");
    AddTranslation("Polish", "Offset:", "Przesunięcie:");
    AddTranslation("Polish", "Update Available!", "Dostępna Aktualizacja!");
    AddTranslation("Polish", "You're up to date!", "Masz najnowszą wersję!");
    AddTranslation("Polish", "Current:", "Obecna:");
    AddTranslation("Polish", "Latest:", "Najnowsza:");
    AddTranslation("Polish", "What's New:", "Co nowego:");
    AddTranslation("Polish", "You have the latest version installed.", "Masz zainstalowaną najnowszą wersję.");
    AddTranslation("Polish", "Check back later for updates.", "Sprawdź później, czy są aktualizacje.");
    AddTranslation("Polish", "Update Now", "Aktualizuj Teraz");
    AddTranslation("Polish", "Skip", "Pomiń");
    AddTranslation("Polish", "Close", "Zamknij");
    AddTranslation("Polish", "Version", "Wersja");
    AddTranslation("Polish", "A modern cross-platform hex editor", "Nowoczesny wieloplatformowy edytor szesnastkowy");
    AddTranslation("Polish", "Features:", "Funkcje:");
    AddTranslation("Polish", "Cross-platform support", "Wsparcie wieloplatformowe");
    AddTranslation("Polish", "Real-time hex editing", "Edycja szesnastkowa w czasie rzeczywistym");
    AddTranslation("Polish", "Dark mode support", "Obsługa trybu ciemnego");
    AddTranslation("Polish", "Check for Updates", "Sprawdź Aktualizacje");

    AddTranslation("Turkish", "Options", "Seçenekler");
    AddTranslation("Turkish", "Dark Mode", "Karanlık Mod");
    AddTranslation("Turkish", "Auto-reload modified file", "Değiştirilen dosyayı otomatik yeniden yükle");
    AddTranslation("Turkish", "Add to context menu (right-click files)", "Bağlam menüsüne ekle (dosyalara sağ tıklayın)");
    AddTranslation("Turkish", "Language:", "Dil:");
    AddTranslation("Turkish", "Default bytes per line:", "Satır başına varsayılan bayt:");
    AddTranslation("Turkish", "8 bytes", "8 bayt");
    AddTranslation("Turkish", "16 bytes", "16 bayt");
    AddTranslation("Turkish", "OK", "Tamam");
    AddTranslation("Turkish", "Cancel", "İptal");
    AddTranslation("Turkish", "File", "Dosya");
    AddTranslation("Turkish", "Open", "Aç");
    AddTranslation("Turkish", "Save", "Kaydet");
    AddTranslation("Turkish", "Exit", "Çıkış");
    AddTranslation("Turkish", "View", "Görünüm");
    AddTranslation("Turkish", "Disassembly", "Ayrıştırma");
    AddTranslation("Turkish", "Search", "Ara");
    AddTranslation("Turkish", "find and Replace...", "Bul ve Değiştir...");
    AddTranslation("Turkish", "Go To...", "Git...");
    AddTranslation("Turkish", "Tools", "Araçlar");
    AddTranslation("Turkish", "Options...", "Seçenekler...");
    AddTranslation("Turkish", "Help", "Yardım");
    AddTranslation("Turkish", "About HexViewer", "HexViewer Hakkında");
    AddTranslation("Turkish", "find:", "Bul:");
    AddTranslation("Turkish", "Replace:", "Değiştir:");
    AddTranslation("Turkish", "Replace", "Değiştir");
    AddTranslation("Turkish", "Go", "Git");
    AddTranslation("Turkish", "Offset:", "Ofset:");
    AddTranslation("Turkish", "Update Available!", "Güncelleme Mevcut!");
    AddTranslation("Turkish", "You're up to date!", "Güncelsiniz!");
    AddTranslation("Turkish", "Current:", "Mevcut:");
    AddTranslation("Turkish", "Latest:", "En Son:");
    AddTranslation("Turkish", "What's New:", "Yenilikler:");
    AddTranslation("Turkish", "You have the latest version installed.", "En son sürüm yüklü.");
    AddTranslation("Turkish", "Check back later for updates.", "Güncellemeler için daha sonra tekrar kontrol edin.");
    AddTranslation("Turkish", "Update Now", "Şimdi Güncelle");
    AddTranslation("Turkish", "Skip", "Atla");
    AddTranslation("Turkish", "Close", "Kapat");
    AddTranslation("Turkish", "Version", "Sürüm");
    AddTranslation("Turkish", "A modern cross-platform hex editor", "Modern bir çapraz platform onaltılık düzenleyici");
    AddTranslation("Turkish", "Features:", "Özellikler:");
    AddTranslation("Turkish", "Cross-platform support", "Çapraz platform desteği");
    AddTranslation("Turkish", "Real-time hex editing", "Gerçek zamanlı onaltılık düzenleme");
    AddTranslation("Turkish", "Dark mode support", "Karanlık mod desteği");
    AddTranslation("Turkish", "Check for Updates", "Güncellemeleri Kontrol Et");

    AddTranslation("Swedish", "Options", "Alternativ");
    AddTranslation("Swedish", "Dark Mode", "Mörkt Läge");
    AddTranslation("Swedish", "Auto-reload modified file", "Ladda om ändrad fil automatiskt");
    AddTranslation("Swedish", "Add to context menu (right-click files)", "Lägg till i snabbmenyn (högerklicka på filer)");
    AddTranslation("Swedish", "Language:", "Språk:");
    AddTranslation("Swedish", "Default bytes per line:", "Standardbyte per rad:");
    AddTranslation("Swedish", "8 bytes", "8 byte");
    AddTranslation("Swedish", "16 bytes", "16 byte");
    AddTranslation("Swedish", "OK", "OK");
    AddTranslation("Swedish", "Cancel", "Avbryt");
    AddTranslation("Swedish", "File", "Fil");
    AddTranslation("Swedish", "Open", "Öppna");
    AddTranslation("Swedish", "Save", "Spara");
    AddTranslation("Swedish", "Exit", "Avsluta");
    AddTranslation("Swedish", "View", "Visa");
    AddTranslation("Swedish", "Disassembly", "Disassemblering");
    AddTranslation("Swedish", "Search", "Sök");
    AddTranslation("Swedish", "find and Replace...", "Sök och Ersätt...");
    AddTranslation("Swedish", "Go To...", "Gå till...");
    AddTranslation("Swedish", "Tools", "Verktyg");
    AddTranslation("Swedish", "Options...", "Alternativ...");
    AddTranslation("Swedish", "Help", "Hjälp");
    AddTranslation("Swedish", "About HexViewer", "Om HexViewer");
    AddTranslation("Swedish", "find:", "Sök:");
    AddTranslation("Swedish", "Replace:", "Ersätt:");
    AddTranslation("Swedish", "Replace", "Ersätt");
    AddTranslation("Swedish", "Go", "Gå");
    AddTranslation("Swedish", "Offset:", "Offset:");
    AddTranslation("Swedish", "Update Available!", "Uppdatering Tillgänglig!");
    AddTranslation("Swedish", "You're up to date!", "Du är uppdaterad!");
    AddTranslation("Swedish", "Current:", "Nuvarande:");
    AddTranslation("Swedish", "Latest:", "Senaste:");
    AddTranslation("Swedish", "What's New:", "Vad är nytt:");
    AddTranslation("Swedish", "You have the latest version installed.", "Du har den senaste versionen installerad.");
    AddTranslation("Swedish", "Check back later for updates.", "Kom tillbaka senare för uppdateringar.");
    AddTranslation("Swedish", "Update Now", "Uppdatera Nu");
    AddTranslation("Swedish", "Skip", "Hoppa över");
    AddTranslation("Swedish", "Close", "Stäng");
    AddTranslation("Swedish", "Version", "Version");
    AddTranslation("Swedish", "A modern cross-platform hex editor", "En modern plattformsoberoende hex-redigerare");
    AddTranslation("Swedish", "Features:", "Funktioner:");
    AddTranslation("Swedish", "Cross-platform support", "Plattformsoberoende stöd");
    AddTranslation("Swedish", "Real-time hex editing", "Realtids hex-redigering");
    AddTranslation("Swedish", "Dark mode support", "Mörkt läge stöd");
    AddTranslation("Swedish", "Check for Updates", "Leta efter Uppdateringar");

    AddTranslation("Arabic", "Options", "خيارات");
    AddTranslation("Arabic", "Dark Mode", "الوضع الداكن");
    AddTranslation("Arabic", "Auto-reload modified file", "إعادة تحميل الملف المعدل تلقائيًا");
    AddTranslation("Arabic", "Add to context menu (right-click files)", "إضافة إلى قائمة السياق (انقر بزر الماوس الأيمن على الملفات)");
    AddTranslation("Arabic", "Language:", "اللغة:");
    AddTranslation("Arabic", "Default bytes per line:", "البايتات الافتراضية لكل سطر:");
    AddTranslation("Arabic", "8 bytes", "8 بايت");
    AddTranslation("Arabic", "16 bytes", "16 بايت");
    AddTranslation("Arabic", "OK", "موافق");
    AddTranslation("Arabic", "Cancel", "إلغاء");
    AddTranslation("Arabic", "File", "ملف");
    AddTranslation("Arabic", "Open", "فتح");
    AddTranslation("Arabic", "Save", "حفظ");
    AddTranslation("Arabic", "Exit", "خروج");
    AddTranslation("Arabic", "View", "عرض");
    AddTranslation("Arabic", "Disassembly", "فك التجميع");
    AddTranslation("Arabic", "Search", "بحث");
    AddTranslation("Arabic", "find and Replace...", "بحث واستبدال...");
    AddTranslation("Arabic", "Go To...", "الذهاب إلى...");
    AddTranslation("Arabic", "Tools", "أدوات");
    AddTranslation("Arabic", "Options...", "خيارات...");
    AddTranslation("Arabic", "Help", "مساعدة");
    AddTranslation("Arabic", "About HexViewer", "حول HexViewer");
    AddTranslation("Arabic", "find:", "بحث:");
    AddTranslation("Arabic", "Replace:", "استبدال:");
    AddTranslation("Arabic", "Replace", "استبدال");
    AddTranslation("Arabic", "Go", "اذهب");
    AddTranslation("Arabic", "Offset:", "الإزاحة:");
    AddTranslation("Arabic", "Update Available!", "تحديث متاح!");
    AddTranslation("Arabic", "You're up to date!", "أنت محدث!");
    AddTranslation("Arabic", "Current:", "الحالي:");
    AddTranslation("Arabic", "Latest:", "الأحدث:");
    AddTranslation("Arabic", "What's New:", "ما الجديد:");
    AddTranslation("Arabic", "You have the latest version installed.", "لديك أحدث إصدار مثبت.");
    AddTranslation("Arabic", "Check back later for updates.", "تحقق لاحقًا من التحديثات.");
    AddTranslation("Arabic", "Update Now", "تحديث الآن");
    AddTranslation("Arabic", "Skip", "تخطي");
    AddTranslation("Arabic", "Close", "إغلاق");
    AddTranslation("Arabic", "Version", "الإصدار");
    AddTranslation("Arabic", "A modern cross-platform hex editor", "محرر سداسي عشري حديث متعدد المنصات");
    AddTranslation("Arabic", "Features:", "الميزات:");
    AddTranslation("Arabic", "Cross-platform support", "دعم متعدد المنصات");
    AddTranslation("Arabic", "Real-time hex editing", "تحرير سداسي عشري في الوقت الفعلي");
    AddTranslation("Arabic", "Dark mode support", "دعم الوضع الداكن");
    AddTranslation("Arabic", "Check for Updates", "التحقق من التحديثات");

    AddTranslation("Hindi", "Options", "विकल्प");
    AddTranslation("Hindi", "Dark Mode", "डार्क मोड");
    AddTranslation("Hindi", "Auto-reload modified file", "संशोधित फ़ाइल स्वचालित रूप से पुनः लोड करें");
    AddTranslation("Hindi", "Add to context menu (right-click files)", "संदर्भ मेनू में जोड़ें (फ़ाइलों पर राइट-क्लिक करें)");
    AddTranslation("Hindi", "Language:", "भाषा:");
    AddTranslation("Hindi", "Default bytes per line:", "प्रति पंक्ति डिफ़ॉल्ट बाइट्स:");
    AddTranslation("Hindi", "8 bytes", "8 बाइट्स");
    AddTranslation("Hindi", "16 bytes", "16 बाइट्स");
    AddTranslation("Hindi", "OK", "ठीक है");
    AddTranslation("Hindi", "Cancel", "रद्द करें");
    AddTranslation("Hindi", "File", "फ़ाइल");
    AddTranslation("Hindi", "Open", "खोलें");
    AddTranslation("Hindi", "Save", "सहेजें");
    AddTranslation("Hindi", "Exit", "बाहर निकलें");
    AddTranslation("Hindi", "View", "देखें");
    AddTranslation("Hindi", "Disassembly", "डिसअसेंबली");
    AddTranslation("Hindi", "Search", "खोजें");
    AddTranslation("Hindi", "find and Replace...", "खोजें और बदलें...");
    AddTranslation("Hindi", "Go To...", "इस पर जाएं...");
    AddTranslation("Hindi", "Tools", "उपकरण");
    AddTranslation("Hindi", "Options...", "विकल्प...");
    AddTranslation("Hindi", "Help", "सहायता");
    AddTranslation("Hindi", "About HexViewer", "HexViewer के बारे में");
    AddTranslation("Hindi", "find:", "खोजें:");
    AddTranslation("Hindi", "Replace:", "बदलें:");
    AddTranslation("Hindi", "Replace", "बदलें");
    AddTranslation("Hindi", "Go", "जाएं");
    AddTranslation("Hindi", "Offset:", "ऑफसेट:");
    AddTranslation("Hindi", "Update Available!", "अपडेट उपलब्ध!");
    AddTranslation("Hindi", "You're up to date!", "आप अपडेट हैं!");
    AddTranslation("Hindi", "Current:", "वर्तमान:");
    AddTranslation("Hindi", "Latest:", "नवीनतम:");
    AddTranslation("Hindi", "What's New:", "नया क्या है:");
    AddTranslation("Hindi", "You have the latest version installed.", "आपके पास नवीनतम संस्करण स्थापित है।");
    AddTranslation("Hindi", "Check back later for updates.", "अपडेट के लिए बाद में फिर से जांचें।");
    AddTranslation("Hindi", "Update Now", "अभी अपडेट करें");
    AddTranslation("Hindi", "Skip", "छोड़ें");
    AddTranslation("Hindi", "Close", "बंद करें");
    AddTranslation("Hindi", "Version", "संस्करण");
    AddTranslation("Hindi", "A modern cross-platform hex editor", "एक आधुनिक क्रॉस-प्लेटफ़ॉर्म हेक्स संपादक");
    AddTranslation("Hindi", "Features:", "विशेषताएं:");
    AddTranslation("Hindi", "Cross-platform support", "क्रॉस-प्लेटफ़ॉर्म समर्थन");
    AddTranslation("Hindi", "Real-time hex editing", "वास्तविक समय हेक्स संपादन");
    AddTranslation("Hindi", "Dark mode support", "डार्क मोड समर्थन");
    AddTranslation("Hindi", "Check for Updates", "अपडेट के लिए जाँचें");

    AddTranslation("Czech", "Options", "Možnosti");
    AddTranslation("Czech", "Dark Mode", "Tmavý Režim");
    AddTranslation("Czech", "Auto-reload modified file", "Automaticky znovu načíst upravený soubor");
    AddTranslation("Czech", "Add to context menu (right-click files)", "Přidat do kontextové nabídky (klikněte pravým tlačítkem na soubory)");
    AddTranslation("Czech", "Language:", "Jazyk:");
    AddTranslation("Czech", "Default bytes per line:", "Výchozí bajty na řádek:");
    AddTranslation("Czech", "8 bytes", "8 bajtů");
    AddTranslation("Czech", "16 bytes", "16 bajtů");
    AddTranslation("Czech", "OK", "OK");
    AddTranslation("Czech", "Cancel", "Zrušit");
    AddTranslation("Czech", "File", "Soubor");
    AddTranslation("Czech", "Open", "Otevřít");
    AddTranslation("Czech", "Save", "Uložit");
    AddTranslation("Czech", "Exit", "Ukončit");
    AddTranslation("Czech", "View", "Zobrazit");
    AddTranslation("Czech", "Disassembly", "Disassemblace");
    AddTranslation("Czech", "Search", "Hledat");
    AddTranslation("Czech", "find and Replace...", "Najít a Nahradit...");
    AddTranslation("Czech", "Go To...", "Přejít na...");
    AddTranslation("Czech", "Tools", "Nástroje");
    AddTranslation("Czech", "Options...", "Možnosti...");
    AddTranslation("Czech", "Help", "Nápověda");
    AddTranslation("Czech", "About HexViewer", "O programu HexViewer");
    AddTranslation("Czech", "find:", "Najít:");
    AddTranslation("Czech", "Replace:", "Nahradit:");
    AddTranslation("Czech", "Replace", "Nahradit");
    AddTranslation("Czech", "Go", "Přejít");
    AddTranslation("Czech", "Offset:", "Posun:");
    AddTranslation("Czech", "Update Available!", "Aktualizace k dispozici!");
    AddTranslation("Czech", "You're up to date!", "Máte aktuální verzi!");
    AddTranslation("Czech", "Current:", "Aktuální:");
    AddTranslation("Czech", "Latest:", "Nejnovější:");
    AddTranslation("Czech", "What's New:", "Co je nového:");
    AddTranslation("Czech", "You have the latest version installed.", "Máte nainstalovanou nejnovější verzi.");
    AddTranslation("Czech", "Check back later for updates.", "Vraťte se později pro aktualizace.");
    AddTranslation("Czech", "Update Now", "Aktualizovat nyní");
    AddTranslation("Czech", "Skip", "Přeskočit");
    AddTranslation("Czech", "Close", "Zavřít");
    AddTranslation("Czech", "Version", "Verze");
    AddTranslation("Czech", "A modern cross-platform hex editor", "Moderní multiplatformní hexadecimální editor");
    AddTranslation("Czech", "Features:", "Funkce:");
    AddTranslation("Czech", "Cross-platform support", "Multiplatformní podpora");
    AddTranslation("Czech", "Real-time hex editing", "Úprava hexadecimálních hodnot v reálném čase");
    AddTranslation("Czech", "Dark mode support", "Podpora tmavého režimu");
    AddTranslation("Czech", "Check for Updates", "Zkontrolovat aktualizace");

    AddTranslation("Greek", "Options", "Επιλογές");
    AddTranslation("Greek", "Dark Mode", "Σκοτεινή Λειτουργία");
    AddTranslation("Greek", "Auto-reload modified file", "Αυτόματη επαναφόρτωση τροποποιημένου αρχείου");
    AddTranslation("Greek", "Add to context menu (right-click files)", "Προσθήκη στο μενού περιβάλλοντος (δεξί κλικ στα αρχεία)");
    AddTranslation("Greek", "Language:", "Γλώσσα:");
    AddTranslation("Greek", "Default bytes per line:", "Προεπιλεγμένα bytes ανά γραμμή:");
    AddTranslation("Greek", "8 bytes", "8 bytes");
    AddTranslation("Greek", "16 bytes", "16 bytes");
    AddTranslation("Greek", "OK", "OK");
    AddTranslation("Greek", "Cancel", "Ακύρωση");
    AddTranslation("Greek", "File", "Αρχείο");
    AddTranslation("Greek", "Open", "Άνοιγμα");
    AddTranslation("Greek", "Save", "Αποθήκευση");
    AddTranslation("Greek", "Exit", "Έξοδος");
    AddTranslation("Greek", "View", "Προβολή");
    AddTranslation("Greek", "Disassembly", "Αποσυναρμολόγηση");
    AddTranslation("Greek", "Search", "Αναζήτηση");
    AddTranslation("Greek", "find and Replace...", "Εύρεση και Αντικατάσταση...");
    AddTranslation("Greek", "Go To...", "Μετάβαση σε...");
    AddTranslation("Greek", "Tools", "Εργαλεία");
    AddTranslation("Greek", "Options...", "Επιλογές...");
    AddTranslation("Greek", "Help", "Βοήθεια");
    AddTranslation("Greek", "About HexViewer", "Σχετικά με το HexViewer");
    AddTranslation("Greek", "find:", "Εύρεση:");
    AddTranslation("Greek", "Replace:", "Αντικατάσταση:");
    AddTranslation("Greek", "Replace", "Αντικατάσταση");
    AddTranslation("Greek", "Go", "Μετάβαση");
    AddTranslation("Greek", "Offset:", "Μετατόπιση:");
    AddTranslation("Greek", "Update Available!", "Διαθέσιμη Ενημέρωση!");
    AddTranslation("Greek", "You're up to date!", "Είστε ενημερωμένοι!");
    AddTranslation("Greek", "Current:", "Τρέχουσα:");
    AddTranslation("Greek", "Latest:", "Τελευταία:");
    AddTranslation("Greek", "What's New:", "Τι νέο υπάρχει:");
    AddTranslation("Greek", "You have the latest version installed.", "Έχετε εγκατεστημένη την τελευταία έκδοση.");
    AddTranslation("Greek", "Check back later for updates.", "Ελέγξτε αργότερα για ενημερώσεις.");
    AddTranslation("Greek", "Update Now", "Ενημέρωση Τώρα");
    AddTranslation("Greek", "Skip", "Παράλειψη");
    AddTranslation("Greek", "Close", "Κλείσιμο");
    AddTranslation("Greek", "Version", "Έκδοση");
    AddTranslation("Greek", "A modern cross-platform hex editor", "Ένας σύγχρονος διαπλατφορμικός δεκαεξαδικός επεξεργαστής");
    AddTranslation("Greek", "Features:", "Χαρακτηριστικά:");
    AddTranslation("Greek", "Cross-platform support", "Υποστήριξη πολλαπλών πλατφορμών");
    AddTranslation("Greek", "Real-time hex editing", "Επεξεργασία δεκαεξαδικών σε πραγματικό χρόνο");
    AddTranslation("Greek", "Dark mode support", "Υποστήριξη σκοτεινής λειτουργίας");
    AddTranslation("Greek", "Check for Updates", "Έλεγχος για Ενημερώσεις");

    AddTranslation("Danish", "Options", "Indstillinger");
    AddTranslation("Danish", "Dark Mode", "Mørk Tilstand");
    AddTranslation("Danish", "Auto-reload modified file", "Genindlæs ændret fil automatisk");
    AddTranslation("Danish", "Add to context menu (right-click files)", "Føj til genvejsmenu (højreklik på filer)");
    AddTranslation("Danish", "Language:", "Sprog:");
    AddTranslation("Danish", "Default bytes per line:", "Standard bytes per linje:");
    AddTranslation("Danish", "8 bytes", "8 bytes");
    AddTranslation("Danish", "16 bytes", "16 bytes");
    AddTranslation("Danish", "OK", "OK");
    AddTranslation("Danish", "Cancel", "Annuller");
    AddTranslation("Danish", "File", "Fil");
    AddTranslation("Danish", "Open", "Åbn");
    AddTranslation("Danish", "Save", "Gem");
    AddTranslation("Danish", "Exit", "Afslut");
    AddTranslation("Danish", "View", "Vis");
    AddTranslation("Danish", "Disassembly", "Disassemblering");
    AddTranslation("Danish", "Search", "Søg");
    AddTranslation("Danish", "find and Replace...", "find og Erstat...");
    AddTranslation("Danish", "Go To...", "Gå til...");
    AddTranslation("Danish", "Tools", "Værktøjer");
    AddTranslation("Danish", "Options...", "Indstillinger...");
    AddTranslation("Danish", "Help", "Hjælp");
    AddTranslation("Danish", "About HexViewer", "Om HexViewer");
    AddTranslation("Danish", "find:", "find:");
    AddTranslation("Danish", "Replace:", "Erstat:");
    AddTranslation("Danish", "Replace", "Erstat");
    AddTranslation("Danish", "Go", "Gå");
    AddTranslation("Danish", "Offset:", "Forskydning:");
    AddTranslation("Danish", "Update Available!", "Opdatering Tilgængelig!");
    AddTranslation("Danish", "You're up to date!", "Du er opdateret!");
    AddTranslation("Danish", "Current:", "Nuværende:");
    AddTranslation("Danish", "Latest:", "Seneste:");
    AddTranslation("Danish", "What's New:", "Hvad er nyt:");
    AddTranslation("Danish", "You have the latest version installed.", "Du har den seneste version installeret.");
    AddTranslation("Danish", "Check back later for updates.", "Kom tilbage senere for opdateringer.");
    AddTranslation("Danish", "Update Now", "Opdater Nu");
    AddTranslation("Danish", "Skip", "Spring over");
    AddTranslation("Danish", "Close", "Luk");
    AddTranslation("Danish", "Version", "Version");
    AddTranslation("Danish", "A modern cross-platform hex editor", "En moderne platformsuafhængig hex-editor");
    AddTranslation("Danish", "Features:", "Funktioner:");
    AddTranslation("Danish", "Cross-platform support", "Platformsuafhængig support");
    AddTranslation("Danish", "Real-time hex editing", "Realtids hex-redigering");
    AddTranslation("Danish", "Dark mode support", "Mørk tilstand support");
    AddTranslation("Danish", "Check for Updates", "Tjek for Opdateringer");

    AddTranslation("Norwegian", "Options", "Alternativer");
    AddTranslation("Norwegian", "Dark Mode", "Mørk Modus");
    AddTranslation("Norwegian", "Auto-reload modified file", "Last inn endret fil automatisk");
    AddTranslation("Norwegian", "Add to context menu (right-click files)", "Legg til i hurtigmeny (høyreklikk filer)");
    AddTranslation("Norwegian", "Language:", "Språk:");
    AddTranslation("Norwegian", "Default bytes per line:", "Standard bytes per linje:");
    AddTranslation("Norwegian", "8 bytes", "8 bytes");
    AddTranslation("Norwegian", "16 bytes", "16 bytes");
    AddTranslation("Norwegian", "OK", "OK");
    AddTranslation("Norwegian", "Cancel", "Avbryt");
    AddTranslation("Norwegian", "File", "Fil");
    AddTranslation("Norwegian", "Open", "Åpne");
    AddTranslation("Norwegian", "Save", "Lagre");
    AddTranslation("Norwegian", "Exit", "Avslutt");
    AddTranslation("Norwegian", "View", "Vis");
    AddTranslation("Norwegian", "Disassembly", "Disassemblering");
    AddTranslation("Norwegian", "Search", "Søk");
    AddTranslation("Norwegian", "find and Replace...", "Finn og Erstatt...");
    AddTranslation("Norwegian", "Go To...", "Gå til...");
    AddTranslation("Norwegian", "Tools", "Verktøy");
    AddTranslation("Norwegian", "Options...", "Alternativer...");
    AddTranslation("Norwegian", "Help", "Hjelp");
    AddTranslation("Norwegian", "About HexViewer", "Om HexViewer");
    AddTranslation("Norwegian", "find:", "Finn:");
    AddTranslation("Norwegian", "Replace:", "Erstatt:");
    AddTranslation("Norwegian", "Replace", "Erstatt");
    AddTranslation("Norwegian", "Go", "Gå");
    AddTranslation("Norwegian", "Offset:", "Forskyvning:");
    AddTranslation("Norwegian", "Update Available!", "Oppdatering Tilgjengelig!");
    AddTranslation("Norwegian", "You're up to date!", "Du er oppdatert!");
    AddTranslation("Norwegian", "Current:", "Nåværende:");
    AddTranslation("Norwegian", "Latest:", "Siste:");
    AddTranslation("Norwegian", "What's New:", "Hva er nytt:");
    AddTranslation("Norwegian", "You have the latest version installed.", "Du har den nyeste versjonen installert.");
    AddTranslation("Norwegian", "Check back later for updates.", "Sjekk tilbake senere for oppdateringer.");
    AddTranslation("Norwegian", "Update Now", "Oppdater Nå");
    AddTranslation("Norwegian", "Skip", "Hopp over");
    AddTranslation("Norwegian", "Close", "Lukk");
    AddTranslation("Norwegian", "Version", "Versjon");
    AddTranslation("Norwegian", "A modern cross-platform hex editor", "En moderne plattformuavhengig hex-editor");
    AddTranslation("Norwegian", "Features:", "Funksjoner:");
    AddTranslation("Norwegian", "Cross-platform support", "Plattformuavhengig støtte");
    AddTranslation("Norwegian", "Real-time hex editing", "Sanntids hex-redigering");
    AddTranslation("Norwegian", "Dark mode support", "Mørk modus støtte");
    AddTranslation("Norwegian", "Check for Updates", "Se etter Oppdateringer");

    AddTranslation("Finnish", "Options", "Asetukset");
    AddTranslation("Finnish", "Dark Mode", "Tumma Tila");
    AddTranslation("Finnish", "Auto-reload modified file", "Lataa muokattu tiedosto automaattisesti");
    AddTranslation("Finnish", "Add to context menu (right-click files)", "Lisää kontekstivalikkoon (hiiren oikea painike tiedostoissa)");
    AddTranslation("Finnish", "Language:", "Kieli:");
    AddTranslation("Finnish", "Default bytes per line:", "Oletustavu per rivi:");
    AddTranslation("Finnish", "8 bytes", "8 tavua");
    AddTranslation("Finnish", "16 bytes", "16 tavua");
    AddTranslation("Finnish", "OK", "OK");
    AddTranslation("Finnish", "Cancel", "Peruuta");
    AddTranslation("Finnish", "File", "Tiedosto");
    AddTranslation("Finnish", "Open", "Avaa");
    AddTranslation("Finnish", "Save", "Tallenna");
    AddTranslation("Finnish", "Exit", "Poistu");
    AddTranslation("Finnish", "View", "Näytä");
    AddTranslation("Finnish", "Disassembly", "Disassembly");
    AddTranslation("Finnish", "Search", "Hae");
    AddTranslation("Finnish", "find and Replace...", "Etsi ja Korvaa...");
    AddTranslation("Finnish", "Go To...", "Siirry...");
    AddTranslation("Finnish", "Tools", "Työkalut");
    AddTranslation("Finnish", "Options...", "Asetukset...");
    AddTranslation("Finnish", "Help", "Ohje");
    AddTranslation("Finnish", "About HexViewer", "Tietoja HexViewer");
    AddTranslation("Finnish", "find:", "Etsi:");
    AddTranslation("Finnish", "Replace:", "Korvaa:");
    AddTranslation("Finnish", "Replace", "Korvaa");
    AddTranslation("Finnish", "Go", "Siirry");
    AddTranslation("Finnish", "Offset:", "Siirtymä:");
    AddTranslation("Finnish", "Update Available!", "Päivitys Saatavilla!");
    AddTranslation("Finnish", "You're up to date!", "Olet ajan tasalla!");
    AddTranslation("Finnish", "Current:", "Nykyinen:");
    AddTranslation("Finnish", "Latest:", "Uusin:");
    AddTranslation("Finnish", "What's New:", "Mitä uutta:");
    AddTranslation("Finnish", "You have the latest version installed.", "Sinulla on uusin versio asennettuna.");
    AddTranslation("Finnish", "Check back later for updates.", "Tarkista myöhemmin päivityksiä.");
    AddTranslation("Finnish", "Update Now", "Päivitä Nyt");
    AddTranslation("Finnish", "Skip", "Ohita");
    AddTranslation("Finnish", "Close", "Sulje");
    AddTranslation("Finnish", "Version", "Versio");
    AddTranslation("Finnish", "A modern cross-platform hex editor", "Moderni alustariippumaton hex-editori");
    AddTranslation("Finnish", "Features:", "Ominaisuudet:");
    AddTranslation("Finnish", "Cross-platform support", "Alustariippumaton tuki");
    AddTranslation("Finnish", "Real-time hex editing", "Reaaliaikainen hex-muokkaus");
    AddTranslation("Finnish", "Dark mode support", "Tumman tilan tuki");
    AddTranslation("Finnish", "Check for Updates", "Tarkista Päivitykset");

    AddTranslation("Vietnamese", "Options", "Tùy chọn");
    AddTranslation("Vietnamese", "Dark Mode", "Chế độ Tối");
    AddTranslation("Vietnamese", "Auto-reload modified file", "Tự động tải lại tệp đã sửa đổi");
    AddTranslation("Vietnamese", "Add to context menu (right-click files)", "Thêm vào menu ngữ cảnh (nhấp chuột phải vào tệp)");
    AddTranslation("Vietnamese", "Language:", "Ngôn ngữ:");
    AddTranslation("Vietnamese", "Default bytes per line:", "Byte mặc định mỗi dòng:");
    AddTranslation("Vietnamese", "8 bytes", "8 byte");
    AddTranslation("Vietnamese", "16 bytes", "16 byte");
    AddTranslation("Vietnamese", "OK", "OK");
    AddTranslation("Vietnamese", "Cancel", "Hủy");
    AddTranslation("Vietnamese", "File", "Tệp");
    AddTranslation("Vietnamese", "Open", "Mở");
    AddTranslation("Vietnamese", "Save", "Lưu");
    AddTranslation("Vietnamese", "Exit", "Thoát");
    AddTranslation("Vietnamese", "View", "Xem");
    AddTranslation("Vietnamese", "Disassembly", "Phân tích");
    AddTranslation("Vietnamese", "Search", "Tìm kiếm");
    AddTranslation("Vietnamese", "find and Replace...", "Tìm và Thay thế...");
    AddTranslation("Vietnamese", "Go To...", "Đi tới...");
    AddTranslation("Vietnamese", "Tools", "Công cụ");
    AddTranslation("Vietnamese", "Options...", "Tùy chọn...");
    AddTranslation("Vietnamese", "Help", "Trợ giúp");
    AddTranslation("Vietnamese", "About HexViewer", "Giới thiệu về HexViewer");
    AddTranslation("Vietnamese", "find:", "Tìm:");
    AddTranslation("Vietnamese", "Replace:", "Thay thế:");
    AddTranslation("Vietnamese", "Replace", "Thay thế");
    AddTranslation("Vietnamese", "Go", "Đi");
    AddTranslation("Vietnamese", "Offset:", "Độ lệch:");
    AddTranslation("Vietnamese", "Update Available!", "Cập nhật Có sẵn!");
    AddTranslation("Vietnamese", "You're up to date!", "Bạn đã cập nhật!");
    AddTranslation("Vietnamese", "Current:", "Hiện tại:");
    AddTranslation("Vietnamese", "Latest:", "Mới nhất:");
    AddTranslation("Vietnamese", "What's New:", "Có gì mới:");
    AddTranslation("Vietnamese", "You have the latest version installed.", "Bạn đã cài đặt phiên bản mới nhất.");
    AddTranslation("Vietnamese", "Check back later for updates.", "Kiểm tra lại sau để cập nhật.");
    AddTranslation("Vietnamese", "Update Now", "Cập nhật Ngay");
    AddTranslation("Vietnamese", "Skip", "Bỏ qua");
    AddTranslation("Vietnamese", "Close", "Đóng");
    AddTranslation("Vietnamese", "Version", "Phiên bản");
    AddTranslation("Vietnamese", "A modern cross-platform hex editor", "Trình chỉnh sửa hex đa nền tảng hiện đại");
    AddTranslation("Vietnamese", "Features:", "Tính năng:");
    AddTranslation("Vietnamese", "Cross-platform support", "Hỗ trợ đa nền tảng");
    AddTranslation("Vietnamese", "Real-time hex editing", "Chỉnh sửa hex theo thời gian thực");
    AddTranslation("Vietnamese", "Dark mode support", "Hỗ trợ chế độ tối");
    AddTranslation("Vietnamese", "Check for Updates", "Kiểm tra Cập nhật");


     LanguageMap::SafestrCopy(GetDefaultLanguagePtr(), "English", 32);
  }
  
  static void SetLanguage(const char* language) {
    if (findLanguage(language)) {
      LanguageMap::SafestrCopy(GetDefaultLanguagePtr(), language, 32);
    }
  }
  
  static const char* Get(const char* key) {
    LanguageMap* langMap = findLanguage(GetDefaultLanguagePtr());
    if (langMap) {
      const char* result = langMap->Get(key);
      if (result) return result;
    }
    
    LanguageMap* englishMap = findLanguage("English");
    if (englishMap) {
      const char* result = englishMap->Get(key);
      if (result) return result;
    }
    
    return key;
  }
  
  static const char* T(const char* key) {
    return Get(key);
  }
  
  static void Cleanup() {
    LanguageMap** languagesPtr = GetLanguagesPtr();
    while (*languagesPtr) {
      LanguageMap* temp = *languagesPtr;
      *languagesPtr = (*languagesPtr)->next;
      
      TranslationEntry* entry = temp->entries;
      while (entry) {
        TranslationEntry* tempEntry = entry;
        entry = entry->next;
#ifdef _WIN32
        HeapFree(GetProcessHeap(), 0, tempEntry);
#else
        free(tempEntry);
#endif
      }
#ifdef _WIN32
      HeapFree(GetProcessHeap(), 0, temp);
#else
      free(temp);
#endif
    }
  }
};
