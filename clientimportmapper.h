#ifndef CLIENTIMPORTMAPPER_H
#define CLIENTIMPORTMAPPER_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include "columnmatcher.h"

struct Client;

namespace ClientImport {

// The fields the mapping step offers for a client. Keys are internal; the
// aliases cover QuickBooks-style lists, Invoice Ninja CSV reports and the
// Invoice Ninja database column names.
QVector<TargetField> fields();

// Builds a Client from one source row. 'columns' is field key -> column index
// (see ColumnMatcher::toColumnMap). Returns false and sets 'error' when the
// row cannot be imported (no name at all).
bool mapRow(const QStringList& row,
            const QHash<QString, int>& columns,
            Client& out,
            QString& error);

} // namespace ClientImport

#endif // CLIENTIMPORTMAPPER_H