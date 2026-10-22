#ifndef ITEM_H
#define ITEM_H

#include <QString>

struct Item {
    int id{0};
    QString name;
    QString description;
    int priceCents{0}; // Stored in cents (e.g. $19.99 = 1999)
    bool taxable{true};
    bool isActive{true};
};

#endif // ITEM_H