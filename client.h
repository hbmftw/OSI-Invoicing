#ifndef CLIENT_H
#define CLIENT_H

#include <QString>

// A lightweight struct for passing data between the database layer
// and the UI
struct Client {
    int id{0};
    QString firstName;
    QString lastName;
    QString businessName;
    QString email;
    QString phoneNumber;
    QString address1;
    QString address2;
    QString city;
    QString state;
    QString postalCode;
    QString country;
    QString createdAt;
    bool taxExempt{false};
};


#endif // CLIENT_H
