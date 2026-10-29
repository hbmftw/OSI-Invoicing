#include "csvreader.h"

#include <QFile>
#include <QObject>
#include <QStringConverter>

CsvData CsvReader::readFile(const QString& path)
{
    CsvData out;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        out.error = QObject::tr("Cannot open %1: %2").arg(path, file.errorString());
        return out;
    }
    const QByteArray bytes = file.readAll();

    QString text;
    if (const auto encoding = QStringConverter::encodingForData(bytes)) {
        // The file starts with a byte order mark (UTF-8/16/32).
        QStringDecoder decoder(*encoding);
        text = decoder(bytes);
    } else {
        QStringDecoder utf8(QStringConverter::Utf8);
        text = utf8(bytes);
        if (utf8.hasError()) {
            QStringDecoder latin1(QStringConverter::Latin1);
            text = latin1(bytes);
        }
    }
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);

    QVector<QStringList> all = parse(text, detectDelimiter(text));
    if (all.isEmpty()) {
        out.error = QObject::tr("The file is empty.");
        return out;
    }

    out.headers = all.takeFirst();
    for (QString& header : out.headers)
        header = header.trimmed();
    out.rows = all;
    return out;
}

QVector<QStringList> CsvReader::parse(const QString& text, QChar delimiter)
{
    QVector<QStringList> rows;
    QStringList row;
    QString field;
    bool inQuotes = false;
    const int n = text.size();

    auto endField = [&]() {
        row << field;
        field.clear();
    };
    auto endRow = [&]() {
        endField();
        bool any = false;
        for (const QString& f : row) {
            if (!f.isEmpty()) {
                any = true;
                break;
            }
        }
        if (any)                    // blank lines are ignored
            rows.push_back(row);
        row.clear();
    };

    for (int i = 0; i < n; ++i) {
        const QChar c = text.at(i);

        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < n && text.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');      // doubled quote
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;                         // includes commas and newlines
            }
        } else if (c == QLatin1Char('"') && field.isEmpty()) {
            inQuotes = true;
        } else if (c == delimiter) {
            endField();
        } else if (c == QLatin1Char('\n')) {
            endRow();
        } else if (c == QLatin1Char('\r')) {
            if (i + 1 < n && text.at(i + 1) == QLatin1Char('\n'))
                ++i;
            endRow();
        } else {
            field += c;
        }
    }
    if (!field.isEmpty() || !row.isEmpty())
        endRow();

    return rows;
}

QChar CsvReader::detectDelimiter(const QString& text)
{
    int commas = 0;
    int semicolons = 0;
    int tabs = 0;
    bool inQuotes = false;

    for (const QChar c : text) {
        if (c == QLatin1Char('"')) {
            inQuotes = !inQuotes;
        } else if (!inQuotes) {
            if (c == QLatin1Char('\n') || c == QLatin1Char('\r'))
                break;                              // header line only
            if (c == QLatin1Char(','))
                ++commas;
            else if (c == QLatin1Char(';'))
                ++semicolons;
            else if (c == QLatin1Char('\t'))
                ++tabs;
        }
    }

    if (tabs > commas && tabs > semicolons)
        return QLatin1Char('\t');
    if (semicolons > commas)
        return QLatin1Char(';');
    return QLatin1Char(',');
}

QString CsvReader::quote(const QString& field)
{
    if (!field.contains(QLatin1Char(',')) && !field.contains(QLatin1Char('"'))
        && !field.contains(QLatin1Char('\n')) && !field.contains(QLatin1Char('\r')))
        return field;

    QString escaped = field;
    escaped.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}
