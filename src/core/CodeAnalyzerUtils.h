#ifndef CODE_ANALYZER_UTILS_H
#define CODE_ANALYZER_UTILS_H

#include <string>
#include <vector>

namespace CerebrumLux {

namespace CodeAnalyzerUtils {

/**
 * @brief Verilen C++ dosyasındaki anlamlı kod satırı sayısını (LOC) hesaplar.
 *        Boş satırları ve sadece yorum satırlarını (// veya /* */ /*) göz ardı eder.
 * @param filePath Analiz edilecek dosyanın yolu.
 * @return Anlamlı kod satırı sayısı. Hata durumunda -1 döner.
 */
int countMeaningfulLinesOfCode(const std::string& filePath);

} // namespace CodeAnalyzerUtils

} // namespace CerebrumLux

#endif // CODE_ANALYZER_UTILS_H