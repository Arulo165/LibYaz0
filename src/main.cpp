#include "szsDecode.h"
#include "sarcReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main() {
    std::string szs_path;
    std::string output_dir;
    
    std::cout << "=================================\n";
    std::cout << "      SZS Extractor Tool\n";
    std::cout << "=================================\n\n";
    
    // Pfad zur SZS-Datei abfragen
    std::cout << "Bitte gebe den Pfad zur SZS-Datei ein: ";
    std::getline(std::cin, szs_path);
    
    if (szs_path.empty()) {
        std::cerr << "\nFehler: Kein Pfad angegeben!\n";
        std::cout << "\nDrücke Enter zum Beenden...";
        std::cin.get();
        return 1;
    }
    
    // Ausgabe-Verzeichnis abfragen
    std::cout << "Ausgabe-Verzeichnis (Enter für 'extracted'): ";
    std::getline(std::cin, output_dir);
    
    if (output_dir.empty()) {
        output_dir = "extracted";
    }
    
    std::cout << "\n---------------------------------\n";
    std::cout << "Lade SZS-Datei: " << szs_path << "\n";
    std::cout << "Ausgabe-Ordner: " << output_dir << "\n";
    std::cout << "---------------------------------\n\n";
    
    // Prüfe ob Datei existiert
    if (!fs::exists(szs_path)) {
        std::cerr << "Fehler: Datei existiert nicht: " << szs_path << "\n\n";
        std::cout << "Drücke Enter zum Beenden...";
        std::cin.get();
        return 1;
    }
    
    // SZS laden
    std::ifstream file(szs_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Fehler: Konnte Datei nicht öffnen: " << szs_path << "\n\n";
        std::cout << "Drücke Enter zum Beenden...";
        std::cin.get();
        return 1;
    }
    
    std::size_t file_size = file.tellg();
    file.seekg(0);
    std::vector<u8> szs_data(file_size);
    file.read(reinterpret_cast<char*>(szs_data.data()), file_size);
    file.close();
    
    std::cout << "Dateigröße: " << file_size << " bytes\n";
    
    // Dekomprimieren
    std::cout << "Dekomprimiere SZS...\n";
    u32 sarc_size = 0;
    u8* sarc_data = SZSDecompressor::decompress(
        szs_data.data(), szs_data.size(), &sarc_size
    );
    
    if (!sarc_data) {
        std::cerr << "\nFehler: Dekomprimierung fehlgeschlagen!\n\n";
        std::cout << "Drücke Enter zum Beenden...";
        std::cin.get();
        return 1;
    }
    
    std::cout << "Dekomprimierte Größe: " << sarc_size << " bytes\n";
    
    // SARC öffnen
    std::cout << "Öffne SARC-Archiv...\n";
    SARCReader sarc;
    if (!sarc.open(sarc_data)) {
        std::free(sarc_data);
        std::cerr << "\nFehler: Konnte SARC nicht öffnen!\n\n";
        std::cout << "Drücke Enter zum Beenden...";
        std::cin.get();
        return 1;
    }
    
    // Output-Verzeichnis erstellen
    fs::create_directories(output_dir);
    
    // Dateien extrahieren
    auto files = sarc.listFiles();
    std::cout << "\n---------------------------------\n";
    std::cout << "Extrahiere " << files.size() << " Dateien...\n";
    std::cout << "---------------------------------\n\n";
    
    int success_count = 0;
    int fail_count = 0;
    
    for (const auto& file : files) {
        fs::path output_path = output_dir;
        output_path /= file.name;
        
        // Unterverzeichnisse erstellen
        fs::create_directories(output_path.parent_path());
        
        // Datei schreiben
        std::ofstream out(output_path, std::ios::binary);
        if (out.is_open()) {
            out.write(reinterpret_cast<const char*>(file.data), file.size);
            out.close();
            std::cout << "  ✓ " << file.name << " (" << file.size << " bytes)\n";
            success_count++;
        } else {
            std::cerr << "  ✗ Fehler: " << file.name << "\n";
            fail_count++;
        }
    }
    
    std::free(sarc_data);
    
    std::cout << "\n=================================\n";
    std::cout << "         Fertig!\n";
    std::cout << "=================================\n";
    std::cout << "Erfolgreich: " << success_count << " Dateien\n";
    if (fail_count > 0) {
        std::cout << "Fehlgeschlagen: " << fail_count << " Dateien\n";
    }
    std::cout << "Ausgabe in: " << fs::absolute(output_dir) << "\n";
    std::cout << "=================================\n\n";
    
    std::cout << "Drücke Enter zum Beenden...";
    std::cin.get();
    
    return 0;
}