#include "CodeAnalyzerUtils.h"
#include "logger.h" // LOG_DEFAULT için
#include <fstream>
#include <sstream>
#include <algorithm> // std::all_of için
#include <cctype>    // std::isspace için (trim için)
#include <regex> // Regex ile yorum satırlarını ve boşlukları temizlemek için

namespace CerebrumLux {

namespace CodeAnalyzerUtils {

// Regex objeleri fonksiyon kapsamı dışında veya static olarak tanımlanmalı
// Tek satırlık yorumlar (//) ve boşluklar
static const std::regex single_line_comment_regex(R"(\)\/\/.*)");
// Çok satırlık yorumlar (/* ... */)
static const std::regex multi_line_comment_start_regex(R"(\)\/\*.*)");
static const std::regex multi_line_comment_end_regex(R"(.*\*\/)");


int countMeaningfulLinesOfCode(const std::string& filePath) {
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "CodeAnalyzerUtils: Anlamli kod satirlari sayiliyor: " << filePath); // Log seviyesi TRACE'e düşürüldü
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "CodeAnalyzerUtils: Dosya acilamadi: " << filePath); // WARNING seviyesi korundu
        return 0; // Dosya açılamazsa 0 döndür
    }

    int meaningfulLines = 0;
    std::string line;
    bool inMultiLineComment = false; // /* ... */ yorum bloğu içinde miyiz durumu
    size_t lineNumber = 0; // Hata raporlama için satır numarası

    while (std::getline(file, line)) {
        lineNumber++;
        try {
            // Baştaki ve sondaki boşlukları (ve yeni satır karakterlerini) temizle
            std::string trimmedLine = line;
            
            trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t\n\r"));
            trimmedLine.erase(trimmedLine.find_last_not_of(" \t\n\r") + 1);

            if (trimmedLine.empty()) { // Temizledikten sonra satır boşsa
                continue; // Line is entirely whitespace or empty
            }

            // Handle multi-line comments
            if (std::regex_search(trimmedLine, multi_line_comment_start_regex)) {
                inMultiLineComment = true;
            }
            if (std::regex_search(trimmedLine, multi_line_comment_end_regex)) {
                inMultiLineComment = false;
                continue; // Line with comment end is not counted as meaningful code
            }
            if (inMultiLineComment) {
                continue; // Lines within a multi-line comment are not counted
            }

            // Remove single-line comments (//...)
            std::string cleanedLine = std::regex_replace(trimmedLine, single_line_comment_regex, "");

            // Yorumlardan ve boşluklardan sonra hala anlamlı bir içerik kaldıysa say
            // Not: std::regex_replace boşluk bırakabilir, bu yüzden tekrar trim gerekli.
            cleanedLine.erase(0, cleanedLine.find_first_not_of(" \t\n\r"));
            cleanedLine.erase(cleanedLine.find_last_not_of(" \t\n\r") + 1);

            if (!cleanedLine.empty()) {
                meaningfulLines++;
            }
        } catch (const std::regex_error& e) { // Regex hatalarını daha spesifik yakala
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "CodeAnalyzerUtils: Satir " << lineNumber << " islenirken regex hatasi: " << e.what() << " Dosya: '" << filePath << "'");
            throw; 
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "CodeAnalyzerUtils: Satir " << lineNumber << " islenirken genel hata: " << e.what() << " Dosya: '" << filePath << "'");
            throw; 
        }
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "CodeAnalyzerUtils: " << filePath << " için anlamli LOC: " << meaningfulLines); // Log seviyesi TRACE'e düşürüldü
    return meaningfulLines;
}

} // namespace CodeAnalyzerUtils

} // namespace CerebrumLux
