#ifndef COLUMNMATCHER_H
#define COLUMNMATCHER_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

// One OSI field a source column can be mapped to.
struct TargetField {
    enum class Type { Text, Integer, Money, Date, Email, Phone, Bool };

    QString key;            // internal id, e.g. "postalCode"
    QString label;          // shown to the user
    QStringList aliases;    // other header names this field goes by
    Type type{Type::Text};
};

struct MatchWeights {
    double name{0.60};              // header name similarity
    double type{0.40};              // do the sampled values look right?
    double autoThreshold{0.75};     // >= : pre-filled, shown green
    double suggestThreshold{0.50};  // >= : suggested, shown amber
};

struct FieldMatch {
    int column{-1};         // -1 = unmapped
    double score{0.0};
    bool isAuto{false};     // true = confident, false = suggestion only
};

class ColumnMatcher
{
public:
    explicit ColumnMatcher(const MatchWeights& weights = MatchWeights{})
        : m_w(weights) {}

    // One FieldMatch per target field, same order as 'fields'.
    // Assignment is one-to-one: a source column is used at most once.
    QVector<FieldMatch> match(const QStringList& headers,
                              const QVector<QStringList>& sampleRows,
                              const QVector<TargetField>& fields) const;

    double score(const QString& header,
                 const QStringList& samples,
                 const TargetField& field) const;

    // field key -> source column index (-1 when unmapped)
    static QHash<QString, int> toColumnMap(const QVector<TargetField>& fields,
                                           const QVector<FieldMatch>& matches);

private:
    static QString norm(const QString& s);
    static double similarity(const QString& a, const QString& b);
    static double typeFit(const QStringList& samples, TargetField::Type type);
    static bool fits(const QString& value, TargetField::Type type);

    MatchWeights m_w;
};

#endif // COLUMNMATCHER_H
