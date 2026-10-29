#include "columnmatcher.h"

#include <QDate>
#include <algorithm>

QString ColumnMatcher::norm(const QString& s)
{
    QString out;
    for (const QChar c : s.toLower())
        if (c.isLetterOrNumber())
            out += c;
    return out;
}

double ColumnMatcher::similarity(const QString& a, const QString& b)
{
    if (a.isEmpty() || b.isEmpty())
        return 0.0;
    if (a == b)
        return 1.0;

    const int la = a.size();
    const int lb = b.size();
    const int shorter = std::min(la, lb);
    const int longer = std::max(la, lb);

    // One name inside the other ("state" in "stateprovince"): good, but the
    // more extra characters, the weaker the match. This keeps
    // "State/Province" ahead of "Shipping State/Province" for a "state" alias.
    if (shorter >= 4 && (a.contains(b) || b.contains(a)))
        return 0.55 + 0.25 * double(shorter) / double(longer);

    // Levenshtein distance, two rows.
    QVector<int> prev(lb + 1);
    QVector<int> cur(lb + 1);
    for (int j = 0; j <= lb; ++j)
        prev[j] = j;
    for (int i = 1; i <= la; ++i) {
        cur[0] = i;
        for (int j = 1; j <= lb; ++j) {
            const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, cur);
    }
    return 1.0 - double(prev[lb]) / double(longer);
}

bool ColumnMatcher::fits(const QString& value, TargetField::Type type)
{
    using T = TargetField::Type;
    const QString s = value.trimmed();

    switch (type) {
    case T::Email:
        return s.contains('@') && s.contains('.');

    case T::Date: {
        const QStringList formats{QStringLiteral("yyyy-MM-dd"),
                                  QStringLiteral("M/d/yyyy"),
                                  QStringLiteral("MM/dd/yyyy"),
                                  QStringLiteral("d-MMM-yyyy"),
                                  QStringLiteral("yyyy/MM/dd")};
        for (const QString& f : formats)
            if (QDate::fromString(s, f).isValid())
                return true;
        return false;
    }

    case T::Money: {
        QString m = s;
        m.remove('$');
        m.remove(',');
        bool ok = false;
        m.toDouble(&ok);
        return ok;
    }

    case T::Integer: {
        bool ok = false;
        s.toLongLong(&ok);
        return ok;
    }

    case T::Bool: {
        static const QStringList accepted{QStringLiteral("y"),    QStringLiteral("yes"),
                                          QStringLiteral("true"), QStringLiteral("1"),
                                          QStringLiteral("n"),    QStringLiteral("no"),
                                          QStringLiteral("false"), QStringLiteral("0")};
        return accepted.contains(s.toLower());
    }

    case T::Phone: {
        int digits = 0;
        for (const QChar c : s)
            if (c.isDigit())
                ++digits;
        return digits >= 7 && digits <= 15;
    }

    case T::Text:
        return true;
    }
    return false;
}

double ColumnMatcher::typeFit(const QStringList& samples, TargetField::Type type)
{
    if (type == TargetField::Type::Text)
        return 0.5;     // text fits anything: neutral

    int total = 0;
    int good = 0;
    for (const QString& v : samples) {
        if (v.trimmed().isEmpty())
            continue;
        ++total;
        if (fits(v, type))
            ++good;
    }
    return total ? double(good) / double(total) : 0.0;
}

double ColumnMatcher::score(const QString& header,
                            const QStringList& samples,
                            const TargetField& field) const
{
    const QString h = norm(header);

    double best = std::max(similarity(h, norm(field.key)),
                           similarity(h, norm(field.label)));
    for (const QString& alias : field.aliases)
        best = std::max(best, similarity(h, norm(alias)));

    // A header that says "shipping" should not win a field that isn't one.
    const QString fieldText = (field.key + field.label).toLower();
    if (h.contains(QLatin1String("ship")) && !fieldText.contains(QLatin1String("ship")))
        best *= 0.5;

    // Exact name/alias match is full marks; otherwise blend name and data.
    const double base = (best >= 1.0)
                            ? 1.0
                            : m_w.name * best + m_w.type * typeFit(samples, field.type);

    // Prefer columns that actually contain data: an exact match on an empty
    // column ("Client Phone") loses to one with values ("Contact Phone").
    int filled = 0;
    for (const QString& s : samples)
        if (!s.trimmed().isEmpty())
            ++filled;
    const double fill = samples.isEmpty() ? 0.0 : double(filled) / double(samples.size());

    return base * (0.9 + 0.1 * fill);
}

QVector<FieldMatch> ColumnMatcher::match(const QStringList& headers,
                                         const QVector<QStringList>& sampleRows,
                                         const QVector<TargetField>& fields) const
{
    struct Pair {
        int field;
        int col;
        double score;
    };
    QVector<Pair> pairs;

    for (int c = 0; c < headers.size(); ++c) {
        QStringList samples;
        for (const QStringList& row : sampleRows)
            if (c < row.size())
                samples << row.at(c);

        for (int f = 0; f < fields.size(); ++f)
            pairs.push_back({f, c, score(headers.at(c), samples, fields.at(f))});
    }

    // Best score first; on a tie the earlier column wins.
    std::sort(pairs.begin(), pairs.end(), [](const Pair& a, const Pair& b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.col < b.col;
    });

    QVector<FieldMatch> result(fields.size());
    QVector<bool> columnUsed(headers.size(), false);

    for (const Pair& p : pairs) {
        if (p.score < m_w.suggestThreshold)
            break;
        if (result[p.field].column != -1 || columnUsed[p.col])
            continue;
        result[p.field] = {p.col, p.score, p.score >= m_w.autoThreshold};
        columnUsed[p.col] = true;
    }
    return result;
}

QHash<QString, int> ColumnMatcher::toColumnMap(const QVector<TargetField>& fields,
                                               const QVector<FieldMatch>& matches)
{
    QHash<QString, int> map;
    for (int i = 0; i < fields.size() && i < matches.size(); ++i)
        map.insert(fields.at(i).key, matches.at(i).column);
    return map;
}
