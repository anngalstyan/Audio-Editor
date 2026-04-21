#ifndef CAPTION_PARSER_H
#define CAPTION_PARSER_H

#include <QString>
#include <vector>
#include <memory>
#include "../Caption.h"

class ILogger;

/**
 * @brief SRT subtitle file parser and exporter
 * 
 * Handles reading and writing SubRip (.srt) caption files.
 * Also supports exporting to plain text format.
 * 
 * SRT Format:
 * ```
 * 1
 * 00:00:01,234 --> 00:00:04,567
 * Caption text here
 * 
 * 2
 * 00:00:05,000 --> 00:00:08,000
 * Another caption
*/
class CaptionParser {
public:
    explicit CaptionParser(std::shared_ptr<ILogger> logger);
    ~CaptionParser() = default;

    std::vector<Caption> parseSRT(const QString& filePath);

    bool exportTXT(const QString& filePath, const std::vector<Caption>& captions);

    bool exportSRT(const QString& filePath, const std::vector<Caption>& captions);

private:
    double parseTimestamp(const QString& timestamp);

    QString formatTimestamp(double seconds);

    std::shared_ptr<ILogger> logger_;
};

#endif // CAPTION_PARSER_H