#ifndef CSVREADER_H
#define CSVREADER_H

#include <QChar>
#include <QString>
#include <QStringList>
#include <QVector>

struct CsvData {
    QStringList headers;
    QVector<QStringList> rows;
    QString error;              // empty when the file was read fine

    bool ok() const { return error.isEmpty(); }
};

// Small, dependency-free CSV reader: quoted fields, doubled quotes, commas and
// line breaks inside quotes, CRLF/LF, UTF-8/UTF-16 (with or without BOM) and a
// Latin-1 fallback. Delimiter (comma, semicolon or tab) is detected.
class CsvReader
{
public:
    static CsvData readFile(const QString& path);

    static QVector<QStringList> parse(const QString& text, QChar delimiter);
    static QChar detectDelimiter(const QString& text);

    // Quotes a field for writing (used for the "problem rows" file).
    static QString quote(const QString& field);
};

#endif // CSVREADER_H
